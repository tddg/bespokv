#pragma once

#include "server.h"
#include "net.h"
#include "link.h"

class Link;

typedef std::unordered_map<AddrType, Link *> DumbConnTable_t;

typedef struct _ContinuumItem {
	uint32_t point;
	Link *conn;
} ContinuumItem_t;

class Continuum {
	public:
		Continuum(DumbConnTable_t &dumb_map, int32_t total_weight);
		//Link *ketama_get_conn(const char *key);
		Link *ketama_get_conn(const std::string &key);
	private:
		uint32_t ketama_hashi(const char *str);
		void ketama_md5_digest(const char *str, unsigned char md5pwd[16]);

		std::vector<std::shared_ptr<ContinuumItem_t> > array;
};
