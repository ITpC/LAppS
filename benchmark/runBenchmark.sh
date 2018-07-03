#!/bin/bash

export CLIENTS=96
export BSIZE=1024
export URI="wss://localhost:5083/echo"

echo "Usage: $0 [clients [block size [URI]]]"

if [ -f ./echo_client_tls.cpp -a ! -f ./benchmark_tls ]
then
  g++ -pthread -O3 -std=c++14 echo_client_tls.cpp -o benchmark_tls -lboost_system -lcrypto -lssl
fi

LAPPS_PID=$(ps ax | grep lapps | grep -v grep | awk '{print $1}')


if [ "${1}x" != "x" ]
then
  CLIENTS=$1
fi

if [ "${2}x" != "x" ]
then
  BSIZE=$2
fi

if [ "${3}x" != "x" ]
then
  URI=$3
fi

if [ "${LAPPS_PID}x" == "x" ]
then
  echo LAPPS is not started on localhost. benchmarking uri is $URI 
fi

echo Running $CLIENTS echo-processes with $BSIZE bytes large messages

rm -f log.*; let j=0;while [ $j -lt $CLIENTS ]; do ./benchmark_tls $URI $BSIZE >log.$j & let j++; done 


sleep 10

let j=0;
while [ $j -lt 300 ]
do
  egrep "^Mess" log.* | sed -e 's/ms$//g' | awk -v rpsavg=0 -v latency_avg=0 -v clients=$CLIENTS -v bsize=$BSIZE '{ rpsavg=rpsavg+($2-rpsavg)/(NR);latency_avg=latency_avg+($4-latency_avg)/(NR);}END{print "clients:",clients,"total rps:", (rpsavg/latency_avg)*1000*clients, "rp-per-snapshot:", rpsavg, "avg-snap-time:", latency_avg, "rps/c:", (rpsavg/latency_avg)*1000, "MBit/s:", ((bsize*(rpsavg/latency_avg)*1000)*clients*8)/1000000}';
  sleep 10
  let j+=10
done

pkill benchmark_tls

LAPPS_PID=$(ps ax | grep lapps | egrep -v "grep|gdb" | awk '{print $1}')

if [ "${LAPPS_PID}x" != "x" ]
then
  kill -15 $LAPPS_PID
else
  echo "No LAppS to stop"
fi

egrep "^Mess" log.* | sed -e 's/ms$//g' | awk -v rpsavg=0 -v latency_avg=0 -v clients=$CLIENTS -v bsize=$BSIZE '{ rpsavg=rpsavg+($2-rpsavg)/(NR);latency_avg=latency_avg+($4-latency_avg)/(NR);}END{print "clients:",clients,"total rps:", (rpsavg/latency_avg)*1000*clients, "rp-per-snapshot:", rpsavg, "avg-snap-time:", latency_avg, "rps/c:", (rpsavg/latency_avg)*1000, "MBit/s:", ((bsize*(rpsavg/latency_avg)*1000)*clients*8)/1000000}';
