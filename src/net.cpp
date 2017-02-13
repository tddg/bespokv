#include "server.h"
#include "net.h"
#include "resp.h"
#include "proxy_util.h"
#include "co_ketama.h"
#include <errno.h>

#include "spec.pb.h" // yue: protobuf msg format spec

bool client_conn_ = false;

NetworkServer::NetworkServer() : server_addr(std::string("127.0.0.1")), server_port(12346) {
	client_listen_conn = Link::listen("hulk0", 12345);
	if(client_listen_conn == NULL){
		fprintf(stderr, "error opening server socket! %s\n", strerror(errno));
		exit(1);
	}
	// yue: set up client listening event
	fdes = new Fdevents();
	fdes->set(client_listen_conn->fd(), FDEVENT_IN, 0, client_listen_conn);

	fdes->set_proxy(this);

	dumb_conn = new Link();
	rstatus_t status = dumb_conn->connect(server_addr.c_str(), server_port);
	if(status != CO_OK){
		fprintf(stderr, "error opening server socket! %s\n", strerror(errno));
		exit(1);
	}
	dumb_conn->is_server = true; // yue: TODO: later will move it to private 
	dumb_conn->set_nw_func();

	do_consistent_hashing = false;

	num_free_conn = 0;
	conn_count = 0;
}

NetworkServer::NetworkServer(
		const std::string &proxy_addr, 
		int32_t proxy_port, 
		const std::string &server_addr, 
		int32_t server_port,
		bool do_ketama,
		int32_t back_conn)
		: server_addr(server_addr), server_port(server_port) {
	client_listen_conn = Link::listen(proxy_addr.c_str(), proxy_port);
	if(client_listen_conn == NULL){
		fprintf(stderr, "error opening server socket! %s\n", strerror(errno));
		exit(1);
	}
	// yue: set up client listening event
	fdes = new Fdevents();
	fdes->set(client_listen_conn->fd(), FDEVENT_IN, 0, client_listen_conn);

	fdes->set_proxy(this);

	do_consistent_hashing = do_ketama;

	server_back_conn = back_conn; 

	num_free_conn = 0;
	conn_count = 0;
}

DumbNodeTable_t parse_dumb_list(const std::string &config_file) {

	auto config = load_config_file(config_file);
	//CHECK(config.isArray());
	LOG(INFO) << "config.size()=" << config.size();
	auto dumb_list_file = folly::toStdString(config.getDefault("dumb_list", "dumblist.txt").asString());
	DumbNodeTable_t dumb_map = parse_dumbnodes_std_str(dumb_list_file);

	return 	dumb_map;
}

/**
 * yue: This is the right constructor that should always be used
 */
NetworkServer::NetworkServer(
		const std::string &proxy_addr, 
		int32_t proxy_client_port, 
		int32_t proxy_peer_port,
		const std::string &config_file,
		const std::string &datalet_file,
		const std::string &shard,
		const bool recover) : do_consistent_hashing(false) {
	client_listen_conn = Link::listen(proxy_addr.c_str(), proxy_client_port);
	if(client_listen_conn == NULL){
		fprintf(stderr, "error opening server socket on client listen port! %s\n", strerror(errno));
		exit(1);
	}
	// yue: set up client listening event
	fdes = new Fdevents();
	fdes->set(client_listen_conn->fd(), FDEVENT_IN, 0, client_listen_conn);

	fdes->set_proxy(this);
#ifdef USE_KETAMA
	do_consistent_hashing = do_ketama;
#endif

	is_master_proxy = true;
	num_free_conn = 0;
	server_back_conn = 1024; //ali: FIXME: fix hard coded value
	conn_count = 0;

	if (access(config_file.c_str(), F_OK) == -1)	return;
	auto config = load_config_file(config_file);
	//CHECK(config.isArray());
	
	LOG(INFO) << "config.size()=" << config.size();
	//auto datalet_list_file = folly::toStdString(config.getDefault("datalet_list", "datalet_list.txt").asString());
	dumb_map = parse_dumbnodes_std_str(datalet_file);

	auto zk_host_str = folly::toStdString(config.getDefault("zk_host", "localhost:2181").asString());
	auto kafka_broker_str = folly::toStdString(config.getDefault("kafka_broker", "172.17.0.3:9092").asString());
	kafka_broker = kafka_broker_str;
	kafka_topic = shard;
	zk_host = zk_host_str;
	replicas = config.getDefault("num_replicas", 2).asInt();
	recovery = recover;

	auto rl_server_str = folly::toStdString(config.getDefault("rl_host", "127.0.0.:12121").asString());

	std::size_t pos = rl_server_str.find(":");
	rl_port = atoi(rl_server_str.substr(pos+1).c_str()) ;
	rl_host = rl_server_str.substr(0, pos);

	auto consistency_model_str = folly::toStdString(config.getDefault("consistency_model", "strong").asString());
	if (consistency_model_str == "strong") {
		consistency_model = strong;
	} else if (consistency_model_str == "eventual") {
		consistency_model = eventual;
	} else if (consistency_model_str == "without") {
		consistency_model = without;
	}

	auto consistency_tech_str = folly::toStdString(config.getDefault("consistency_tech", "cr").asString());
	if (consistency_tech_str == "cr") {
		consistency_technique = cr;
	} else if (consistency_tech_str == "zk") {
		consistency_technique = zk;
	} else if (consistency_tech_str == "redis") {
		consistency_technique = redis;
	} else if (consistency_tech_str == "rl") {
		consistency_technique = rl;
	}

	auto topology_str = folly::toStdString(config.getDefault("topology", "ms").asString());
	if (topology_str == "ms") {
		topology = ms;
	} else if (topology_str == "no") {
		topology = no;
	} else if (topology_str == "slave") {
		topology = slave;
		is_master_proxy = false;
	} else if (topology_str == "aa") {
		topology = aa;
	}
}

NetworkServer::~NetworkServer() {
	if (zkc)	delete zkc;
	if (fdes)	delete fdes;
	if (fdes_rpl)	delete fdes_rpl;
	if (client_listen_conn)	delete client_listen_conn;
	if (peer_listen_conn)	delete peer_listen_conn;
	if (dumb_conn)	delete dumb_conn;
	
	lazy_consumer->join();
	if (lazy_consumer)	delete lazy_consumer;
}

Link *NetworkServer::get_conn() {
	Link *conn = new Link();
	CHECK(conn != NULL);
	conn->is_server = true;
	return conn;
	if (!free_conn_pool.empty()) {
		//fprintf(stderr, "Using a free connection!\n");
		CHECK(this->num_free_conn > 0);
		conn = free_conn_pool.front(); 
		this->num_free_conn--;
		free_conn_pool.pop_front();
	}
	// Check if we have any free connection
	else if (conn_count < server_back_conn) {
		conn = new Link();
		//fprintf(stderr, "Launching a connection... %d!\n", server_back_conn);
		if(conn == NULL){
			fprintf(stderr, "error alloc conn!\n");
			exit(1);
		}

		conn_pool.push_back(conn);
	}
	// Round robin use the available connections once we have reached the optimal limit
	else {
		//fprintf(stderr, "Round robin connection!\n");
		conn = conn_pool.front();	//ali TODO: Need NULL check?
		conn_pool.pop_front();
		conn_pool.push_back(conn);
	}

	// Mark as master and add an entry
	conn->is_server = true;
	master_conn_pool.push_back(conn);

	return conn;
}

