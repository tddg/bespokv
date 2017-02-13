/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef NET_LINK_H_
#define NET_LINK_H_

#include <deque>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "net.h"
#include "server.h"
#include "link.h"
#include "fde.h"
#include "co_ketama.h"
#include "util/bytes.h"

#include "link_redis.h"
#include "co_zk.h"
#include "redlock-cpp/redlock-cpp/redlock.h"
#include "redlock-cpp/redlock-cpp/ConLock.h"
#include "co_kafka.h"

#define USE_PROTOBUF  // yue: this enables protobuf support
#define USE_KAFKA	  //ali: this enables kafka for producer/consumer testing

#ifdef USE_PROTOBUF
const uint32_t MAGIC = 0x06121983;
#endif

class Link;
class NetworkServer;
class Buffer;
class Continuum;

typedef rstatus_t (*link_recv_t)(NetworkServer *, Link *);
typedef rstatus_t (*link_send_t)(NetworkServer *, Link *);
typedef void (*link_msgq_t)(Link *, Buffer *);

typedef std::unordered_map<AddrType, Link *> DumbConnTable_t;

typedef enum {
	REQ=0,   // yue: regular requests from client side
	ADMIN=1, // yue: admin messages between proxies
	PROP=2,  // yue: update propogation between proxies
} conn_t;

class Link{
	private:
		int dummy_;
		int sock;
		bool noblock_;
		bool error_;
		std::vector<Bytes> recv_data;
	
		RedisLink *redis;

		Continuum *continuum;
#ifdef USE_KETAMA
		DumbConnTable_t dumb_map;
#else
		Link *datalet_conn;
#endif

		static int min_recv_buf;
		static int min_send_buf;
		//int dummy;
	public:
		const static int MAX_PACKET_SIZE = 128 * 1024 * 1024;

		std::string dumb_addr;
		int dumb_port;
		char remote_ip[INET_ADDRSTRLEN];
		int remote_port;

		bool auth;
		bool ignore_key_range;

		conn_t type;

#ifdef USE_SMART_PTR
		std::shared_ptr<Buffer> input;
		std::shared_ptr<Buffer> output;
		std::deque<std::shared_ptr<Buffer>> imsg_q;
		std::deque<std::shared_ptr<Buffer>> omsg_q;
		//std::deque<CLock> lock_q;
#else
		Buffer *input;
		Buffer *output;
		std::deque<Buffer *> imsg_q; // yue
		std::deque<Buffer *> omsg_q; // yue
#endif
		
		double create_time;
		double active_time;

		bool is_server; // yue: whether or not this conn is associated w/ dumb nodes
		bool is_master;
		bool is_client;
		int number_slaves;
		int32_t weight;
		int slave_number; // track which slave number it is
		
		bool head;
		bool tail;

		// Flag to find out if the current command handled by this Link connection needs to be fwded to slaves
		bool fwd_to_replicas;

		// While we are parsing lets set the key so that we can acquire lock later
		std::string *key;
		std::string *cmnd;

		// Lock id returned by ZK lock
		std::string *lock_id;

		Link *s_conn;
		//Link *slave_conn[2];

		Link(bool is_server=false);
		~Link();
		void close();
		void nodelay(bool enable=true);
		// noblock(true) is supposed to corperate with IO Multiplex,
		// otherwise, flush() may cause a lot unneccessary write calls.
		void noblock(bool enable=true);
		void keepalive(bool enable=true);
		void set_nw_func(); // yue
		void set_admin_func(); // yue
		void set_repl_func(); // yue
#ifdef USE_KETAMA
		void insert_dumbconn(AddrType addr, Link *conn);
		DumbConnTable_t get_dumb_map();
		void init_continuum(int32_t total_dumb_weight);
		//Link *get_server_conn(const Request *req);
		Link *get_server_conn_mapped(const std::string &key);
#else
		void set_server_conn(Link *conn);
		Link *get_server_conn();
#endif

		void clear_recv_data() {
			if (this->recv_data.size() > 0) {
				this->recv_data[0].reclaim_data(); // yue: only delete op field
			}
			this->recv_data.clear();
		}

		void set_dummy();

