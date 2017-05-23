# VOLTRON - README #


## Dependencies ##
* gcc 4.8 (required by folly)
* autoconf-2.69 (requires v >= 2.69)
* google-glog
* protobuf
* libopenssl
* gflags-master
* boost_1_55_0: ./b2 install
* folly (requires double_conversion)
* libuuid
* libevent
* lz4 1.7.1 (redlock requires v >= 1.7.1)
* zookeeper (server + C binding lib)


## How to compile? ##

### Compile Voltron ###

Go to the src directory, for debugging mode, run 'make'

To enable compiler level optimization, run 'make opti'


### Compile datalet application ###

Go to the apps directory, for debugging mode, run 'make'

To enable compiler level optimization, run 'make opti'


### Compile client lib ###

To compile client lib, go to the libckv dir and compile proto file:
```
cd apps/clibs/libckv
protoc --cpp_out=. ckv_proto.proto
```

Move the header file generated to the include folder:
```
cp *.h include/
```

Next, create a directory to compile the lib:
```
mkdir build
cd build/
cmake ..
make
```
libckv.a will be available in build directory

### Compile benchmark ###

First, compile the client lib as shown in previous step.

Then go to the bench directory and run 'make'


## How to run? ##

### Datalet backend ###

First, you should have a backend datalet running, e.g., a Redis node:  
Go to the Redis dir:  
```
$ ./redis-server --port 12346
```   

Under apps/ , we implemented two applications for Voltron. If you want the datalet backend to be a key-value store, type:
```
$ cd apps/ckv
$ ./conkv -l 192.168.0.170 -p 11111 -t 1 
```   

### Voltron ###

To run the Voltron executable, go to the src dir:  
```
$ ./conproxy --config /root/conrun/conf/c1.json --datalets /root/conrun/conf/d1.cfg --shard shard1 --proxyAddr 192.168.0.170 --proxyClientPort 12345 
```

Configuration files include:   
A JSON formatted file specifying all options (num_replicas option might be a bit confusing and here it indicates how many replicas excluding the master replica), as below:
```
{
	"zk_host": "192.168.0.173:2181",
	"kafka_broker": "192.168.0.173:9092",
	"consistency_model": "strong",
	"consistency_tech": "cr",
	"topology": "ms",
	"num_replicas": "2",
}
```   
And a datalet list file specifying all datalets, as below:
```
# 0: master; 1: slave
192.168.0.171:11111:0
192.168.0.171:11112:1
192.168.0.171:11113:1
```
### Benchmarking ###

To run the YCSB trace bench:
```
$ cd bench
$ ./bench_client -d 40 -l trace_dir/kv1M_op1M_uniform_text.run -t 32 -m 6 -r 2 -f hosts.cfg -R 0 -W 0
```
Where hosts.cfg is an example host file including all hosts, -m indicates how many hosts, -r indicates how many replicas, -R specifies which replica to serve READ (-1 means any replica), and -W indicates which replica to serve WRITE (again, -1 means any replica for active/active topology).
Example host file is shown as below:
```
192.168.0.171:12345 192.168.0.171:12348
192.168.0.171:12346 192.168.0.171:12349
192.168.0.171:12347 192.168.0.171:12350
192.168.0.172:12345 192.168.0.172:12348
192.168.0.172:12346 192.168.0.172:12349
192.168.0.172:12347 192.168.0.172:12350
```

To run the Redis benchmark:  
```
$ ./redis-benchmark -h hulk0 -p 12345 -c 50 -n 100000 -t set,get -P 32 -r 1000000  
```   
This will send requests to conproxy, which will serve as a proxy forwarding requests between benchmark clients and Redis backend datalets.

### Deployment ###

To launch zk and MQ on cloud, run:
```
bin/zookeeper-server-start.sh -daemon config/zookeeper.properties
bin/MQ-server-start.sh -daemon config/server.properties
```

To launch a cluster of Voltron + conkv nodes, first add the data node info in slap.sh, then run:  
```
$ cd scripts  
$ ./slap.sh runckv  
$ ./slap.sh runcon
```
Docker based Voltron is ==PARTIALLY== supported. To run containerized deployment:  
```
$ ./slap.sh docker_runckv
$ ./slap.sh docker_runcon
```

### Configuration file setting for different scenarios ###
```
-----Master-Slave topology with strong consistency-----

Master:
{
	"zk_host": "192.168.0.173:2181",
	"kafka_broker": "192.168.0.173:9092",
	"consistency_model": "strong",
	"consistency_tech": "cr",
	"topology": "ms",
	"num_replicas": "2",
}

Slave:
{
	"zk_host": "192.168.0.173:2181",
	"kafka_broker": "192.168.0.173:9092",
	"consistency_model": "strong",
	"consistency_tech": "cr",
	"topology": "slave",
	"num_replicas": "0",
}


-----Master-Slave topology with eventual consistency-----

Master:
{
	"zk_host": "192.168.0.173:2181",
	"kafka_broker": "192.168.0.173:9092",
	"consistency_model": "eventual",
	"consistency_tech": "cr",
	"topology": "ms",
	"num_replicas": "0",
}

Slave:
{
	"zk_host": "192.168.0.173:2181",
	"kafka_broker": "192.168.0.173:9092",
	"consistency_model": "evenual",
	"consistency_tech": "cr",
	"topology": "slave",
	"num_replicas": "0",
}


-----Master-Slave topology without consistency-----

Master:
{
	"zk_host": "192.168.0.173:2181",
	"kafka_broker": "192.168.0.173:9092",
	"consistency_model": "without",
	"consistency_tech": "none",
	"topology": "ms",
	"num_replicas": "2",
}

Slave:
{
	"zk_host": "192.168.0.173:2181",
	"kafka_broker": "192.168.0.173:9092",
	"consistency_model": "without",
	"consistency_tech": "none",
	"topology": "slave",
	"num_replicas": "0",
}

-----No topology and without consistency-----
{
	"zk_host": "172.17.0.2:2181",
	"kafka_broker": "172.17.0.3:9092",
	"consistency_model": "without",
	"consistency_tech": "none",
	"topology": "no",
	"num_replicas": "0",
}


-----Active-Active toplogy with strong consistency-----

{
	"zk_host": "172.17.0.2:2181",
	"kafka_broker": "172.17.0.3:9092",
	"consistency_model": "strong",
	"consistency_tech": "rl",
	"topology": "aa",
	"num_replicas": "0",
	"rl_host": "127.0.0.1:12121"
}

-----Active-Active toplogy with eventual consistency-----

{
	"zk_host": "192.168.0.173:2181",
	"kafka_broker": "192.168.0.173:9092",
	"consistency_model": "eventual",
	"consistency_tech": "cr",
	"topology": "aa",
	"num_replicas": "0",
} 


```