/*
 * From the available connection provides a new connection to be used as slave
 * Also updates the ms_map for proxy
 */
Link *NetworkServer::get_slave_conn(Link *master) {
	Link *conn = new Link();
	conn->is_server = false;
	return conn;
	if (!free_conn_pool.empty()) {
		//fprintf(stderr, "Using a free connection!\n");
		CHECK(this->num_free_conn > 0);
		conn = free_conn_pool.front();
		this->num_free_conn--;
		free_conn_pool.pop_front();
	}
	// Check if we have any free connection
	else if (conn_count < server_back_conn) {
		conn = new Link();
		//fprintf(stderr, "Launching a connection... %d!\n", server_back_conn);
		if(conn == NULL){
			fprintf(stderr, "error alloc conn!\n");
			exit(1);
		}

		conn_pool.push_back(conn);
	}
	// Round robin use the available connections once we have reached the optimal limit
	else {
		//fprintf(stderr, "Round robin connection!\n");
		conn = conn_pool.front();	//ali TODO: Need NULL check?
		conn_pool.pop_front();
		conn_pool.push_back(conn);
	}

	// Mark as slave and add an entry
	conn->is_server = false;
	slave_conn_pool.push_back(conn);

	return conn;
}

/*
 * Returns the slaves of master
 */
std::vector<Link *> NetworkServer::get_slaves(Link *master) {
	std::vector<Link *> slaves;
	for (std::vector<std::pair<Link *, std::vector<Link *>>>::const_iterator i = ms_map.begin(); i != ms_map.end(); i++) {
		if (i->first == master) {
			slaves =  i->second;
			break;
		}
	}
	return slaves;
}

/*
 * Returns the slave of master at index number
 */
Link *NetworkServer::get_slave_number(Link *master, int number) {
	std::vector<Link *> slaves;
	for (std::vector<std::pair<Link *, std::vector<Link *>>>::const_iterator i = ms_map.begin(); i != ms_map.end(); i++) {
		if (i->first == master) {
			return i->second.at(number);
		}
	}
	return NULL;
}

/*
 * Returns the master connection of a slave connection
 */
Link *NetworkServer::get_master_of_slave(Link *slave) {
#if 0
	if (slave == NULL) {
		CHECK(ms_map.size() == 1);
		return ms_map[0].first;
	}
#endif
	Link *master = NULL;
	for (std::vector<std::pair<Link *, std::vector<Link *>>>::const_iterator i = ms_map.begin(); i != ms_map.end(); i++) {
		if (std::find(i->second.begin(), i->second.end(), slave) != i->second.end()){
			master = i->first;
			break;
		}
	}
	return master;
}


void NetworkServer::put_conn(Link *conn) {
	free_conn_pool.push_back(conn);
	num_free_conn++;
}

Link *NetworkServer::get_client_conn() {
	return get_conn();
}

Link *NetworkServer::establish_s_conn(Link *c_conn) {
	Link *server_conn = get_conn();
	rstatus_t status = server_conn->connect("127.0.0.1", 11116);
	if (status != CO_OK) {
		fprintf(stderr, "error init socket: %s\n", strerror(errno));
		//exit(1);
		return NULL;
	}
	
	server_conn->nodelay();
	server_conn->noblock();
	server_conn->create_time = millitime();
	server_conn->active_time = server_conn->create_time;
	server_conn->is_server = true;
	server_conn->set_nw_func();

	c_conn->s_conn = server_conn;
	return server_conn;
}

// Launch a new server connection for each incoming client connection
Link *NetworkServer::establish_server_conn() {
	Link *server_conn = get_conn();
	rstatus_t status = server_conn->connect(this->server_addr.c_str(), this->server_port);
	if (status != CO_OK) {
		fprintf(stderr, "error init socket: %s\n", strerror(errno));
		//exit(1);
		return NULL;
	}

	server_conn->nodelay();
	server_conn->noblock();
	server_conn->create_time = millitime();
	server_conn->active_time = server_conn->create_time;
	server_conn->is_server = true; // yue: TODO: later will move it to private
	server_conn->set_nw_func();

	// launch slave 1
	Link *slave1 = get_slave_conn(server_conn);
	rstatus_t status1 = server_conn->connect(this->server_addr.c_str(), this->server_port + 1);
	if (status1 != CO_OK) {
		fprintf(stderr, "error init socket: %s\n", strerror(errno));
		//exit(1);
		return NULL;
	}
	slave1->nodelay();
	slave1->noblock();
	slave1->create_time = millitime();
	slave1->active_time = slave1->create_time;
	slave1->is_server = true;
	slave1->set_repl_func(); // set different functions pointers to distinguish between replica and master

	//launch slave 2
	Link *slave2 = get_slave_conn(server_conn);
	rstatus_t status2 = server_conn->connect(this->server_addr.c_str(), this->server_port + 2);
	if (status2 != CO_OK) {
		fprintf(stderr, "error init socket: %s\n", strerror(errno));
		//exit(1);
		return NULL;
	}
	slave2->nodelay();
	slave2->noblock();
	slave2->create_time = millitime();
	slave2->active_time = slave2->create_time;
	slave2->is_server = true;
	slave2->set_repl_func(); // set different functions pointers to distinguish between replica and master

	std::pair <Link *, std::vector<Link *>> pair;
	pair.first = server_conn;
	pair.second = {slave1, slave2};
	ms_map.push_back(pair);

	return server_conn;
}

