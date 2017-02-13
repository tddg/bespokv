/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netdb.h>

#include "co_zk.h"
#include "link.h"
#include "msg.h"

#include "link_redis.cpp"

#ifdef USE_PROTOBUF
#include "ckv_proto.pb.h"
#endif

#define INIT_BUFFER_SIZE	8

int Link::min_recv_buf = 8 * 1024;
int Link::min_send_buf = 8 * 1024;


Link::Link(bool is_server) {
	sock = -1;
	noblock_ = true;
	error_ = false;
	remote_ip[0] = '\0';
	remote_port = -1;
	auth = false;
	ignore_key_range = false;

	redis = NULL;

	is_server = is_server;
	type = REQ;
	weight = 0;
	
	if(is_server){
		input = output = NULL;
	}else{
		// alloc memory lazily
#ifdef USE_SMART_PTR
		input = std::make_shared<Buffer>(INIT_BUFFER_SIZE);
		output = std::make_shared<Buffer>(INIT_BUFFER_SIZE);
#else
		input = new Buffer(INIT_BUFFER_SIZE);
		output = new Buffer(INIT_BUFFER_SIZE);
#endif
	}
}

Link::~Link(){
#ifndef USE_SMART_PTR
	if(input){
		delete input;
	}
	if(output){
		delete output;
	}
#endif
	this->close();
}

void Link::close(){
	if(sock >= 0){
		::close(sock);
	}
}

void Link::nodelay(bool enable){
	int opt = enable? 1 : 0;
	::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *)&opt, sizeof(opt));
}

void Link::keepalive(bool enable){
	int opt = enable? 1 : 0;
	::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&opt, sizeof(opt));
}

void Link::noblock(bool enable){
	noblock_ = enable;
	if(enable){
		//::fcntl(sock, F_SETFL, O_NONBLOCK | O_RDWR);
		if (::fcntl(sock, F_SETFL, ::fcntl(sock, F_GETFL)|O_NONBLOCK) < 0) {
			perror("setting O_NONBLOCK");
			::close(sock);
			return;
		}
	}else{
		::fcntl(sock, F_SETFL, O_RDWR);
	}
}

void Link::set_nw_func() {
	if (!this->is_server) { // yue: if it is client side conn
		this->recv_nw = req_recv;
		this->send_nw = rsp_send;
		// yue: TODO enq/deq ...

	} else { // yue: else if it is dumb node side conn
		this->recv_nw = rsp_recv;
		this->send_nw = req_send;
		// yue: TODO enq/deq ...

	}
}

void Link::set_admin_func() {
	this->recv_nw = admin_replica_recv;
}

void Link::set_repl_func() {
	this->recv_nw = replica_rsp_recv;
	this->send_nw = replica_req_send;
}

#ifdef USE_KETAMA
DumbConnTable_t Link::get_dumb_map() {
	return this->dumb_map;
}

void Link::insert_dumbconn(AddrType addr, Link *conn) {
	this->dumb_map.insert(std::make_pair(addr, conn));
}

void Link::init_continuum(int32_t total_dumb_weight) {
	this->continuum = new Continuum(this->dumb_map, total_dumb_weight);
	CHECK(this->continuum);
}
#else
void Link::set_server_conn(Link *conn) {
	this->datalet_conn = conn;
}

Link *Link::get_server_conn() {
	return this->datalet_conn;
}
#endif  // USE_KETAMA

#if 0
void TestCreateHandler(ZKErrorCode errcode, const std::string& path, const std::string& value, void* context) {
	if (errcode == kZKSucceed) {
		printf("TestCreateHandler-[kZKSucceed] path=%s value=%s\n", path.c_str(), value.c_str());
		std::string *tmp = new std::string(value);
		*((std::string **)context) = tmp;
		//fprintf(stderr, "shit: %s\n", );
	} else if (errcode == kZKError) {
		printf("TestCreateHandler-[kZKError] path=%s\n", path.c_str());
	} else if (errcode == kZKNotExist) {
		printf("TestCreateHandler-[kZKNotExist] path=%s\n", path.c_str());
	} else if (errcode == kZKExisted) {
		printf("TestCreateHandler-[kZKExisted] path=%s\n", path.c_str());
	}
}

