#! /bin/bash



YEAR=$1
MONTH=$2
DAY=$3


echo " "
echo "========================================================="
echo "  Shackleton run"
echo -n "  "
date
echo "========================================================="
echo " "

echo " "
echo " "
echo " "
echo "  Shackleton run build_libs ------------------------------"
echo " "
echo " "
${SHACKLETON_ROOT}/libs/rebuild.sh
status=$?
if [ ! $status -eq 0 ]
then
  echo "Shackleton run: rebuild failed."
  exit $status
fi


echo " "
echo " "
echo " "
echo "  Shackleton run build_apps ------------------------------"
echo " "
echo " "
cd ${SHACKLETON_ROOT}/apps/C
./m
status=$?
if [ ! $status -eq 0 ]
then
  echo "Shackleton run: apps build failed."
  exit $status
fi


echo " "
echo " "
echo " "
echo "  Shackleton run build_apps ------------------------------"
echo " "
echo " "
cd ${SHACKLETON_ROOT}/apps/CPP
./m
status=$?
if [ ! $status -eq 0 ]
then
  echo "Shackleton run: apps build failed."
  exit $status
fi


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





HTML_ROOT=/var/www/html/perf
WORKING_DESTINATION=${SHACKLETON_ROOT}/results/tests/${YEAR}/${MONTH}/${DAY}
FINAL_DESTINATION=${HTML_ROOT}/results/tests/${YEAR}/${MONTH}/${DAY}
echo " "
echo " "
echo " "
echo "---------------------------------------------------------"
echo "  Shackleton: Copying results."
echo "    from ${WORKING_DESTINATION}"
echo "    to ${FINAL_DESTINATION}"
echo -n "  "
date
echo "---------------------------------------------------------"
echo " "
echo " "

mkdir -p ${FINAL_DESTINATION}
cp -pr ${WORKING_DESTINATION}/* ${FINAL_DESTINATION}





JSONIZE=${SHACKLETON_ROOT}/utils/mjson/jsonize_tree/jsonize_tree
JSONIZE_TARGET=${HTML_ROOT}/results/tests
JSONIZE_DESTINATION=${HTML_ROOT}/results/results.json
echo " "
echo " "
echo " "
echo "---------------------------------------------------------"
echo "  Jsonizing results directory"
echo "    target:      ${JSONIZE_TARGET}"
echo "    destination: ${JSONIZE_DESTINATION}"
echo -n "  "
date
echo "---------------------------------------------------------"
echo " "
echo " "
${JSONIZE} ${JSONIZE_TARGET} > ${JSONIZE_DESTINATION}





echo " "
echo " "
echo "---------------------------------------------------------"
echo "  Shackleton run complete"
echo -n "  "
date
echo "---------------------------------------------------------"
echo " "
echo " "
echo " "

exit 0
