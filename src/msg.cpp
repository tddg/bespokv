#include "server.h"
#include "link.h"
#include "msg.h"
#include "net.h"
#include "string.h"

#include "spec.pb.h"

int ev_counter = 0;

rstatus_t admin_replica_recv(NetworkServer *proxy, Link *conn) {
	char buf[1024];
	int ret = ::recv(conn->fd(), buf, 1024, 0);
	std::string data = std::string(buf, ret);
	spec::Message msg;
	msg.ParseFromString(data);

	LOG(INFO) << "replica_recv.key=" << msg.key() << ", replica_recv.val=" << msg.val();

	return CO_OK;
}

rstatus_t req_recv(NetworkServer *proxy, Link *conn) {
	// yue: read is actually doing network recv, while recv() is for protocol
	// preprocessing
	CHECK(conn != NULL);
#ifdef USE_SMART_PTR
	std::shared_ptr<Buffer> msg = conn->msg_read();
#else
	Buffer *msg = conn->msg_read();
#endif
	if (msg == NULL) {
		LOG(INFO) << "req_recv: fd=" << conn->fd() << ", read_len<=0, delete link";
		perror("req_recv");
		conn->mark_error();
		proxy->dec_conn_count();
		Fdevents *fdes = proxy->get_fdes();
		// yue: delete the event and free up the memory
		proxy->dec_conn_count();
		fdes->del(conn->fd());
#ifdef RECLAIM_CONN
		delete conn;
		return CO_OK;
#else
		return CO_C_DROP;
#endif
	}

	// yue: set the msg owner to be the client conn
	msg->owner = conn;

	while (msg->size()) {
		char *curr_ptr = msg->data();
		Link *out_conn = req_mapped_forward(proxy, conn, msg);
		CHECK(out_conn != NULL);

		if (out_conn && out_conn->imsg_q.empty()) {
			Fdevents *fdes = proxy->get_fdes();
			fdes->set(out_conn->fd(), FDEVENT_OUT, 1, out_conn);
			ev_counter++;
		}
		// yue: break the long msg into segments
		int req_sz = msg->size_diff();
#ifdef USE_SMART_PTR
		std::shared_ptr<Buffer> req = std::make_shared<Buffer>(req_sz);

		// TODO: ali: following is used to check the accuracy of MS strong topology
		if (out_conn->fwd_to_replicas) {
				req->fwd_to_replicas = true;
		}

#else
		Buffer *req = new Buffer(req_sz);
#endif
		CHECK(req != NULL);
		CHECK(conn->last_recv()->size() > 0);
		req->append(curr_ptr, req_sz);
#ifdef USE_PROTOBUF
		conn->clear_recv_data();
#endif

		// yue: set the req owner this client conn
		req->owner = conn;
		if (req->owner) {
			if (req->owner->is_server)	fprintf(stderr, "req_recv is server\n");
		} else fprintf(stderr, "req_recv owner is null\n");

		req->id = 1;

#ifdef USE_KAFKA
		// if it is a set request only then we fwd it to kafka
		if ((proxy->topology == ms || proxy->topology == aa) && out_conn->fwd_to_replicas) {
			CHECK(out_conn->kproducer != NULL);
			out_conn->kproducer->produce(req->data(), req->size(), "kevin");
		}
#endif

		// A-A strong consistency requires read + write lock for time being
		if (proxy->consistency_model == strong && proxy->topology == aa) {
			if (proxy->consistency_technique == rl) {
				bool flag = out_conn->conlock->lock((*out_conn->key).c_str(), 50);

				if (out_conn->fwd_to_replicas) {
					if (flag) {
						// Lock acquired
						std::vector<Link *> slaves = proxy->get_slaves(out_conn);
						for (std::vector<Link *>::const_iterator i = slaves.begin(); i != slaves.end(); i++) {
							Link *slave = *i;
							if (slave && slave->imsg_q.empty()) {
								Fdevents *fdes = proxy->get_fdes();
								fdes->set(slave->fd(), FDEVENT_OUT, 1, slave);
								ev_counter++;
							}
							slave->imsg_q.push_back(req);
							//fprintf(stderr, "acquired lock\n");
						}
					} else {
						fprintf(stderr, "NOT acquired lock\n");
						conn->imsg_q.front(); conn->imsg_q.push_front(msg);
						Fdevents *fdes = proxy->get_fdes();
						fdes->set(conn->fd(), FDEVENT_IN, 1, conn);
						return CO_OK;
					}
				}
			}
		}

		// Acquire lock on the key in M-S
		// Strong consistency
		// Command is set
		if (proxy->consistency_model == strong && proxy->topology == ms && out_conn->fwd_to_replicas) {

			// Enable for acquiring lock with redlock
			if (proxy->consistency_technique == rl) {
				//while (!out_conn->dlm->Lock((*out_conn->key).c_str(), 100, my_lock));
				out_conn->my_lock = new CLock();
				bool flag = out_conn->dlm->Lock((*out_conn->key).c_str(), 1000, *out_conn->my_lock);
				if (flag) {
					// do resource job
					//out_conn->dlm->Unlock(my_lock);
				} else {
					conn->imsg_q.front(); conn->imsg_q.push_front(msg);
					Fdevents *fdes = proxy->get_fdes();
					fdes->set(conn->fd(), FDEVENT_IN, 1, conn);
					return CO_OK;
				}
			}

			// Enable for acquiring lock through ZK
			if (proxy->consistency_technique == zk) {
				// Testing lock
				out_conn->zkc->CreateIfNotExists("/zookeeper/"+ *out_conn->key);
				out_conn->zkc->Get("/zookeeper", false);
				out_conn->lock_id = new std::string(out_conn->zkc->Lock("/zookeeper/" + *out_conn->key, true));
				out_conn->zkc->Unlock(*out_conn->lock_id);
				out_conn->zkc->DeleteIfExists("/zookeeper/"+ *out_conn->key);
			}
		}

#ifndef USE_KAFKA
		// Use the extra thread to serve the replica requests
		if (proxy->consistency_model == eventual && proxy->topology == ms && out_conn->fwd_to_replicas) {
			std::vector<Link *> slaves = proxy->get_slaves(out_conn);
			for (std::vector<Link *>::const_iterator i = slaves.begin(); i != slaves.end(); i++) {
				Link *slave = *i;
				if (slave && slave->imsg_q.empty()) {
					//TODO: ali: I believe that following should be get_fdes - double check
					//           if we decide sometime later not to use Kafka
					Fdevents *fdes = proxy->get_fdes_rpl();
					fdes->set(slave->fd(), FDEVENT_OUT, 1, slave);
					ev_counter++;
				}

				slave->imsg_q.push_back(req);
			}
		}
#endif

		// Fwd the req.
		CHECK(req->id == 1);
		out_conn->imsg_q.push_back(req);

		// Propagate to slaves if we do not want to monitor any ack.
		// No consistency with M-S
		// Mo topology with multiple replicas
		if ((proxy->topology == no && (proxy->replicas > 0) && out_conn->fwd_to_replicas)
				|| (proxy->consistency_model == without && proxy->topology == ms && out_conn->fwd_to_replicas)){
			std::vector<Link *> slaves = proxy->get_slaves(out_conn);
			for (std::vector<Link *>::const_iterator i = slaves.begin(); i != slaves.end(); i++) {
				Link *slave = *i;
				if (slave && slave->imsg_q.empty()) {
					Fdevents *fdes = proxy->get_fdes();
					fdes->set(slave->fd(), FDEVENT_OUT, 1, slave);
					ev_counter++;
				}
#if 0
				Buffer *req_slave = new Buffer(req_sz);
				CHECK(req_slave != NULL);
				req_slave->append(curr_ptr, req_sz);
#endif
				slave->imsg_q.push_back(req);
			}
		}
	}

#ifndef USE_SMART_PTR
	delete msg;
#endif
	return CO_OK;
}

