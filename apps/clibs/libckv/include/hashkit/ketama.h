#pragma once

#include <cassert>
#include <vector>
#include <algorithm>
#include "Connection.h"
#include "hashkit/hashkit.h"

namespace douban {
namespace mc {
namespace hashkit {


typedef struct continuum_item_s {
  uint32_t hash_value;
  size_t conn_idx;
  douban::mc::Connection* conn;
	std::vector<douban::mc::Connection *> *replica_conns; // yue: a small array of conns connecting to replicas of "conn"

  static struct compare_s {
    bool operator() (const struct continuum_item_s& left, const struct continuum_item_s& right) {
      return left.hash_value < right.hash_value;
    }
  } compare;
} continuum_item_t;


class KetamaSelector {
 public:
  KetamaSelector();
  void setHashFunction(hash_function_t fn);
  void enableFailover();
  void disableFailover();

  void reset();
  void addServers(douban::mc::Connection* conns, size_t nConns);
  void addServersReplica(douban::mc::Connection **conns, size_t nHosts, size_t nReplicas); // yue

  int getServer(const char* key, size_t key_len, bool check_alive = true);
  douban::mc::Connection* getConn(const char* key, size_t key_len, bool check_alive = true);
	douban::mc::Connection *getConnReplica(const char* key, size_t key_len, int which_replica,
			bool check_alive = true); // yue

 protected:
  std::vector<continuum_item_t>::iterator getServerIt(const char* key, size_t key_len,
                                                      bool check_alive);
	douban::mc::Connection *getServerConnReplica(const char* key, size_t key_len, int which_replica,
			bool check_alive); // yue

  std::vector<continuum_item_t> m_continuum;
  size_t m_nServers;
	size_t m_nReplicas;
  bool m_useFailover;
  hash_function_t m_hashFunction;
  static const size_t s_pointerPerHash;
  static const size_t s_pointerPerServer;
  static const hash_function_t s_defaultHashFunction;

#ifndef NDEBUG
  bool m_sorted;
#endif
};

} // namespace hashkit
} // namespace mc
} // namespace douban
