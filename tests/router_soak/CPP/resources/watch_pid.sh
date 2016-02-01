#! /bin/bash                                                                    
                                                                                
                                                                                
PID=$1                                                                          
                                                                                
total_time=0

doze=10

while [ 1 ]                                                                     
do                                                                              
  # example output of top:
  #   top -b -p ${PID} -n 1 | tail -2
  #   PID   USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
  #   13363 mick      20   0  151548   7952   5356 S   0.0  0.1   0:00.21 vim

  echo -n "time: ${total_time} "
  top -b -p ${PID} -n 1 | tail -1 | awk '{print "mem: " $6 " cpu: " $9}'
  sleep $doze                                                                      
  let total_time+=${doze}
done                                                                            

