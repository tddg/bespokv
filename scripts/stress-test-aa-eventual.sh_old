#!/usr/bin/env bash

HOME=~/ClusterOn/ali
CON_HOME=$HOME/conproxy

function run_ckv {
SERVER=$1
PORT=$2
#NUM_PROC=$3
JOB_DIR=$HOME/conrun/con_jobs
JOBFILE=_runckv.job
BIN=${CON_HOME}/apps/ckv/conkv
mkdir -p ${JOB_DIR}
rm ${JOB_DIR}/$JOBFILE

echo "$SERVER: $BIN -t 1 -l localhost -p ${PORT}"
echo "LD_LIBRARY_PATH=/usr/local/lib $BIN -t 1 -l localhost -p ${PORT}" \
> ${JOB_DIR}/$JOBFILE
parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function run_ckv_wrapper {
run_ckv hulk0 11111 &
sleep 0.5
run_ckv hulk0 11112 &
sleep 0.5
run_ckv hulk0 11113 &
sleep 0.5

run_ckv hulk0 11114 &
sleep 0.5
run_ckv hulk0 11115 &
sleep 0.5
run_ckv hulk0 11116 &
sleep 0.5

run_ckv hulk0 11117 &
sleep 0.5
run_ckv hulk0 11118 &
sleep 0.5
run_ckv hulk0 11119 &
sleep 0.5

run_ckv hulk0 11121 &
sleep 0.5
run_ckv hulk0 11122 &
sleep 0.5
run_ckv hulk0 11123 &
sleep 0.5
#
#run_ckv hulk0 11113 &
#sleep 0.5
#run_ckv hulk0 11114 &
#sleep 0.5
#run_ckv hulk0 11115 &
#sleep 0.5
#
#run_ckv hulk0 11116 &
#sleep 0.5
#run_ckv hulk0 11117 &
#sleep 0.5
#run_ckv hulk0 11118 &
#sleep 0.5
#
#run_ckv hulk0 11119 &
#sleep 0.5
#run_ckv hulk0 11120 &
#sleep 0.5
#run_ckv hulk0 11121 &
#sleep 0.5
#
#run_ckv hulk0 11122 &
#sleep 0.5
#run_ckv hulk0 11123 &
#sleep 0.5
#run_ckv hulk0 11124 &
#sleep 0.5
#

run_ckv hulk1 11111 &
sleep 0.5
run_ckv hulk1 11112 &
sleep 0.5
run_ckv hulk1 11113 &
sleep 0.5

run_ckv hulk1 11114 &
sleep 0.5
run_ckv hulk1 11115 &
sleep 0.5
run_ckv hulk1 11116 &
sleep 0.5

run_ckv hulk1 11117 &
sleep 0.5
run_ckv hulk1 11118 &
sleep 0.5
run_ckv hulk1 11119 &
sleep 0.5

run_ckv hulk1 11121 &
sleep 0.5
run_ckv hulk1 11122 &
sleep 0.5
run_ckv hulk1 11123 &
sleep 0.5

#run_ckv hulk1 11113 &
#sleep 0.5
#run_ckv hulk1 11114 &
#sleep 0.5
#run_ckv hulk1 11115 &
#sleep 0.5
#
#run_ckv hulk1 11116 &
#sleep 0.5
#run_ckv hulk1 11117 &
#sleep 0.5
#run_ckv hulk1 11118 &
#sleep 0.5
#
#run_ckv hulk1 11119 &
#sleep 0.5
#run_ckv hulk1 11120 &
#sleep 0.5
#run_ckv hulk1 11121 &
#sleep 0.5
#
#run_ckv hulk1 11122 &
#sleep 0.5
#run_ckv hulk1 11123 &
#sleep 0.5
#run_ckv hulk1 11124 &
#sleep 0.5


run_ckv hulk2 11111 &
sleep 0.5
run_ckv hulk2 11112 &
sleep 0.5
run_ckv hulk2 11113 &
sleep 0.5

run_ckv hulk2 11114 &
sleep 0.5
run_ckv hulk2 11115 &
sleep 0.5
run_ckv hulk2 11116 &
sleep 0.5

run_ckv hulk2 11117 &
sleep 0.5
run_ckv hulk2 11118 &
sleep 0.5
run_ckv hulk2 11119 &
sleep 0.5

run_ckv hulk2 11121 &
sleep 0.5
run_ckv hulk2 11122 &
sleep 0.5
run_ckv hulk2 11123 &
sleep 0.5

#run_ckv hulk2 11113 &
#sleep 0.5
#run_ckv hulk2 11114 &
#sleep 0.5
#run_ckv hulk2 11115 &
#sleep 0.5
#
#run_ckv hulk2 11116 &
#sleep 0.5
#run_ckv hulk2 11117 &
#sleep 0.5
#run_ckv hulk2 11118 &
#sleep 0.5
#
#run_ckv hulk2 11119 &
#sleep 0.5
#run_ckv hulk2 11120 &
#sleep 0.5
#run_ckv hulk2 11121 &
#sleep 0.5
#
#run_ckv hulk2 11122 &
#sleep 0.5
#run_ckv hulk2 11123 &
#sleep 0.5
#run_ckv hulk2 11124 &
#sleep 0.5
#
run_ckv hulk3 11111 &
sleep 0.5
run_ckv hulk3 11112 &
sleep 0.5
run_ckv hulk3 11113 &
sleep 0.5

run_ckv hulk3 11114 &
sleep 0.5
run_ckv hulk3 11115 &
sleep 0.5
run_ckv hulk3 11116 &
sleep 0.5

run_ckv hulk3 11117 &
sleep 0.5
run_ckv hulk3 11118 &
sleep 0.5
run_ckv hulk3 11119 &
sleep 0.5

run_ckv hulk3 11121 &
sleep 0.5
run_ckv hulk3 11122 &
sleep 0.5
run_ckv hulk3 11123 &
sleep 0.5

#run_ckv hulk3 11113 &
#sleep 0.5
#run_ckv hulk3 11114 &
#sleep 0.5
#run_ckv hulk3 11115 &
#sleep 0.5
#
#run_ckv hulk3 11116 &
#sleep 0.5
#run_ckv hulk3 11117 &
#sleep 0.5
#run_ckv hulk3 11118 &
#sleep 0.5
#
#run_ckv hulk3 11119 &
#sleep 0.5
#run_ckv hulk3 11120 &
#sleep 0.5
#run_ckv hulk3 11121 &
#sleep 0.5
#
#run_ckv hulk3 11122 &
#sleep 0.5
#run_ckv hulk3 11123 &
#sleep 0.5
#run_ckv hulk3 11124 &
#sleep 0.5
#

run_ckv hulk4 11111 &
sleep 0.5
run_ckv hulk4 11112 &
sleep 0.5
run_ckv hulk4 11113 &
sleep 0.5

run_ckv hulk4 11114 &
sleep 0.5
run_ckv hulk4 11115 &
sleep 0.5
run_ckv hulk4 11116 &
sleep 0.5

run_ckv hulk4 11117 &
sleep 0.5
run_ckv hulk4 11118 &
sleep 0.5
run_ckv hulk4 11119 &
sleep 0.5

run_ckv hulk4 11121 &
sleep 0.5
run_ckv hulk4 11122 &
sleep 0.5
run_ckv hulk4 11123 &
sleep 0.5

#run_ckv hulk4 11113 &
#sleep 0.5
#run_ckv hulk4 11114 &
#sleep 0.5
#run_ckv hulk4 11115 &
#sleep 0.5
#
#run_ckv hulk4 11116 &
#sleep 0.5
#run_ckv hulk4 11117 &
#sleep 0.5
#run_ckv hulk4 11118 &
#sleep 0.5
#
#run_ckv hulk4 11119 &
#sleep 0.5
#run_ckv hulk4 11120 &
#sleep 0.5
#run_ckv hulk4 11121 &
#sleep 0.5
#
#run_ckv hulk4 11122 &
#sleep 0.5
#run_ckv hulk4 11123 &
#sleep 0.5
#run_ckv hulk4 11124 &
#sleep 0.5
#
run_ckv hulk5 11111 &
sleep 0.5
run_ckv hulk5 11112 &
sleep 0.5
run_ckv hulk5 11113 &
sleep 0.5

run_ckv hulk5 11114 &
sleep 0.5
run_ckv hulk5 11115 &
sleep 0.5
run_ckv hulk5 11116 &
sleep 0.5

run_ckv hulk5 11117 &
sleep 0.5
run_ckv hulk5 11118 &
sleep 0.5
run_ckv hulk5 11119 &
sleep 0.5

run_ckv hulk5 11121 &
sleep 0.5
run_ckv hulk5 11122 &
sleep 0.5
run_ckv hulk5 11123 &
sleep 0.5

#run_ckv hulk5 11113 &
#sleep 0.5
#run_ckv hulk5 11114 &
#sleep 0.5
#run_ckv hulk5 11115 &
#sleep 0.5
#
#run_ckv hulk5 11116 &
#sleep 0.5
#run_ckv hulk5 11117 &
#sleep 0.5
#run_ckv hulk5 11118 &
#sleep 0.5
#
#run_ckv hulk5 11119 &
#sleep 0.5
#run_ckv hulk5 11120 &
#sleep 0.5
#run_ckv hulk5 11121 &
#sleep 0.5
#
#run_ckv hulk5 11122 &
#sleep 0.5
#run_ckv hulk5 11123 &
#sleep 0.5
#run_ckv hulk5 11124 &
#sleep 0.5

}

function kill_conkv {
SERVER=$1
JOB_DIR=$HOME/conrun/con_jobs
JOBFILE=_killckv.job
mkdir -p ${JOB_DIR}
rm ${JOB_DIR}/$JOBFILE
echo "pkill -9 conkv" >> ${JOB_DIR}/$JOBFILE
parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function kill_ckv_wrapper {
kill_conkv hulk0
sleep 0.5
kill_conkv hulk1
sleep 0.5
kill_conkv hulk2
sleep 0.5
kill_conkv hulk3
sleep 0.5
kill_conkv hulk4
sleep 0.5
kill_conkv hulk5
sleep 0.5
}

function run_clusteron {
SERVER=$1
PORT=$2
#SHARD_PREFIX=$3
SHARD=$3
CONF=$4
#DL_PREFIX=$5
DL=$5
#NUM_PROC=$6
CONF_DIR=$HOME/conrun/conf
JOB_DIR=$HOME/conrun/con_jobs
JOBFILE=_runcon.job
BIN=${CON_HOME}/src/conproxy
mkdir -p ${CONF_DIR}
mkdir -p ${JOB_DIR}
rm ${JOB_DIR}/$JOBFILE

#SERVER_PORT=`echo "scale=0;$PORT+$id"|bc -l`
#SERVER_PORT=$(($PORT+$id))

echo "$SERVER: $BIN --config $CONF_DIR/$CONF --datalets $CONF_DIR/${DL} \
--shard ${SHARD} --proxyAddr $SERVER --proxyClientPort ${PORT}" 
echo "LD_LIBRARY_PATH=/usr/local/lib:$HOME/conproxy/src/redlock-cpp/hiredis:$HOME/conproxy/src/kafka/src:$HOME/conproxy/src/kafka/src-cpp:/usr/lib \
$BIN --config $CONF_DIR/$CONF --datalets $CONF_DIR/${DL} \
--shard ${SHARD} --proxyAddr $SERVER --proxyClientPort ${PORT}" \
>> ${JOB_DIR}/$JOBFILE
parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function run_clusteron_wrapper {
# strong
run_clusteron hulk0 12345 aa1 aa-ev.json aa-d0 &
sleep 0.5
run_clusteron hulk0 12346 aa1 aa-ev.json aa-d1 &
sleep 0.5
run_clusteron hulk0 12347 aa1 aa-ev.json aa-d2 &
sleep 0.5

run_clusteron hulk0 12355 aa1 aa-ev.json aa-d3 &
sleep 0.5
run_clusteron hulk0 12356 aa1 aa-ev.json aa-d4 &
sleep 0.5
run_clusteron hulk0 12357 aa1 aa-ev.json aa-d5 &
sleep 0.5

run_clusteron hulk0 12365 aa1 aa-ev.json aa-d6 &
sleep 0.5
run_clusteron hulk0 12366 aa1 aa-ev.json aa-d7 &
sleep 0.5
run_clusteron hulk0 12367 aa1 aa-ev.json aa-d8 &
sleep 0.5

run_clusteron hulk0 12375 aa1 aa-ev.json aa-d9 &
sleep 0.5
run_clusteron hulk0 12376 aa1 aa-ev.json aa-d10 &
sleep 0.5
run_clusteron hulk0 12377 aa1 aa-ev.json aa-d11 &
sleep 0.5


run_clusteron hulk1 12345 aa2 aa-ev.json aa-d0 &
sleep 0.5
run_clusteron hulk1 12346 aa2 aa-ev.json aa-d1 &
sleep 0.5
run_clusteron hulk1 12347 aa2 aa-ev.json aa-d2 &
sleep 0.5

run_clusteron hulk1 12355 aa2 aa-ev.json aa-d3 &
sleep 0.5
run_clusteron hulk1 12356 aa2 aa-ev.json aa-d4 &
sleep 0.5
run_clusteron hulk1 12357 aa2 aa-ev.json aa-d5 &
sleep 0.5

run_clusteron hulk1 12365 aa2 aa-ev.json aa-d6 &
sleep 0.5
run_clusteron hulk1 12366 aa2 aa-ev.json aa-d7 &
sleep 0.5
run_clusteron hulk1 12367 aa2 aa-ev.json aa-d8 &
sleep 0.5

run_clusteron hulk1 12375 aa2 aa-ev.json aa-d9 &
sleep 0.5
run_clusteron hulk1 12376 aa2 aa-ev.json aa-d10 &
sleep 0.5
run_clusteron hulk1 12377 aa2 aa-ev.json aa-d11 &
sleep 0.5


run_clusteron hulk2 12345 aa3 aa-ev.json aa-d0 &
sleep 0.5
run_clusteron hulk2 12346 aa3 aa-ev.json aa-d1 &
sleep 0.5
run_clusteron hulk2 12347 aa3 aa-ev.json aa-d2 &
sleep 0.5

run_clusteron hulk2 12355 aa3 aa-ev.json aa-d3 &
sleep 0.5
run_clusteron hulk2 12356 aa3 aa-ev.json aa-d4 &
sleep 0.5
run_clusteron hulk2 12357 aa3 aa-ev.json aa-d5 &
sleep 0.5

run_clusteron hulk2 12365 aa3 aa-ev.json aa-d6 &
sleep 0.5
run_clusteron hulk2 12366 aa3 aa-ev.json aa-d7 &
sleep 0.5
run_clusteron hulk2 12367 aa3 aa-ev.json aa-d8 &
sleep 0.5

run_clusteron hulk2 12375 aa3 aa-ev.json aa-d9 &
sleep 0.5
run_clusteron hulk2 12376 aa3 aa-ev.json aa-d10 &
sleep 0.5
run_clusteron hulk2 12377 aa3 aa-ev.json aa-d11 &
sleep 0.5


run_clusteron hulk3 12345 aa4 aa-ev.json aa-d0 &
sleep 0.5
run_clusteron hulk3 12346 aa4 aa-ev.json aa-d1 &
sleep 0.5
run_clusteron hulk3 12347 aa4 aa-ev.json aa-d2 &
sleep 0.5

run_clusteron hulk3 12355 aa4 aa-ev.json aa-d3 &
sleep 0.5
run_clusteron hulk3 12356 aa4 aa-ev.json aa-d4 &
sleep 0.5
run_clusteron hulk3 12357 aa4 aa-ev.json aa-d5 &
sleep 0.5

run_clusteron hulk3 12365 aa4 aa-ev.json aa-d6 &
sleep 0.5
run_clusteron hulk3 12366 aa4 aa-ev.json aa-d7 &
sleep 0.5
run_clusteron hulk3 12367 aa4 aa-ev.json aa-d8 &
sleep 0.5

run_clusteron hulk3 12375 aa4 aa-ev.json aa-d9 &
sleep 0.5
run_clusteron hulk3 12376 aa4 aa-ev.json aa-d10 &
sleep 0.5
run_clusteron hulk3 12377 aa4 aa-ev.json aa-d11 &
sleep 0.5


run_clusteron hulk4 12345 aa5 aa-ev.json aa-d0 &
sleep 0.5
run_clusteron hulk4 12346 aa5 aa-ev.json aa-d1 &
sleep 0.5
run_clusteron hulk4 12347 aa5 aa-ev.json aa-d2 &
sleep 0.5

run_clusteron hulk4 12355 aa5 aa-ev.json aa-d3 &
sleep 0.5
run_clusteron hulk4 12356 aa5 aa-ev.json aa-d4 &
sleep 0.5
run_clusteron hulk4 12357 aa5 aa-ev.json aa-d5 &
sleep 0.5

run_clusteron hulk4 12365 aa5 aa-ev.json aa-d6 &
sleep 0.5
run_clusteron hulk4 12366 aa5 aa-ev.json aa-d7 &
sleep 0.5
run_clusteron hulk4 12367 aa5 aa-ev.json aa-d8 &
sleep 0.5

run_clusteron hulk4 12375 aa5 aa-ev.json aa-d9 &
sleep 0.5
run_clusteron hulk4 12376 aa5 aa-ev.json aa-d10 &
sleep 0.5
run_clusteron hulk4 12377 aa5 aa-ev.json aa-d11 &
sleep 0.5


run_clusteron hulk5 12345 aa6 aa-ev.json aa-d0 &
sleep 0.5
run_clusteron hulk5 12346 aa6 aa-ev.json aa-d1 &
sleep 0.5
run_clusteron hulk5 12347 aa6 aa-ev.json aa-d2 &
sleep 0.5

run_clusteron hulk5 12355 aa6 aa-ev.json aa-d3 &
sleep 0.5
run_clusteron hulk5 12356 aa6 aa-ev.json aa-d4 &
sleep 0.5
run_clusteron hulk5 12357 aa6 aa-ev.json aa-d5 &
sleep 0.5

run_clusteron hulk5 12365 aa6 aa-ev.json aa-d6 &
sleep 0.5
run_clusteron hulk5 12366 aa6 aa-ev.json aa-d7 &
sleep 0.5
run_clusteron hulk5 12367 aa6 aa-ev.json aa-d8 &
sleep 0.5

run_clusteron hulk5 12375 aa6 aa-ev.json aa-d9 &
sleep 0.5
run_clusteron hulk5 12376 aa6 aa-ev.json aa-d10 &
sleep 0.5
run_clusteron hulk5 12377 aa6 aa-ev.json aa-d11 &
sleep 0.5
}

function kill_clusteron {
SERVER=$1
JOB_DIR=$HOME/conrun/con_jobs
JOBFILE=_killcon.job
mkdir -p ${JOB_DIR}
rm ${JOB_DIR}/$JOBFILE
echo "pkill -9 conproxy" >> ${JOB_DIR}/$JOBFILE
parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function kill_clusteron_wrapper {
kill_clusteron hulk0 &
sleep 0.5
kill_clusteron hulk1 &
sleep 0.5
kill_clusteron hulk2 &
sleep 0.5
kill_clusteron hulk3 &
sleep 0.5
kill_clusteron hulk4 &
sleep 0.5
kill_clusteron hulk5 &
sleep 0.5
}																

function gen_conf {
ZK_HOST=$1
ZK_PORT=$2
KAFKA_HOST=$3
KAFKA_PORT=$4
CONSISTENCY_MODEL=$5
CONSISTENCY_TECH=$6
TOPOLOGY=$7
NUM_REPLICAS=$8
CONF=$9
CONF_DIR=$HOME/conrun/conf
mkdir -p ${CONF_DIR}
machine=${10}

echo "
{
	\"zk_host\": \"${ZK_HOST}:${ZK_PORT}\",
	\"kafka_broker\": \"${KAFKA_HOST}:${KAFKA_PORT}\",
	\"consistency_model\": \"${CONSISTENCY_MODEL}\",
	\"consistency_tech\": \"${CONSISTENCY_TECH}\",
	\"topology\": \"${TOPOLOGY}\",
	\"num_replicas\": \"${NUM_REPLICAS}\",
} " > ${CONF_DIR}/$CONF

echo "localhost:11111:0" > ${CONF_DIR}/aa-d0
echo "localhost:11112:0" > ${CONF_DIR}/aa-d1
echo "localhost:11113:0" > ${CONF_DIR}/aa-d2

echo "localhost:11114:0" > ${CONF_DIR}/aa-d3
echo "localhost:11115:0" > ${CONF_DIR}/aa-d4
echo "localhost:11116:0" > ${CONF_DIR}/aa-d5

echo "localhost:11117:0" > ${CONF_DIR}/aa-d6
echo "localhost:11118:0" > ${CONF_DIR}/aa-d7
echo "localhost:11119:0" > ${CONF_DIR}/aa-d8

echo "localhost:11121:0" > ${CONF_DIR}/aa-d9
echo "localhost:11122:0" > ${CONF_DIR}/aa-d10
echo "localhost:11123:0" > ${CONF_DIR}/aa-d11

}																										


function gen_conf_wrapper {
gen_conf 127.0.0.1 2181 127.0.0.1 9092 eventual cr aa 0 aa-ev.json hulk0
CONF_DIR=$HOME/conrun/
echo $HOME
scp -r $CONF_DIR hulk0:$HOME
scp -r $CONF_DIR hulk1:$HOME
scp -r $CONF_DIR hulk2:$HOME
scp -r $CONF_DIR hulk3:$HOME
scp -r $CONF_DIR hulk4:$HOME
scp -r $CONF_DIR hulk5:$HOME
}

function launch_kafka {
SERVER=$1
JOB_DIR=$HOME/conrun/con_jobs
JOBFILE=_killcon.job
mkdir -p ${JOB_DIR}
rm ${JOB_DIR}/$JOBFILE
echo "/root/ClusterOn/ali/launch.sh" >> ${JOB_DIR}/$JOBFILE
parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function launch_kafka_wrapper {
launch_kafka hulk1 &
sleep 0.5
launch_kafka hulk2 &
sleep 0.5
launch_kafka hulk3 &
sleep 0.5
launch_kafka hulk4 &
sleep 0.5
launch_kafka hulk5 &
sleep 0.5

}


case "$1" in
run_ckv_all)
	run_ckv_wrapper $*
	;;
kill_ckv_all)
	kill_ckv_wrapper $*
	;;
run_con_all)
	run_clusteron_wrapper $*
	;;
kill_con_all)
	kill_clusteron_wrapper $*
	;;
gen_conf_all)
	gen_conf_wrapper $*
	;;
launch_kafka_all)
	launch_kafka_wrapper $*
	;;
*)
	echo "Usage: $0 (run_ckv_all|kill_ckv_all|run_con_all|kill_con_all|gen_conf_all|...)"
	;;
esac