rstatus_t req_send(NetworkServer *proxy, Link *conn) {
#ifdef USE_SMART_PTR
	std::shared_ptr<Buffer> smsg;
#else
	Buffer *smsg;
#endif
	if (!conn->imsg_q.empty()) {
		smsg = conn->imsg_q.front(); conn->imsg_q.pop_front();
	} else {
		Fdevents *fdes = proxy->get_fdes();
		fdes->clr(conn->fd(), FDEVENT_OUT);  // yue: nothing to send so clear ev
		ev_counter--;

		return CO_OK;  // yue: nothing to send
	}

	int len = conn->msg_send(smsg);
	if (len <= 0) {
		LOG(ERROR) << "req_send: fd=" << conn->fd() << ", write=" << len << ", delete link";
		conn->mark_error();
		Fdevents *fdes = proxy->get_fdes();
		fdes->del(conn->fd());
		return CO_ERR;
	}

	// yue: this is buggy so far: we should do on-demand connection where we
	// initialize the events for both in and out
	Fdevents *fdes = proxy->get_fdes();
	conn->omsg_q.push_back(smsg);
	fdes->set(conn->fd(), FDEVENT_IN, 1, conn);
	
	if (smsg->owner) {
		if (smsg->owner->is_server)	fprintf(stderr, "req_send is server\n");
	} else fprintf(stderr, "req_send owner is null; rsp->count=%d\n", smsg->_count);
	CHECK(smsg->id == 1);

	return CO_OK;
}