void TestDeleteHandler(ZKErrorCode errcode, const std::string& path, void* context) {
	if (errcode == kZKSucceed) {
		printf("TestDeleteHandler-[kZKSucceed] path=%s\n", path.c_str());
	} else if (errcode == kZKError) {
		printf("TestDeleteHandler-[kZKError] path=%s\n", path.c_str());
	} else if (errcode == kZKNotExist) {
		printf("TestDeleteHandler-[kZKNotExist] path=%s\n", path.c_str());
	} else if (errcode == kZKNotEmpty) {
		printf("TestDeleteHandler-[kZKNotEmpty] path=%s\n", path.c_str());
	}
}
#endif

#ifdef USE_KETAMA
Link *Link::get_server_conn_mapped(const std::string &key) {
	//const char *key = (*req)[1].data();

	//Request::const_iterator it;
	//LOG(INFO) << "field0=" << (*req)[0].data() << ", key=" << key << ", ksz=" << (*req)[1].size() 
	//	<< ", val=" << (*req)[2].data();
	//int i = 0;
	//for (it = req->begin(); it != req->end(); it++) {
	//	LOG(INFO) << i++ << ": " << it->data();
	//}
	//LOG(INFO) << "req.size=" << req->size();
	//LOG(INFO) << "req.key=" << (*req)[1].String() << ", req.val=" << (*req)[2].String();
	

	/**
	// ZooKeeper Usage
	// FIXME: Get from the arguments
	bool consistency = true;

	// Get key from the command
	std::string key = (*req)[1].String();


	// FIXME: Get the ZK connection detail from the arguments
	if(consistency) {

		// Get command
		std::string cmnd = (*req)[0].String();
		//bool rw = (cmnd == "set") ? true : false;
		bool rw = false;

		// Launch connection to already running ZK server
		std::string guid = zkc->CreateIfNotExists("/zookeeper/"+key);
		//zkc->DeleteIfExists("/zookeeper/"+key);
		//std::string lock1 = zkc->Lock("/zookeeper/" + key, rw);
		//std::string lock2 = zkc->Lock("/zookeeper/" + key, rw);

		//zkc->Unlock(lock1);
		//zkc->Unlock(lock2);

		//bool reult2 = zkc->Lock("/zookeeper/" + key, 1);
		//while(!zkc->Lock("/zookeeper/" + key, 1));
	}


	std::string *context; 
	//char **context;// = (char *)malloc(1024);
	//context[0] = '';

	bool succeed = zkclient.Create("/zookeeper/"+key, "", ZOO_EPHEMERAL | ZOO_SEQUENCE, TestCreateHandler, &context);
	//bool succeed = zkclient.Create("/zookeeper/"+key, "", 0, TestCreateHandler, context);
	if (!succeed) {
		fprintf(stderr, "ZKClient failed to create...\n");
	} 
	sleep(2);
	fprintf(stderr, "damn: %s\n", context->c_str());
	succeed = zkclient.Delete("/zookeeper/"+key, TestDeleteHandler, NULL);
	if (!succeed) {
		fprintf(stderr, "ZKClient failed to delete...\n");
	}

	**/

	//LOG(INFO) << "key=" << key.c_str();

	// FIXME: make sure hash is perform only on the master nodes not the slave nodes
	return this->continuum->ketama_get_conn(key);
}
#endif // USE_KETAMA

// TODO: check less than 256
static bool is_ip(const char *host){
	int dot_count = 0;
	int digit_count = 0;
	for(const char *p = host; *p; p++){
		if(*p == '.'){
			dot_count += 1;
			if(digit_count >= 1 && digit_count <= 3){ 
				digit_count = 0;
			}else{
				return false;
			}   
		}else if(*p >= '0' && *p <= '9'){
			digit_count += 1;
		}else{
			return false;
		}   
	}   
	return dot_count == 3;
}

