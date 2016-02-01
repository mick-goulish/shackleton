#! /bin/bash




#-------------------------------
# Make old stuff go away.
#-------------------------------
rm -rf ./temp
mkdir  ./temp

YEAR=$1
MONTH=$2
DAY=$3


# In this test, number of client pairs is fixed at 1.
CLIENT_PAIRS=1
TEST_NAME=router_1
LANGUAGE=CPP

#--------------------------------------
# Production
#--------------------------------------
# N_MESSAGES=5000000
# TEMP
N_MESSAGES=500
REPORT_FREQUENCY=100000

#--------------------------------------
# Dev
#--------------------------------------
#N_MESSAGES=1000000
#REPORT_FREQUENCY=100000





function check_cores(){
  core_files=$(ls ./core* 2> /dev/null | wc -l)

  if [ "${core_files}" != "0" ]
  then
     echo "${core_iles} core files found"
     saved_core_files=$(ls ./saved_core* 2> /dev/null | wc -l)
     if [ "${saved_core_files}" != "0" ]
     then
       echo "A core file has already been saved.  Removing new core files."
       rm -f ./core*
     else
       echo "No core file has already been saved."
       first_core_file=`ls -1 core* | head -1`
       echo "saving core file ${first_core_file}"
       mv ${first_core_file} saved_${first_core_file}
     fi
  else
      echo "no core files found."
  fi
}



echo "At start of test_1.sh : checking for cores"
check_cores


echo "=============================================================="
echo "  router throughput test for ${CLIENT_PAIRS} client pairs"
echo "=============================================================="
date


RESULT_ROOT=${SHACKLETON_ROOT}/results/tests/${YEAR}/${MONTH}/${DAY}/${TEST_NAME}/${LANGUAGE}


RECV_RESULT_FILE=${RESULT_ROOT}/receiver/tp_report.txt
SEND_RESULT_FILE=${RESULT_ROOT}/sender/tp_report.txt

echo "RECV_RESULT_FILE at ${RECV_RESULT_FILE}"
echo "SEND_RESULT_FILE at ${SEND_RESULT_FILE}"


mkdir -p ${RESULT_ROOT}/sender
mkdir -p ${RESULT_ROOT}/router
mkdir -p ${RESULT_ROOT}/receiver




echo "killing old routers..."
pkill qdrouterd
sleep 2
pkill watch_pid

ROUTERS_STILL_RUNNING=`ps -aef | grep qdrouterd | grep -v grep | wc -l`
echo "ROUTERS_STILL_RUNNING == ${ROUTERS_STILL_RUNNING}"
while [ ${ROUTERS_STILL_RUNNING} -gt 0 ]
do
  pkill qdrouterd
  sleep 2
  ROUTERS_STILL_RUNNING=`ps -aef | grep qdrouterd | grep -v grep | wc -l`
  echo "ROUTERS_STILL_RUNNING == ${ROUTERS_STILL_RUNNING}"
done


echo "After pkill : checking for cores"
check_cores


INSTALL_ROOT=${SHACKLETON_ROOT}/install

PROTRON_INSTALL_DIR=${INSTALL_ROOT}/proton
DISPATCH_INSTALL_DIR=${INSTALL_ROOT}/dispatch

export LD_LIBRARY_PATH=${DISPATCH_INSTALL_DIR}/lib64:${PROTRON_INSTALL_DIR}/lib64
export PYTHONPATH=${DISPATCH_INSTALL_DIR}/lib/qpid-dispatch/python:${DISPATCH_INSTALL_DIR}/lib/python2.7/site-packages

CONFIG_DIR=./config

ROUTER=${DISPATCH_INSTALL_DIR}/sbin/qdrouterd

CONFIG_FILE=${CONFIG_DIR}/qdrouterd.conf

echo "----------------------------------------------------------"
echo "running router ${ROUTER} with config ${CONFIG_FILE}"
echo "----------------------------------------------------------"
echo "router command: "
echo ${ROUTER} --config ${CONFIG_FILE}  
${ROUTER} --config ${CONFIG_FILE}  &
ROUTER_PID=$!
echo "router pid: ${ROUTER_PID}"