rstatus_t replica_req_send(NetworkServer *proxy, Link *conn) {
	CHECK(conn->is_master != true);
#ifdef USE_SMART_PTR
	std::shared_ptr<Buffer> smsg;
#else
	Buffer *smsg;
#endif
	if (!conn->imsg_q.empty()) {
		smsg = conn->imsg_q.front(); conn->imsg_q.pop_front();
	} else {
		Fdevents *fdes = proxy->get_fdes();
		fdes->clr(conn->fd(), FDEVENT_OUT);  // yue: nothing to send so clear ev
		ev_counter--;

		return CO_OK;  // yue: nothing to send
	}

	CHECK(smsg->size() > 0);
	int len = conn->msg_send(smsg);
	if (len <= 0) {
		LOG(ERROR) << "replica_req_send: fd=" << conn->fd() << ", write=" << len << ", delete link";
		conn->mark_error();
		Fdevents *fdes = proxy->get_fdes();
		fdes->del(conn->fd());
		return CO_ERR;
	}

	// yue: this is buggy so far: we should do on-demand connection where we
	// initialize the events for both in and out
	Fdevents *fdes = proxy->get_fdes();
	conn->omsg_q.push_back(smsg);
	fdes->set(conn->fd(), FDEVENT_IN, 1, conn);

	return CO_OK;
}

/**
 * yue: Handler for replica responce; simply check the rsp content wo bothering
 * forwarding it back
 */
rstatus_t replica_rsp_recv(NetworkServer *proxy, Link *conn) {
	CHECK(conn->is_master != true);
#ifdef USE_SMART_PTR
	std::shared_ptr<Buffer> msg;
#else
	Buffer *msg;
#endif
	msg = conn->msg_read();
	if (msg == NULL) {
		LOG(ERROR) << "replica_rsp_recv: fd=" << conn->fd() << ", read_len<=0, delete link";
		conn->mark_error();
		Fdevents *fdes = proxy->get_fdes();
		fdes->del(conn->fd());
		return CO_OK;
	}


	Link *master = proxy->get_master_of_slave(conn);
	CHECK(master != NULL);
	CHECK(master->is_server);
	CHECK(conn!=master);

	// Following serve the set req. coming from head in chain replication
	if(proxy->consistency_model == strong  && proxy->consistency_technique == cr
			&& proxy->topology == ms && master->fwd_to_replicas) {
		//fprintf(stderr, "Current slave number %d\n", conn->slave_number);
		if (conn->slave_number < proxy->replicas - 1)
		{
			// Start the chain replication from the first slave
			Link *slave = proxy->get_slave_number(master, conn->slave_number + 1);
			CHECK(conn!=slave);

			if (slave && slave->imsg_q.empty()) {
				Fdevents *fdes = proxy->get_fdes();
				fdes->set(slave->fd(), FDEVENT_OUT, 1, slave);
				ev_counter++;
			}
			CHECK(slave->is_master != true);

			// pop up the entry which we do not need anymore
			CHECK(!conn->omsg_q.empty());
			std::shared_ptr<Buffer> pmsg = conn->omsg_q.front(); conn->omsg_q.pop_front();
			CHECK(pmsg->owner != NULL);
			CHECK(pmsg->owner->is_client);
			slave->imsg_q.push_back(pmsg);
			CHECK(pmsg->fwd_to_replicas);

#ifndef USE_SMART_PTR
			/* ali: FIXME: do something meaningful replica msg before we delete it */
			delete msg;
#endif

			return CO_OK;
		}

#ifndef USE_SMART_PTR
		/* yue: FIXME: do something meaningful on msg before we delete it */
		delete msg;
#endif

		CHECK(!conn->omsg_q.empty());
		std::shared_ptr<Buffer> pmsg = conn->omsg_q.front(); conn->omsg_q.pop_front();
		CHECK(pmsg != NULL);

		Link *out_conn = pmsg->owner;
		CHECK(out_conn != NULL);

		if (out_conn && out_conn->imsg_q.empty()) {
			Fdevents *fdes = proxy->get_fdes();
			fdes->set(out_conn->fd(), FDEVENT_OUT, 1, out_conn);
			ev_counter++;
		}

		out_conn->imsg_q.push_back(msg);

		return CO_OK;
	}

	// Following serve the get req. treating conn as tail in chain replication
	if (proxy->consistency_model == strong  && proxy->consistency_technique == cr
			&& proxy->topology == ms && conn->tail && (!master->fwd_to_replicas)) {

		// Following will auto pop the conn->omsg_q
		Link *out_conn = rsp_mapped_forward(proxy, conn, msg);

		if (out_conn && out_conn->imsg_q.empty()) {
			Fdevents *fdes = proxy->get_fdes();
			fdes->set(out_conn->fd(), FDEVENT_OUT, 1, out_conn);
			ev_counter++;
		}
		out_conn->imsg_q.push_back(msg);
		conn->tail = false;

		return CO_OK;
	}

	// pop up the entry which we do not need anymore
	if (!conn->omsg_q.empty()) {
		conn->omsg_q.front(); conn->omsg_q.pop_front();
	}

#ifndef USE_SMART_PTR
	/* ali: FIXME: do something meaningful on msg before we delete it */
	delete msg;
#endif

	return CO_OK;
}

