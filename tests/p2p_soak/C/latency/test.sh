#! /bin/bash


pkill reactor-recv
sleep 1
pkill reactor-send
sleep 1


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

TIMESTAMPING=-t

#-----------------------------------
# Production 
#-----------------------------------
N_MESSAGES=10000000
REPORT_FREQUENCY=100000

#-----------------------------------
# Dev 
#-----------------------------------
#N_MESSAGES=2000000
#REPORT_FREQUENCY=100000



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

# Since this is the latency-only test, we
# do not need the report from the sender.
rm ${RESULT_ROOT}/sender/report.txt





echo " "
echo " "
echo -n "soak: done at "
date
echo " "
echo " "



