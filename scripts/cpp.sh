#!/usr/bin/env bash

alias BEGINCOMMENT='if [ ]; then'
alias ENDCOMMENT='fi'

HOME=/home/yue
CON_HOME=$HOME/ClusterOn/conproxy
SSDB_HOME=$HOME/ClusterOn/ssdb

function gen_ssdb_conf {
	SERVER=$1
	PORT=$2
	SSDB_CONF_DIR=$HOME/ssdbconf/${SERVER}_conf
	CONF=ssdb.conf
	mkdir -p ${SSDB_CONF_DIR}

	echo "
work_dir = ./var
pidfile = ./var/ssdb.pid

server:
	ip: $SERVER
	port: $PORT

replication:
	binlog: yes
		capacity: 20000000
	sync_speed: -1
	slaveof:

logger:
	#level: debug
	output: log.txt
	rotate:
		size: 1000000000

leveldb:
	# in MB
	cache_size: 100
	# in KB
	block_size: 32
	# in MB
	write_buffer_size: 64
	# in MB
	compaction_speed: 1000
	# yes|no
	compression: no
	" > ${SSDB_CONF_DIR}/$CONF
}

function gen_conf {
	ZK_HOST_STR=$1
	#ZK_PORT=$2
	KAFKA_HOST_STR=$2
	#KAFKA_PORT=$4
	CONSISTENCY_MODEL=$3
	CONSISTENCY_TECH=$4
	TOPOLOGY=$5
	NUM_REPLICAS=$6
	CONF=$7
	CONF_DIR=$HOME/conrun/conf
	mkdir -p ${CONF_DIR}

	echo "
	{
		\"zk_host\": \"${ZK_HOST_STR}\",
		\"kafka_broker\": \"${KAFKA_HOST_STR}\",
		\"consistency_model\": \"${CONSISTENCY_MODEL}\",
		\"consistency_tech\": \"${CONSISTENCY_TECH}\",
		\"topology\": \"${TOPOLOGY}\",
		\"num_replicas\": \"${NUM_REPLICAS}\",
	} " > ${CONF_DIR}/$CONF
}

function gen_datalets {
	HOST=$1
	START=$2
	RANGE=$3
	B_PORT=$4
	NUM_DATALETS=$5
	CONF_PREFIX=$6
	MODE=$7
	CONF_DIR=$HOME/conrun/conf
	mkdir -p ${CONF_DIR}
	rm ${CONF_DIR}/${CONF_PREFIX}.cfg

	d=0
	while [ $d -lt ${NUM_DATALETS} ]; do
		SERVER_PORT=$((${B_PORT}+$d))
		ID=$((($START+$d)%${RANGE}+1))
		echo "$d $HOST$ID"
		if [[ $d -eq 0 && $MODE == "MS" ]]; then
			echo "$HOST$ID:${SERVER_PORT}:0" > ${CONF_DIR}/${CONF_PREFIX}.cfg
		else
			echo "$HOST$ID:${SERVER_PORT}:1" >> ${CONF_DIR}/${CONF_PREFIX}.cfg
		fi
		d=$(($d+1))
	done
}

function gen_datalet {
	HOST=$1
	PORT=$2
	CONF_PREFIX=$3
	CONF_DIR=$HOME/conrun/conf
	mkdir -p ${CONF_DIR}
	rm ${CONF_DIR}/${CONF_PREFIX}.cfg

	echo "$HOST:$PORT:0" > ${CONF_DIR}/${CONF_PREFIX}.cfg
	#echo "$HOST:$PORT:0"
}