		int fd() const{
			return sock;
		}
		bool error() const{
			return error_;
		}
		void mark_error(){
			error_ = true;
		}
		void reset_error() {
			error_ = false;
		}

		static Link* alloc_and_connect(const char *ip, int port);
		rstatus_t connect(const char *ip, int port);
		static Link* listen(const char *ip, int port);
		Link* alloc_and_accept();
		rstatus_t accept(Link *conn);

		// read network data info buffer
		int read();
#ifdef USE_SMART_PTR
		std::shared_ptr<Buffer> msg_read();  // yue
		int msg_write(std::shared_ptr<Buffer> smsg); // yue
		int msg_send(std::shared_ptr<Buffer> smsg); // yue
		int flush_msg(std::shared_ptr<Buffer> msg); // yue
		int kafka_msg_send(char *msg, int len); //ali
#else
		Buffer *msg_read();  // yue
		int msg_write(Buffer *smsg); // yue
		int msg_send(Buffer *smsg); // yue
		int flush_msg(Buffer *msg); // yue
#endif
		int write();
		// flush buffered data to network
		// REQUIRES: nonblock
		int flush();

		// yue: the following are func pointers registered for different uses
		link_recv_t recv_nw;
		link_send_t send_nw;
		//link_msgq_t enq_iq;
		//link_msgq_t deq_iq;
		//link_msgq_t enq_oq;
		//link_msgq_t enq_oq;

		/**
		 * parse received data, and return -
		 * NULL: error
		 * empty vector: recv not ready
		 * vector<Bytes>: recv ready
		 */
		const std::vector<Bytes>* recv(); 
		/**
		 * yue:
		 * parse received msg, and return -
		 * NULL: error
		 * empty vector: recv not ready
		 * vector<Bytes>: recv ready
		 */
#ifdef USE_SMART_PTR
		const std::vector<Bytes> *parse(std::shared_ptr<Buffer> msg); 
#ifdef USE_PROTOBUF
		const std::vector<Bytes> *msg_parse(std::shared_ptr<Buffer> msg);
#endif // USE_PTOROBUF
#else
		const std::vector<Bytes> *parse(Buffer *msg); 
#ifdef USE_PROTOBUF
		const std::vector<Bytes> *msg_parse(Buffer *msg);
#endif // USE_PTOROBUF
#endif // USE_SMART_PTR
		// wait until a response received.
		const std::vector<Bytes>* response();

		// need to call flush to ensure all data has flush into network
		int send(const std::vector<std::string> &packet);
		int send(const std::vector<Bytes> &packet);
		int send(const Bytes &s1);
		int send(const Bytes &s1, const Bytes &s2);
		int send(const Bytes &s1, const Bytes &s2, const Bytes &s3);
		int send(const Bytes &s1, const Bytes &s2, const Bytes &s3, const Bytes &s4);
		int send(const Bytes &s1, const Bytes &s2, const Bytes &s3, const Bytes &s4, const Bytes &s5);

		const std::vector<Bytes>* last_recv(){
			return &recv_data;
		}
		
		/** these methods will send a request to the server, and wait until a response received.
		 * @return
		 * NULL: error
		 * vector<Bytes>: response ready
		 */
		const std::vector<Bytes>* request(const Bytes &s1);
		const std::vector<Bytes>* request(const Bytes &s1, const Bytes &s2);
		const std::vector<Bytes>* request(const Bytes &s1, const Bytes &s2, const Bytes &s3);
		const std::vector<Bytes>* request(const Bytes &s1, const Bytes &s2, const Bytes &s3, const Bytes &s4);
		const std::vector<Bytes>* request(const Bytes &s1, const Bytes &s2, const Bytes &s3, const Bytes &s4, const Bytes &s5);


		// ali: For every client connection we create a ZK connection
		zookeeper::ZooKeeper *zkc;

		CRedLock *dlm;
		CLock *my_lock;
		ConLock *conlock;

		// ali: For every client connection we create a kafka producer/consumer
		conkafka::COnKafka_Producer *kproducer;
		conkafka::COnKafka_Consumer *kconsumer;

		//int served_slaves;
};

#endif
