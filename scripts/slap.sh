#!/usr/bin/env bash
source `pwd`/app.sh

function gen_conf_wrapper {
	#gen_conf hulk3 2181 hulk3 9092 strong cr ms 2 m.json
	#gen_conf hulk3 2181 hulk3 9092 strong cr slave 0 s.json

	gen_conf hulk3 2181 hulk3 9092 eventual cr ms 0 me.json
	gen_conf hulk3 2181 hulk3 9092 eventual cr slave 0 se.json
}

function gen_datalets_wrapper {
	limit=3
	i=0
	while [ $i -lt $limit ]; do
		M_PORT=11111
		PORT=$((${M_PORT}+$i*3))
		S1_PORT=$(($PORT+1))
		S2_PORT=$(($PORT+2))
		
		# strong config
		#gen_datalets hulk1 $PORT 3 h1_d$i MS
		#gen_datalets hulk2 $PORT 3 h2_d$i MS
		#gen_datalets hulk1 ${S2_PORT} 1 h1_s$i S
		#gen_datalets hulk2 ${S2_PORT} 1 h2_s$i S
		#gen_datalets hulk3 $PORT 3 h3_d$i MS
		#gen_datalets hulk4 $PORT 3 h4_d$i MS
		#gen_datalets hulk3 ${S2_PORT} 1 h3_s$i S
		#gen_datalets hulk4 ${S2_PORT} 1 h4_s$i S

		# eventual config
		#gen_datalets hulk1 $PORT 1 h1_me$i MS
		#gen_datalets hulk2 $PORT 1 h2_me$i MS
		#gen_datalets hulk1 ${S1_PORT} 1 h1_se${i}_1 S
		#gen_datalets hulk2 ${S1_PORT} 1 h2_se${i}_1 S
		#gen_datalets hulk1 ${S2_PORT} 1 h1_se${i}_2 S
		#gen_datalets hulk2 ${S2_PORT} 1 h2_se${i}_2 S
		gen_datalets hulk3 $PORT      1 h3_me$i MS
		gen_datalets hulk4 $PORT      1 h4_me$i MS
		gen_datalets hulk3 ${S1_PORT} 1 h3_se${i}_1 S
		gen_datalets hulk4 ${S1_PORT} 1 h4_se${i}_1 S
		gen_datalets hulk3 ${S2_PORT} 1 h3_se${i}_2 S
		gen_datalets hulk4 ${S2_PORT} 1 h4_se${i}_2 S

		echo "$i $PORT"
		i=$(($i+1))
	done
}

function docker_run_clusteron_wrapper {
	docker_run_clusteron hulk1 con_docker 12345 shard1 c1.json d1.cfg 
	docker_run_clusteron hulk1 con_docker 12346 shard2 c1.json d2.cfg 
	#run_clusteron hulk1 con_docker 12347 shard3 c1.json d3.cfg 
	#run_clusteron hulk2 con_docker 12345 shard4 c1.json d4.cfg 
	#run_clusteron hulk2 con_docker 12346 shard5 c1.json d5.cfg 
	#run_clusteron hulk2 con_docker 12347 shard6 c1.json d6.cfg 

	docker_run_clusteron hulk1 con_docker 12348 shard1 s1.json s1.cfg 
	docker_run_clusteron hulk1 con_docker 12349 shard2 s1.json s2.cfg 
	#run_clusteron hulk1 con_docker 12350 shard3 s1.json s3.cfg 
	#run_clusteron hulk2 con_docker 12348 shard4 s1.json s4.cfg 
	#run_clusteron hulk2 con_docker 12349 shard5 s1.json s5.cfg 
	#run_clusteron hulk2 con_docker 12350 shard6 s1.json s6.cfg 
}

function run_clusteron_wrapper {
	# strong
	i=0
	while [ $i -lt 3 ]; do
		port=$((22222+$i))
		run_clusteron hulk1 $port h1s_$i m.json h1_d$i &
		sleep 1
		run_clusteron hulk2 $port h2s_$i m.json h2_d$i &
		sleep 1
		run_clusteron hulk3 $port h3s_$i m.json h3_d$i &
		sleep 1
		run_clusteron hulk4 $port h4s_$i m.json h4_d$i &
		sleep 1
		i=$(($i+1))
	done
	i=0
	while [ $i -lt 3 ]; do
		port=$((33333+$i))
		run_clusteron hulk1 $port h1s_$i s.json h1_s$i &
		sleep 1
		run_clusteron hulk2 $port h2s_$i s.json h2_s$i &
		sleep 1
		run_clusteron hulk3 $port h3s_$i s.json h3_s$i &
		sleep 1
		run_clusteron hulk4 $port h4s_$i s.json h4_s$i &
		sleep 1
		i=$(($i+1))
	done

	# eventual 
	#i=0
	#while [ $i -lt 3 ]; do
	#	port=$((22222+$i))
	#	run_clusteron hulk1 $port h1_se_$i me.json h1_me$i &
	#	sleep 1
	#	run_clusteron hulk2 $port h2_se_$i me.json h2_me$i &
	#	sleep 1
	#	run_clusteron hulk3 $port h3_se_$i me.json h3_me$i &
	#	sleep 1
	#	run_clusteron hulk4 $port h4_se_$i me.json h4_me$i &
	#	sleep 1
	#	i=$(($i+1))
	#done
	#i=0
	#while [ $i -lt 3 ]; do
	#	port=$((33333+$i))
	#	run_clusteron hulk1 $port h1_se_$i se.json h1_se${i}_1 &
	#	sleep 1
	#	run_clusteron hulk2 $port h2_se_$i se.json h2_se${i}_1 &
	#	sleep 1
	#	run_clusteron hulk3 $port h3_se_$i se.json h3_se${i}_1 &
	#	sleep 1
	#	run_clusteron hulk4 $port h4_se_$i se.json h4_se${i}_1 &
	#	sleep 1
	#	i=$(($i+1))
	#done
	#i=0
	#while [ $i -lt 3 ]; do
	#	port=$((44444+$i))
	#	run_clusteron hulk1 $port h1_se_$i se.json h1_se${i}_2 &
	#	sleep 1
	#	run_clusteron hulk2 $port h2_se_$i se.json h2_se${i}_2 &
	#	sleep 1
	#	run_clusteron hulk3 $port h3_se_$i se.json h3_se${i}_2 &
	#	sleep 1
	#	run_clusteron hulk4 $port h4_se_$i se.json h4_se${i}_2 &
	#	sleep 1
	#	i=$(($i+1))
	#done
}