Link* Link::alloc_and_connect(const char *host, int port){
	Link *link;
	int sock = -1;

	char ip_resolve[INET_ADDRSTRLEN];
	if(!is_ip(host)){
		struct hostent *hptr = gethostbyname(host);
		for(int i=0; hptr && hptr->h_addr_list[i] != NULL; i++){
			struct in_addr *addr = (struct in_addr *)hptr->h_addr_list[i];
			if(inet_ntop(AF_INET, addr, ip_resolve, sizeof(ip_resolve))){
				//printf("resolve %s: %s\n", host, ip_resolve);
				host = ip_resolve;
				break;
			}
		}
	}

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((short)port);
	inet_pton(AF_INET, host, &addr.sin_addr);

	if((sock = ::socket(AF_INET, SOCK_STREAM, 0)) == -1){
		goto sock_err;
	}
	if(::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		goto sock_err;
	}

	//log_debug("fd: %d, connect to %s:%d", sock, ip, port);
	link = new Link();
	link->sock = sock;
	link->keepalive(true);
	return link;
sock_err:
	//log_debug("connect to %s:%d failed: %s", ip, port, strerror(errno));
	if(sock >= 0){
		::close(sock);
	}
	return NULL;
}

rstatus_t Link::connect(const char *host, int port){
	int sock = -1;

	char ip_resolve[INET_ADDRSTRLEN];
	if(!is_ip(host)){
		struct hostent *hptr = gethostbyname(host);
		for(int i=0; hptr && hptr->h_addr_list[i] != NULL; i++){
			struct in_addr *addr = (struct in_addr *)hptr->h_addr_list[i];
			if(inet_ntop(AF_INET, addr, ip_resolve, sizeof(ip_resolve))){
				//printf("resolve %s: %s\n", host, ip_resolve);
				host = ip_resolve;
				break;
			}
		}
	}

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((short)port);
	inet_pton(AF_INET, host, &addr.sin_addr);

	if((sock = ::socket(AF_INET, SOCK_STREAM, 0)) == -1){
		goto sock_err;
	}
	if(::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		goto sock_err;
	}

	this->sock = sock;
	this->keepalive(true);
	return CO_OK;
sock_err:
	if(sock >= 0){
		::close(sock);
	}
	return CO_ERR;
}

Link* Link::listen(const char *ip, int port){
	Link *link;
	int sock = -1;

	int opt = 1;
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((short)port);
	inet_pton(AF_INET, ip, &addr.sin_addr);

	if((sock = ::socket(AF_INET, SOCK_STREAM, 0)) == -1){
		goto sock_err;
	}
	if(::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
		goto sock_err;
	}
	if(::bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		goto sock_err;
	}
	if(::listen(sock, 1024) == -1){
		goto sock_err;
	}
	//log_debug("server socket fd: %d, listen on: %s:%d", sock, ip, port);

	link = new Link(true);
	link->sock = sock;
	snprintf(link->remote_ip, sizeof(link->remote_ip), "%s", ip);
	link->remote_port = port;
	return link;
sock_err:
	//log_debug("listen %s:%d failed: %s", ip, port, strerror(errno));
	if(sock >= 0){
		::close(sock);
	}
	return NULL;
}

Link* Link::alloc_and_accept(){
	Link *link;
	int client_sock;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	while((client_sock = ::accept(sock, (struct sockaddr *)&addr, &addrlen)) == -1){
		if(errno != EINTR){
			//log_error("socket %d accept failed: %s", sock, strerror(errno));
			return NULL;
		}
	}

	struct linger opt = {1, 0};
	int ret = ::setsockopt(client_sock, SOL_SOCKET, SO_LINGER, (void *)&opt, sizeof(opt));
	if (ret != 0) {
		//log_error("socket %d set linger failed: %s", client_sock, strerror(errno));
	}

	link = new Link();
	link->sock = client_sock;
	link->keepalive(true);
	inet_ntop(AF_INET, &addr.sin_addr, link->remote_ip, sizeof(link->remote_ip));
	link->remote_port = ntohs(addr.sin_port);
	return link;
}

rstatus_t Link::accept(Link *conn) {
	int client_sock;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	while((client_sock = ::accept(sock, (struct sockaddr *)&addr, &addrlen)) == -1){
		if(errno != EINTR){
			//log_error("socket %d accept failed: %s", sock, strerror(errno));
			return CO_ERR;
		}
	}
	fprintf(stderr, "accept and return sfd=%d\n", client_sock);

	struct linger opt = {1, 0};
	int ret = ::setsockopt(client_sock, SOL_SOCKET, SO_LINGER, (void *)&opt, sizeof(opt));
	if (ret != 0) {
		//log_error("socket %d set linger failed: %s", client_sock, strerror(errno));
	}

	conn->sock = client_sock;
	conn->keepalive(true);
	inet_ntop(AF_INET, &addr.sin_addr, this->remote_ip, sizeof(conn->remote_ip));
	conn->remote_port = ntohs(addr.sin_port);
	return CO_OK;
}

