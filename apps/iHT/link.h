#pragma once

#include <event.h>

#include "conkv.h"
#include "str.h"

class String;

typedef enum {
	conn_listening,  /* yue: listener state */
	conn_slave       /* yue: worker state */
} conn_t;

typedef struct _cq_item_t {
	int sfd;
	conn_t type;
	int event_flags;
} CQ_ITEM;

class Link {
	private:
		int sfd;
		conn_t type;
		int ev_flags;
		bool noblock_;
		bool error_;
		std::vector<Bytes> read_buf;

	public:
		Link();
		~Link();

		void set_sfd(const int sfd);
		int get_sfd() const;
		void set_ev_flags(const int event_flags);
		int get_ev_flags() const;
		void set_type(const conn_t type);
		conn_t get_type() const;
		std::vector<Bytes> get_read_buf();
		int accept();
		int msg_read();
		int msg_write();
		const std::vector<Bytes> *msg_parse();
		void mark_error();
		void close();
		
		struct event event;
		short which;
		String *input;
		String *output;
		struct _thread_t *thread;
};
