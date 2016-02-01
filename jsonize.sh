#! /bin/bash

export SHACKLETON_ROOT=/home/mick/shackleton
HTML_ROOT=/var/www/html/perf

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