int Link::read(){
	if(input->total() == INIT_BUFFER_SIZE){
		input->grow();
	}
	int ret = 0;
	int want;
	input->nice();
	while((want = input->space()) > 0){
		// test
		//want = 1;
		int len = ::read(sock, input->slot(), want);
		if(len == -1){
			if(errno == EINTR){
				continue;
			}else if(errno == EWOULDBLOCK){
				break;
			}else{
				//log_debug("fd: %d, read: -1, want: %d, error: %s", sock, want, strerror(errno));
				return -1;
			}
		}else{
			//log_debug("fd: %d, want=%d, read: %d", sock, want, len);
			if(len == 0){
				return 0;
			}
			ret += len;
			input->incr(len);
		}
		if(!noblock_){
			break;
		}
	}
	//log_debug("read %d", ret);
	return ret;
}

#ifdef USE_SMART_PTR
std::shared_ptr<Buffer> Link::msg_read(){
#else
Buffer *Link::msg_read(){
#endif
#ifdef USE_SMART_PTR
	std::shared_ptr<Buffer> msg = std::make_shared<Buffer>(INIT_BUFFER_SIZE);
#else
	Buffer *msg = new Buffer(INIT_BUFFER_SIZE);
#endif
	if(msg->total() == INIT_BUFFER_SIZE){
		msg->grow();
	}
	int ret = 0;
	int want;
	msg->nice();
	//fprintf(stderr, "msg_read:sfd=%d\n", sock);
	while((want = msg->space()) > 0){
		// test
		//want = 1;
		int len = ::read(sock, msg->slot(), want);
		if(len == -1){
			if(errno == EINTR){
				continue;
			} else if(errno == EWOULDBLOCK){
				//fprintf(stderr, "msg_read ret -1: %s\n", strerror(errno));
				break;
			} else{
#ifndef USE_SMART_PTR
				delete msg;
#endif
				//log_debug("fd: %d, read: -1, want: %d, error: %s", sock, want, strerror(errno));
				return NULL;
			}
		}else{
			//if (len == 0 && errno == EAGAIN) {
			//	fprintf(stderr, "resource temp unavailable try again later\n");
			//}
			//log_debug("fd: %d, want=%d, read: %d", sock, want, len);
			if(len == 0){
				fprintf(stderr, "msg_read: read_bytes=%d, buf_size=%d\n", ret, msg->size());
#ifndef USE_SMART_PTR
				delete msg;
#endif
				return NULL;
			}
			ret += len;
			msg->incr(len);
		}
		if(!noblock_){
			break;
		}
	}
	//log_debug("read %d", ret);
	return msg;
}

int Link::write(){
	if(output->total() == INIT_BUFFER_SIZE){
		output->grow();
	}
	int ret = 0;
	int want;
	while((want = output->size()) > 0){
		// test
		//want = 1;
		int len = ::write(sock, output->data(), want);
		if(len == -1){
			if(errno == EINTR){
				continue;
			}else if(errno == EWOULDBLOCK){
				break;
			}else{
				//log_debug("fd: %d, write: -1, error: %s", sock, strerror(errno));
				return -1;
			}
		}else{
			//log_debug("fd: %d, want: %d, write: %d", sock, want, len);
			if(len == 0){
				// ?
				break;
			}
			ret += len;
			output->decr(len);
		}
		if(!noblock_){
			break;
		}
	}
	output->nice();
	return ret;
}

#ifdef USE_SMART_PTR
int Link::msg_write(std::shared_ptr<Buffer> smsg){
#else
int Link::msg_write(Buffer *smsg){
#endif
	if(smsg->total() == INIT_BUFFER_SIZE){
		smsg->grow();
	}
	// yue: reset data ptr so smsg->data() won't be invalid
	//smsg->reset_dataptr();
	int ret = 0;
	int want;
	while((want = smsg->size()) > 0){
		// test
		//want = 1;
		int len = ::write(sock, smsg->data(), want);
		if(len == -1){
			if(errno == EINTR){
				continue;
			}else if(errno == EWOULDBLOCK){
				break;
			}else{
				//log_debug("fd: %d, write: -1, error: %s", sock, strerror(errno));
				return -1;
			}
		}else{
			//log_debug("fd: %d, want: %d, write: %d", sock, want, len);
			if(len == 0){
				// ?
				break;
			}
			ret += len;
			smsg->decr(len);
		}
		if(!noblock_){
			break;
		}
	}
	smsg->nice();
	return ret;
}

/*
 * yue: Network send func that sends out segmented req.
 * grow() and nice() calls are removed here.
 */
#ifdef USE_SMART_PTR
int Link::msg_send(std::shared_ptr<Buffer> smsg) {
#else
int Link::msg_send(Buffer *smsg) {
#endif
	int ret = 0;
	int want;
	int msg_sz = smsg->size();
	//LOG(INFO) << "smsg->size=" << smsg->size();
	while((want = smsg->size()) > 0){
		// test
		//want = 1;
		CHECK(smsg->size() > 0);

		int len = ::write(sock, smsg->data(), want);
		if(len == -1){
			if(errno == EINTR){
				continue;
			}else if(errno == EWOULDBLOCK){
				break;
			}else{
				//perror(strerror(errno));
				return -1;
			}
		}else{
			//log_debug("fd: %d, want: %d, write: %d", sock, want, len);
			if(len == 0){
				// ?
				break;
			}
			ret += len;
			smsg->decr(len);
		}
		if(!noblock_){
			break;
		}
	}
	//smsg->nice();
	//smsg->rewind();
	smsg->rewind(msg_sz);
	CHECK(smsg->size() > 0);
	return ret;
}


#ifdef USE_SMART_PTR
int Link::kafka_msg_send(char *msg, int len) {
#else
int Link::kafka_msg_send(Buffer *smsg) {
#endif
	int ret = 0;
	int want = len;
	while(want > 0){
		// test
		//want = 1;
		int len = ::write(sock, msg, want);
		if(len == -1){
			if(errno == EINTR){
				continue;
			}else if(errno == EWOULDBLOCK){
				break;
			}else{
				return -1;
			}
		}else{
			if(len == 0){
				break;
			}
			ret += len;
			want = want - len;
		}
		if(!noblock_){
			break;
		}
	}
	if (ret <= 0) {
		fprintf(stderr, "Length = %d\n", ret);
		perror("kafka send");
	}
	return ret;
}

int Link::flush(){
	int len = 0;
	while(!output->empty()){
		int ret = this->write();
		if(ret == -1){
			return -1;
		}
		len += ret;
	}
	return len;
}

#ifdef USE_SMART_PTR
int Link::flush_msg(std::shared_ptr<Buffer> msg){
#else
int Link::flush_msg(Buffer *msg){
#endif
	int len = 0;
	while(!msg->empty()){
		int ret = this->msg_write(msg);
		if(ret == -1){
			return -1;
		}
		len += ret;
	}
	return len;
}

#ifdef USE_PROTOBUF

/**
 * yue: where the protobuf-encoded request is parsed
 */
#ifdef USE_SMART_PTR
const std::vector<Bytes> *Link::msg_parse(std::shared_ptr<Buffer> msg) {
#else
const std::vector<Bytes> *Link::msg_parse(Buffer *msg) {
#endif
	int parsed = 0;
	int old_sz = msg->size();
	char *ptr = msg->data();
	
	msg->remember_sz(old_sz);
	if (msg->size_diff() == 45) fprintf(stderr, "size_diff=45\n");

	uint32_t *magic;
	magic = (uint32_t *)ptr;
	CHECK(*magic == MAGIC);
	ptr += sizeof(uint32_t);
	parsed += sizeof(uint32_t);

	size_t *sz  = (size_t *)ptr;
	size_t totalSize = *sz;
	CHECK(totalSize < 1073741826);
	ptr += sizeof(size_t);
	parsed += sizeof(size_t);

	char *head = msg->data();

	size_t typeLen = *((size_t *)ptr);
	ptr += sizeof(size_t);
	CHECK((size_t)(ptr-head) < totalSize);
	parsed += sizeof(size_t);

	std::string cmd(ptr, typeLen);
	//fprintf(stderr, "op=%s\n", cmd.c_str());
	if (cmd == "protoSpec.GetMessage") {
		//std::string *get_str = new std::string ("get");
		char *get_str = (char *)malloc(3);
		memcpy(get_str, "get", 3);
		Bytes bytes_get(get_str, 3);
		this->recv_data.push_back(bytes_get);
		CHECK(this->recv_data.size() == 1);
	} else if (cmd == "protoSpec.PutMessage") {
		//std::string *put_str = new std::string ("set");
		//std::string put_string("set");
		char *put_str = (char *)malloc(3);
		memcpy(put_str, "set", 3);
		//put_str[3] = '\0';
		Bytes bytes_set(put_str, 3);
		this->recv_data.push_back(bytes_set);
		CHECK(this->recv_data.size() == 1);
	} else if (cmd == "protoSpec.DelMessage") {
		//std::string *del_str = new std::string ("del");
		char *del_str = (char *)malloc(3);
		memcpy(del_str, "del", 3);
		Bytes bytes_del(del_str);
		this->recv_data.push_back(bytes_del);
		CHECK(this->recv_data.size() == 1);
	} else {
		/* yue: FIXME: so far left blank; will add exception handling logic here */
		; 
	}
	ptr += typeLen;
	parsed += typeLen;

	size_t dataLen = *((size_t *)ptr);
	ptr += sizeof(size_t);
	if ((size_t)(ptr-head) >= totalSize) {
		fprintf(stderr, "diff=%zu totalSize=%zu typeLen=%zu dataLen=%zu\n", ptr-head, totalSize, typeLen, dataLen);
	}
	CHECK((size_t)(ptr-head) < totalSize);
	CHECK((size_t)(ptr+dataLen-head) == totalSize);
	parsed += sizeof(size_t);

	/* yue: try to use a ptr to see if the memory corruption bug is gone */
	protoSpec::Request *req = new protoSpec::Request();
	CHECK(req != NULL);
	std::string *payload = new std::string(ptr, dataLen);
	CHECK(payload != NULL);
	if (cmd == "protoSpec.GetMessage") {
		req->ParseFromString(*payload);
		this->recv_data.push_back(Bytes(req->mutable_get()->key()));
		CHECK(this->recv_data.size() == 2);
	} else if (cmd == "protoSpec.PutMessage") {
		bool okay = req->ParseFromString(*payload);
		CHECK(okay == true);
		//req->SerializeToString(payload);
		this->recv_data.push_back(Bytes(req->mutable_put()->key()));
		this->recv_data.push_back(Bytes(req->mutable_put()->value()));
		CHECK(this->recv_data.size() == 3);
	} else if (cmd == "protoSpec.DelMessage") {
		req->ParseFromString(*payload);
		this->recv_data.push_back(Bytes(req->mutable_del()->key()));
		CHECK(this->recv_data.size() == 2);
	}
	//this->recv_data.push_back(Bytes(ptr, dataLen));
	ptr += dataLen;
	parsed += dataLen;
	msg->decr(parsed);

	delete req;
	delete payload;
	return &this->recv_data;
}

#endif // USE_PROTOBUF

/*
 * yue: recv() actually does protocol parse job.
 * I add a new function named parse(msg) to avoid confusion.
 */
#ifdef USE_SMART_PTR
const std::vector<Bytes>* Link::parse(std::shared_ptr<Buffer> msg) {
#else
const std::vector<Bytes>* Link::parse(Buffer *msg) {
#endif
	this->recv_data.clear();

	if(msg->empty()){
		return &this->recv_data;
	}

	// TODO: 记住上回的解析状态
#ifdef USE_PROTOBUF
	return this->msg_parse(msg);
#else
	int parsed = 0;
	int size = msg->size();
	int old_sz = size;
	char *head = msg->data();

	// ignore leading empty lines
	while(size > 0 && (head[0] == '\n' || head[0] == '\r')){
		head ++;
		size --;
		parsed ++;
	}

	// yue: Redis protocol parser
	if(head[0] == '*'){
		if(redis == NULL){
			redis = new RedisLink();
		}
		const std::vector<Bytes> *ret = redis->recv_req(msg);
		if(ret){
			this->recv_data = *ret;
			return &this->recv_data;
		}else{
			return NULL;
		}
	}

	msg->remember_sz(old_sz);
	
	while(size > 0){
		char *body = (char *)memchr(head, '\n', size);
		if(body == NULL){
			break;
		}
		body++;

		int head_len = body - head;
		if(head_len == 1 || (head_len == 2 && head[0] == '\r')){
			// packet end
			parsed += head_len;
			msg->decr(parsed);
			return &this->recv_data;;
		}
		if(head[0] < '0' || head[0] > '9'){
			//log_warn("bad format");
			return NULL;
		}

		char head_str[20];
		if(head_len > (int)sizeof(head_str) - 1){
			return NULL;
		}
		memcpy(head_str, head, head_len - 1); // no '\n'
		head_str[head_len - 1] = '\0';

		int body_len = atoi(head_str);
		if(body_len < 0){
			//log_warn("bad format");
			return NULL;
		}
		//log_debug("size: %d, head_len: %d, body_len: %d", size, head_len, body_len);
		size -= head_len + body_len;
		if(size < 0){
			break;
		}

		this->recv_data.push_back(Bytes(body, body_len));

		head += head_len + body_len;
		parsed += head_len + body_len;
		if(size > 0 && head[0] == '\n'){
			head += 1;
			size -= 1;
			parsed += 1;
		}else if(size > 1 && head[0] == '\r' && head[1] == '\n'){
			head += 2;
			size -= 2;
			parsed += 2;
		}else{
			break;
		}
		if(parsed > MAX_PACKET_SIZE){
			 //log_warn("fd: %d, exceed max packet size, parsed: %d", this->sock, parsed);
			 return NULL;
		}
	}

	if(msg->space() == 0){
		msg->nice();
		if(msg->space() == 0){
			if(msg->grow() == -1){
				//log_error("fd: %d, unable to resize input buffer!", this->sock);
				return NULL;
			}
			//log_debug("fd: %d, resize input buffer, %s", this->sock, input->stats().c_str());
		}
	}

	// not ready
	this->recv_data.clear();
	return &this->recv_data;
#endif  // USE_PROTOBUF
}

const std::vector<Bytes>* Link::recv() {
	this->recv_data.clear();

	if(input->empty()){
		return &this->recv_data;
	}

	// TODO: 记住上回的解析状态
	int parsed = 0;
	int size = input->size();
	char *head = input->data();

	// ignore leading empty lines
	while(size > 0 && (head[0] == '\n' || head[0] == '\r')){
		head ++;
		size --;
		parsed ++;
	}

	// Redis protocol supports
	if(head[0] == '*'){
		if(redis == NULL){
			redis = new RedisLink();
		}
		const std::vector<Bytes> *ret = redis->recv_req(input);
		if(ret){
			this->recv_data = *ret;
			return &this->recv_data;
		}else{
			return NULL;
		}
	}
	
	while(size > 0){
		char *body = (char *)memchr(head, '\n', size);
		if(body == NULL){
			break;
		}
		body ++;

		int head_len = body - head;
		if(head_len == 1 || (head_len == 2 && head[0] == '\r')){
			// packet end
			parsed += head_len;
			input->decr(parsed);
			return &this->recv_data;;
		}
		if(head[0] < '0' || head[0] > '9'){
			//log_warn("bad format");
			return NULL;
		}

		char head_str[20];
		if(head_len > (int)sizeof(head_str) - 1){
			return NULL;
		}
		memcpy(head_str, head, head_len - 1); // no '\n'
		head_str[head_len - 1] = '\0';

		int body_len = atoi(head_str);
		if(body_len < 0){
			//log_warn("bad format");
			return NULL;
		}
		//log_debug("size: %d, head_len: %d, body_len: %d", size, head_len, body_len);
		size -= head_len + body_len;
		if(size < 0){
			break;
		}

		this->recv_data.push_back(Bytes(body, body_len));

		head += head_len + body_len;
		parsed += head_len + body_len;
		if(size > 0 && head[0] == '\n'){
			head += 1;
			size -= 1;
			parsed += 1;
		}else if(size > 1 && head[0] == '\r' && head[1] == '\n'){
			head += 2;
			size -= 2;
			parsed += 2;
		}else{
			break;
		}
		if(parsed > MAX_PACKET_SIZE){
			 //log_warn("fd: %d, exceed max packet size, parsed: %d", this->sock, parsed);
			 return NULL;
		}
	}

	if(input->space() == 0){
		input->nice();
		if(input->space() == 0){
			if(input->grow() == -1){
				//log_error("fd: %d, unable to resize input buffer!", this->sock);
				return NULL;
			}
			//log_debug("fd: %d, resize input buffer, %s", this->sock, input->stats().c_str());
		}
	}

	// not ready
	this->recv_data.clear();
	return &this->recv_data;
}

void Link::set_dummy()
{
	dummy_ = 1;
}

int Link::send(const std::vector<std::string> &resp){
	if(resp.empty()){
		return 0;
	}
	
	for(unsigned i=0; i<resp.size(); i++){
		output->append_record(resp[i]);
	}
	output->append('\n');
	return 0;
}

int Link::send(const std::vector<Bytes> &resp){
	for(unsigned i=0; i<resp.size(); i++){
		output->append_record(resp[i]);
	}
	output->append('\n');
	return 0;
}

int Link::send(const Bytes &s1){
	output->append_record(s1);
	output->append('\n');
	return 0;
}

int Link::send(const Bytes &s1, const Bytes &s2){
	output->append_record(s1);
	output->append_record(s2);
	output->append('\n');
	return 0;
}

int Link::send(const Bytes &s1, const Bytes &s2, const Bytes &s3){
	output->append_record(s1);
	output->append_record(s2);
	output->append_record(s3);
	output->append('\n');
	return 0;
}

int Link::send(const Bytes &s1, const Bytes &s2, const Bytes &s3, const Bytes &s4){
	output->append_record(s1);
	output->append_record(s2);
	output->append_record(s3);
	output->append_record(s4);
	output->append('\n');
	return 0;
}

int Link::send(const Bytes &s1, const Bytes &s2, const Bytes &s3, const Bytes &s4, const Bytes &s5){
	output->append_record(s1);
	output->append_record(s2);
	output->append_record(s3);
	output->append_record(s4);
	output->append_record(s5);
	output->append('\n');
	return 0;
}

const std::vector<Bytes>* Link::response(){
	while(1){
		const std::vector<Bytes> *resp = this->recv();
		if(resp == NULL){
			return NULL;
		}else if(resp->empty()){
			if(this->read() <= 0){
				return NULL;
			}
		}else{
			return resp;
		}
	}
	return NULL;
}

const std::vector<Bytes>* Link::request(const Bytes &s1){
	if(this->send(s1) == -1){
		return NULL;
	}
	if(this->flush() == -1){
		return NULL;
	}
	return this->response();
}

const std::vector<Bytes>* Link::request(const Bytes &s1, const Bytes &s2){
	if(this->send(s1, s2) == -1){
		return NULL;
	}
	if(this->flush() == -1){
		return NULL;
	}
	return this->response();
}

const std::vector<Bytes>* Link::request(const Bytes &s1, const Bytes &s2, const Bytes &s3){
	if(this->send(s1, s2, s3) == -1){
		return NULL;
	}
	if(this->flush() == -1){
		return NULL;
	}
	return this->response();
}

const std::vector<Bytes>* Link::request(const Bytes &s1, const Bytes &s2, const Bytes &s3, const Bytes &s4){
	if(this->send(s1, s2, s3, s4) == -1){
		return NULL;
	}
	if(this->flush() == -1){
		return NULL;
	}
	return this->response();
}

const std::vector<Bytes>* Link::request(const Bytes &s1, const Bytes &s2, const Bytes &s3, const Bytes &s4, const Bytes &s5){
	if(this->send(s1, s2, s3, s4, s5) == -1){
		return NULL;
	}
	if(this->flush() == -1){
		return NULL;
	}
	return this->response();
}

#if 0
int main(){
	//Link link;
	//link.listen("127.0.0.1", 8888);
	Link *link = Link::connect("127.0.0.1", 8080);
	printf("%d\n", link);
	getchar();
	return 0;
}
#endif
