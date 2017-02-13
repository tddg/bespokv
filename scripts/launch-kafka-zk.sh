#!/usr/bin/env bash

pkill -9 -f kafka
pkill -9 -f zookeeper
rm -rf /tmp/zookeeper/
rm -rf /tmp/kafka-logs/
sleep 1
/root/ClusterOn/ali/kafka_2.11-0.10.0.0/bin/zookeeper-server-start.sh /root/ClusterOn/ali/kafka_2.11-0.10.0.0/config/zookeeper.properties &
sleep 1
/root/ClusterOn/ali/kafka_2.11-0.10.0.0/bin/kafka-server-start.sh /root/ClusterOn/ali/kafka_2.11-0.10.0.0/config/server.properties &