sleep 3

echo "this router is running: "
ps -aef | grep qdrouterd | grep -v grep

# Now that the router is running, start gathering its resource usage data.
#./watch_pid.sh ${ROUTER_PID} >  ${RESULT_ROOT}/router/tp_report.txt &
#WATCH_ROUTER_PID=$!


echo " "
echo "Starting apps..."
echo " "

APP_ROOT=${SHACKLETON_ROOT}/apps/CPP

RECV=${APP_ROOT}/simple_recv
SEND=${APP_ROOT}/simple_send

echo "results being written to: ${RESULT_ROOT}"
echo " "

sleep 3

#---------------------------------------------
# start receivers
#---------------------------------------------
rm ${RESULT_ROOT}/receiver/*
receiver_pids=()
count=0
while [ $count -lt ${CLIENT_PAIRS} ]
do
  echo "starting receiver $count"
  echo "receiver command:"
  echo "${RECV} -m ${N_MESSAGES} -f ${REPORT_FREQUENCY} -q ${count} -d ${RESULT_ROOT}/receiver "
  ${RECV} -m ${N_MESSAGES} -f ${REPORT_FREQUENCY} -q ${count} -d ${RESULT_ROOT}/receiver &
  RECV_PID=$!
  receiver_pids+=(${RECV_PID}) 
  count=$(( $count + 1 ))
done

sleep 3

#---------------------------------------------
# start senders
#---------------------------------------------
rm ${RESULT_ROOT}/sender/*
sender_pids=()
count=0
while [ $count -lt ${CLIENT_PAIRS} ]
do
  echo "starting sender $count"
  echo "sender command:"
  # TEMP -- get rid of -P
  echo ${SEND} -m ${N_MESSAGES} -P 100 -f ${REPORT_FREQUENCY} -q ${count} -d ${RESULT_ROOT}/sender
  ${SEND} -m ${N_MESSAGES} -P 100 -f ${REPORT_FREQUENCY} -q ${count} -d ${RESULT_ROOT}/sender  &
  SEND_PID=$!
  sender_pids+=(${SEND_PID})
  count=$(( $count + 1 ))
done


receiver_done_file="/tmp/simple_recv_${RECV_PID}_is_done"
found_file="false"

wait_count=100
while [ ${wait_count} -gt 0 ]
do
  echo "waiting for receiver to finish: ${wait_count}   ( looking for file ${receiver_done_file} )"
  sleep 15
  wait_count=$(( $wait_count - 1 ))
  if [ -f ${receiver_done_file} ]
  then
    echo "receiver done file exists -- exiting"
    found_file="true"
    break
  fi
done

if [ ${found_file} == "false" ]
then
  echo "ERROR: timeout while waiting for sender to finish."
fi

sleep 3


# There is only one receiver -- and it left its report at 
# ${RESULT_ROOT}/receiver/tp_report_PID.txt .
# That is not useful in this test -- get rid of that PID.
mv ${RESULT_ROOT}/receiver/tp_report_*.txt ${RESULT_ROOT}/receiver/tp_report.txt


#---------------------------------------------
# process data 
#---------------------------------------------

STATS=${SHACKLETON_ROOT}/utils/stats/stats

echo "processing data..."

mem=`cat ${RESULT_ROOT}/router/tp_report.txt | tail -3 | head -1 | awk '{print $4}'`


echo "Moving sender file..."
mv ${RESULT_ROOT}/sender/tp_report_*.txt  ${RESULT_ROOT}/sender/tp_report.txt


echo " "
echo "... making sure all processes are killed..."
echo " "

kill -1 ${ROUTER_PID}

sleep 5

kill -9 ${ROUTER_PID}
kill -9 ${WATCH_ROUTER_PID}

pkill -9 simple_send
pkill -9 simple_recv


echo "  router throughput test for ${CLIENT_PAIRS} done"
date

echo " "
echo " "
echo " "
