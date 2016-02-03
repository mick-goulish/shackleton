#! /bin/bash




#-------------------------------
# Make old stuff go away.
#-------------------------------
rm -f /tmp/simple_recv_*_is_done


YEAR=$1
MONTH=$2
DAY=$3
CLIENT_PAIRS=$4


TEST_NAME=router_scale_2
LANGUAGE=CPP



N_MESSAGES=1000
INTER_APP_PAUSE=0.01





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



RESULT_ROOT=${SHACKLETON_ROOT}/results/tests/${YEAR}/${MONTH}/${DAY}/${TEST_NAME}/${LANGUAGE}
INSTALL_ROOT=${SHACKLETON_ROOT}/install
PROTRON_INSTALL_DIR=${INSTALL_ROOT}/proton
DISPATCH_INSTALL_DIR=${INSTALL_ROOT}/dispatch
export LD_LIBRARY_PATH=${DISPATCH_INSTALL_DIR}/lib64:${PROTRON_INSTALL_DIR}/lib64
export PYTHONPATH=${DISPATCH_INSTALL_DIR}/lib/qpid-dispatch/python:${DISPATCH_INSTALL_DIR}/lib/python2.7/site-packages
CONFIG_DIR=./config
ROUTER=${DISPATCH_INSTALL_DIR}/sbin/qdrouterd
CONFIG_FILE=${CONFIG_DIR}/qdrouterd.conf




mkdir -p ${RESULT_ROOT}/sender
mkdir -p ${RESULT_ROOT}/router
rm   -rf ${RESULT_ROOT}/receiver/temp
mkdir -p ${RESULT_ROOT}/receiver/temp



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





echo "----------------------------------------------------------"
echo "running router ${ROUTER} with config ${CONFIG_FILE}"
echo "----------------------------------------------------------"
echo "RUNNING UNDER VALGRIND"
#valgrind --tool=callgrind --instr-atstart=yes --collect-atstart=no  ${ROUTER} --config ${CONFIG_FILE}  &
#echo "router command:"
#echo "${ROUTER} --config ${CONFIG_FILE} "
${ROUTER} --config ${CONFIG_FILE}  &
ROUTER_PID=$!
echo "router pid: ${ROUTER_PID}"

sleep 3

echo "this router is running: "
ps -aef | grep qdrouterd | grep -v grep

# Now that the router is running, start gathering its resource usage data.
RR=${SHACKLETON_ROOT}/utils/rr/rr
ROUTER_WATCH_FILE=${RESULT_ROOT}/router/watch_${CLIENT_PAIRS}.txt
ROUTER_CPU_STATS_FILE=${RESULT_ROOT}/router/cpu_stats_${CLIENT_PAIRS}.txt
ROUTER_MEM_STATS_FILE=${RESULT_ROOT}/router/mem_stats_${CLIENT_PAIRS}.txt
ROUTER_RESULT_FILE=${RESULT_ROOT}/router/tp_report.txt


${RR} ${ROUTER_PID} 10 >  ${ROUTER_WATCH_FILE} &
WATCH_ROUTER_PID=$!



APP_ROOT=${SHACKLETON_ROOT}/apps/CPP
RECV=${APP_ROOT}/simple_recv
SEND=${APP_ROOT}/simple_send
receiver_pids=()
sender_pids=()


start_signal_file=/tmp/start
rm -f ${start_signal_file}


#############################################################
# Start all the receivers
#############################################################
count=0
while [ $count -lt ${CLIENT_PAIRS} ]
do
  #echo "starting receiver ${count} "
  #echo "receiver command:"
  #echo "${RECV} -m ${N_MESSAGES} -f 100000 -q ${count} -d ${RESULT_ROOT}/receiver/temp "
  ${RECV} -m ${N_MESSAGES} -f 100000 -q ${count} -d ${RESULT_ROOT}/receiver/temp &
  RECV_PID=$!
  #echo "n_to_pid: receiver ${count} is pid ${RECV_PID}"
  receiver_pids+=(${RECV_PID}) 
  count=$(( $count + 1 ))
  sleep ${INTER_APP_PAUSE}
done


echo "resting after starting all receivers"
sleep 10


#CONTINUE=/tmp/c1
#rm -f ${CONTINUE}
#echo -n "waiting for file ${CONTINUE}. wait starting at: "
#date
#wait_count=1
#while [ $wait_count -lt 1001 ]
#do
#  echo "${wait_count}"
#  sleep 15
#  if [ -e ${CONTINUE} ]
#  then
#    break
#  fi
#  wait_count=$(( $wait_count + 1 ))
#done
#
#echo -n "done waiting at ${date}"




#############################################################
# Start all the senders
#############################################################
count=0
while [ $count -lt ${CLIENT_PAIRS} ]
do
  # echo "starting sender ${count} "
  ${SEND} -m ${N_MESSAGES} -q ${count} -d ${RESULT_ROOT}/sender -F ${start_signal_file}  &
  SEND_PID=$!
  #echo "n_to_pid: sender ${count} is pid ${SEND_PID}"
  sender_pids+=(${SEND_PID})
  count=$(( $count + 1 ))
  sleep ${INTER_APP_PAUSE}
done



