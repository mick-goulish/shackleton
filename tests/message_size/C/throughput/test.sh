#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#



#! /bin/bash


#-------------------------------
# Make old stuff go away.
#-------------------------------
rm -f ./temp/*



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


#===============================================================
#===============================================================

echo "running test: ${TEST_NAME} ${LANGUAGE} ${MEASUREMENT}"

RESULT_ROOT=${SHACKLETON_ROOT}/results/tests/${YEAR}/${MONTH}/${DAY}/${TEST_NAME}/${LANGUAGE}
echo "reports being written to ${RESULT_ROOT}"

#-----------------------------------------
# Make sure the destination dirs exist.
#-----------------------------------------
mkdir -p ${RESULT_ROOT}/receiver
mkdir -p ${RESULT_ROOT}/sender


# In case it's already there, during testing...
rm -f ${RESULT_ROOT}/receiver/all_results

#--------------------------
# test parameters
#--------------------------
N_MESSAGES=2000000
REPORT_FREQUENCY=2000000
TIMESTAMPING=


MSG_SIZE_ARRAY=(       100     200       500     1000     2000     5000     7000     8000     9000    10000    11000    12000    15000   17000   20000    25000    30000   50000   70000  100000 )
N_MESSAGES_ARRAY=( 2000000  2000000  2000000  2000000  2000000  1000000  1000000  1000000  1000000  1000000   900000   800000   700000  500000  500000   500000   400000  400000  250000  100000 )
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
  taskset -c ${CPU_A} ${RECV} ${TIMESTAMPING}        \
                              -c ${N_MESSAGES}       \
                              -r ${REPORT_FREQUENCY} \
                              -d ${RESULT_ROOT}/receiver &
  RECV_PID=$!

  sleep 2

  echo "running ${SEND}"
  taskset -c ${CPU_B} ${SEND} ${TIMESTAMPING}        \
                              -c ${N_MESSAGES}       \
                              -r ${REPORT_FREQUENCY} \
                              -s ${MSG_SIZE}         \
                              -d ${RESULT_ROOT}/sender &
  SEND_PID=$!


  echo " "
  echo "waiting for test to complete."
  wait


  #----------------------------------------------------------------------------------
  # Save just the message size and throughput from this result in "all_results".
  #----------------------------------------------------------------------------------

  # receiver -----
  TP=`cat ${RESULT_ROOT}/receiver/report.txt | awk '{print $8}'`
  echo "${MSG_SIZE}  ${TP}" >>  ${RESULT_ROOT}/receiver/all_results

  # sender -----
  TP=`cat ${RESULT_ROOT}/sender/report.txt   | awk '{print $8}'`
  echo "${MSG_SIZE}  ${TP}" >>  ${RESULT_ROOT}/sender/all_results



  #-------------------------------------------------------------
  # And keep the whole report around, in a list of reports.
  #-------------------------------------------------------------
  report=`cat ${RESULT_ROOT}/receiver/report.txt`
  echo "msg_size: ${MSG_SIZE}  ${report}"  >> ${RESULT_ROOT}/receiver/all_reports

  report=`cat ${RESULT_ROOT}/sender/report.txt`
  echo "msg_size: ${MSG_SIZE}  ${report}"  >> ${RESULT_ROOT}/sender/all_reports
done



# receiver text processing --------------------------------------------
cp ${RESULT_ROOT}/receiver/all_results  ./recv_all_results
cp ${RESULT_ROOT}/receiver/all_reports  ./recv_all_reports

cat recv_all_results  | awk '{print $1 " * " $2}' | bc >  bytes_per_second
cat recv_all_reports  | awk '{print $2 " " $6}' >  recv_msg_size_vs_cpu

paste recv_all_results bytes_per_second | awk '{print $1 " " $3}' > message-size_bps



# sender text processing --------------------------------------------
cp ${RESULT_ROOT}/sender/all_reports  ./send_all_reports

cat send_all_reports  | awk '{print $2 " " $6}' >  send_msg_size_vs_cpu



# graphics processing ---------------------------

gnuplot ./gnuplot_script

cp r*jpg ${RESULT_ROOT}/receiver
cp s*jpg ${RESULT_ROOT}/sender



echo " "
echo " "
echo -n "done at "
date
echo " "
echo " "



