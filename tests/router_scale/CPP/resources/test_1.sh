#! /bin/bash


YEAR=$1
MONTH=$2
DAY=$3
CLIENT_PAIRS=$4

TEST_NAME=router_scale
LANGUAGE=CPP


#---------------------------------------
# Production
#---------------------------------------
N_MESSAGES=10000
REPORT_FREQUENCY=10001
PAUSE=1000


#---------------------------------------
# Development
#---------------------------------------
#N_MESSAGES=100
#REPORT_FREQUENCY=100
#DELAY=20






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


echo "--------------------------------------------------------------"
echo "  router scale test for ${CLIENT_PAIRS} client pairs"
echo "--------------------------------------------------------------"
date

RESULT_ROOT=${SHACKLETON_ROOT}/results/tests/${YEAR}/${MONTH}/${DAY}/${TEST_NAME}/${LANGUAGE}

#---------------------------------------------------------------
# This script 'owns' RESULT_N_ROOT -- so if it already exists
# (i.e. from an earlier degunning run today) I can remove it.
#---------------------------------------------------------------
RESULT_N_ROOT=${RESULT_ROOT}/${CLIENT_PAIRS}
rm -rf   ${RESULT_N_ROOT}/router
mkdir -p ${RESULT_N_ROOT}/router



RECV_RESULT_FILE=${RESULT_ROOT}/receiver/tp_report.txt
SEND_RESULT_FILE=${RESULT_ROOT}/sender/tp_report.txt

echo "RECV_RESULT_FILE at ${RECV_RESULT_FILE}"
echo "SEND_RESULT_FILE at ${SEND_RESULT_FILE}"


mkdir -p ${RESULT_ROOT}/sender
mkdir -p ${RESULT_ROOT}/receiver
mkdir -p ${RESULT_ROOT}/router


ROUTERS_STILL_RUNNING=`ps -aef | grep qdrouterd | grep -v grep | wc -l`
echo "ROUTERS_STILL_RUNNING == ${ROUTERS_STILL_RUNNING}"
while [ ${ROUTERS_STILL_RUNNING} -gt 0 ]
do
  pkill qdrouterd
  sleep 2
  ROUTERS_STILL_RUNNING=`ps -aef | grep qdrouterd | grep -v grep | wc -l`
  echo "ROUTERS_STILL_RUNNING == ${ROUTERS_STILL_RUNNING}"
done


echo "AFter pkill : checking for cores"
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

echo "---------------------------------"
echo "router running: "
echo "---------------------------------"
ps -aef | grep qdrouterd | grep -v grep


RR=${SHACKLETON_ROOT}/utils/rr/rr
# Now that the router is running, start gathering its resource usage data.
echo "starting RR with this command:"
echo "${RR} ${ROUTER_PID} 10 >  ${RESULT_ROOT}/router/tp_report.txt"
${RR} ${ROUTER_PID} 10 >  ${RESULT_ROOT}/router/tp_report.txt &
RR_PID=$!



echo " "
echo "Starting apps..."
echo " "

APP_ROOT=${SHACKLETON_ROOT}/apps/CPP

RECV=${APP_ROOT}/simple_recv
SEND=${APP_ROOT}/simple_send

echo "results being written to: ${RESULT_N_ROOT}"
echo " "


sleep 2


