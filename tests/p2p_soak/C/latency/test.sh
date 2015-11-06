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



TEST_NAME=p2p_soak
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
#  Latency Test
#  timestamp each message.
#===============================================================

MEASUREMENT=latency

echo "running test: ${TEST_NAME} ${LANGUAGE} ${MEASUREMENT}"

RESULT_ROOT=${SHACKLETON_ROOT}/results/tests/${YEAR}/${MONTH}/${DAY}/${TEST_NAME}/${LANGUAGE}
echo "reports being written to ${RESULT_ROOT}"


#-------------------------------------------------------
# Make sure the destination dirs exist.
# Only the receiver is involved in this latency test.
#-------------------------------------------------------
mkdir -p ${RESULT_ROOT}/receiver
mkdir -p ${RESULT_ROOT}/sender




#--------------------------
# test parameters
#--------------------------
N_MESSAGES=100000000
REPORT_FREQUENCY=1000000
TIMESTAMPING=-t



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
                            -d ${RESULT_ROOT}/sender &
SEND_PID=$!


echo " "
echo "soak: waiting for test to complete."
wait
echo "creating display graphics"



#--------------------------------------------------------------
#  report text processing for receiver only -- no point in
#  asking sender about latency.
#  Move the report.txt file to avoid collision with the 
#  throughput test.
#--------------------------------------------------------------
mv ${RESULT_ROOT}/receiver/report.txt ${RESULT_ROOT}/receiver/l_report.txt
RECV_REPORT=${RESULT_ROOT}/receiver/l_report.txt

cat ${RECV_REPORT}  |  awk '{print $2 " " $10}' > ${RESULT_ROOT}/receiver/latency.txt


cp ${RESULT_ROOT}/receiver/latency.txt  ./temp/receiver_latency.txt



#--------------------------------------------------------------
#  Make images for receiver
#--------------------------------------------------------------
gnuplot ./gnuplot_script


cp ./temp/receiver_latency.jpg   ${RESULT_ROOT}/receiver/latency.jpg

echo " "
echo " "
echo -n "soak: done at "
date
echo " "
echo " "