// Establish datalet (server) connections for each incoming client connection
void NetworkServer::establish_server_conn_each(Link *c_conn) {
	DumbNodeTable_t::iterator it;
	int32_t total_weight = 0;
	// Data structure to populate the map
	std::pair <Link *, std::vector<Link *>> pair;
	int s_number = 0;
	for (it = dumb_map.begin(); it != dumb_map.end(); it++) {
		LOG(INFO) << "backend=" << it->second.addr << ":" << it->second.port << ":" << it->second.weight;
		if (this->topology == ms) {
			if (it->second.replica_role == master_t) {
				Link *server_conn = get_conn();
				CHECK(server_conn != NULL);
				//rstatus_t status = server_conn->connect(this->server_addr.c_str(), this->server_port);
				fprintf(stderr, "connect to master:%s:%d; num_replicas:%zu\n", it->second.addr.c_str(), it->second.port, dumb_map.size());
				rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
				if (status != CO_OK) {
					fprintf(stderr, "error init socket: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
					//return NULL;
				}

				server_conn->dumb_addr = it->first;
				LOG(INFO) << "backend.key=" << server_conn->dumb_addr;
				//server_conn->dumb_port = it->second.port;
				server_conn->weight = it->second.weight;
				total_weight += it->second.weight;
				server_conn->nodelay();
				server_conn->noblock();
				server_conn->create_time = millitime();
				server_conn->active_time = server_conn->create_time;
				server_conn->is_server = true; // yue: TODO: later will move it to private
				server_conn->is_master = true;
				server_conn->number_slaves = this->replicas;
				server_conn->set_nw_func();
				server_conn->slave_number = - 1;
				server_conn->is_client = false;
				pair.first = server_conn; // key is the master node

				// FIXME: Undo the hard coded part
				if (this->consistency_technique == zk) {
					// Launch connection to ZK per client per connection to datalet
					server_conn->zkc = new zookeeper::ZooKeeper("localhost:2181,localhost:2182");
				}

#ifdef USE_KAFKA
				server_conn->kproducer = new conkafka::COnKafka_Producer(this->kafka_broker, this->kafka_topic);
				//server_conn->kconsumer = new conkafka::COnKafka_Consumer(this->kafka_broker, "fake error", "ClusterOn");
#endif

#ifdef USE_KETAMA
				c_conn->insert_dumbconn(it->first, server_conn);
#else
				c_conn->set_server_conn(server_conn);
#endif
			} else if (it->second.replica_role == slave_t) {
				// get the slave conn object
				Link *slave = get_slave_conn();

				// FIXME: fixe the hardcoded ip and port for replicas
				//std::pair <std::string, std::vector<std:string>> local_pair = this->local_cluster_map.;

				// TODO: fix the hardcoded port and ip use from the local_pair
				fprintf(stderr, "connect to slave:%s:%d\n", it->second.addr.c_str(), it->second.port);
				rstatus_t status1 = slave->connect(it->second.addr.c_str(), it->second.port);
				if (status1 != CO_OK) {
					fprintf(stderr, "error init socket: %s\n", strerror(errno));
					//exit(1);
					exit(EXIT_FAILURE);
				}
				slave->nodelay();
				slave->noblock();
				slave->create_time = millitime();
				slave->active_time = slave->create_time;
				slave->is_server = true;
				slave->is_master = false;
				slave->number_slaves = -1;
				slave->is_client = false;
				slave->slave_number = s_number;
				slave->set_repl_func();

				pair.second.push_back(slave);
				
				s_number++;
				// FIXME: Undo the hard coded part
				if (this->consistency_technique == zk) {
					// Launch connection to ZK per client per connection to datalet
					slave->zkc = new zookeeper::ZooKeeper("localhost:2183");
				}
			}
		} else if (this->topology == slave) {
			Link *server_conn = get_conn();
			CHECK(server_conn != NULL);
			//rstatus_t status = server_conn->connect(this->server_addr.c_str(), this->server_port);
			fprintf(stderr, "connect to slave:%s:%d; num_replicas:%zu\n", it->second.addr.c_str(), it->second.port, dumb_map.size());
			rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
			if (status != CO_OK) {
				fprintf(stderr, "error init socket: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
				//return NULL;
			}

			server_conn->dumb_addr = it->first;
			LOG(INFO) << "backend.key=" << server_conn->dumb_addr;
			//server_conn->dumb_port = it->second.port;
			server_conn->weight = it->second.weight;
			total_weight += it->second.weight;
			server_conn->nodelay();
			server_conn->noblock();
			server_conn->create_time = millitime();
			server_conn->active_time = server_conn->create_time;
			server_conn->is_server = true; // yue: TODO: later will move it to private
			server_conn->is_master = true;
			server_conn->number_slaves = this->replicas;
			server_conn->set_nw_func();
			server_conn->slave_number = -1;
			server_conn->is_client = false;

#ifdef USE_KETAMA
				c_conn->insert_dumbconn(it->first, server_conn);
#else
				c_conn->set_server_conn(server_conn);
#endif
		} else if (this->topology == aa) {

			if (this->consistency_model == eventual) {
				/* yue: TODO */
				if (it->second.replica_role == master_t) {
					Link *server_conn = get_conn();
					CHECK(server_conn != NULL);
					//rstatus_t status = server_conn->connect(this->server_addr.c_str(), this->server_port);
					fprintf(stderr, "connect to master:%s:%d; num_replicas:%zu\n", it->second.addr.c_str(), it->second.port, dumb_map.size());
					rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
					if (status != CO_OK) {
						fprintf(stderr, "error init socket: %s\n", strerror(errno));
						exit(EXIT_FAILURE);
						//return NULL;
					}

					server_conn->dumb_addr = it->first;
					LOG(INFO) << "backend.key=" << server_conn->dumb_addr;
					//server_conn->dumb_port = it->second.port;
					server_conn->weight = it->second.weight;
					total_weight += it->second.weight;
					server_conn->nodelay();
					server_conn->noblock();
					server_conn->create_time = millitime();
					server_conn->active_time = server_conn->create_time;
					server_conn->is_server = true; // yue: TODO: later will move it to private
					server_conn->is_master = true;
					server_conn->number_slaves = this->replicas;
					server_conn->set_nw_func();
					server_conn->slave_number = - 1;
					server_conn->is_client = false;
					pair.first = server_conn; // key is the master node

					// FIXME: Undo the hard coded part
					if (this->consistency_technique == zk) {
						// Launch connection to ZK per client per connection to datalet
						server_conn->zkc = new zookeeper::ZooKeeper("localhost:2181,localhost:2182");
					}

	#ifdef USE_KAFKA
					server_conn->kproducer = new conkafka::COnKafka_Producer(this->kafka_broker, this->kafka_topic);
					//server_conn->kconsumer = new conkafka::COnKafka_Consumer(this->kafka_broker, "fake error", "ClusterOn");
	#endif

	#ifdef USE_KETAMA
					c_conn->insert_dumbconn(it->first, server_conn);
	#else
					c_conn->set_server_conn(server_conn);
	#endif
				}
				else {

					// AA there should not be a slave
					CHECK(0);
				}
			}
			if (this->consistency_model == strong) {
				if (it->second.replica_role == master_t) {
					Link *server_conn = get_conn();
					CHECK(server_conn != NULL);
					//rstatus_t status = server_conn->connect(this->server_addr.c_str(), this->server_port);
					fprintf(stderr, "connect to master:%s:%d; num_replicas:%zu\n", it->second.addr.c_str(), it->second.port, dumb_map.size());
					rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
					if (status != CO_OK) {
						fprintf(stderr, "error init socket: %s\n", strerror(errno));
						exit(EXIT_FAILURE);
						//return NULL;
					}

					server_conn->dumb_addr = it->first;
					LOG(INFO) << "backend.key=" << server_conn->dumb_addr;
					//server_conn->dumb_port = it->second.port;
					server_conn->weight = it->second.weight;
					total_weight += it->second.weight;
					server_conn->nodelay();
					server_conn->noblock();
					server_conn->create_time = millitime();
					server_conn->active_time = server_conn->create_time;
					server_conn->is_server = true; // yue: TODO: later will move it to private
					server_conn->is_master = true;
					server_conn->number_slaves = this->replicas;
					server_conn->set_nw_func();
					server_conn->slave_number = - 1;
					server_conn->is_client = false;
					pair.first = server_conn; // key is the master node

					// FIXME: Undo the hard coded part
					if (this->consistency_technique == zk) {
						// Launch connection to ZK per client per connection to datalet
						server_conn->zkc = new zookeeper::ZooKeeper("localhost:2181,localhost:2182");
					}

	#ifdef USE_KAFKA
					server_conn->kproducer = new conkafka::COnKafka_Producer(this->kafka_broker, this->kafka_topic);
					//server_conn->kconsumer = new conkafka::COnKafka_Consumer(this->kafka_broker, "fake error", "ClusterOn");
	#endif

	#ifdef USE_KETAMA
					c_conn->insert_dumbconn(it->first, server_conn);
	#else
					c_conn->set_server_conn(server_conn);
	#endif
					// AA strong topology requires locking
					server_conn->conlock = new ConLock(this->rl_host.c_str(), this->rl_port);

				} else if (it->second.replica_role == slave_t) {
					// get the slave conn object
					Link *slave = get_slave_conn();

					// FIXME: fixe the hardcoded ip and port for replicas
					//std::pair <std::string, std::vector<std:string>> local_pair = this->local_cluster_map.;

					// TODO: fix the hardcoded port and ip use from the local_pair
					fprintf(stderr, "connect to slave:%s:%d\n", it->second.addr.c_str(), it->second.port);
					rstatus_t status1 = slave->connect(it->second.addr.c_str(), it->second.port);
					if (status1 != CO_OK) {
						fprintf(stderr, "error init socket: %s\n", strerror(errno));
						//exit(1);
						exit(EXIT_FAILURE);
					}
					slave->nodelay();
					slave->noblock();
					slave->create_time = millitime();
					slave->active_time = slave->create_time;
					slave->is_server = true;
					slave->is_master = false;
					slave->number_slaves = -1;
					slave->is_client = false;
					slave->slave_number = s_number;
					slave->set_repl_func();

					pair.second.push_back(slave);

					s_number++;
					// FIXME: Undo the hard coded part
					if (this->consistency_technique == zk) {
						// Launch connection to ZK per client per connection to datalet
						slave->zkc = new zookeeper::ZooKeeper("localhost:2183");
					}
				}
			}
		} else if (this->topology == no) {
			Link *server_conn = get_conn();
			CHECK(server_conn != NULL);
			//rstatus_t status = server_conn->connect(this->server_addr.c_str(), this->server_port);
			fprintf(stderr, "connect to master:%s:%d; num_replicas:%zu\n", it->second.addr.c_str(), it->second.port, dumb_map.size());
			rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
			if (status != CO_OK) {
				fprintf(stderr, "error init socket: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			server_conn->dumb_addr = it->first;
			LOG(INFO) << "backend.key=" << server_conn->dumb_addr;
			total_weight += it->second.weight;
			server_conn->nodelay();
			server_conn->noblock();
			server_conn->create_time = millitime();
			server_conn->active_time = server_conn->create_time;
			server_conn->is_server = true; // yue: TODO: later will move it to private
			server_conn->is_master = true;
			server_conn->number_slaves = 0;
			server_conn->set_nw_func();
			server_conn->slave_number = -1;
			server_conn->is_client = false;
			pair.first = server_conn; // key is the master node

			// FIXME: Undo the hard coded part
			if (this->consistency_technique == zk) {
				// Launch connection to ZK per client per connection to datalet
				server_conn->zkc = new zookeeper::ZooKeeper("localhost:2181,localhost:2182");
			}

#ifdef USE_KAFKA
			server_conn->kproducer = new conkafka::COnKafka_Producer(this->kafka_broker, this->kafka_topic);
			//server_conn->kconsumer = new conkafka::COnKafka_Consumer(this->kafka_broker, "fake error", "ClusterOn");
#endif

#ifdef USE_KETAMA
			c_conn->insert_dumbconn(it->first, server_conn);
#else
			c_conn->set_server_conn(server_conn);
#endif
		}

	}

	if (this->topology == ms) {
		// push back to the local data structure
		ms_map.push_back(pair);
	} else if (this->topology == aa && this->consistency_model == strong) {
		ms_map.push_back(pair);
	}



#ifdef DO_KETAMA
	CHECK(total_weight > 0);
	c_conn->init_continuum(total_weight);
#endif
}

Link *NetworkServer::accept_conn() {
	Link *conn = get_client_conn(); 
	rstatus_t status = client_listen_conn->accept(conn);
	if (status != CO_OK) {
		fprintf(stderr, "accept failed: %s\n", strerror(errno));
		return NULL;
	}
	conn_count++;
	conn->nodelay();
	conn->noblock();
	conn->create_time = millitime();
	conn->active_time = conn->create_time;

	conn->is_server = false;
	conn->set_nw_func();

	CHECK(conn->recv_nw !=NULL && conn->send_nw !=NULL);

	return conn;
}

void NetworkServer::accept_conn(Link *conn) {
	rstatus_t status = client_listen_conn->accept(conn);
	if (status != CO_OK) {
		fprintf(stderr, "accept failed: %s\n", strerror(errno));
		return;
	}
	conn_count++;
	conn->nodelay();
	conn->noblock();
	conn->create_time = millitime();
	conn->active_time = conn->create_time;

	conn->is_server = false;
	conn->set_nw_func();

	CHECK(conn->recv_nw !=NULL && conn->send_nw !=NULL);
}

Link *NetworkServer::accept_peer_conn() {
	Link *conn = get_client_conn(); 
	rstatus_t status = peer_listen_conn->accept(conn);
	if (status != CO_OK) {
		fprintf(stderr, "accept peer conn failed: %s\n", strerror(errno));
		return NULL;
	}
	conn_count++;
	conn->nodelay();
	conn->noblock();
	conn->create_time = millitime();
	conn->active_time = conn->create_time;

	//conn->type = ADMIN;
	conn->type = PROP;
	conn->is_server = false;
	conn->set_nw_func();

	CHECK(conn->recv_nw !=NULL && conn->send_nw !=NULL);

	return conn;
}

void NetworkServer::insert_conn_map(Link *key, Link *val) {
	this->conn_map.insert(std::pair<Link *, Link *>(key, val));
}

void NetworkServer::erase_conn_map(Link *key) {
	this->conn_map.erase(key);
}

Link *NetworkServer::get_connection_to_local_dumbnode() {
	Link *server_conn;
	DumbNodeTable_t::iterator it;
	for (it = dumb_map.begin(); it != dumb_map.end(); it++) {
		if (this->topology == slave) {
			server_conn = get_conn();
			CHECK(server_conn != NULL);
			rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
			if (status != CO_OK) {
				fprintf(stderr, "consumer error init socket: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			server_conn->noblock();
			server_conn->nodelay();

			return server_conn;
		}
		if (this->topology == ms) {
			if (it->second.replica_role == master_t) {
				server_conn = get_conn();
				CHECK(server_conn != NULL);
				rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
				if (status != CO_OK) {
					fprintf(stderr, "error init socket: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				server_conn->noblock();
				server_conn->nodelay();

				return server_conn;
			}
		}
	}
}


rstatus_t NetworkServer::consume_kafka() {

	conkafka::COnKafka_Consumer *kconsumer = new conkafka::COnKafka_Consumer(this->kafka_broker, this->kafka_topic);
	int64_t start_offset = RdKafka::Topic::OFFSET_BEGINNING;
	kconsumer->set_offset("kevin", start_offset);

	// Create connection with datalet provided in the cfg file and consume from kafka
	Link *server_conn;
	DumbNodeTable_t::iterator it;
	for (it = dumb_map.begin(); it != dumb_map.end(); it++) {
		if (this->topology == slave) {
			server_conn = get_conn();
			CHECK(server_conn != NULL);
			//rstatus_t status = server_conn->connect(this->server_addr.c_str(), this->server_port);
			fprintf(stderr, "connect to kafka datalet:%s:%d; num_replicas:%zu\n", it->second.addr.c_str(), it->second.port, dumb_map.size());
			rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
			if (status != CO_OK) {
				fprintf(stderr, "consumer error init socket: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			server_conn->noblock();
			server_conn->nodelay();
		}
		else if (this->topology == aa) {
			server_conn = get_conn();
			CHECK(server_conn != NULL);
			//rstatus_t status = server_conn->connect(this->server_addr.c_str(), this->server_port);
			fprintf(stderr, "connect to kafka datalet:%s:%d; num_replicas:%zu\n", it->second.addr.c_str(), it->second.port, dumb_map.size());
			rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
			if (status != CO_OK) {
				fprintf(stderr, "consumer error init socket: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			server_conn->noblock();
			server_conn->nodelay();
		} else
			return CO_ERR;
	}

	// Now we start lazy consumption of data
	const Fdevents::events_t *events_rpl;

	Fdevents *fdes_rpl = this->get_fdes_rpl();
	fdes_rpl->set(server_conn->fd(), FDEVENT_OUT, 1, server_conn);

	repeat1:
	RdKafka::Message *msg_kafka = kconsumer->consume("kevin", false);
	std::shared_ptr<Buffer> req = std::make_shared<Buffer>(msg_kafka->len());
	req->append(static_cast<const char *>(msg_kafka->payload()), msg_kafka->len());
	if (msg_kafka->len() == 0)
		goto repeat1;

	if (server_conn && server_conn->imsg_q.empty()) {
		Fdevents *fdes = this->get_fdes_rpl();
		fdes->set(server_conn->fd(), FDEVENT_OUT, 1, server_conn);
		ev_counter++;
	}
	server_conn->imsg_q.push_back(req);
	int count = 1;
	delete msg_kafka;

	while (1) {
		events_rpl = fdes_rpl->wait(50);
		if (events_rpl == NULL) {
			fprintf(stderr, "events.wait error: %s", strerror(errno));
			return CO_ERR;
		}

		for (int i = 0; i < (int)events_rpl->size(); i++) {
			const Fdevent *fdes_rpl = events_rpl->at(i);

			if (fdes_rpl->events & FDEVENT_IN) { // ali: Stupid piece of code wasted half day
				Link *conn = (Link *)fdes_rpl->data.ptr;
				std::shared_ptr<Buffer> msg;
				msg = conn->msg_read();
				if (msg == NULL) {
					LOG(ERROR) << "rsp_recv: fd=" << conn->fd() << ", read_len<=0, delete link";
					conn->mark_error();
					count++;
					CHECK(count < 5);
					continue;
				}
				LOG(INFO) << "replica_rsp_recv=" << msg->data();

				repeat2:
				RdKafka::Message *msg_kafka = kconsumer->consume("kevin", false);
				std::shared_ptr<Buffer> req = std::make_shared<Buffer>(msg_kafka->len());
				req->append(static_cast<const char *>(msg_kafka->payload()), msg_kafka->len());
				if (msg_kafka->len() == 0)
					goto repeat2;

				if (conn && conn->imsg_q.empty()) {
					Fdevents *fdes = this->get_fdes_rpl();
					fdes->set(conn->fd(), FDEVENT_OUT, 1, conn);
					ev_counter++;
				}
				conn->imsg_q.push_back(req);
				delete msg_kafka;
			}
			else if (fdes_rpl->events & FDEVENT_OUT) { // ali: Stupid piece of code wasted half day
				Link *conn = (Link *)fdes_rpl->data.ptr;
				std::shared_ptr<Buffer> smsg;
				if (!conn->imsg_q.empty()) {
					smsg = conn->imsg_q.front(); conn->imsg_q.pop_front();
				} else {
					Fdevents *fdes_rpl = this->get_fdes_rpl();
					fdes_rpl->clr(conn->fd(), FDEVENT_OUT);  // yue: nothing to send so clear ev
					ev_counter--;

					continue;
				}
				LOG(INFO) << "replica_req_send";

				int len = conn->msg_send(smsg);
				if (len <= 0) {
					LOG(ERROR) << "req_send: fd=" << conn->fd() << ", write=" << len << ", delete link";
					conn->mark_error();
					return CO_ERR;
				}
				Fdevents *fdes_rpl = this->get_fdes_rpl();
				fdes_rpl->set(conn->fd(), FDEVENT_IN, 1, conn);
			}
		}
	}
	return CO_OK;
}



rstatus_t NetworkServer::recover_kafka() {

	conkafka::COnKafka_Consumer *kconsumer = new conkafka::COnKafka_Consumer(this->kafka_broker, this->kafka_topic);
	int64_t start_offset = RdKafka::Topic::OFFSET_BEGINNING;
	kconsumer->set_offset("kevin", start_offset);

	// Create connection with datalet provided in the cfg file and consume from kafka
	//Link *server_conn = new Link(get_connection_to_local_dumbnode());

	Link *server_conn;
	DumbNodeTable_t::iterator it;
	for (it = dumb_map.begin(); it != dumb_map.end(); it++) {
		if (this->topology == slave) {
			server_conn = get_conn();
			CHECK(server_conn != NULL);
			rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
			if (status != CO_OK) {
				fprintf(stderr, "consumer error init socket: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			server_conn->noblock();
			server_conn->nodelay();
		}
		if (this->topology == ms) {
			if (it->second.replica_role == master_t) {
				server_conn = get_conn();
				CHECK(server_conn != NULL);
				rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
				if (status != CO_OK) {
					fprintf(stderr, "error init socket: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				server_conn->noblock();
				server_conn->nodelay();
			}
		}
		if (this->topology == aa) {
			if (it->second.replica_role == master_t) {
				server_conn = get_conn();
				CHECK(server_conn != NULL);
				rstatus_t status = server_conn->connect(it->second.addr.c_str(), it->second.port);
				if (status != CO_OK) {
					fprintf(stderr, "error init socket: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				server_conn->noblock();
				server_conn->nodelay();
			}
		}
	}

	// Now we start lazy consumption of data
	const Fdevents::events_t *events_recover;

	Fdevents *fdes_recover = this->get_fdes_recover();
	fdes_recover->set(server_conn->fd(), FDEVENT_OUT, 1, server_conn);

	RdKafka::Message *msg_kafka = kconsumer->consume("kevin", false);
	std::shared_ptr<Buffer> req = std::make_shared<Buffer>(msg_kafka->len());
	req->append(static_cast<const char *>(msg_kafka->payload()), msg_kafka->len());
	if (msg_kafka->len() == 0) {
		fprintf(stderr, "Nothing to recover?\n I am done recovering everything..\n");
		delete server_conn;
		return CO_OK;
	}

	if (server_conn && server_conn->imsg_q.empty()) {
		Fdevents *fdes = this->get_fdes_recover();
		fdes->set(server_conn->fd(), FDEVENT_OUT, 1, server_conn);
		ev_counter++;
	}
	server_conn->imsg_q.push_back(req);
	int count = 1;
	delete msg_kafka;

	while (1) {
		events_recover = fdes_recover->wait(50);
		if (events_recover == NULL) {
			fprintf(stderr, "events.wait error: %s", strerror(errno));
			return CO_ERR;
		}

		for (int i = 0; i < (int)events_recover->size(); i++) {
			const Fdevent *fdes_rpl = events_recover->at(i);

			if (fdes_rpl->events & FDEVENT_IN) { // ali: Stupid piece of code wasted half day
				Link *conn = (Link *)fdes_rpl->data.ptr;
				std::shared_ptr<Buffer> msg;
				msg = conn->msg_read();
				if (msg == NULL) {
					LOG(ERROR) << "rsp_recv: fd=" << conn->fd() << ", read_len<=0, delete link";
					conn->mark_error();
					count++;
					//CHECK(count < 5);
					continue;
				}
				LOG(INFO) << "replica_rsp_recv=" << msg->data();

				RdKafka::Message *msg_kafka = kconsumer->consume("kevin", false);
				std::shared_ptr<Buffer> req = std::make_shared<Buffer>(msg_kafka->len());
				req->append(static_cast<const char *>(msg_kafka->payload()), msg_kafka->len());
				if (msg_kafka->len() == 0) {
					fprintf(stderr, "Done..\nI am done recovering everything..\n");
					delete server_conn;
					return CO_OK;
				}

				if (conn && conn->imsg_q.empty()) {
					Fdevents *fdes = this->get_fdes_recover();
					fdes->set(conn->fd(), FDEVENT_OUT, 1, conn);
					ev_counter++;
				}
				conn->imsg_q.push_back(req);
				delete msg_kafka;
			}
			else if (fdes_rpl->events & FDEVENT_OUT) { // ali: Stupid piece of code wasted half day
				Link *conn = (Link *)fdes_rpl->data.ptr;
				std::shared_ptr<Buffer> smsg;
				if (!conn->imsg_q.empty()) {
					smsg = conn->imsg_q.front(); conn->imsg_q.pop_front();
				} else {
					Fdevents *fdes_rpl = this->get_fdes_recover();
					fdes_rpl->clr(conn->fd(), FDEVENT_OUT);  // yue: nothing to send so clear ev
					ev_counter--;

					continue;
				}
				LOG(INFO) << "replica_req_send";

				int len = conn->msg_send(smsg);
				if (len <= 0) {
					LOG(ERROR) << "req_send: fd=" << conn->fd() << ", write=" << len << ", delete link";
					conn->mark_error();
					return CO_ERR;
				}
				Fdevents *fdes_rpl = this->get_fdes_recover();
				fdes_rpl->set(conn->fd(), FDEVENT_IN, 1, conn);
			}
		}
	}
	return CO_OK;
}

/*
 * yue: batch loop triggers pending events in a batch
 */
rstatus_t NetworkServer::batch_loop() {

	// Create time for heartbeat
	struct epoll_event epollEvent;
	struct epoll_event newEvents;
	int timerfd;
	int epollfd;
	struct itimerspec timerValue;

	/* set timerfd */
	timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timerfd < 0) {
		printf("failed to create timer fd\n");
		exit(1);
	}
	bzero(&timerValue, sizeof(timerValue));
	timerValue.it_value.tv_sec = 1;
	timerValue.it_value.tv_nsec = 0;
	timerValue.it_interval.tv_sec = 30;
	timerValue.it_interval.tv_nsec = 0;

	/* set events */
	epollfd = epoll_create1(0);
	epollEvent.events = EPOLLIN;
	epollEvent.data.fd = timerfd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, timerfd, &epollEvent);

	/* start timer */
	if (timerfd_settime(timerfd, 0, &timerValue, NULL) < 0) {
		printf("could not start timer\n");
		exit(1);
	}

	const Fdevents::events_t *events;

	// For eventual consistency we need a separate thread to handle replicas
	// If we use single thread then avg throuphput is going to be same no matter when do we ack.
	if ((this->consistency_model == eventual && this->topology == slave) || (this->consistency_model == eventual && this->topology == aa)) {

#ifndef USE_KAFKA

		fdes_rpl = new Fdevents();
		fdes_rpl->set_proxy(this);

		// Launch a separate thread for batch_loop_eventual
		// Main thread will act as producer where as new thread only consumes
		// FIXME: No locking required as its going to be just two threads, one produce other consumes???
		lazy_consumer = new std::thread(&NetworkServer::batch_loop_eventual, this);
		CHECK(lazy_consumer != NULL);
		//lazy_consumer->detach();
#else
		fdes_rpl = new Fdevents();
		fdes_rpl->set_proxy(this);

		if (!this->is_master_proxy) {
			lazy_consumer = new std::thread(&NetworkServer::consume_kafka, this);
			CHECK(lazy_consumer != NULL);
		}
		if (this->is_master_proxy && this->consistency_model == eventual && this->topology == aa) {
			lazy_consumer = new std::thread(&NetworkServer::consume_kafka, this);
			CHECK(lazy_consumer != NULL);
		}

#endif
	}

	// Following handles the fault tolerance in case of MS topology
	if (this->recovery) {
		if (this->topology == ms  || this->topology == slave) {
			if (this->consistency_model == strong) {

				fdes_recover = new Fdevents();
				fdes_recover->set_proxy(this);

				// head
				if (this->is_master_proxy) {
					fault_handler = new std::thread(&NetworkServer::recover_kafka, this);
					CHECK(fault_handler != NULL);
				}

				// middle

				// tail
				if (!this->is_master_proxy) {
					fault_handler = new std::thread(&NetworkServer::recover_kafka, this);
					CHECK(fault_handler != NULL);
				}

			}
			if (this->consistency_model == eventual) {

				fdes_recover = new Fdevents();
				fdes_recover->set_proxy(this);

				// head
				if (this->is_master_proxy) {
					fault_handler = new std::thread(&NetworkServer::recover_kafka, this);
					CHECK(fault_handler != NULL);
				}

				// middle/tail
				// No need we are already consuming data

			}
		}
		if (this->topology == aa) {

			fdes_recover = new Fdevents();
			fdes_recover->set_proxy(this);

			fault_handler = new std::thread(&NetworkServer::recover_kafka, this);
			CHECK(fault_handler != NULL);
		}
	}

	int count_num = 0;
	while (true) {

		// Verify the Cluster Topology Map periodically
		// Set heartbeat
		uint64_t timersElapsed = 0;
        int numEvents = epoll_wait(epollfd, &newEvents, 1, 0);
        if (numEvents > 0) {
        	count_num++;
            (void) read(epollEvent.data.fd, &timersElapsed, 8);
            //printf("timers elapsed: %d\n", count_num);
            this->zkc->SetHeartBeat(this->physical_map, std::to_string(count_num));
        }

        events = fdes->wait(50); // yue: FIXME: so far timeout hardcoded
		if (events == NULL) {
			fprintf(stderr, "events.wait error: %s", strerror(errno));
			return CO_ERR;
		}

		for (int i = 0; i < (int)events->size(); i++) {
			const Fdevent *fde = events->at(i);
			if (fde->data.ptr == client_listen_conn) {
				/*
				 * yue: why we comment out the following accept func call is because we
				 * have to get everything initialized for datalet backend before we can
				 * serve requests from client side correctly.  Otherwise client lib
				 * just tries to lazily connect without proceeding with the following
				 * actual requests...
				 */
				//Link *c_conn = accept_conn();
				// yue: and this c_conn will be initialized later in this IF
				Link *c_conn = get_client_conn(); 
				if (c_conn) {
					conn_count++;
					client_conn_ = true;

					// map a client connection to a server connection
#if 0
					if (!this->do_consistent_hashing) {
						Link *serv_conn = establish_server_conn();

						fprintf(stderr, "No ketama");
						// FIXME: Undo the hard coded part
						serv_conn->zkc = new zookeeper::ZooKeeper("localhost:2181");
						insert_conn_map(c_conn, serv_conn);
					} 
#endif
					fprintf(stderr, "Try to connect to local/remote datalets\n");
					c_conn->is_client = true;
					establish_server_conn_each(c_conn);
					accept_conn(c_conn);

					fdes->set(c_conn->fd(), FDEVENT_IN, 1, c_conn);
				} else {
					fprintf(stderr, "Failed to allocate conn\n");
				}
			} else if (fde->data.ptr == peer_listen_conn) { // yue: where we setup conn with peer proxies
				Link *p_conn = accept_peer_conn();
				if (p_conn) {
					Link *ps_conn = establish_s_conn(p_conn);
					if (ps_conn == NULL) {
						fprintf(stderr, "Failed to establish conn to peer's slave\n");
						continue;
					}
					fdes->set(p_conn->fd(), FDEVENT_IN, 1, p_conn);
				} else {
					fprintf(stderr, "Failed to accept peer conn\n");
				}
			} else if (fde->events & FDEVENT_IN) { // yue: where we read data from client/server/peer_proxies
				recv(fde);
			} else if (fde->events & FDEVENT_OUT) { // yue: where we send data to client/server
				send(fde);
			}
		}
	}

	return CO_OK;
}

rstatus_t NetworkServer::batch_loop_eventual(void) {
	fprintf(stderr, "in eventual thread\n");
	const Fdevents::events_t *events_rpl;
	// fdes_rpl = new Fdevents();

	while (true) {
		events_rpl = fdes_rpl->wait(50); // yue: FIXME: so far timeout hardcoded
		if (events_rpl == NULL) {
			fprintf(stderr, "events.wait error: %s", strerror(errno));
			return CO_ERR;
		}

		for (int i = 0; i < (int)events_rpl->size(); i++) {
			const Fdevent *fdes_rpl = events_rpl->at(i);

			if (fdes_rpl->events & FDEVENT_IN) { // ali: where we send data to client/server

				Link *conn = (Link *)fdes_rpl->data.ptr;
#ifdef USE_SMART_PTR
				std::shared_ptr<Buffer> msg;
#else
				Buffer *msg;
#endif
				msg = conn->msg_read();
				if (msg == NULL) {
					LOG(ERROR) << "rsp_recv: fd=" << conn->fd() << ", read_len<=0, delete link";
					conn->mark_error();
					continue;
					//return CO_OK;
				}

				CHECK(!conn->omsg_q.empty());
				conn->omsg_q.pop_front();

				LOG(INFO) << "replica_rsp_recv=" << msg->data();

				//return CO_OK;

			}
			else if (fdes_rpl->events & FDEVENT_OUT) { // ali: where we read data from client/server/peer_proxies

				Link *conn = (Link *)fdes_rpl->data.ptr;

#ifdef USE_SMART_PTR
				std::shared_ptr<Buffer> smsg;
#else
				Buffer *smsg;
#endif
				if (!conn->imsg_q.empty()) {
					smsg = conn->imsg_q.front(); conn->imsg_q.pop_front();
				} else {
					Fdevents *fdes_rpl = this->get_fdes_rpl();
					fdes_rpl->clr(conn->fd(), FDEVENT_OUT);  // yue: nothing to send so clear ev
					ev_counter--;

					continue;
					//return CO_OK;  // yue: nothing to send
				}
				LOG(INFO) << "replica_req_send";

				int len = conn->msg_send(smsg);
				if (len <= 0) {
					LOG(ERROR) << "req_send: fd=" << conn->fd() << ", write=" << len << ", delete link";
					conn->mark_error();
					return CO_ERR;
				}

				// yue: this is buggy so far: we should do on-demand connection where we
				// initialize the events for both in and out
				Fdevents *fdes_rpl = this->get_fdes_rpl();
				conn->omsg_q.push_back(smsg);
				fdes_rpl->set(conn->fd(), FDEVENT_IN, 1, conn);

				//return CO_OK;
			}
		}
	}

	return CO_OK;
}

/*
 * yue: core loop itself runs inside another loop in driver and triggers a
 * single event at a time
 */
rstatus_t NetworkServer::core_loop(void) {
	int nsd;

	nsd = fdes->event_wait(50);  // yue: FIXME: timeout hardcoded
	if (nsd < 0) {
		return nsd;
	}

	return CO_OK;
}

/*
 * yue: process the event
 */
void NetworkServer::event_process(const struct Fdevent *fde) {
	if (fde->data.ptr == client_listen_conn) {
		Link *c_conn = accept_conn();
		if (c_conn) {
			conn_count++;
			client_conn_ = true;
			fdes->set(c_conn->fd(), FDEVENT_IN, 1, c_conn);

			// map a client connection to a server connection
			Link *serv_conn = establish_server_conn();
			insert_conn_map(c_conn, serv_conn);
		}
	} else if (fde->events & FDEVENT_IN) { // yue: where we read data from client/server
		recv(fde);
	} else if (fde->events & FDEVENT_OUT) { // yue: where we send data to client/server
		send(fde);
	}
}

/*
 * yue: wrapper calling registered funcs
 */
rstatus_t NetworkServer::recv(const struct Fdevent *fde) {
	Link *conn = (Link *)fde->data.ptr;
	if (conn->error())	return CO_ERR;

#ifdef TEST_W_UGLY_CLI
	// if its client connecting for first time send info/ack back
	if (client_conn_ == true){
		client_conn_ = false;
		rstatus_t ret = proc_info(conn);

		// Just read the message so its marked
		Buffer *msg = conn->msg_read();
		if (msg == NULL) {
			fprintf(stderr, "recv: fd: %d, read failed, delete link", conn->fd());
			conn->mark_error();
			return CO_ERR;
		}

		//send to client back
		conn->write();

		//fdes->set(conn->fd(), FDEVENT_IN, 1, conn);
		return ret;
	}
#endif

	rstatus_t ret = conn->recv_nw(this, conn);
	if (ret != CO_OK && ret != CO_C_DROP) {
		fprintf(stderr, "recv on %d failed: %s", conn->fd(), strerror(errno));
	} else if (ret == CO_C_DROP) {
		//delete conn;
#ifndef RECLAIM_CONN
		// yue: reset the error field
		conn->reset_error();
		// yue: put client conn into free conn pool
		put_conn(conn);
#if 0
		if (!this->do_consistent_hashing) {
			// yue: reset err field and put associated server conn into free conn pool
			Link *serv_conn = get_mapped_server_conn(conn);
			serv_conn->reset_error();
			put_conn(serv_conn);
			// yue: erase the client_conn to server_conn mapping
			erase_conn_map(conn);
		}
#endif
#endif // not define RECLAIM_CONN
	}

	return ret;
}

// Generate ack PONG message for SSDB
rstatus_t NetworkServer::proc_info(Link *link){

	Response resp;

	resp.push_back("ok");
	resp.push_back("conproxy");
	resp.push_back("version");
	resp.push_back("0.0.1");

	link->send(resp.resp);

	return CO_OK;
}

/*
 * yue: wrapper calling registered funcs
 */
rstatus_t NetworkServer::send(const struct Fdevent *fde) {
	Link *conn = (Link *)fde->data.ptr;
	if (conn->error())	return CO_ERR;

	rstatus_t ret = conn->send_nw(this, conn);
	if (ret != CO_OK) {
		fprintf(stderr, "send on %d failed: %s\n", conn->fd(), strerror(errno));
		//delete conn;
	}

	return ret;
}

Fdevents *NetworkServer::get_fdes() {
	return fdes;
}

Fdevents *NetworkServer::get_fdes_rpl() {
	return fdes_rpl;
}

Fdevents *NetworkServer::get_fdes_recover() {
	return fdes_recover;
}

Link *NetworkServer::get_client_listen_conn() {
	return client_listen_conn;
}

Link *NetworkServer::get_dumb_conn() {
	return this->dumb_conn;
}

#ifdef USE_SMART_PTR
Link *NetworkServer::get_mapped_server_conn(Link *conn, std::shared_ptr<Buffer> msg) {
#else
Link *NetworkServer::get_mapped_server_conn(Link *conn, Buffer *msg) {
#endif
#if 0
	if (!this->do_consistent_hashing) {
		return conn_map[conn];
	}
#endif
	CHECK(msg != NULL);
	// yue: parse the raw packet and get the request
	const Request *req = conn->parse(msg);
	if (req == NULL) {
		LOG(WARNING) << "fd: " << conn->fd() << ", conn parse error, TODO delete conn";
		this->conn_count--;
		this->fdes->del(conn->fd());
#ifdef RECLAIM_CONN
		delete conn;
#else
		// yue: reset the error field
		conn->reset_error();
		// yue: put client conn into free conn pool
		put_conn(conn);
		// yue: FIXME: clean all server conns... 

		// yue: reset err field and put associated server conn into free conn pool
		//Link *serv_conn = get_mapped_server_conn(conn);
		//serv_conn->reset_error();
		//put_conn(serv_conn);
		//// yue: erase the client_conn to server_conn mapping
		//erase_conn_map(conn);
#endif // yue: RECLAIM_CONN
		return NULL;
	}

	// Get key from the command
	// FIXME: fix memory leaks
	std::string *cmnd = (*req)[0].get_new_String();
	std::string *key  = (*req)[1].get_new_String();

	//std::string val = (*req)[2].String();

	//// yue: protobuf snippet for testing; TODO: later integrate it with zk's interactive heartbeat exchange
	//spec::Message buf;
	//buf.set_key(key);
	//buf.set_val(val);
	//std::string msg;
	//buf.SerializeToString(&msg);

	//LOG(INFO) << "replica_send.ley=" << key << ", replica_send.val=" << val << ", size=" << msg.size();
	//ssize_t ret = ::send(this->peer_fd, msg.c_str(), msg.size(), 0);
	//if (ret == -1) {
	//	fprintf(stderr, "replica protogating failed\n");
	//}

#ifdef USE_KETAMA
	Link *mapped_conn = conn->get_server_conn_mapped(*key);
#else
	Link *mapped_conn = conn->get_server_conn();
#endif
	CHECK(mapped_conn != NULL);

	// yue: TODO: zk logic should go here later
	// Acquire lock over here as we have key information so that we do not parse twice
	if (consistency_model >= eventual || (consistency_model >= strong && topology ==aa)) {
		mapped_conn->key = key;
		mapped_conn->cmnd = cmnd;

	}

	if (consistency_model >= strong && topology ==aa) {
		if (*cmnd == "set") {
			mapped_conn->fwd_to_replicas = true;
		}
		return mapped_conn;
	}

	// Set the flag to either fwd req to slaves or not
	// TODO: have it for all the commands that append/write/set/post etc.
	// TODO: May be we need to look at protocol anyways.
	if (*cmnd == "set") {
		mapped_conn->fwd_to_replicas = true;
		mapped_conn->head = true;
		mapped_conn->tail = false;
	} else {
		mapped_conn->fwd_to_replicas = false;

		if (consistency_model == strong && consistency_technique == cr && topology == ms) {
			// fwd the tail
			Link *tail = get_slave_number(mapped_conn, replicas -1);
			tail->fwd_to_replicas = false;
			tail->tail = true;

			delete cmnd;
			delete key;
			return tail;
		}
	}

	delete cmnd;
	delete key;
	return mapped_conn;
}


void NetworkServer::inc_conn_count() {
	this->conn_count++;
}

bool NetworkServer::is_ketama_mapping_enabled() {
	return this->do_consistent_hashing;
}

int NetworkServer::get_peer_fd() {
	return this->peer_fd;
}

void NetworkServer::dec_conn_count() {
	this->conn_count--;
}

long unsigned NetworkServer::get_events_size() {
	return this->fdes->get_events_size();
}