function docker_run_conkv_wrapper {
	docker_run_ckv hulk1 con_docker 11111
	docker_run_ckv hulk1 con_docker 11112
	docker_run_ckv hulk1 con_docker 11113
	docker_run_ckv hulk1 con_docker 11114
	docker_run_ckv hulk1 con_docker 11115
	docker_run_ckv hulk1 con_docker 11116
	docker_run_ckv hulk1 con_docker 11117
	docker_run_ckv hulk1 con_docker 11118
	docker_run_ckv hulk1 con_docker 11119
}

function run_conkv_wrapper {
	i=0
	while [ $i -lt 9 ]; do
		port=$((11111+$i))
		run_ckv hulk1 $port &
		sleep 1
		run_ckv hulk2 $port &
		sleep 1
		run_ckv hulk3 $port &
		sleep 1
		run_ckv hulk4 $port &
		sleep 1
		i=$(($i+1))
	done
}

function docker_kill_clusteron_wrapper {
	docker_kill_container hulk1 con_docker 12345
	docker_kill_container hulk1 con_docker 12346
	docker_kill_container hulk1 con_docker 12347
	docker_kill_container hulk1 con_docker 12348
	docker_kill_container hulk1 con_docker 12349
	docker_kill_container hulk1 con_docker 12350

	#kill_container hulk2 con_docker 12345
	#kill_container hulk2 con_docker 12346
	#kill_container hulk2 con_docker 12347
	#kill_container hulk2 con_docker 12348
	#kill_container hulk2 con_docker 12349
	#kill_container hulk2 con_docker 12350
}

function kill_clusteron_wrapper {
	kill_clusteron hulk1 &
	sleep 1
	kill_clusteron hulk2 &
	sleep 1
	kill_clusteron hulk3 &
	sleep 1
	kill_clusteron hulk4 &
	sleep 1
}

function docker_kill_conkv_wrapper {
	docker_kill_container hulk1 con_docker 11111
	docker_kill_container hulk1 con_docker 11112
	docker_kill_container hulk1 con_docker 11113
	docker_kill_container hulk1 con_docker 11114
	docker_kill_container hulk1 con_docker 11115
	docker_kill_container hulk1 con_docker 11116
	docker_kill_container hulk1 con_docker 11117
	docker_kill_container hulk1 con_docker 11118
	docker_kill_container hulk1 con_docker 11119

	docker_kill_container hulk2 con_docker 11111
	docker_kill_container hulk2 con_docker 11112
	docker_kill_container hulk2 con_docker 11113
	docker_kill_container hulk2 con_docker 11114
	docker_kill_container hulk2 con_docker 11115
	docker_kill_container hulk2 con_docker 11116
	docker_kill_container hulk2 con_docker 11117
	docker_kill_container hulk2 con_docker 11118
	docker_kill_container hulk2 con_docker 11119
}

function kill_conkv_wrapper {
	kill_conkv hulk1 &
	sleep 1
	kill_conkv hulk2 &
	sleep 1
	kill_conkv hulk3 &
	sleep 1
	kill_conkv hulk4 &
	sleep 1
}

case "$1" in
genconf)
	gen_conf_wrapper $*
	;;
gendl)
	gen_datalets_wrapper $*
	;;
d_runcon)
	docker_run_clusteron_wrapper $*
	;;
runcon)
	run_clusteron_wrapper $*
	;;
d_runckv)
	docker_run_conkv_wrapper $*
	;;
runckv)
	run_conkv_wrapper $*
	;;
runredis)
	run_redis $*
	;;
runmc)
	run_memcached $*
	;;
runldb)
	run_leveldb $*
	;;
runb)
	run_bench $*
	;;
d_killcon)
	docker_kill_clusteron_wrapper $*
	;;
killcon)
	kill_clusteron_wrapper $*
	;;
d_killckv)
	docker_kill_conkv_wrapper $*
	;;
killckv)
	kill_conkv_wrapper $*
	;;
*)
	echo "Usage: $0 (runcon|runckv|runredis|runmc|runb|...)"
	;;
esac
