#! /bin/bash



YEAR=$1
MONTH=$2
DAY=$3



echo " "
echo "========================================================="
echo "  Shackleton just_run_tests   ${YEAR}  ${MONTH}  ${DAY}"
echo -n "  "
date
echo "========================================================="
echo " "



echo " "
echo " "
echo " "
echo "  Shackleton run tests ------------------------------"
echo " "
echo " "

tests=()
tests+=("${SHACKLETON_ROOT}/tests/p2p_soak/C/throughput")
tests+=("${SHACKLETON_ROOT}/tests/p2p_soak/C/latency")
tests+=("${SHACKLETON_ROOT}/tests/message_size/C/throughput")
tests+=("${SHACKLETON_ROOT}/tests/router_1/CPP/throughput")
tests+=("${SHACKLETON_ROOT}/tests/router_soak/CPP/resources")
tests+=("${SHACKLETON_ROOT}/tests/router_scale/CPP/resources")

echo "router_soak test removed from list 05 Jan 2016 pending investigation."

echo "tests will be: ${tests}"

size=${#tests[@]}
size=$(( $size - 1 ))

for i in $(seq 0 $size)
do
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  echo "============================================================="
  echo "Shackleton: start test ${i}  ${tests[$i]} "
  date
  echo "============================================================="
  cd ${tests[$i]}
  ./test.sh ${YEAR} ${MONTH} ${DAY}
  echo "============================================================="
  echo "Shackleton: end test ${i}  ${tests[$i]}  "
  date
  echo "============================================================="
  echo " "
  echo " "
  echo " "
done



echo " "
echo "---------------------------------------------------------"
echo "  Shackleton just_run_tests complete"
echo -n "  "
date
echo "---------------------------------------------------------"
echo " "
echo " "
echo " "

exit 0
