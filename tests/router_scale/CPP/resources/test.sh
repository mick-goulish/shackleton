#! /bin/bash

YEAR=$1
MONTH=$2
DAY=$3


#--------------------------------------------------------------
# Production
#--------------------------------------------------------------
echo "running in production mode."
CLIENT_PAIRS_ARRAY=( 10 20 50 100 200 )

#--------------------------------------------------------------
# Development
#--------------------------------------------------------------
#echo "running in development mode."
#CLIENT_PAIRS_ARRAY=( 10 20 50 100 200 )


array_size=${#CLIENT_PAIRS_ARRAY[@]}
array_size=$(( $array_size - 1 ))


#----------------------------------------------------------
# Bug Alert : the following few lines are copied from 
# the underlying test which this script calls.
#----------------------------------------------------------
TEST_NAME=router_scale
LANGUAGE=CPP
RESULT_ROOT=${SHACKLETON_ROOT}/results/tests/${YEAR}/${MONTH}/${DAY}/${TEST_NAME}/${LANGUAGE}

rm ${RESULT_ROOT}/router/tp_report.txt
echo -e "client_pairs\tmem" > ${RESULT_ROOT}/router/tp_report.txt

echo "started file ${RESULT_ROOT}/router/tp_report.txt"


for i in $(seq 0 $array_size)
do
  client_pairs=${CLIENT_PAIRS_ARRAY[$i]}
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  echo "###################################################################"
  echo "  Running router_scale test for ${client_pairs} client pairs."
  echo "###################################################################"
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  ./test_1.sh ${YEAR} ${MONTH} ${DAY} ${client_pairs}
done

