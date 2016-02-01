#! /bin/bash

PID=$1

while [ 1 ]
do
  echo "-----------------------------------"
  date
  top -b -p ${PID} -n 1
  sleep 15
done


