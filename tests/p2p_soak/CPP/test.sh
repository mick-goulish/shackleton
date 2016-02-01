#! /bin/bash




TEST_NAME=p2p_soak
LANGUAGE=CPP



YEAR=$1
MONTH=$2
DAY=$3


pkill direct_send
pkill simple_recv


APP_ROOT=${SHACKLETON_ROOT}/apps/CPP

SEND=${APP_ROOT}/direct_send
RECV=${APP_ROOT}/simple_recv

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
# And are clean.
#-----------------------------------------
rm    -rf ${RESULT_ROOT}/receiver
rm    -rf ${RESULT_ROOT}/sender
mkdir -p  ${RESULT_ROOT}/receiver
mkdir -p  ${RESULT_ROOT}/sender




#--------------------------
# test parameters
#--------------------------

#----------------------------------------
# Production
#----------------------------------------
#N_MESSAGES=10000000
#REPORT_FREQUENCY=100000

#----------------------------------------
# Dev
#----------------------------------------
N_MESSAGES=1000000
REPORT_FREQUENCY=100000



echo "running ${SEND}"
taskset -c ${CPU_B} ${SEND} -m ${N_MESSAGES}       \
                            -f ${REPORT_FREQUENCY} \
                            -q 1                   \
                            -d ${RESULT_ROOT}/sender &
SEND_PID=$!

sleep 2

echo " "
echo "starting ${RECV}"
taskset -c ${CPU_A} ${RECV} -m ${N_MESSAGES}       \
                            -f ${REPORT_FREQUENCY} \
                            -q 1                   \
                            -d ${RESULT_ROOT}/receiver &
RECV_PID=$!



echo " "
echo "soak: waiting for test to complete."
wait



#------------------------------------------------------------------------------
# receiver report text processing
# move the report.txt file to a different name to avoid collision 
# with the latency test
# These report files will be generated with names like tp_report_PID.txt
# In this test, there will be only one of these.  Move it to the final name.
#------------------------------------------------------------------------------
mv ${RESULT_ROOT}/receiver/tp_report*.txt ${RESULT_ROOT}/receiver/tp_report.txt


#--------------------------------------------------------------
#  sender report text processing
#--------------------------------------------------------------
mv ${RESULT_ROOT}/sender/tp_report*.txt ${RESULT_ROOT}/sender/tp_report.txt




echo " "
echo " "
echo -n "soak: done at "
date
echo " "
echo " "



