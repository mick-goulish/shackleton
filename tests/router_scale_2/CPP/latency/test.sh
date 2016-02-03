#! /bin/bash

YEAR=$1
MONTH=$2
DAY=$3


#--------------------------------------------------------------
# Production
#--------------------------------------------------------------
echo "running in production mode."
CLIENT_PAIRS_ARRAY=( 100 200 500 1000 2000 3000 4000 5000 )

#--------------------------------------------------------------
# Development
#--------------------------------------------------------------
#echo "running in development mode."
#CLIENT_PAIRS_ARRAY=( 10 20 50 100 200 )


array_size=${#CLIENT_PAIRS_ARRAY[@]}
array_size=$(( $array_size - 1 ))


for i in $(seq 0 $array_size)
do
  client_pairs=${CLIENT_PAIRS_ARRAY[$i]}
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  echo "###################################################################"
  echo "  Running router_scale_2 test for ${client_pairs} client pairs."
  echo "###################################################################"
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  ./test_1.sh ${YEAR} ${MONTH} ${DAY} ${client_pairs}
done


