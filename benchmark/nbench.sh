#!/bin/bash

export CLIENTS=96
export BSIZE=1024


if [ "${1}x" != "x" ]
then
  CLIENTS=$1
fi

if [ "${2}x" != "x" ]
then
  BSIZE=$2
fi


echo Running $CLIENTS echo-processes with $BSIZE bytes large messages

rm -f log.*; let j=0;while [ $j -lt $CLIENTS ]; do ./benchmark ws://localhost:5083/echo $BSIZE >log.$j & let j++; done 


sleep 10

while [ 1 ]
do
  egrep "^Mess" log.* | sed -e 's/ms$//g' | awk -v rpsavg=0 -v latency_avg=0 -v clients=$CLIENTS -v bsize=$BSIZE '{ rpsavg=rpsavg+($2-rpsavg)/(NR);latency_avg=latency_avg+($4-latency_avg)/(NR);}END{print "clients:",clients,"total rps:", (rpsavg/latency_avg)*1000*clients, "rp-per-snapshot:", rpsavg, "avg-snap-time:", latency_avg, "rps/c:", (rpsavg/latency_avg)*1000, "MBit/s:", ((bsize*(rpsavg/latency_avg)*1000)*clients*8)/1000000}';
  sleep 10
done
