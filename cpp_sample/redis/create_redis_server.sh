#/bin/bash

set -e


nodes=""
port=7000
while [ $port -lt 7100 ]; do
	redis-server --port $port --cluster-enabled yes  --cluster-config-file nodes-$port.conf   --cluster-node-timeout 5000 --daemonize yes
	nodes="$nodes 127.0.0.1:$port"
	port=$(($port+1))
done

sleep 1
redis-cli --cluster create $nodes --cluster-replicas 1

port=7001
while [ $port -lt 7100 ]; do
	redis-cli -h 127.0.0.1 -p 7000 cluster meet 127.0.0.1 $port
	port=$((port+1))
done

redis-cli --cluster fix 127.0.0.1:7000

