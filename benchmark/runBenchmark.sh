#!/bin/bash

if [ -f ./echo_client_tls.cpp -a ! -f ./benchmark_tls ]
then
  g++ -pthread -O3 -std=c++14 echo_client_tls.cpp -o benchmark_tls -lboost_system -lcrypto -lssl
fi

if [ -f ./echo_client.cpp -a ! -f ./benchmark_notls ]
then
  g++ -pthread -O3 -std=c++14 echo_client.cpp -o benchmark_notls -lboost_system
fi

export clients=80

if [ "${1}x" != "x" ]
then
  clients=$1
fi

rm log.*; let j=0; while [ $j -lt $clients ]; do ./benchmark_tls > log.$j & let j++; done

sleep 3;

clients_started=$(ps ax | grep benchmark | grep -v grep | wc -l)

echo clients started: $clients_started

while [ 1 ]
do
  count_bms=$(ps ax | grep benchmark | grep -v grep | wc -l)
  if [ $count_bms -eq 0 ]
  then
    break;
  else
    sleep 1;
  fi
done

echo "All results bellow are for ${clients_started} clients"
echo

grep tabdata log.* | awk '{print $2, $3}' | awk -v clients=$clients_started '{count[$1]++; avg[$1]=($2+(count[$1]-1)*avg[$1])/count[$1]}END{for(i in avg) print avg[i]*count[i], i, avg[i], i*avg[i]*count[i]/1024/1024*8 " Mbit/s, clients: " count[i]}' | sort -nr