function docker_run_clusteron {
	SERVER=$1
	BIN=$2
	PORT=$3
	SHARD=$4
	CONF=$5
	DL=$6
	HOST_CONF_DIR=/root/conrun/conf
	MAPPED_CONF_DIR=/conproxy/src/conf
	JOB_DIR=/root/conrun/con_jobs
	JOBFILE=_runcon.job
	mkdir -p ${HOST_CONF_DIR}
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE
	echo "docker run -d -p $PORT:$PORT -e CONF_FILE=$CONF -e DL_FILE=$DL -e PORT=$PORT \
--add-host hulk0:192.163.0.170 \
--add-host hulk1:192.163.0.171 \
--add-host hulk2:192.163.0.172 \
--add-host hulk3:192.163.0.173 \
--add-host hulk4:192.163.0.174 \
--add-host hulk5:192.163.0.175 \
-e SHARD=$SHARD \
-v ${HOST_CONF_DIR}/$CONF:${MAPPED_CONF_DIR}/$CONF \
-v ${HOST_CONF_DIR}/$DL:${MAPPED_CONF_DIR}/$DL \
-i -t tddg/$BIN" >> ${JOB_DIR}/$JOBFILE
	
	#parallel -j 3% --sshloginfile myhosts -a ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
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

	#id=0
	#while [ $id -lt ${NUM_PROC} ]; do
	#SERVER_PORT=`echo "scale=0;$PORT+$id"|bc -l`
	#SERVER_PORT=$(($PORT+$id))

	echo "$SERVER: $BIN --config ${CONF_DIR}/$CONF --datalets ${CONF_DIR}/${DL}.cfg \
--shard ${SHARD} --proxyAddr $SERVER --proxyClientPort ${PORT}" 
	echo "LD_LIBRARY_PATH=/usr/local/lib:$HOME/ClusterOn/conproxy/src/redlock-cpp/hiredis:$HOME/ClusterOn/conproxy/src/kafka/src:$HOME/ClusterOn/conproxy/src/kafka/src-cpp:/usr/lib \
$BIN --config ${CONF_DIR}/$CONF --datalets ${CONF_DIR}/${DL}.cfg \
--shard ${SHARD} --proxyAddr $SERVER --proxyClientPort ${PORT}" \
	>> ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE

	#id=$(($id+1))
	#done
}

function docker_run_ckv {
	SERVER=$1
	BIN=$2
	PORT=$3
	JOB_DIR=/root/conrun/con_jobs
	JOBFILE=_runckv.job
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE
	echo "docker run -d -p $PORT:$PORT -e PORT=$PORT -e RUN_CON=false -i -t tddg/$BIN" \
		>> ${JOB_DIR}/$JOBFILE
	
	#parallel -j 3% --sshloginfile myhosts -a ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function run_redis {
	SERVER=$1
	PORT=$2
	JOB_DIR=$HOME/rdsrun/rds_jobs
	JOBFILE=_runrds.job
	BIN=redis-server
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE

	echo "$SERVER: $BIN --port ${PORT}"
	echo "$BIN --port ${PORT}" \
	> ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function run_leveldb {
	SERVER=$1
	CONF=$HOME/ssdbconf/${SERVER}_conf/ssdb.conf
	JOB_DIR=$HOME/ssdbrun/ssdb_jobs
	JOBFILE=_runssdb.job
	BIN=${SSDB_HOME}/ssdb-server
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE

	echo "$SERVER: $BIN $CONF"
	echo "$BIN $CONF" \
	> ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function run_ckv {
	SERVER=$1
	PORT=$2
	#NUM_PROC=$3
	JOB_DIR=$HOME/conrun/con_jobs
	JOBFILE=_runckv.job
	BIN=${CON_HOME}/apps/ckv/conkv
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE

	#id=0
	#while [ $id -lt ${NUM_PROC} ]; do
	#SERVER_PORT=$(($PORT+$id))

	echo "$SERVER: $BIN -t 1 -l $SERVER -p ${PORT}"
	echo "LD_LIBRARY_PATH=/usr/local/lib $BIN -t 1 -l $SERVER -p ${PORT}" \
	> ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE

	#rm ${JOB_DIR}/$JOBFILE
	#id=$(($id+1))
	#done
}

function docker_kill_container {
	SERVER=$1
	BIN=$2
	PORT=$3
	JOB_DIR=/root/conrun/con_jobs
	JOBFILE=_killcon.sh
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE
	# echo "docker ps|grep -e \"tddg/$BIN.*$PORT\"|awk '{print \$1}'" >> ${JOB_DIR}/$JOBFILE
	echo "dockerID=\$(docker ps|grep -e \"tddg/$BIN.*$PORT\"|awk '{print \$1}') && docker stop \$dockerID" >> ${JOB_DIR}/$JOBFILE
	# echo 'echo $dockerID' >> ${JOB_DIR}/$JOBFILE
	# echo 'docker stop $dockerID' >> ${JOB_DIR}/$JOBFILE

	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function kill_clusteron {
	SERVER=$1
	JOB_DIR=$HOME/conrun/con_jobs
	JOBFILE=_killcon.job
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE
	echo "pkill conproxy" >> ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function kill_conkv {
	SERVER=$1
	JOB_DIR=$HOME/conrun/con_jobs
	JOBFILE=_killckv.job
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE
	echo "sudo pkill conkv" >> ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function kill_redis {
	SERVER=$1
	JOB_DIR=$HOME/rdsrun/rds_jobs
	JOBFILE=_killrds.job
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE
	echo "sudo pkill redis-server" >> ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function kill_leveldb {
	SERVER=$1
	JOB_DIR=$HOME/ssdbrun/ssdb_jobs
	JOBFILE=_killssdb.job
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE
	echo "sudo pkill ssdb-server" >> ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

#function run_bench {
#	;
#}