rstatus_t rsp_recv(NetworkServer *proxy, Link *conn) {
#ifdef USE_SMART_PTR
	std::shared_ptr<Buffer> msg;
#else
	Buffer *msg;
#endif
	msg = conn->msg_read();
	if (msg == NULL) {
		LOG(ERROR) << "rsp_recv: fd=" << conn->fd() << ", read_len<=0, delete link";
		conn->mark_error();
		Fdevents *fdes = proxy->get_fdes();
		fdes->del(conn->fd());
#if 0
		// yue: FIXME later
		proxy->dec_conn_count();
		Fdevents *fdes = proxy->get_fdes();
		fdes->del(conn->fd());
#endif
		return CO_OK;
	}
	msg->id = 2;
	static int32_t _count = 0;
	msg->_count = _count++;

	// Unlock the redlock
	if (proxy->consistency_model == strong && proxy->topology == ms && conn->fwd_to_replicas) {
		if (proxy->consistency_technique == zk) {
			// Unlock on the key
			conn->zkc->Unlock(*conn->lock_id);
		}
		// Enable for unlocking for redlock
		if (proxy->consistency_technique == rl) {
			// Testing lock
			conn->dlm->Unlock(*conn->my_lock);
			free(conn->my_lock);
			free(conn->cmnd);
			free(conn->key);
		}
		if (proxy->consistency_technique == cr) {

			Link *slave = proxy->get_slave_number(conn, 0);
			CHECK(slave != NULL);
			if (slave && slave->imsg_q.empty()) {
				Fdevents *fdes = proxy->get_fdes();
				fdes->set(slave->fd(), FDEVENT_OUT, 1, slave);
				ev_counter++;
			}
			CHECK(slave->is_master != true);

#if 0
			conn->output = msg;
			slave->input = conn->input;
			slave->imsg_q.push_back(conn->input);
#endif
			CHECK(!conn->omsg_q.empty());
			std::shared_ptr<Buffer> pmsg = conn->omsg_q.front(); conn->omsg_q.pop_front();
			CHECK(pmsg->owner->is_client);
			slave->imsg_q.push_back(pmsg);
			return CO_OK;
		}
	}

	if (proxy->consistency_model == strong && proxy->topology == aa) {
		// TODO: Enable the check later
		conn->conlock->unlock(conn->key->c_str());
		
		free(conn->cmnd);
		free(conn->key);
	}

	Link *out_conn = rsp_mapped_forward(proxy, conn, msg);

	if (out_conn && out_conn->imsg_q.empty()) {
		//conn_count++;
		Fdevents *fdes = proxy->get_fdes();
		fdes->set(out_conn->fd(), FDEVENT_OUT, 1, out_conn);
		ev_counter++;
	}

	out_conn->imsg_q.push_back(msg);

	return CO_OK;
}

