#pragma once

#include <map>
#include <utility>

#include "server.h"
#include "link.h"
#include "fde.h"
#include "co_zk.h"
#include "redlock-cpp/redlock-cpp/redlock.h"
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

class Link;
class Fdevents;

typedef enum {
	master_t = 0,
	slave_t = 1
} replica_role_t;

class DumbNode {
	public:
		DumbNode() {}
		//DumbNode(int32_t w) {
		//	weight = w;
		//}
		std::string addr;
		int32_t port;
		int32_t weight;
		replica_role_t replica_role;
		//NetworkServer parent_proxy;
};

//typedef std::unordered_map<AddrType, DumbNode, AddrHash, AddrEqual > DumbNodeTable_t;
typedef std::unordered_map<AddrType, DumbNode> DumbNodeTable_t;
typedef std::unordered_map<AddrType, Link *> DumbConnTable_t;
DumbNodeTable_t parse_dumb_list(const std::string &config_file);

// What kind of mechanism you want to use to suppor consistency
enum cons_tech {
	none = 0,	// none
	zk = 1,     // ZooKeeper
	rl = 2,     // redLock
	redis = 3,  // simple redis - our own implementation
	cr = 4 	    // chain replication - lockless consistency for M-S topology
};

enum supp_top {
	no = 0,   // none just one node
	ms = 1,		// master-slave
	slave = -1,
	aa = 2    // active-active
};

enum cons_mod {
	without = 0,
	eventual = 1,
	strong = 2
};


class NetworkServer {
	public:
		NetworkServer();
		NetworkServer(const std::string &proxy_addr, int32_t port, 
				const std::string &server_addr, int32_t server_port, 
				bool do_ketama, int32_t back_conn);
		NetworkServer(const std::string &proxy_addr, int32_t client_port, 
				int32_t peer_port, const std::string &config_file,
				const std::string &datalet_file, const std::string &shard,
				const bool recover);
		virtual ~NetworkServer();

		rstatus_t batch_loop();
		rstatus_t batch_loop_eventual();
		rstatus_t core_loop();
		Link *proxy_listen();
		Link *connect_to_server();
		Link *accept_conn();
		void accept_conn(Link *conn);
		Link *accept_peer_conn();
		Link *get_client_conn();
		Link *establish_s_conn(Link *conn);
		Link *establish_server_conn();
		void establish_server_conn_each(Link *conn);
		void put_conn(Link *conn);
		void event_process(const struct Fdevent *fde);
		rstatus_t send(const struct Fdevent *fde);
		rstatus_t recv(const struct Fdevent *fde);
		Fdevents *get_fdes();
		Fdevents *get_fdes_rpl();
		Fdevents *get_fdes_recover();
		Link *get_client_listen_conn();
#ifdef USE_SMART_PTR
		Link *get_mapped_server_conn(Link *conn, std::shared_ptr<Buffer> msg=NULL);
#else
		Link *get_mapped_server_conn(Link *conn, Buffer *msg=NULL);
#endif
		Link *get_dumb_conn();
		rstatus_t proc_info(Link *link);
		void inc_conn_count();
		void dec_conn_count();
		long unsigned get_events_size();
		bool is_ketama_mapping_enabled();
		int get_peer_fd();
		Link *get_slave_conn(Link *master = NULL);
		std::vector<Link *> get_slaves(Link *master);
		Link *get_slave_number(Link *master, int number);
		Link *get_master_of_slave(Link *slave = NULL);
		rstatus_t consume_kafka();
		rstatus_t recover_kafka();
		DumbNodeTable_t dumb_map;
		Link *get_connection_to_local_dumbnode();

		// ali: list of all slave connections
		std::deque<Link *> slave_conn_pool;

		// No need for following as slave connection needs to be handled at more finer granularity
		//Link *slave_conn[2]; // yue: hardcoded slave connections; ali: hard coded replication of 3

		// Cluster physical topology that will be fetched from the ZK -- all the proxies and datanodes
		// This contains information like which dumbnode/datanode fall under which proxy
		std::vector<std::pair<std::string, std::vector<std::string>>> global_cluster_top;

		// Cluster lodical map that will be fetched from the ZK -- all the proxies and datanodes
		// This contains information like which dumbnode/datalet is master and which are it's slaves
		std::vector<std::pair<std::string, std::vector<std::string>>> global_cluster_map;

		// This contains information like which dumbnode/datanode fall under current proxy
		std::vector<std::pair<std::string, std::vector<std::string>>> local_cluster_top;

		// This contains information like which dumbnode/datalet is master and which are slaves for current proxy
		std::vector<std::pair<std::string, std::vector<std::string>>> local_cluster_map;

		// Consistency level
		cons_mod consistency_model; // None = 0; Eventual = 1; Strong = 2

		cons_tech consistency_technique; // none, zk lock, redlock, cr

		// Cluster topology
		supp_top topology; // none , master-slave

		// Number of extra copies
		int replicas; // original + replicas = replication factor

		// Local connection to ZK to find out the top and map
		zookeeper::ZooKeeper *zkc;
		std::string zk_host;

		// redlock server - redis server
		std::string rl_host;
		int rl_port;

		// kafka broker read from the conf file
		std::string kafka_broker;

		bool is_master_proxy;

		std::string kafka_topic;

		// proxy and local dumbnode info
		std::pair <std::string, std::string> physical_map;

	private:
		Link *get_conn();
		void insert_conn_map(Link *key, Link *val);
		void erase_conn_map(Link *key);

		Fdevents *fdes;
		Fdevents *fdes_rpl;
		Fdevents *fdes_recover;
		Link *client_listen_conn; // yue: listen on client event
		Link *peer_listen_conn; // yue: listen on proxy peer event
		Link *dumb_conn;
		std::deque<Link *> free_conn_pool;
		std::deque<Link *> conn_pool;

		// ali: list of all master connections
		std::deque<Link *> master_conn_pool;

		// ali: contains mapping of master and slaves, just like conn_map contains for incoming and outgoing connections
		// It's important to understand that this is connection level information and DIFFERENT than information collected from ZK or maintained in local/global datastructures
		std::vector<std::pair<Link *, std::vector<Link *>>> ms_map;

		int32_t num_free_conn;
		//std::vector<Link *> server_conn_pool;
		//int32_t server_conn_count;

		bool do_consistent_hashing;

		std::string server_addr;
		int32_t server_port;
		int32_t conn_count;
		int32_t server_back_conn;

		std::map<Link *, Link *> conn_map;
//		std::deque<Link *> connection_map;
		int peer_fd;
		
		std::thread *lazy_consumer;

		// Fault tolerance stuff
		bool recovery;
		std::thread *fault_handler;
};

