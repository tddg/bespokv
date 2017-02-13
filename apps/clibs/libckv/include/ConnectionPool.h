#pragma once

#include <vector>
#include "Common.h"
#include "Connection.h"
#include "hashkit/ketama.h"

namespace douban {
namespace mc {

class ConnectionPool {
 public:
  ConnectionPool();
  ~ConnectionPool();
  void setHashFunction(hash_function_options_t fn_opt);
  int init(const char* const * hosts, const uint32_t* ports, const size_t n,
           const char* const * aliases = NULL);
  int init_replica(char ***hosts, uint32_t **ports, size_t num_hosts, size_t num_replicas,
					int get_from_which_replica, int set_to_which_replica,
					const char * const *aliases = NULL); // yue
  const char* getServerAddressByKey(const char* key, size_t keyLen);
  void enableConsistentFailover();
  void disableConsistentFailover();
  void dispatchRetrieval(op_code_t op, const char* const* keys, const size_t* keyLens,
                    size_t n_keys);
  void dispatchStorage(op_code_t op,
                        const char* const* keys, const size_t* keyLens,
                        const flags_t* flags, const exptime_t exptime,
                        const cas_unique_t* cas_uniques, const bool noreply,
                        const char* const* vals, const size_t* val_lens,
                        size_t nItems);
  void dispatchDeletion(const char* const* keys, const size_t* keyLens,
                       const bool noreply, size_t nItems);
  void dispatchTouch(const char* const* keys, const size_t* keyLens,
                     const exptime_t exptime, const bool noreply, size_t nItems);
  void dispatchIncrDecr(op_code_t op, const char* key, const size_t keyLen,
                        const uint64_t delta, const bool noreply);
  void broadcastCommand(const char * const cmd, const size_t cmdLens);

  err_code_t waitPoll();

  void collectRetrievalResult(std::vector<retrieval_result_t*>& results);
  void collectMessageResult(std::vector<message_result_t*>& results);
  void collectBroadcastResult(std::vector<broadcast_result_t>& results);
  void collectUnsignedResult(std::vector<unsigned_result_t*>& results);
  void reset();
  void setPollTimeout(int timeout);
  void setConnectTimeout(int timeout);
  void setRetryTimeout(int timeout);

 protected:
  void markDeadAll(pollfd_t* pollfds, const char*);
  void markDeadConn(Connection* conn, const char* reason, pollfd_t* fd_ptr);

  uint32_t m_nActiveConn; // wait for poll
  uint32_t m_nInvalidKey;
  std::vector<Connection*> m_activeConns;
  hashkit::KetamaSelector m_connSelector;
  Connection *m_conns;
	Connection **m_conns_replica; // yue: for supporting replicas
  size_t m_nConns;
	size_t m_nReplicas; // yue: for supporting replica routing in ketama
	int m_nGetFromWhichReplica; // yue: GET from which replica
	int m_nSetToWhichReplica; // yue: SET to which replica
  int m_pollTimeout;
};

} // namespace mc
} // namespace douban
