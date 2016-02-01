#! /bin/bash




TEST_NAME=p2p_soak
LANGUAGE=C



YEAR=$1
MONTH=$2
DAY=$3


pkill reactor-send
pkill reactor-recv


APP_ROOT=${SHACKLETON_ROOT}/apps/C

SEND=${APP_ROOT}/reactor-send
RECV=${APP_ROOT}/reactor-recv

CPU_A=1
CPU_B=2


#===============================================================
#  Throughput Test
#  In the pure throughput test, we do not 
#  timestamp messages.
#===============================================================

RESULT_ROOT=${SHACKLETON_ROOT}/results/tests/${YEAR}/${MONTH}/${DAY}/${TEST_NAME}/${LANGUAGE}
echo "reports being written to ${RESULT_ROOT}"

#-----------------------------------------
# Make sure the destination dirs exist.
#-----------------------------------------
mkdir -p ${RESULT_ROOT}/receiver
mkdir -p ${RESULT_ROOT}/sender




#--------------------------
# test parameters
#--------------------------

#----------------------------------------
# Production
#----------------------------------------
N_MESSAGES=100000000
REPORT_FREQUENCY=1000000
TIMESTAMPING=

#----------------------------------------
# Dev
#----------------------------------------
#N_MESSAGES=5000000
#REPORT_FREQUENCY=100000
#TIMESTAMPING=



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



mv ${RESULT_ROOT}/receiver/report.txt ${RESULT_ROOT}/receiver/tp_report.txt
mv ${RESULT_ROOT}/sender/report.txt ${RESULT_ROOT}/sender/tp_report.txt




echo " "
echo " "
echo -n "soak: done at "
date
echo " "
echo " "



