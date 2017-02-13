#!/usr/bin/env bash

alias BEGINCOMMENT='if [ ]; then'
alias ENDCOMMENT='fi'

CON_HOME=/root/ClusterOn/conproxy

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
	CONF_DIR=/root/conrun/conf
	mkdir -p ${CONF_DIR}

	echo "
	{
		\"zk_host\": \"${ZK_HOST}:${ZK_PORT}\",
		\"kafka_broker\": \"${KAFKA_HOST}:${KAFKA_PORT}\",
		\"consistency_model\": \"${CONSISTENCY_MODEL}\",
		\"consistency_tech\": \"${CONSISTENCY_TECH}\",
		\"topology\": \"${TOPOLOGY}\",
		\"num_replicas\": \"${NUM_REPLICAS}\",
	} " > ${CONF_DIR}/$CONF
}

function gen_datalets {
	HOST=$1
	B_PORT=$2
	NUM_DATALETS=$3
	CONF_PREFIX=$4
	MODE=$5
	CONF_DIR=/root/conrun/conf
	mkdir -p ${CONF_DIR}
	rm ${CONF_DIR}/${CONF_PREFIX}.cfg

	d=0
	while [ $d -lt ${NUM_DATALETS} ]; do
		SERVER_PORT=$((${B_PORT}+$d))
		if [[ $d -eq 0 && $MODE == "MS" ]]; then
			echo "$HOST:${SERVER_PORT}:0" > ${CONF_DIR}/${CONF_PREFIX}.cfg
		else
			echo "$HOST:${SERVER_PORT}:1" >> ${CONF_DIR}/${CONF_PREFIX}.cfg
		fi
		d=$(($d+1))
	done
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
	CONF_DIR=/root/conrun/conf
	JOB_DIR=/root/conrun/con_jobs
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
	echo "LD_LIBRARY_PATH=/usr/local/lib:/root/ClusterOn/conproxy/src/redlock-cpp/hiredis:/root/ClusterOn/conproxy/src/kafka/src:/root/ClusterOn/conproxy/src/kafka/src-cpp:/usr/lib \
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

function run_ckv {
	SERVER=$1
	PORT=$2
	#NUM_PROC=$3
	JOB_DIR=/root/conrun/con_jobs
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
	JOB_DIR=/root/conrun/con_jobs
	JOBFILE=_killcon.job
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE
	echo "pkill conproxy" >> ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

function kill_conkv {
	SERVER=$1
	JOB_DIR=/root/conrun/con_jobs
	JOBFILE=_killckv.job
	mkdir -p ${JOB_DIR}
	rm ${JOB_DIR}/$JOBFILE
	echo "pkill conkv" >> ${JOB_DIR}/$JOBFILE
	parallel --gnu -S $SERVER -a ${JOB_DIR}/$JOBFILE
}

#function run_bench {
#	;
#}
