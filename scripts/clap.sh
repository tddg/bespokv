#!/usr/bin/env bash
source `pwd`/cpp.sh

function gen_ssdb_conf_wrapper {
	i=1
	while [ $i -le 24 ] ; do
		port=$((11111+($i-1)%3))
		#echo $port
		gen_ssdb_conf cvm$i $port cvm${i}.conf
		i=$(($i+1))
	done
}

function gen_conf_wrapper {
	# 4 kafka node cluster
	## strong
	#gen_conf cvm50:2181,cvm51:2181,cvm52:2181,cvm53:2181 cvm50:9092,cvm51:9092,cvm52:9092,cvm53:9092 strong cr ms    2 m.json
	#gen_conf cvm50:2181,cvm51:2181,cvm52:2181,cvm53:2181 cvm50:9092,cvm51:9092,cvm52:9092,cvm53:9092 strong cr slave 0 s.json

	## eventual
	#gen_conf cvm50:2181,cvm51:2181,cvm52:2181,cvm53:2181 cvm50:9092,cvm51:9092,cvm52:9092,cvm53:9092 eventual cr ms    0 me.json
	#gen_conf cvm50:2181,cvm51:2181,cvm52:2181,cvm53:2181 cvm50:9092,cvm51:9092,cvm52:9092,cvm53:9092 eventual cr slave 0 se.json
	
	## 1 kafka node cluster
	## strong
	#gen_conf cvm50:2181 cvm50:9092 strong cr ms    2 m1.json
	#gen_conf cvm50:2181 cvm50:9092 strong cr slave 0 s1.json

	## eventual
	#gen_conf cvm50:2181 cvm50:9092 eventual cr ms    0 me1.json
	#gen_conf cvm50:2181 cvm50:9092 eventual cr slave 0 se1.json
	#
	## 1 kafka node cluster
	## strong
	#gen_conf cvm51:2181 cvm51:9092 strong cr ms    2 m2.json
	#gen_conf cvm51:2181 cvm51:9092 strong cr slave 0 s2.json

	## eventual
	#gen_conf cvm51:2181 cvm51:9092 eventual cr ms    0 me2.json
	#gen_conf cvm51:2181 cvm51:9092 eventual cr slave 0 se2.json

	i=0
	while [ $i -lt 8 ]; do 
		# MS strong
		id=$((50+$i))
		conf_id=$(($i+1))
		#gen_conf cvm$id:2181 cvm$id:9092 strong cr ms    2 m${conf_id}_3r.json
		#gen_conf cvm$id:2181 cvm$id:9092 strong cr ms    1 m${conf_id}_2r.json
		#gen_conf cvm$id:2181 cvm$id:9092 without none ms 0 m${conf_id}_1r.json
		#gen_conf cvm$id:2181 cvm$id:9092 strong cr slave 0 s${conf_id}.json

		# MS eventual
		#gen_conf cvm$id:2181 cvm$id:9092 eventual cr ms    0 me8.json
		#gen_conf cvm$id:2181 cvm$id:9092 eventual cr slave 0 se8.json
		# 
		gen_conf cvm$id:2181 cvm$id:9092 eventual cr aa    0 ae${conf_id}.json

		# AA eventual
		i=$(($i+1))
	done
}

