#! /bin/bash





TEST_NAME=message_size
LANGUAGE=C



YEAR=$1
MONTH=$2
DAY=$3




APP_ROOT=${SHACKLETON_ROOT}/apps/C

SEND=${APP_ROOT}/reactor-send
RECV=${APP_ROOT}/reactor-recv

CPU_A=1
CPU_B=2


RESULT_ROOT=${SHACKLETON_ROOT}/results/tests/${YEAR}/${MONTH}/${DAY}/${TEST_NAME}/${LANGUAGE}
echo "reports being written to ${RESULT_ROOT}"

#-----------------------------------------
# Make sure the destination dirs exist.
#-----------------------------------------
mkdir -p ${RESULT_ROOT}/receiver
mkdir -p ${RESULT_ROOT}/sender


# In case it's already there, during testing...
rm -f ${RESULT_ROOT}/receiver/tp_results.txt

#--------------------------
# test parameters
#--------------------------
TIMESTAMPING=


#-------------------------------------------------
#  Production 
#-------------------------------------------------
#MSG_SIZE_ARRAY=(       100     200       500     1000     2000     5000     7000     8000     9000    10000    11000    12000    15000   17000   20000    25000    30000   50000   70000  100000 )
#N_MESSAGES_ARRAY=( 2000000  2000000  2000000  2000000  2000000  1000000  1000000  1000000  1000000  1000000   900000   800000   700000  500000  500000   500000   400000  400000  250000  100000 )


#-------------------------------------------------
#  Development 
#-------------------------------------------------
MSG_SIZE_ARRAY=(       100     200       500     1000     2000     5000     7000     8000     9000    10000    11000    12000    15000   17000   20000    25000    30000   50000   70000  100000 )
N_MESSAGES_ARRAY=( 100000  100000  100000  100000  100000  50000  50000  30000  30000  20000   15000   15000   10000  10000  10000   10000   10000  10000  10000  3000 )
array_size=${#MSG_SIZE_ARRAY[@]}
array_size=$(( $array_size - 1 ))

# In case we have already run this today, for debugging purposes or whatever...
rm -f ${RESULT_ROOT}/receiver/*
rm -f ${RESULT_ROOT}/sender/*


#--------------------------------------------------------
# Run the test once for each message size, varying the 
# number of messages sent so that it does not take for
# frikking ever to run.
#--------------------------------------------------------
for i in $(seq 0 $array_size)
do

  MSG_SIZE=${MSG_SIZE_ARRAY[$i]}
  N_MESSAGES=${N_MESSAGES_ARRAY[$i]}
  REPORT_FREQUENCY=$N_MESSAGES
  
  echo "========================================================"
  echo "message size test $MSG_SIZE   n_messages  $N_MESSAGES"
  echo "========================================================"

  echo " "
  echo "starting ${RECV}"
  taskset -c ${CPU_A} ${RECV} ${TIMESTAMPING}            \
                              -c ${N_MESSAGES}           \
                              -r ${REPORT_FREQUENCY}     \
                              -d ${RESULT_ROOT}/receiver \
			      -s ${MSG_SIZE}             \
			      -print_message_size   &
  RECV_PID=$!

  sleep 2

  echo "running ${SEND}"
  taskset -c ${CPU_B} ${SEND} ${TIMESTAMPING}          \
                              -c ${N_MESSAGES}         \
                              -r ${REPORT_FREQUENCY}   \
                              -s ${MSG_SIZE}           \
                              -d ${RESULT_ROOT}/sender \
			      -print_message_size  &
  SEND_PID=$!


  echo " "
  echo "waiting for test to complete."
  wait


  #-------------------------------------------------------------
  # And keep the whole report around, in a list of reports.
  #-------------------------------------------------------------
  if [ $i -eq 0 ]
  then
    cp ${RESULT_ROOT}/receiver/report.txt ${RESULT_ROOT}/receiver/tp_report.txt
    rm ${RESULT_ROOT}/receiver/report.txt
    #mv ${RESULT_ROOT}/receiver/report.txt ${RESULT_ROOT}/receiver/report_$i.txt
    cp ${RESULT_ROOT}/sender/report.txt   ${RESULT_ROOT}/sender/tp_report.txt
    rm ${RESULT_ROOT}/sender/report.txt
    #mv ${RESULT_ROOT}/sender/report.txt   ${RESULT_ROOT}/sender/report_$i.txt
  else
    cat ${RESULT_ROOT}/receiver/report.txt | tail -1 >> ${RESULT_ROOT}/receiver/tp_report.txt
    #mv  ${RESULT_ROOT}/receiver/report.txt ${RESULT_ROOT}/receiver/report_$i.txt
    rm ${RESULT_ROOT}/receiver/report.txt
    cat ${RESULT_ROOT}/sender/report.txt   | tail -1 >> ${RESULT_ROOT}/sender/tp_report.txt
    #mv  ${RESULT_ROOT}/sender/report.txt   ${RESULT_ROOT}/sender/report_$i.txt
    rm ${RESULT_ROOT}/sender/report.txt
  fi
done





echo " "
echo " "
echo -n "done at "
date
echo " "
echo " "



