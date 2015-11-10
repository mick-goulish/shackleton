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
echo "  Shackleton run tests ------------------------------"
echo " "
echo " "

tests=()
tests+=("${SHACKLETON_ROOT}/tests/p2p_soak/C/throughput")
tests+=("${SHACKLETON_ROOT}/tests/p2p_soak/C/latency")
tests+=("${SHACKLETON_ROOT}/tests/message_size/C/throughput")

size=${#tests[@]}
size=$(( $size - 1 ))

for i in $(seq 0 $size)
do
  echo "start test ${i}  ${tests[$i]} ------------------------------ "
  cd ${tests[$i]}
  ./test.sh ${YEAR} ${MONTH} ${DAY}
  echo "end test ${i}  ${tests[$i]} ------------------------------ "
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
echo "  Copying results."
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
JSONIZE_TARGET=${HTML_ROOT}/results/tests/${YEAR}
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