function gen_datalets_wrapper {
	limit=1
	i=0
	while [ $i -lt $limit ]; do
		M_PORT=11111
		PORT=$((${M_PORT}+$i*3))
		S1_PORT=$(($PORT+1))
		S2_PORT=$(($PORT+2))
		
		# strong config 1 replica
		#gen_datalets cvm 0  48 $PORT 1 m1_d${i}_1r MS
		#gen_datalets cvm 3  48 $PORT 1 m2_d${i}_1r MS
		#gen_datalets cvm 6  48 $PORT 1 m3_d${i}_1r MS
		#gen_datalets cvm 9  48 $PORT 1 m4_d${i}_1r MS
		#gen_datalets cvm 12 48 $PORT 1 m5_d${i}_1r MS
		#gen_datalets cvm 15 48 $PORT 1 m6_d${i}_1r MS
		#gen_datalets cvm 18 48 $PORT 1 m7_d${i}_1r MS
		#gen_datalets cvm 21 48 $PORT 1 m8_d${i}_1r MS
		
		# strong config 2 replicas
		#gen_datalets cvm 0  48 $PORT 2 m1_d${i}_2r MS
		#gen_datalets cvm 3  48 $PORT 2 m2_d${i}_2r MS
		#gen_datalets cvm 6  48 $PORT 2 m3_d${i}_2r MS
		#gen_datalets cvm 9  48 $PORT 2 m4_d${i}_2r MS
		#gen_datalets cvm 12 48 $PORT 2 m5_d${i}_2r MS
		#gen_datalets cvm 15 48 $PORT 2 m6_d${i}_2r MS
		#gen_datalets cvm 18 48 $PORT 2 m7_d${i}_2r MS
		#gen_datalets cvm 21 48 $PORT 2 m8_d${i}_2r MS
		#gen_datalets cvm 1  48 $S1_PORT 1 m1_s${i}_2r S
		#gen_datalets cvm 4  48 $S1_PORT 1 m2_s${i}_2r S
		#gen_datalets cvm 7  48 $S1_PORT 1 m3_s${i}_2r S
		#gen_datalets cvm 10 48 $S1_PORT 1 m4_s${i}_2r S
		#gen_datalets cvm 13 48 $S1_PORT 1 m5_s${i}_2r S
		#gen_datalets cvm 16 48 $S1_PORT 1 m6_s${i}_2r S
		#gen_datalets cvm 19 48 $S1_PORT 1 m7_s${i}_2r S
		#gen_datalets cvm 22 48 $S1_PORT 1 m8_s${i}_2r S
		
		# MS strong config 3 replicas
		#gen_datalets cvm 0 48 $PORT 3 m1_d$i MS
		#gen_datalets cvm 3 48 $PORT 3 m2_d$i MS
		#gen_datalets cvm 6 48 $PORT 3 m3_d$i MS
		#gen_datalets cvm 9 48 $PORT 3 m4_d$i MS
		#gen_datalets cvm 12 48 $PORT 3 m5_d$i MS
		#gen_datalets cvm 15 48 $PORT 3 m6_d$i MS
		#gen_datalets cvm 18 48 $PORT 3 m7_d$i MS
		#gen_datalets cvm 21 48 $PORT 3 m8_d$i MS
		#gen_datalets cvm 24 48 $PORT 3 m9_d$i MS
		#gen_datalets cvm 27 48 $PORT 3 m10_d$i MS
		#gen_datalets cvm 30 48 $PORT 3 m11_d$i MS
		#gen_datalets cvm 33 48 $PORT 3 m12_d$i MS
		#gen_datalets cvm 36 48 $PORT 3 m13_d$i MS
		#gen_datalets cvm 39 48 $PORT 3 m14_d$i MS
		#gen_datalets cvm 42 48 $PORT 3 m15_d$i MS
		#gen_datalets cvm 45 48 $PORT 3 m16_d$i MS
		#gen_datalets cvm 2 48 ${S2_PORT} 1 m1_s$i S
		#gen_datalets cvm 5 48 ${S2_PORT} 1 m2_s$i S
		#gen_datalets cvm 8 48 ${S2_PORT} 1 m3_s$i S
		#gen_datalets cvm 11 48 ${S2_PORT} 1 m4_s$i S
		#gen_datalets cvm 14 48 ${S2_PORT} 1 m5_s$i S
		#gen_datalets cvm 17 48 ${S2_PORT} 1 m6_s$i S
		#gen_datalets cvm 20 48 ${S2_PORT} 1 m7_s$i S
		#gen_datalets cvm 23 48 ${S2_PORT} 1 m8_s$i S
		#gen_datalets cvm 26 48 ${S2_PORT} 1 m9_s$i S
		#gen_datalets cvm 29 48 ${S2_PORT} 1 m10_s$i S
		#gen_datalets cvm 32 48 ${S2_PORT} 1 m11_s$i S
		#gen_datalets cvm 35 48 ${S2_PORT} 1 m12_s$i S
		#gen_datalets cvm 38 48 ${S2_PORT} 1 m13_s$i S
		#gen_datalets cvm 41 48 ${S2_PORT} 1 m14_s$i S
		#gen_datalets cvm 44 48 ${S2_PORT} 1 m15_s$i S
		#gen_datalets cvm 47 48 ${S2_PORT} 1 m16_s$i S

		# MS eventual config
		#gen_datalets cvm 0  48 $PORT 1 m1_de$i MS
		#gen_datalets cvm 1  48 ${S1_PORT} 1 m1_se${i}_1 S
		#gen_datalets cvm 2  48 ${S2_PORT} 1 m1_se${i}_2 S
		#gen_datalets cvm 3  48 $PORT 1 m2_de$i MS
		#gen_datalets cvm 4  48 ${S1_PORT} 1 m2_se${i}_1 S
		#gen_datalets cvm 5  48 ${S2_PORT} 1 m2_se${i}_2 S
		#gen_datalets cvm 6  48 $PORT 1 m3_de$i MS
		#gen_datalets cvm 7  48 ${S1_PORT} 1 m3_se${i}_1 S
		#gen_datalets cvm 8  48 ${S2_PORT} 1 m3_se${i}_2 S
		#gen_datalets cvm 9  48 $PORT 1 m4_de$i MS
		#gen_datalets cvm 10 48 ${S1_PORT} 1 m4_se${i}_1 S
		#gen_datalets cvm 11 48 ${S2_PORT} 1 m4_se${i}_2 S
		#gen_datalets cvm 12 48 $PORT 1 m5_de$i MS
		#gen_datalets cvm 13 48 ${S1_PORT} 1 m5_se${i}_1 S
		#gen_datalets cvm 14 48 ${S2_PORT} 1 m5_se${i}_2 S
		#gen_datalets cvm 15 48 $PORT 1 m6_de$i MS
		#gen_datalets cvm 16 48 ${S1_PORT} 1 m6_se${i}_1 S
		#gen_datalets cvm 17 48 ${S2_PORT} 1 m6_se${i}_2 S
		#gen_datalets cvm 18 48 $PORT 1 m7_de$i MS
		#gen_datalets cvm 19 48 ${S1_PORT} 1 m7_se${i}_1 S
		#gen_datalets cvm 20 48 ${S2_PORT} 1 m7_se${i}_2 S
		#gen_datalets cvm 21 48 $PORT 1 m8_de$i MS
		#gen_datalets cvm 22 48 ${S1_PORT} 1 m8_se${i}_1 S
		#gen_datalets cvm 23 48 ${S2_PORT} 1 m8_se${i}_2 S
		#gen_datalets cvm 24 48 $PORT 1 m9_de$i MS
		#gen_datalets cvm 25 48 ${S1_PORT} 1 m9_se${i}_1 S
		#gen_datalets cvm 26 48 ${S2_PORT} 1 m9_se${i}_2 S
		#gen_datalets cvm 27 48 $PORT 1 m10_de$i MS
		#gen_datalets cvm 28 48 ${S1_PORT} 1 m10_se${i}_1 S
		#gen_datalets cvm 29 48 ${S2_PORT} 1 m10_se${i}_2 S
		#gen_datalets cvm 30 48 $PORT 1 m11_de$i MS
		#gen_datalets cvm 31 48 ${S1_PORT} 1 m11_se${i}_1 S
		#gen_datalets cvm 32 48 ${S2_PORT} 1 m11_se${i}_2 S
		#gen_datalets cvm 33 48 $PORT 1 m12_de$i MS
		#gen_datalets cvm 34 48 ${S1_PORT} 1 m12_se${i}_1 S
		#gen_datalets cvm 35 48 ${S2_PORT} 1 m12_se${i}_2 S
		#gen_datalets cvm 36 48 $PORT 1 m13_de$i MS
		#gen_datalets cvm 37 48 ${S1_PORT} 1 m13_se${i}_1 S
		#gen_datalets cvm 38 48 ${S2_PORT} 1 m13_se${i}_2 S
		#gen_datalets cvm 39 48 $PORT 1 m14_de$i MS
		#gen_datalets cvm 40 48 ${S1_PORT} 1 m14_se${i}_1 S
		#gen_datalets cvm 41 48 ${S2_PORT} 1 m14_se${i}_2 S
		#gen_datalets cvm 42 48 $PORT 1 m15_de$i MS
		#gen_datalets cvm 43 48 ${S1_PORT} 1 m15_se${i}_1 S
		#gen_datalets cvm 44 48 ${S2_PORT} 1 m15_se${i}_2 S
		#gen_datalets cvm 45 48 $PORT 1 m16_de$i MS
		#gen_datalets cvm 46 48 ${S1_PORT} 1 m16_se${i}_1 S
		#gen_datalets cvm 47 48 ${S2_PORT} 1 m16_se${i}_2 S

		#echo "$i $PORT"
		i=$(($i+1))

		# AA eventual config
		num=0
		while [ $num -lt 16 ]; do 
			shard=$(($num+1))
			id=$(($num*3+1))
			gen_datalet cvm$id 11111 ae${shard}_1
			id=$(($num*3+2))
			gen_datalet cvm$id 11112 ae${shard}_2
			id=$(($num*3+3))
			gen_datalet cvm$id 11113 ae${shard}_3
			num=$(($num+1))
		done
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
	# varying num replicas
	#run_clusteron cvm1  22222 m1s m1_1r.json m1_d0_1r &
	#sleep 1
	##run_clusteron cvm2  33333 m1shar s1.json    m1_s0_2r &
	##sleep 1
	#run_clusteron cvm4  22222 m2s m1_1r.json m2_d0_1r &
	#sleep 1
	##run_clusteron cvm5  33333 m2shar s1.json    m2_s0_2r &
	##sleep 1
	#run_clusteron cvm7  22222 m3s m2_1r.json m3_d0_1r &
	#sleep 1
	##run_clusteron cvm8  33333 m3shar s2.json    m3_s0_2r &
	##sleep 1
	#run_clusteron cvm10 22222 m4s m2_1r.json m4_d0_1r &
	#sleep 1
	##run_clusteron cvm11 33333 m4shar s2.json    m4_s0_2r &
	##sleep 1
	#run_clusteron cvm13 22222 m5s m3_1r.json m5_d0_1r &
	#sleep 1
	##run_clusteron cvm14 33333 m5shar s3.json    m5_s0_2r &
	##sleep 1
	#run_clusteron cvm16 22222 m6s m3_1r.json m6_d0_1r &
	#sleep 1
	##run_clusteron cvm17 33333 m6shar s3.json    m6_s0_2r &
	##sleep 1
	#run_clusteron cvm19 22222 m7s m4_1r.json m7_d0_1r &
	#sleep 1
	##run_clusteron cvm20 33333 m7shar s4.json    m7_s0_2r &
	##sleep 1
	#run_clusteron cvm22 22222 m8s m4_1r.json m8_d0_1r &
	#sleep 1
	##run_clusteron cvm23 33333 m8shar s4.json    m8_s0_2r &
	##sleep 1

	# strong
	#run_clusteron cvm1  22222 m1shar m1.json m1_d0 &
	#sleep 1
	#run_clusteron cvm3  33333 m1shar s1.json m1_s0 &
	#sleep 1
	#run_clusteron cvm4  22222 m2shar m1.json m2_d0 &
	#sleep 1
	#run_clusteron cvm6  33333 m2shar s1.json m2_s0 &
	#sleep 1
	#run_clusteron cvm7  22222 m3shar m1.json m3_d0 &
	#sleep 1
	#run_clusteron cvm9  33333 m3shar s1.json m3_s0 &
	#sleep 1
	#run_clusteron cvm10 22222 m4shar m2.json m4_d0 &
	#sleep 1
	#run_clusteron cvm12 33333 m4shar s2.json m4_s0 &
	#sleep 1
	#run_clusteron cvm13 22222 m5shar m2.json m5_d0 &
	#sleep 1
	#run_clusteron cvm15 33333 m5shar s2.json m5_s0 &
	#sleep 1
	#run_clusteron cvm16 22222 m6shar m2.json m6_d0 &
	#sleep 1
	#run_clusteron cvm18 33333 m6shar s2.json m6_s0 &
	#sleep 1
	#run_clusteron cvm19 22222 m7shar m3.json m7_d0 &
	#sleep 1
	#run_clusteron cvm21 33333 m7shar s3.json m7_s0 &
	#sleep 1
	#run_clusteron cvm22 22222 m8shar m3.json m8_d0 &
	#sleep 1
	#run_clusteron cvm24 33333 m8shar s3.json m8_s0 &
	#sleep 1
	#run_clusteron cvm25 22222 m9shar m3.json m9_d0 &
	#sleep 1
	#run_clusteron cvm27 33333 m9shar s3.json m9_s0 &
	#sleep 1
	#run_clusteron cvm28 22222 m10shar m4.json m10_d0 &
	#sleep 1
	#run_clusteron cvm30 33333 m10shar s4.json m10_s0 &
	#sleep 1
	#run_clusteron cvm31 22222 m11shar m4.json m11_d0 &
	#sleep 1
	#run_clusteron cvm33 33333 m11shar s4.json m11_s0 &
	#sleep 1
	#run_clusteron cvm34 22222 m12shar m4.json m12_d0 &
	#sleep 1
	#run_clusteron cvm36 33333 m12shar s4.json m12_s0 &
	#sleep 1
	#run_clusteron cvm37 22222 m13shar m5.json m13_d0 &
	#sleep 1
	#run_clusteron cvm39 33333 m13shar s5.json m13_s0 &
	#sleep 1
	#run_clusteron cvm40 22222 m14shar m5.json m14_d0 &
	#sleep 1
	#run_clusteron cvm42 33333 m14shar s5.json m14_s0 &
	#sleep 1
	#run_clusteron cvm43 22222 m15shar m6.json m15_d0 &
	#sleep 1
	#run_clusteron cvm45 33333 m15shar s6.json m15_s0 &
	#sleep 1
	#run_clusteron cvm46 22222 m16shar m6.json m16_d0 &
	#sleep 1
	#run_clusteron cvm48 33333 m16shar s6.json m16_s0 &
	#sleep 1

	##run_clusteron cvm4 22222 m4s m.json m4_d0 &
	##sleep 1
	#run_clusteron cvm2 33333 m4s s.json m4_s0 &
	#sleep 1

	# varying num replicas
	#run_clusteron cvm1  22222 m1se0 me1.json m1_de0 &
	#sleep 1
	#run_clusteron cvm2  22223 m1se0 se1.json m1_se0_1 &
	#sleep 1
	#run_clusteron cvm4  22222 m2se0 me1.json m2_de0 &
	#sleep 1
	#run_clusteron cvm5  22223 m2se0 se1.json m2_se0_1 &
	#sleep 1
	#run_clusteron cvm7  22222 m3se0 me2.json m3_de0 &
	#sleep 1
	#run_clusteron cvm8  22223 m3se0 se2.json m3_se0_1 &
	#sleep 1
	#run_clusteron cvm10 22222 m4se0 me2.json m4_de0 &
	#sleep 1
	#run_clusteron cvm11 22223 m4se0 se2.json m4_se0_1 &
	#sleep 1
	#run_clusteron cvm13 22222 m5se0 me3.json m5_de0 &
	#sleep 1
	#run_clusteron cvm14 22223 m5se0 se3.json m5_se0_1 &
	#sleep 1
	#run_clusteron cvm16 22222 m6se0 me3.json m6_de0 &
	#sleep 1
	#run_clusteron cvm17 22223 m6se0 se3.json m6_se0_1 &
	#sleep 1
	#run_clusteron cvm19 22222 m7se0 me4.json m7_de0 &
	#sleep 1
	#run_clusteron cvm20 22223 m7se0 se4.json m7_se0_1 &
	#sleep 1
	#run_clusteron cvm22 22222 m8se0 me4.json m8_de0 &
	#sleep 1
	#run_clusteron cvm23 22223 m8se0 se4.json m8_se0_1 &
	#sleep 1
	
	# evnetual
	#run_clusteron cvm1  22222 m1se1 me1.json m1_de0 &
	#sleep 1
	#run_clusteron cvm2  22223 m1se1 se1.json m1_se0_1 &
	#sleep 1
	#run_clusteron cvm3  22224 m1se1 se1.json m1_se0_2 &
	#sleep 1
	#run_clusteron cvm4  22222 m2se1 me1.json m2_de0 &
	#sleep 1
	#run_clusteron cvm5  22223 m2se1 se1.json m2_se0_1 &
	#sleep 1
	#run_clusteron cvm6  22224 m2se1 se1.json m2_se0_2 &
	#sleep 1
	#run_clusteron cvm7  22222 m3se1 me2.json m3_de0 &
	#sleep 1
	#run_clusteron cvm8  22223 m3se1 se2.json m3_se0_1 &
	#sleep 1
	#run_clusteron cvm9  22224 m3se1 se2.json m3_se0_2 &
	#sleep 1
	#run_clusteron cvm10 22222 m4se1 me2.json m4_de0 &
	#sleep 1
	#run_clusteron cvm11 22223 m4se1 se2.json m4_se0_1 &
	#sleep 1
	#run_clusteron cvm12 22224 m4se1 se2.json m4_se0_2 &
	#sleep 1
	#run_clusteron cvm13 22222 m5se1 me3.json m5_de0 &
	#sleep 1
	#run_clusteron cvm14 22223 m5se1 se3.json m5_se0_1 &
	#sleep 1
	#run_clusteron cvm15 22224 m5se1 se3.json m5_se0_2 &
	#sleep 1
	#run_clusteron cvm16 22222 m6se1 me3.json m6_de0 &
	#sleep 1
	#run_clusteron cvm17 22223 m6se1 se3.json m6_se0_1 &
	#sleep 1
	#run_clusteron cvm18 22224 m6se1 se3.json m6_se0_2 &
	#sleep 1
	#run_clusteron cvm19 22222 m7se1 me4.json m7_de0 &
	#sleep 1
	#run_clusteron cvm20 22223 m7se1 se4.json m7_se0_1 &
	#sleep 1
	#run_clusteron cvm21 22224 m7se1 se4.json m7_se0_2 &
	#sleep 1
	#run_clusteron cvm22 22222 m8se1 me4.json m8_de0 &
	#sleep 1
	#run_clusteron cvm23 22223 m8se1 se4.json m8_se0_1 &
	#sleep 1
	#run_clusteron cvm24 22224 m8se1 se4.json m8_se0_2 &
	#sleep 1
	#run_clusteron cvm25 22222 m9se4  me5.json m9_de0 &
	#sleep 1
	#run_clusteron cvm26 22223 m9se4  se5.json m9_se0_1 &
	#sleep 1
	#run_clusteron cvm27 22224 m9se4  se5.json m9_se0_2 &
	#sleep 1
	#run_clusteron cvm28 22222 m10se4 me5.json m10_de0 &
	#sleep 1
	#run_clusteron cvm29 22223 m10se4 se5.json m10_se0_1 &
	#sleep 1
	#run_clusteron cvm30 22224 m10se4 se5.json m10_se0_2 &
	#sleep 1
	#run_clusteron cvm31 22222 m11se4 me6.json m11_de0 &
	#sleep 1
	#run_clusteron cvm32 22223 m11se4 se6.json m11_se0_1 &
	#sleep 1
	#run_clusteron cvm33 22224 m11se4 se6.json m11_se0_2 &
	#sleep 1
	#run_clusteron cvm34 22222 m12se4 me6.json m12_de0 &
	#sleep 1
	#run_clusteron cvm35 22223 m12se4 se6.json m12_se0_1 &
	#sleep 1
	#run_clusteron cvm36 22224 m12se4 se6.json m12_se0_2 &
	#sleep 1
	#run_clusteron cvm37 22222 m13se4 me7.json m13_de0 &
	#sleep 1
	#run_clusteron cvm38 22223 m13se4 se7.json m13_se0_1 &
	#sleep 1
	#run_clusteron cvm39 22224 m13se4 se7.json m13_se0_2 &
	#sleep 1
	#run_clusteron cvm40 22222 m14se4 me7.json m14_de0 &
	#sleep 1
	#run_clusteron cvm41 22223 m14se4 se7.json m14_se0_1 &
	#sleep 1
	#run_clusteron cvm42 22224 m14se4 se7.json m14_se0_2 &
	#sleep 1
	#run_clusteron cvm43 22222 m15se4 me8.json m15_de0 &
	#sleep 1
	#run_clusteron cvm44 22223 m15se4 se8.json m15_se0_1 &
	#sleep 1
	#run_clusteron cvm45 22224 m15se4 se8.json m15_se0_2 &
	#sleep 1
	#run_clusteron cvm46 22222 m16se4 me8.json m16_de0 &
	#sleep 1
	#run_clusteron cvm47 22223 m16se4 se8.json m16_se0_1 &
	#sleep 1
	#run_clusteron cvm48 22224 m16se4 se8.json m16_se0_2 &
	#sleep 1
	
	# AA evnetual
	run_clusteron cvm1  22222 a1se  ae1.json ae1_1 &
	sleep 1
	run_clusteron cvm2  22223 a1se  ae1.json ae1_2 &
	sleep 1
	run_clusteron cvm3  22224 a1se  ae1.json ae1_3 &
	sleep 1
	run_clusteron cvm4  22222 a2se  ae1.json ae2_1 &
	sleep 1
	run_clusteron cvm5  22223 a2se  ae1.json ae2_2 &
	sleep 1
	run_clusteron cvm6  22224 a2se  ae1.json ae2_3 &
	sleep 1
	run_clusteron cvm7  22222 a3se  ae2.json ae3_1 &
	sleep 1
	run_clusteron cvm8  22223 a3se  ae2.json ae3_2 &
	sleep 1
	run_clusteron cvm9  22224 a3se  ae2.json ae3_3 &
	sleep 1
	run_clusteron cvm10 22222 a4se  ae2.json ae4_1 &
	sleep 1
	run_clusteron cvm11 22223 a4se  ae2.json ae4_2 &
	sleep 1
	run_clusteron cvm12 22224 a4se  ae2.json ae4_3 &
	sleep 1
	#run_clusteron cvm13 22222 a5se  ae3.json ae5_1 &
	#sleep 1
	#run_clusteron cvm14 22223 a5se  ae3.json ae5_2 &
	#sleep 1
	#run_clusteron cvm15 22224 a5se  ae3.json ae5_3 &
	#sleep 1
	#run_clusteron cvm16 22222 a6se  ae3.json ae6_1 &
	#sleep 1
	#run_clusteron cvm17 22223 a6se  ae3.json ae6_2 &
	#sleep 1
	#run_clusteron cvm18 22224 a6se  ae3.json ae6_3 &
	#sleep 1
	#run_clusteron cvm19 22222 a7se  ae4.json ae7_1 &
	#sleep 1
	#run_clusteron cvm20 22223 a7se  ae4.json ae7_2 &
	#sleep 1
	#run_clusteron cvm21 22224 a7se  ae4.json ae7_3 &
	#sleep 1
	#run_clusteron cvm22 22222 a8se  ae4.json ae8_1 &
	#sleep 1
	#run_clusteron cvm23 22223 a8se  ae4.json ae8_2 &
	#sleep 1
	#run_clusteron cvm24 22224 a8se  ae4.json ae8_3 &
	#sleep 1
	#run_clusteron cvm25 22222 a9se  ae5.json ae9_1 &
	#sleep 1
	#run_clusteron cvm26 22223 a9se  ae5.json ae9_2 &
	#sleep 1
	#run_clusteron cvm27 22224 a9se  ae5.json ae9_3 &
	#sleep 1
	#run_clusteron cvm28 22222 a10se ae5.json ae10_1 &
	#sleep 1
	#run_clusteron cvm29 22223 a10se ae5.json ae10_2 &
	#sleep 1
	#run_clusteron cvm30 22224 a10se ae5.json ae10_3 &
	#sleep 1
	#run_clusteron cvm31 22222 a11se ae6.json ae11_1 &
	#sleep 1
	#run_clusteron cvm32 22223 a11se ae6.json ae11_2 &
	#sleep 1
	#run_clusteron cvm33 22224 a11se ae6.json ae11_3 &
	#sleep 1
	#run_clusteron cvm34 22222 a12se ae6.json ae12_1 &
	#sleep 1
	#run_clusteron cvm35 22223 a12se ae6.json ae12_2 &
	#sleep 1
	#run_clusteron cvm36 22224 a12se ae6.json ae12_3 &
	#sleep 1
	#run_clusteron cvm37 22222 a13se ae7.json ae13_1 &
	#sleep 1
	#run_clusteron cvm38 22223 a13se ae7.json ae13_2 &
	#sleep 1
	#run_clusteron cvm39 22224 a13se ae7.json ae13_3 &
	#sleep 1
	#run_clusteron cvm40 22222 a14se ae7.json ae14_1 &
	#sleep 1
	#run_clusteron cvm41 22223 a14se ae7.json ae14_2 &
	#sleep 1
	#run_clusteron cvm42 22224 a14se ae7.json ae14_3 &
	#sleep 1
	#run_clusteron cvm43 22222 a15se ae8.json ae15_1 &
	#sleep 1
	#run_clusteron cvm44 22223 a15se ae8.json ae15_2 &
	#sleep 1
	#run_clusteron cvm45 22224 a15se ae8.json ae15_3 &
	#sleep 1
	#run_clusteron cvm46 22222 a16se ae8.json ae16_1 &
	#sleep 1
	#run_clusteron cvm47 22223 a16se ae8.json ae16_2 &
	#sleep 1
	#run_clusteron cvm48 22224 a16se ae8.json ae16_3 &
	#sleep 1

	#run_clusteron cvm2 22222 m2se_0 me.json m2_de0 &
	#sleep 1
	#run_clusteron cvm3 22223 m2se_0 se.json m2_se0_1 &
	#sleep 1
	#run_clusteron cvm4 22224 m2se_0 se.json m2_se0_2 &
	#sleep 1
	#run_clusteron cvm3 22222 m3se_0 me.json m3_de0 &
	#sleep 1
	#run_clusteron cvm4 22223 m3se_0 se.json m3_se0_1 &
	#sleep 1
	#run_clusteron cvm1 22224 m3se_0 se.json m3_se0_2 &
	#sleep 1
	#run_clusteron cvm4 22222 m4se_0 me.json m4_de0 &
	#sleep 1
	#run_clusteron cvm1 22223 m4se_0 se.json m4_se0_1 &
	#sleep 1
	#run_clusteron cvm2 22224 m4se_0 se.json m4_se0_2 &
	#sleep 1

	# strong
	#i=0
	#while [ $i -lt 1 ]; do
	#	port=$((22222+$i))
	#	run_clusteron cvm 0 4 $port m1shard_$i m.json m1_d$i &
	#	sleep 1
	#	run_clusteron cvm 1 4 $port m2shard_$i m.json m2_d$i &
	#	sleep 1
	#	run_clusteron cvm 2 4 $port m3shard_$i m.json m3_d$i &
	#	sleep 1
	#	run_clusteron cvm 3 4 $port m4shard_$i m.json m4_d$i &
	#	sleep 1
	#	i=$(($i+1))
	#done
	#i=0
	#while [ $i -lt 1 ]; do
	#	port=$((33333+$i))
	#	run_clusteron cvm 2 4 $port m1shard_$i s.json m1_s$i &
	#	sleep 1
	#	run_clusteron cvm 3 4 $port m2shard_$i s.json m2_s$i &
	#	sleep 1
	#	run_clusteron cvm 0 4 $port m3shard_$i s.json m3_s$i &
	#	sleep 1
	#	run_clusteron cvm 1 4 $port m4shard_$i s.json m4_s$i &
	#	sleep 1
	#	i=$(($i+1))
	#done

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

function run_redis_wrapper {
	i=1
	while [ $i -le 1 ]; do
		port=$((11111+($i-1)%3))
		#echo $port
		run_redis cvm$i $port &
		sleep 1
		i=$(($i+1))
	done
}

function run_leveldb_wrapper {
	i=1
	while [ $i -le 1 ]; do
		run_leveldb cvm$i &
		sleep 1
		i=$(($i+1))
	done
}

function run_conkv_wrapper {
	#i=0
	#while [ $i -lt 3 ]; do
	#	port=$((11111+$i))
	#	run_ckv cvm 0 4 $port &
	#	sleep 1
	#	run_ckv cvm 1 4 $port &
	#	sleep 1
	#	run_ckv cvm 2 4 $port &
	#	sleep 1
	#	run_ckv cvm 3 4 $port &
	#	sleep 1
	#	i=$(($i+1))
	#done

	# strong
	run_ckv cvm1 11111 &
	sleep 1
	run_ckv cvm2 11112 &
	sleep 1
	run_ckv cvm3 11113 &
	sleep 1
	run_ckv cvm4 11111 &
	sleep 1
	run_ckv cvm5 11112 &
	sleep 1
	run_ckv cvm6 11113 &
	sleep 1
	run_ckv cvm7 11111 &
	sleep 1
	run_ckv cvm8 11112 &
	sleep 1
	run_ckv cvm9 11113 &
	sleep 1
	run_ckv cvm10 11111 &
	sleep 1
	run_ckv cvm11 11112 &
	sleep 1
	run_ckv cvm12 11113 &
	sleep 1
	#run_ckv cvm13 11111 &
	#sleep 1
	#run_ckv cvm14 11112 &
	#sleep 1
	#run_ckv cvm15 11113 &
	#sleep 1
	#run_ckv cvm16 11111 &
	#sleep 1
	#run_ckv cvm17 11112 &
	#sleep 1
	#run_ckv cvm18 11113 &
	#sleep 1
	#run_ckv cvm19 11111 &
	#sleep 1
	#run_ckv cvm20 11112 &
	#sleep 1
	#run_ckv cvm21 11113 &
	#sleep 1
	#run_ckv cvm22 11111 &
	#sleep 1
	#run_ckv cvm23 11112 &
	#sleep 1
	#run_ckv cvm24 11113 &
	#sleep 1
	#run_ckv cvm25 11111 &
	#sleep 1
	#run_ckv cvm26 11112 &
	#sleep 1
	#run_ckv cvm27 11113 &
	#sleep 1
	#run_ckv cvm28 11111 &
	#sleep 1
	#run_ckv cvm29 11112 &
	#sleep 1
	#run_ckv cvm30 11113 &
	#sleep 1
	#run_ckv cvm31 11111 &
	#sleep 1
	#run_ckv cvm32 11112 &
	#sleep 1
	#run_ckv cvm33 11113 &
	#sleep 1
	#run_ckv cvm34 11111 &
	#sleep 1
	#run_ckv cvm35 11112 &
	#sleep 1
	#run_ckv cvm36 11113 &
	#sleep 1
	#run_ckv cvm37 11111 &
	#sleep 1
	#run_ckv cvm38 11112 &
	#sleep 1
	#run_ckv cvm39 11113 &
	#sleep 1
	#run_ckv cvm40 11111 &
	#sleep 1
	#run_ckv cvm41 11112 &
	#sleep 1
	#run_ckv cvm42 11113 &
	#sleep 1
	#run_ckv cvm43 11111 &
	#sleep 1
	#run_ckv cvm44 11112 &
	#sleep 1
	#run_ckv cvm45 11113 &
	#sleep 1
	#run_ckv cvm46 11111 &
	#sleep 1
	#run_ckv cvm47 11112 &
	#sleep 1
	#run_ckv cvm48 11113 &
	#sleep 1

	# evnetual
	#run_ckv cvm1 11111 &
	#sleep 1
	#run_ckv cvm2 11112 &
	#sleep 1
	#run_ckv cvm3 11113 &
	#sleep 1
	#run_ckv cvm4 11111 &
	#sleep 1
	#run_ckv cvm5 11112 &
	#sleep 1
	#run_ckv cvm6 11113 &
	#sleep 1
	#run_ckv cvm7 11111 &
	#sleep 1
	#run_ckv cvm8 11112 &
	#sleep 1
	#run_ckv cvm9 11113 &
	#sleep 1
	#run_ckv cvm10 11111 &
	#sleep 1
	#run_ckv cvm11 11112 &
	#sleep 1
	#run_ckv cvm12 11113 &
	#sleep 1

	#run_ckv cvm2 11111 &
	#sleep 1
	#run_ckv cvm3 11112 &
	#sleep 1
	#run_ckv cvm4 11113 &
	#sleep 1
	#run_ckv cvm3 11111 &
	#sleep 1
	#run_ckv cvm4 11112 &
	#sleep 1
	#run_ckv cvm1 11113 &
	#sleep 1
	#run_ckv cvm4 11111 &
	#sleep 1
	#run_ckv cvm1 11112 &
	#sleep 1
	#run_ckv cvm2 11113 &
	#sleep 1
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
	#kill_clusteron cvm1 &
	#sleep 1
	#kill_clusteron cvm2 &
	#sleep 1
	#kill_clusteron cvm3 &
	#sleep 1
	#kill_clusteron cvm4 &
	#sleep 1
	#kill_clusteron cvm5 &
	#sleep 1
	#kill_clusteron cvm6 &
	#sleep 1
	#kill_clusteron cvm7 &
	#sleep 1
	#kill_clusteron cvm8 &
	#sleep 1
	#kill_clusteron cvm9 &
	#sleep 1
	#kill_clusteron cvm10 &
	#sleep 1
	#kill_clusteron cvm11 &
	#sleep 1
	#kill_clusteron cvm12 &
	#sleep 1
	
	i=1
	while [ $i -le 12 ]; do
		kill_clusteron cvm$i
		sleep 1
		i=$(($i+1))
	done
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
	i=1
	while [ $i -le 12 ]; do
		kill_conkv cvm$i
		sleep 1
		i=$(($i+1))
	done
}

function kill_redis_wrapper {
	i=1
	while [ $i -le 24 ]; do
		kill_redis cvm$i
		sleep 1
		i=$(($i+1))
	done
}

function kill_leveldb_wrapper {
	i=1
	while [ $i -le 24 ]; do
		kill_leveldb cvm$i
		sleep 1
		i=$(($i+1))
	done
}

case "$1" in
genssdbconf)
	gen_ssdb_conf_wrapper $*
	;;
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
runrds)
	run_redis_wrapper $*
	;;
runmc)
	run_memcached $*
	;;
runldb)
	run_leveldb_wrapper $*
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
killrds)
	kill_redis_wrapper $*
	;;
killldb)
	kill_leveldb_wrapper $*
	;;
*)
	echo "Usage: $0 (runcon|runckv|runredis|runmc|runb|...)"
	;;
esac
