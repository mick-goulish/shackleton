#! /bin/bash

export SHACKLETON_ROOT=/home/mick/shackleton

echo -n "Shackleton run starting at " ; date

YEAR=`date +%Y`
MONTH=`date +%m`
DAY=`date +%d`

${SHACKLETON_ROOT}/run.sh ${YEAR} ${MONTH} ${DAY}

echo -n "Shackleton run ending at " ; date
