#ifndef CO_ZK_H_
#define CO_ZK_H_

#include <string>
#include <vector>

#include <assert.h>
#include <errno.h>
#include <zookeeper/zookeeper.h>
//#include "net.h"

namespace zookeeper {

class ZooWatcher {
public:
  virtual ~ZooWatcher() {}

  virtual void OnConnected() = 0;
  virtual void OnConnecting() = 0;
  virtual void OnSessionExpired() = 0;

  virtual void OnCreated(const char* path) = 0;
  virtual void OnDeleted(const char* path) = 0;
  virtual void OnChanged(const char* path) = 0;
  virtual void OnChildChanged(const char* path) = 0;
  virtual void OnNotWatching(const char* path) = 0;
};

typedef Stat NodeStat;

class ZooKeeper {
public:
  ZooKeeper(const std::string& server_hosts,
            ZooWatcher* global_watcher = nullptr,
            int timeout_ms = 5000 * 10000);
  ~ZooKeeper();

  // disable copy
  ZooKeeper(const ZooKeeper&) = delete;
  ZooKeeper& operator=(const ZooKeeper&) = delete;
  bool is_connected();
  bool is_expired();
  bool Exists(const std::string& path, bool watch = false, NodeStat* = nullptr);
  NodeStat Stat(const std::string& path);
  std::string Create(const std::string& path,
                     const std::string& value = std::string(),
                     int flag = 0);
  std::string CreateIfNotExists(const std::string& path,
                                const std::string& value = std::string(),
                                int flag = 0);
  void Delete(const std::string& path);
  void DeleteIfExists(const std::string& path);
  void Set(const std::string&path, const std::string& value);
  std::string Get(const std::string& path, bool watch = false);
  std::vector<std::string> GetChildren(const std::string& parent_path, bool watch = false);
  std::string Lock(const std::string& path, const int rw);
  void Unlock(const std::string& path);
  std::vector<std::string> GetProxies();
  std::vector<std::string> GetDumbNodesOfProxy(std::string proxy);
  std::vector<std::string> GetMasters();
  std::vector<std::string> GetSlavesOfMaster(std::string proxy);
  bool isMaster(std::string proxy, std::string dumbnode);
  std::vector<std::string> GetDumbNodesAll(bool fullPath);
  std::vector<std::string> GetSlavesAll(bool fullPath);
  std::vector<std::pair<std::string, std::vector<std::string>>> GetPhysicalMap();
  std::vector<std::pair<std::string, std::vector<std::string>>> GetLogicalTop();
  void SetReversePhysicalMap(std::pair <std::string, std::string> map, bool master);
  void SetPhysicalMap(std::pair <std::string, std::string>  map, bool master);
  void SetShardInfo(std::pair <std::string, std::string> shard_info);
  void SetShardTop(std::pair <std::string, std::string> shard_info);
  void RegisterHeartBeat(std::pair <std::string, std::string> map, bool master);
  void SetHeartBeat(std::pair <std::string, std::string> map, std::string counter);
  void SetChain(std::pair<std::string, std::vector<std::string>> top);
  void SetLogicalTop_old(std::vector<std::pair<std::string, std::vector<std::string>>> map);
  void SetPhysicalMap_old(std::vector<std::pair<std::string, std::vector<std::string>>> map);
//  void AddProxy(std::string proxy, DumbNodeTable_t dumb_map);
//  void UpdateTopology(std::string proxy, DumbNodeTable_t dumb_map);
//  void CreateTopology(std::string proxy, DumbNodeTable_t dumb_map);
  std::vector<std::string> GetAllMastersFromMap();
  std::vector<std::string> GetAllSlavesFromMap();

private:
  zhandle_t* zoo_handle_ = nullptr;
  ZooWatcher* global_watcher_ = nullptr;
  void WatchHandler(int type, int state, const char* path);
  static void GlobalWatchFunc(zhandle_t*, int type, int state,
                              const char* path, void* ctx);
};

}
#endif