#CONTINUE=/tmp/c2
#rm -f ${CONTINUE}
#echo -n "waiting for file ${CONTINUE}. wait starting at: "
#date
#wait_count=1
#while [ $wait_count -lt 1001 ]
#do
#  echo "${wait_count}"
#  sleep 15
#  if [ -e ${CONTINUE} ]
#  then
#    break
#  fi
#  wait_count=$(( $wait_count + 1 ))
#done
#
#echo -n "done waiting at " ; date
#




# Now all the sender-receiver pairs are started, 
# tell the senders to start sending.
WAIT_FOR_IT=5
echo "Starting senders in ${WAIT_FOR_IT} seconds."
sleep ${WAIT_FOR_IT}
echo "Starting senders."
touch ${start_signal_file} 


#----------------------------------------------------------------
# We have asked each sender to send 1000 messages,
# which would take 1000 seconds.  However, we are not going 
# to wait that long.  Just sleep one minute here, to let
# the router monitoring program get a good number of activity 
# measurements -- then kill everything.
#----------------------------------------------------------------
sleep 60


#nap_size=15
#nap_count=0
#while [ $nap_count -lt 20 ]
#do
#  nap_count=$(( $nap_count + 1 ))
#  echo "test.sh sleeping ${nap_count}  ..."
#
#
#  # See how many receivers have completed.
#  done_files=$(ls -1 /tmp/simple_recv_*_is_done 2> /dev/null  | wc -l)
#  echo "test.sh:  ${done_files} done files were found."
#  if [ "${done_files}" == "${CLIENT_PAIRS}" ]
#  then
#    break
#  fi
#
#  echo "number of senders still running:"
#  ps -aef | grep simple_send | grep -v grep | grep -v vim | wc -l
#
#  echo "number of receivers still running:"
#  ps -aef | grep simple_recv | grep -v grep | grep -v vim | wc -l
#
#  sleep ${nap_size}
#done
#
#

#
#if [ "${done_files}" == "${CLIENT_PAIRS}" ]
#then
#  echo "test.sh:  All done files were found."
#else
#  echo "test.sh ERROR: not all done files were found."
#fi
#
#echo "number of senders still running:"
#ps -aef | grep simple_send | grep -v grep | grep -v vim | wc -l
#
#echo "number of receivers still running:"
#ps -aef | grep simple_recv | grep -v grep | grep -v vim | wc -l



echo " "
echo "Kill all processes ..."
echo " "
${SHACKLETON_ROOT}/utils/kill_everything_except_the_test.sh 

rm -f /tmp/simple_recv_*_is_done




#--------------------------------------------------------------
# process data 
#--------------------------------------------------------------
STATS=${SHACKLETON_ROOT}/utils/stats/stats


# Combine all results files into one.
cat ${RESULT_ROOT}/receiver/temp/tp_report*txt | grep latency | awk '{print $2}' > ${RESULT_ROOT}/receiver/temp/all_latencies
result_file=${RESULT_ROOT}/receiver/tp_report.txt
echo -n "client_pairs: ${CLIENT_PAIRS}  "  >> ${result_file}
${STATS} < ${RESULT_ROOT}/receiver/temp/all_latencies | awk '{print "mean: ",$6," sigma:",$8}' >> ${result_file}

ROUTER_WATCH_FILE_LENGTH=`wc -l ${ROUTER_WATCH_FILE} | awk '{print $1}'`
DATA_LENGTH=$(( ${ROUTER_WATCH_FILE_LENGTH} - 1 ))
# Field 2 of ROUTER_WATCH_FILE is CPU.   3 is mem.
tail -${DATA_LENGTH} ${ROUTER_WATCH_FILE} | awk '{print $2}' | stats > ${ROUTER_CPU_STATS_FILE}
tail -${DATA_LENGTH} ${ROUTER_WATCH_FILE} | awk '{print $3}' | stats > ${ROUTER_MEM_STATS_FILE}
# Field 12 is the max value.
MAX_ROUTER_CPU=`cat ${ROUTER_CPU_STATS_FILE} | awk '{print $12}' `
MAX_ROUTER_MEM=`cat ${ROUTER_MEM_STATS_FILE} | awk '{print $12}' `
echo "client_pairs: ${CLIENT_PAIRS}  max_cpu: ${MAX_ROUTER_CPU}  max_mem: ${MAX_ROUTER_MEM}" >> ${ROUTER_RESULT_FILE}

# At this point, we have accumulated the latest CPU result in ROUTER_RESULT_FILE.
# That is the only router file we want to keep.
rm -f ${ROUTER_WATCH_FILE} ${ROUTER_CPU_STATS_FILE} ${ROUTER_MEM_STATS_FILE}






#echo "============================================================="
#echo " checking receiver results..."
#grep "received 100 of 100" ${RESULT_ROOT}/receiver/temp/*txt | wc -l 
#echo " checking sender results..."r
#grep "on_delivery_settle  100" ${RESULT_ROOT}/sender/* | wc -l
#echo "============================================================="
#echo " "
#echo " "
#echo " "
#echo " "
#

rm -f  ${RESULT_ROOT}/sender
rm -rf ${RESULT_ROOT}/receiver/temp



echo "  router_scale_2 test for ${CLIENT_PAIRS} is complete."
date

echo " "
echo " "
echo " "