#---------------------------------------------
# start receivers
#---------------------------------------------
rm ${RESULT_ROOT}/receiver/*
receiver_pids=()
count=0
while [ $count -lt ${CLIENT_PAIRS} ]
do
  echo "starting receiver $count"
  #echo "receiver command:"
  #echo "${RECV} -m ${N_MESSAGES} -f ${REPORT_FREQUENCY} -q ${count} -d ${RESULT_ROOT}/receiver "
  ${RECV} -m ${N_MESSAGES} -f ${REPORT_FREQUENCY} -q ${count} -d ${RESULT_ROOT}/receiver &
  RECV_PID=$!
  receiver_pids+=(${RECV_PID}) 
  count=$(( $count + 1 ))
  sleep 0.1
done


sleep 5

echo "-------------------------------"
echo "removing old done-files."
echo "-------------------------------"
rm -f /tmp/simple_recv_*_is_done


#---------------------------------------------
# start senders
#---------------------------------------------
rm ${RESULT_ROOT}/sender/*
sender_pids=()
count=0
while [ $count -lt ${CLIENT_PAIRS} ]
do
  echo "starting sender $count"
  #echo ${SEND} -m ${N_MESSAGES} -f ${REPORT_FREQUENCY} -q ${count} -d ${RESULT_ROOT}/sender -P ${PAUSE} 
  ${SEND} -m ${N_MESSAGES} -f ${REPORT_FREQUENCY} -q ${count} -d ${RESULT_ROOT}/sender -P ${PAUSE} &
  SEND_PID=$!
  sender_pids+=(${SEND_PID})
  count=$(( $count + 1 ))
  sleep 0.1
done


#---------------------------------------------
# With large numbers of senders and receivers
# I am having a problem with hanging while 
# waiting for them.  Thus, the reaper -- a 
# failsafe killer of things that ought to die.
# If we hang-while-waiting, these will save us.
#---------------------------------------------

FIFTEEN_MINUTES=900
HALF_HOUR=1800

./reaper.sh simple_send  ${FIFTEEN_MINUTES} &
REAPER_1_PID=$!

./reaper.sh simple_recv  ${FIFTEEN_MINUTES} &
REAPER_2_PID=$!

./reaper.sh qdrouterd    ${FIFTEEN_MINUTES} &
REAPER_3_PID=$!


sleep 5

wait_count=100
while [ $wait_count -gt 0 ]
do
  echo "waiting for simple_recv done files.  ${wait_count}"
  done_files=$(ls -1 /tmp/simple_recv_*_is_done 2> /dev/null  | wc -l)
  echo "${done_files} found."
  if [ "${done_files}" == "${CLIENT_PAIRS}" ]
  then
    break
  fi
  wait_count=$(( $wait_count - 1 ))
  sleep 10
done

if [ "${done_files}" == "${CLIENT_PAIRS}" ]
then
  echo "All done files were found."
else
  echo "ERROR: not all done files were found."
fi


rm -f /tmp/simple_recv_*_is_done


sleep 5

echo "waiting successful: deactivating reapers."
kill -9 ${REAPER_1_PID}
kill -9 ${REAPER_2_PID}
kill -9 ${REAPER_3_PID}



#---------------------------------------------
# process data 
#---------------------------------------------

STATS=${SHACKLETON_ROOT}/utils/stats/stats

echo "processing data..."

mem=`cat ${RESULT_N_ROOT}/router/tp_report.txt | tail -3 | head -1 | awk '{print $4}'`

echo -e "${CLIENT_PAIRS}\t${mem}" >> ${RESULT_ROOT}/router/tp_report.txt

#------------------------------------------------------------------
# Now get rid of the result root for this number of client-pairs.
# We will not be needing it anymore.
#------------------------------------------------------------------
rm -rf ${RESULT_N_ROOT}

#----------------------------------------------------
# We also have no further use for the sender 
# and receiver directories.  All we are reporting 
# on here is router behavior.
#----------------------------------------------------
rm -rf ${RESULT_ROOT}/sender
rm -rf ${RESULT_ROOT}/receiver





echo " "
echo "... making sure all processes are killed..."
echo " "

kill -1 ${ROUTER_PID}

sleep 5

kill -9 ${ROUTER_PID}

echo "killing RR program: ${RR_PID}"
kill -9 ${RR_PID}


pkill simple_send
pkill simple_recv


echo "  router throughput test for ${CLIENT_PAIRS} done"
date

echo " "
echo " "
echo " "