rstatus_t rsp_send(NetworkServer *proxy, Link *conn) {
#ifdef USE_SMART_PTR
	std::shared_ptr<Buffer> smsg;
#else
	Buffer *smsg;
#endif
	if (!conn->imsg_q.empty()) {
		smsg = conn->imsg_q.front(); conn->imsg_q.pop_front();
	} else {
		Fdevents *fdes = proxy->get_fdes();
		fdes->clr(conn->fd(), FDEVENT_OUT);  // yue: nothing to send so del ev
		ev_counter--;

		return CO_OK;  // yue: nothing to send
	}
	CHECK(smsg != NULL);

	int len = conn->msg_write(smsg);
	if (len <= 0) {
		LOG(ERROR) << "rsp_send: fd=" << conn->fd() << ", write=" << len << ", delete link";
		conn->mark_error();
		Fdevents *fdes = proxy->get_fdes();
		fdes->del(conn->fd());
		return CO_ERR;
	}

#ifndef USE_SMART_PTR
	delete smsg;
#endif
	return CO_OK;
}

#ifdef USE_SMART_PTR
Link *req_forward(NetworkServer *proxy, Link *conn, std::shared_ptr<Buffer> msg) {
#else
Link *req_forward(NetworkServer *proxy, Link *conn, Buffer *msg) {
#endif

	return proxy->get_dumb_conn();
}

#ifdef USE_SMART_PTR
Link *req_mapped_forward(NetworkServer *proxy, Link *conn, std::shared_ptr<Buffer> msg) {
#else
Link *req_mapped_forward(NetworkServer *proxy, Link *conn, Buffer *msg) {
#endif

	return proxy->get_mapped_server_conn(conn, msg);
}

#ifdef USE_SMART_PTR
Link *rsp_forward(NetworkServer *proxy, Link *conn, std::shared_ptr<Buffer> msg) {
#else
Link *rsp_forward(NetworkServer *proxy, Link *conn, Buffer *msg) {
#endif
	CHECK(!conn->omsg_q.empty());

#ifdef USE_SMART_PTR
	std::shared_ptr<Buffer> pmsg = conn->omsg_q.front(); conn->omsg_q.pop_front();
#else
	Buffer *pmsg = conn->omsg_q.front(); conn->omsg_q.pop_front();
#endif

#if 0
	/* yue: establish msg <-> pmsg (response <-> request) link */
	pmsg->peer = msg;
	msg->peer = pmsg;
#endif

	Link *client_conn = pmsg->owner;
	CHECK(client_conn != conn);

#ifndef USE_SMART_PTR
	delete pmsg;
#endif
	return client_conn;
}

#ifdef USE_SMART_PTR
Link *rsp_mapped_forward(NetworkServer *proxy, Link *conn, std::shared_ptr<Buffer> msg) {
#else
Link *rsp_mapped_forward(NetworkServer *proxy, Link *conn, Buffer *msg) {
#endif
	CHECK(!conn->omsg_q.empty());

#ifdef USE_SMART_PTR
	std::shared_ptr<Buffer> pmsg = conn->omsg_q.front(); conn->omsg_q.pop_front();
#else
	Buffer *pmsg = conn->omsg_q.front(); conn->omsg_q.pop_front();
#endif

#if 0
	/* yue: establish msg <-> pmsg (response <-> request) link */
	pmsg->peer = msg;
	msg->peer = pmsg;
#endif

	Link *client_conn = pmsg->owner;
	if (client_conn == NULL) {
		fprintf(stderr, "pmsg->size=%d, pmsg->id=%d, data=%s, size_diff=%d\n", 
				pmsg->size(), pmsg->id, pmsg->data(), pmsg->size_diff());
		if (conn->is_master)	fprintf(stderr, "master\n");
		else fprintf(stderr, "slave\n");
	}
	CHECK(client_conn != NULL);
	CHECK(client_conn != conn);

	/* yue: FIXME */
	return client_conn;
}
