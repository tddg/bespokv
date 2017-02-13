#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <glog/logging.h>

#include "link.h"

#define INIT_BUF_SIZE 8

Link::Link() {
	this->sfd = -1;
	this->type = (conn_t)-1;
	this->ev_flags = 0;
	this->which = 0;
	this->noblock_ = true;  /* yue: by default its enabled */
	this->error_ = false;
	this->input = new String(INIT_BUF_SIZE);
	this->output = new String(INIT_BUF_SIZE);
}

Link::~Link() {
	if (input)	delete input;
	if (output)	delete output;
	this->close();
}

void Link::close() {
	event_del(&this->event);
	if (this->sfd >= 0)	::close(sfd);
}

void Link::mark_error() {
	this->error_ = true;
}

void Link::set_sfd(const int sfd) {
	this->sfd = sfd;
}

int Link::get_sfd() const {
	return this->sfd;
}

void Link::set_ev_flags(const int event_flags) {
	this->ev_flags = event_flags;
}

int Link::get_ev_flags() const {
	return this->ev_flags;
}

void Link::set_type(const conn_t type) {
	this->type = type;
}

conn_t Link::get_type() const {
	return this->type;
}

std::vector<Bytes> Link::get_read_buf() {
	return this->read_buf;
}

int Link::accept() {
	int ret_sfd;
	socklen_t addrlen;
	struct sockaddr_storage addr;
	
	addrlen = sizeof(addr);
	ret_sfd = ::accept(this->sfd, (struct sockaddr *)&addr, &addrlen);
	if (ret_sfd == -1) {
		perror("accept()");
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return ret_sfd;
		} else if (errno == EMFILE) {
			fprintf(stderr, "Too many open connections\n");
			/* yue: TODO: add decline to listen to new conn logic here */
			return ret_sfd;
		} else {
			return ret_sfd;
		}
	}
	if (::fcntl(ret_sfd, F_SETFL, ::fcntl(ret_sfd, F_GETFL)|O_NONBLOCK) < 0) {
		perror("setting O_NONBLOCK");
		::close(ret_sfd);
		return -1;
	}
	fprintf(stderr, "%d: accept and return %d\n", this->sfd, ret_sfd);
	return ret_sfd;
}

int Link::msg_read() {
	int ret = 0;
	int wanted;
	if (input->total() == INIT_BUF_SIZE) {
		input->grow();
	}
	input->nice();
	while ((wanted = input->avail()) > 0) {
		//fprintf(stderr, "msg_read: wanted=%d\n", wanted);
		int len = ::read(sfd, input->curr(), wanted);
		if (len == -1) {
			if (errno == EINTR) {
				continue;
			} else if (errno == EWOULDBLOCK) {
				break;
			} else {
				return -1;
			}
		} else {
			if (len == 0) {
				return 0;
			}
			ret += len;
			input->incr(len);
		}
		if (!noblock_) {
			break;
		}
	}
	return ret;
}

int Link::msg_write() {
	int ret = 0;
	int wanted;
	if (output->total() == INIT_BUF_SIZE) {
		output->grow();
	}
	while ((wanted = output->size()) > 0) {
		int len = ::write(sfd, output->data(), wanted);
		if (len == -1) {
			if (errno == EINTR) {
				continue;
			} else if (errno == EWOULDBLOCK) {
				break;
			} else {
				return -1;
			}
		} else {
			if (len == 0) {
				break;
			}
			ret += len;
			output->discard(len);
		}
		if (!noblock_) {
			break;
		}
	}
	output->nice();
	return ret;
}

const std::vector<Bytes> *Link::msg_parse() {
	this->read_buf.clear();
	if (this->input->empty()) {
		return &this->read_buf;
	}

	int parsed = 0;
	char *ptr = this->input->data();
#if 0
	int remember_sz_before_parse = this->input->size();
	while (size > 0 && (ptr[0] == '\n'|| ptr[0] == '\r')) {
		ptr++;
		size--;
		parsed++;
	}
#endif
	//for (int i = 0; i < size; i++) {
	//	fprintf(stderr, "%c ", *tmp);
	//	tmp++;
	//}

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
	
	char *head = this->input->data();

	size_t typeLen = *((size_t *)ptr);
	ptr += sizeof(size_t);
	CHECK((size_t)(ptr-head) < totalSize);
	parsed += sizeof(size_t);

	//std::string reqType(ptr, typeLen);
	this->read_buf.push_back(Bytes(ptr, typeLen));
	ptr += typeLen;
	parsed += typeLen;

	size_t dataLen = *((size_t *)ptr);
	ptr += sizeof(size_t);
	CHECK((size_t)(ptr-head) < totalSize);
	CHECK((size_t)(ptr+dataLen-head) == totalSize);
	parsed += sizeof(size_t);
	
	this->read_buf.push_back(Bytes(ptr, dataLen));
	ptr += dataLen;
	parsed += dataLen;
	this->input->discard(parsed);
	//fprintf(stderr, "msg_parse:sz_before_parse=%d, sz_after_parse=%d, parsed=%d\n", 
	//		remember_sz_before_parse, this->input->size(), parsed);
	return &this->read_buf;
}
