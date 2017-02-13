#pragma once

#include <deque>
#include "link.h"

#define HASHPOWER_DEFAULT 16

typedef unsigned long int ub4;
typedef unsigned char     ub1;

#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)

static const uint32_t MAGIC = 0x06121983;

struct settings {
	size_t maxbytes;
	int port;
	char *inter;
	int num_threads;
	unsigned int hashpower_init;
	double factor;
	bool use_slabs;
};

typedef struct _stritem {
	struct _stritem *next;
	struct _stritem *prev;
	struct _stritem *h_next;
	int nbytes;
	unsigned short refcount;
	uint8_t slabs_clsid;
	uint8_t nkey;
} item;

struct _thread_t {
	pthread_t thread_id;
	struct event_base *base;
	struct event notify_event;
	int notify_receive_fd;
	int notify_send_fd;
	std::deque<struct _cq_item_t *> *new_conn_q;
	pthread_mutex_t cq_lock;
}; 

typedef struct _thread_t LIBEVENT_THREAD;
