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

#for i in $(seq 0 $size)
#do
  #echo "test ${i}  ${tests[$i]} ------------------------------ "
  #cd ${tests[$i]}
  #./test.sh ${YEAR} ${MONTH} ${DAY}
  #echo "end test ${i}  ${tests[$i]} ------------------------------ "
#done



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
