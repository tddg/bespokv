#!/usr/bin/env bash

# Please change this according to your ycsb installation
# so that, ${YCSB_HOME}/bin/ycsb is the binary 
YCSB_HOME="/root/Memcaching/ycsb-0.10.0"

workload_spec=kv1M
workload_dir=ycsb_workload_dir

for setting in `ls ${workload_spec}`
do
    echo using predefined workloadb to create transaction records for $setting with 5% updates
    echo generateing $setting.load, the insertions used before benchmark
    ${YCSB_HOME}/bin/ycsb load basic -P ${YCSB_HOME}/workloads/workloadb -P ./${workload_spec}/${setting} \
		> ./${workload_dir}/${setting}.load
    echo generateing $setting.run, the lookup queries used before benchmark
    ${YCSB_HOME}/bin/ycsb run basic -P ${YCSB_HOME}/workloads/workloadb -P ./${workload_spec}/${setting} \
		> ./${workload_dir}/${setting}.run

    # echo using predefined workloadc to create transaction records for $setting with reads only 
    # echo generateing $setting.load, the insertions used before benchmark
    # ${YCSB_HOME}/bin/ycsb load basic -P ${YCSB_HOME}/workloads/workloadc -P ./workloads/$setting.dat > $setting.load
    # echo generateing $setting.run, the lookup queries used before benchmark
    # ${YCSB_HOME}/bin/ycsb run basic -P ${YCSB_HOME}/workloads/workloadc -P ./workloads/$setting.dat > $setting.run
done
