#include "co_zk.h"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <mutex>
#include "zk_error.h"
#include <string.h>
#include <utility>
#include <iostream>

namespace zookeeper {

std::vector<std::vector<std::string>> logical_map(10, std::vector<std::string>(5));

#define CHECK_ZOOCODE_AND_THROW(code)  \
  if (code != ZOK) { throw ZooException(code); }

void ZooKeeper::GlobalWatchFunc(zhandle_t* h, int type, int state, const char* path, void* ctx) {
  auto self = static_cast<ZooKeeper*>(ctx);
  self->WatchHandler(type, state, path);
}

static std::once_flag ONCE_FLAG_SET_DEBUG_LEVEL;
void set_default_debug_level() {
  std::call_once(ONCE_FLAG_SET_DEBUG_LEVEL, []{
    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
  });
}

ZooKeeper::ZooKeeper(const std::string& server_hosts,
                     ZooWatcher* global_watcher,
                     int timeout_ms)
: global_watcher_(global_watcher) {
  set_default_debug_level();

  zoo_handle_ = zookeeper_init(server_hosts.c_str(),
                               GlobalWatchFunc,
                               timeout_ms,
                               nullptr, // client id
                               this,
                               0);
  if (!zoo_handle_) {
    throw ZooSystemErrorFromErrno(errno);
  }
}

ZooKeeper::~ZooKeeper() {
  if (zoo_handle_) {
    auto ret = zookeeper_close(zoo_handle_);
    if (ret != ZOK) {
      // TODO: log information
    }
  }
}

bool ZooKeeper::is_connected() {
  return zoo_state(zoo_handle_) == ZOO_CONNECTED_STATE;
}

bool ZooKeeper::is_expired() {
  return zoo_state(zoo_handle_) == ZOO_EXPIRED_SESSION_STATE;
}

void ZooKeeper::WatchHandler(int type, int state, const char* path) {
  // call global watcher
  if (!global_watcher_) return;

  if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_EXPIRED_SESSION_STATE) {
      global_watcher_->OnSessionExpired();
    } else if (state == ZOO_CONNECTED_STATE) {
      global_watcher_->OnConnected();
    } else if (state == ZOO_CONNECTING_STATE) {
      global_watcher_->OnConnecting();
    } else {
      // TODO:
      assert(0 && "don't know how to process other session event yet");
    }
  } else if (type == ZOO_CREATED_EVENT) {
    global_watcher_->OnCreated(path);
  } else if (type == ZOO_DELETED_EVENT) {
    global_watcher_->OnDeleted(path);
  } else if (type == ZOO_CHANGED_EVENT) {
    global_watcher_->OnChanged(path);
  } else if (type == ZOO_CHILD_EVENT) {
    global_watcher_->OnChildChanged(path);
  } else if (type == ZOO_NOTWATCHING_EVENT) {
    global_watcher_->OnNotWatching(path);
  } else {
    assert(false && "unknown zookeeper event type");
  }
}

bool ZooKeeper::Exists(const std::string& path, bool watch, NodeStat* stat) {
  auto zoo_code = zoo_exists(zoo_handle_, path.c_str(), watch, stat);
  if (zoo_code == ZNONODE) {
    return false;
  } else {
    CHECK_ZOOCODE_AND_THROW(zoo_code);
    return true;
  }
}

NodeStat ZooKeeper::Stat(const std::string& path) {
  NodeStat stat;
  auto zoo_code = zoo_exists(zoo_handle_, path.c_str(), false, &stat);
  CHECK_ZOOCODE_AND_THROW(zoo_code);

  return stat;
}

std::string ZooKeeper::Create(const std::string& path, const std::string& value, int flag) {
  std::string path_buffer;
  path_buffer.resize(path.size() + 64);

  auto zoo_code = zoo_create(zoo_handle_,
                             path.c_str(),
                             value.data(),
                             value.size(),
                             &ZOO_OPEN_ACL_UNSAFE,
                             flag,
                             const_cast<char*>(path_buffer.data()),
                             path_buffer.size());

  CHECK_ZOOCODE_AND_THROW(zoo_code);

  path_buffer.resize(strlen(path_buffer.data()));
  return path_buffer;
}

std::string ZooKeeper::CreateIfNotExists(const std::string& path, const std::string& value, int flag) {
  std::string path_buffer;
  path_buffer.resize(path.size() + 64);

  auto zoo_code = zoo_create(zoo_handle_,
                             path.c_str(),
                             value.data(),
                             value.size(),
                             &ZOO_OPEN_ACL_UNSAFE,
                             flag,
                             const_cast<char*>(path_buffer.data()),
                             path_buffer.size());

  if (zoo_code == ZNODEEXISTS) {
    assert(!(flag & ZOO_SEQUENCE));
    return path;
  }

  CHECK_ZOOCODE_AND_THROW(zoo_code);

  path_buffer.resize(strlen(path_buffer.data()));
  return path_buffer;
}

void ZooKeeper::Delete(const std::string& path) {
  NodeStat stat;
  if (!Exists(path.c_str(), false, &stat)) {
    throw ZooException(ZNONODE);
  }

  auto zoo_code = zoo_delete(zoo_handle_, path.c_str(), stat.version);
  CHECK_ZOOCODE_AND_THROW(zoo_code);
}

void ZooKeeper::DeleteIfExists(const std::string& path) {
  NodeStat stat;
  if (!Exists(path.c_str(), false, &stat)) {
    return;
  }

  auto zoo_code = zoo_delete(zoo_handle_, path.c_str(), stat.version);
  if (zoo_code == ZNONODE) {
    return;
  }
  CHECK_ZOOCODE_AND_THROW(zoo_code);
}

std::string ZooKeeper::Get(const std::string& path, bool watch) {
  std::string value_buffer;

  auto node_stat = Stat(path);

  value_buffer.resize(node_stat.dataLength);

  int buffer_len = value_buffer.size();
  auto zoo_code = zoo_get(zoo_handle_,
                          path.c_str(),
                          watch,
                          const_cast<char*>(value_buffer.data()),
                          &buffer_len,
                          &node_stat);

  CHECK_ZOOCODE_AND_THROW(zoo_code);

  value_buffer.resize(buffer_len);
  return value_buffer;
}

void ZooKeeper::Set(const std::string& path, const std::string& value) {
  auto node_stat = Stat(path);

  auto zoo_code = zoo_set(zoo_handle_,
                          path.c_str(),
                          value.data(),
                          value.size(),
                          node_stat.version);

  CHECK_ZOOCODE_AND_THROW(zoo_code);
}

std::vector<std::string> ZooKeeper::GetChildren(const std::string& parent_path, bool watch) {
  std::vector<std::string> children;

  struct String_vector child_vec;

  auto zoo_code = zoo_get_children(zoo_handle_, parent_path.c_str(), watch, &child_vec);
  CHECK_ZOOCODE_AND_THROW(zoo_code);

  children.reserve(child_vec.count);
  for (int i = 0; i < child_vec.count; ++i) {
    children.push_back(child_vec.data[i]);
  }

  // TODO: release child_vec
  return children;
}


std::string ZooKeeper::Lock(const std::string& path, const int rw) {

  std::vector<std::string> children;
  std::string guid = CreateIfNotExists(path);

  std::string n_path = (rw == 0) ? n_path = path + "/r" : path + "/w";
  std::string req_id = CreateIfNotExists(n_path, "", ZOO_EPHEMERAL | ZOO_SEQUENCE);
  int req_id_int = atoi(req_id.substr( req_id.length() - 8 ).c_str());

  // If its a write lock
  if(rw) {
	  write_label:
	    children = GetChildren(guid, false);

	  if (children.size() > 1){
		  for (std::vector<std::string>::const_iterator i = children.begin(); i != children.end(); i++) {
			  std::string a = *i;

			  int child_id_int = atoi(a.substr( a.length() - 10 ).c_str());

			  if (req_id_int > child_id_int) {
				  bool child_exists = true;
				  while (child_exists) {
					  child_exists = Exists(guid + "/" + a, true);
					  if (!child_exists) {
						  goto write_label;
					  }
				  }
			  }
		  }
	  }
	  return req_id;
  }
  else { // If read lock
	  read_label:
	    children = GetChildren(guid, false);

	  if (children.size() > 1){
		  for (std::vector<std::string>::const_iterator i = children.begin(); i != children.end(); i++) {
			  std::string a = *i;

			  if (a.at(0) == 'r')
				  continue;

			  int child_id_int = atoi(a.substr( a.length() - 10 ).c_str());

			  if (req_id_int > child_id_int) {
				  bool child_exists = true;
				  while (child_exists) {
					  child_exists = Exists(guid + "/" + a, true);
					  if (!child_exists) {
						  goto read_label;
					  }
				  }
			  }
		  }
	  }
	  return req_id;
  }
}

void ZooKeeper::Unlock(const std::string& path) {
	std::string key = path.substr( 0, path.length() - 12).c_str();
	std::vector<std::string> children = GetChildren(key, false);
	if (children.size() > 1)
		DeleteIfExists(path);
	else{
		DeleteIfExists(path);
		DeleteIfExists(key);
	}
}

std::vector<std::string> ZooKeeper::GetProxies() {
	std::vector<std::string> children = GetChildren("/con-map", false);
	return children;
}

std::vector<std::string> ZooKeeper::GetDumbNodesOfProxy(std::string proxy) {
	std::vector<std::string> children = GetChildren("/con-map/" + proxy, false);
	return children;
}

std::vector<std::string> ZooKeeper::GetMasters() {
	std::vector<std::string> children = GetChildren("/con-top", false);
	return children;
}

std::vector<std::string> ZooKeeper::GetSlavesOfMaster(std::string master) {
	std::vector<std::string> children = GetChildren("/con-top/" + master, false);
	return children;
}

std::vector<std::string> ZooKeeper::GetDumbNodesAll(bool fullPath) {
	std::vector<std::string> dumbnodes;
	//Get all proxies
	std::vector<std::string> proxies = GetProxies();
	//populate the dumbnodes
	for (std::vector<std::string>::const_iterator ip = proxies.begin(); ip != proxies.end(); ip++) {
		std::vector<std::string> sub_dumbnodes = GetDumbNodesOfProxy(*ip);
		for (std::vector<std::string>::const_iterator i = sub_dumbnodes.begin(); i != sub_dumbnodes.end(); i++) {
			if (!fullPath)
				dumbnodes.push_back(*i);
			else
				dumbnodes.push_back("/con-map/" + *ip + "/" + *i);
		}
	}
	return dumbnodes;
}


std::vector<std::string> ZooKeeper::GetSlavesAll(bool fullPath) {
	std::vector<std::string> slaves;
	//Get all proxies
	std::vector<std::string> masters = GetMasters();
	//populate the dumbnodes
	for (std::vector<std::string>::const_iterator ip = masters.begin(); ip != masters.end(); ip++) {
		std::vector<std::string> sub_slaves = GetDumbNodesOfProxy(*ip);
		for (std::vector<std::string>::const_iterator i = sub_slaves.begin(); i != sub_slaves.end(); i++) {

			if (!fullPath)
				slaves.push_back(*i);
			else
				slaves.push_back("/con-top/" + *ip + "/" + *i);
		}
	}
	return slaves;
}

std::vector<std::pair<std::string, std::vector<std::string>>> ZooKeeper::GetPhysicalMap() {
	//std::vector<std::vector<std::string>> map(16, std::vector<std::string>(8));
	//std::vector<std::vector<std::string>> map;

	std::vector<std::pair<std::string, std::vector<std::string>>> map;

	//Get all proxies
	std::vector<std::string> proxies = GetProxies();

	for (std::vector<std::string>::const_iterator i = proxies.begin(); i != proxies.end(); i++) {
		map.push_back(std::make_pair(*i, GetDumbNodesOfProxy(*i)));
	}

	return map;
}

std::vector<std::pair<std::string, std::vector<std::string>>> ZooKeeper::GetLogicalTop() {
	std::vector<std::pair<std::string, std::vector<std::string>>> top;
	//Get all proxies
	std::vector<std::string> masters = GetMasters();
	//populate the map
	for (std::vector<std::string>::const_iterator i = masters.begin(); i != masters.end(); i++) {
		top.push_back(std::make_pair(*i, GetSlavesOfMaster(*i)));
	}
	return top;
}

void ZooKeeper::SetPhysicalMap(std::pair <std::string, std::string> map, bool master) {
	CreateIfNotExists("/pm");
	CreateIfNotExists("/pm/" + map.first);
	CreateIfNotExists("/pm/" + map.first + "/" + map.second, master ? "master" : "slave");
	Set("/pm/" + map.first + "/" + map.second, master ? "master" : "slave");
}

void ZooKeeper::SetReversePhysicalMap(std::pair <std::string, std::string> map, bool master) {
	CreateIfNotExists("/rpm");
	CreateIfNotExists("/rpm/" + map.second);
	Set("/rpm/" + map.second, master ? "master" : "slave");
	CreateIfNotExists("/rpm/" + map.second + "/" +  map.first);
}

void ZooKeeper::SetShardInfo(std::pair <std::string, std::string> shard_info) {
	CreateIfNotExists("/si");
	CreateIfNotExists("/si/" + shard_info.first);
	CreateIfNotExists("/si/" + shard_info.first + "/" + shard_info.second);
}

void ZooKeeper::SetShardTop(std::pair <std::string, std::string> shard_info) {
	CreateIfNotExists("/st");
	CreateIfNotExists("/st/" + shard_info.second);
	CreateIfNotExists("/st/" + shard_info.second + "/" + shard_info.first);
}

void ZooKeeper::RegisterHeartBeat(std::pair <std::string, std::string> map, bool master) {
	CreateIfNotExists("/hb");
	CreateIfNotExists("/hb/" + map.first);
	Set("/hb/" + map.first, "0");
}

void ZooKeeper::SetHeartBeat(std::pair <std::string, std::string> map, std::string counter) {
	CreateIfNotExists("/hb");
	CreateIfNotExists("/hb/" + map.first);
	Set("/hb/" + map.first, counter);
}

void ZooKeeper::SetChain(std::pair<std::string, std::vector<std::string>> top) {
	CreateIfNotExists("/msc");
	std::string master = top.first;
	std::vector<std::string> slaves = top.second;
	CreateIfNotExists("/msc/" + master);
	for (std::vector<std::string>::const_iterator it = slaves.begin(); it!=slaves.end(); it++) {
		CreateIfNotExists("/msc/" + master + "/" + *it);
	}
}

void ZooKeeper::SetPhysicalMap_old(std::vector<std::pair<std::string, std::vector<std::string>>> map) {
	CreateIfNotExists("/con-map");
	for (std::vector<std::pair<std::string, std::vector<std::string>>>::const_iterator i = map.begin(); i != map.end(); i++) {
		std::string proxy = i->first;
		std::vector<std::string> dumbnodes = i->second;
		CreateIfNotExists("/con-map/" + proxy);
		for (std::vector<std::string>::const_iterator it = dumbnodes.begin(); it != dumbnodes.end(); it++) {
			CreateIfNotExists("/con-map/" + proxy + "/" + *it);
		}
	}
}

void ZooKeeper::SetLogicalTop_old(std::vector<std::pair<std::string, std::vector<std::string>>> top) {
	CreateIfNotExists("/con-top");
	for (std::vector<std::pair<std::string, std::vector<std::string>>>::const_iterator i = top.begin(); i != top.end(); i++) {
		std::string master = i->first;
		std::vector<std::string> slaves = i->second;
		CreateIfNotExists("/con-top/" + master);
		for (std::vector<std::string>::const_iterator it = slaves.begin(); it!=slaves.end(); it++) {
			CreateIfNotExists("/con-top/" + master + "/" + *it);
		}
	}
}

bool ZooKeeper::isMaster(std::string proxy, std::string dumbnode) {
	std::string master = Get("/con-map/" + proxy + "/" + dumbnode, false);
	if (master.compare("M"))
		return true;
	return false;
}

/**
void ZooKeeper::AddProxy(std::string proxy, DumbNodeTable_t dumb_map) {

	// Value over here can be weight of the proxy
	std::string path = CreateIfNotExists("/con-map/" + proxy, "1");

	// First entry in dumb list file is treated as Master at the moment
	int first_entry = 1;

	// Now add all the dumbnodes to the zk database
	DumbNodeTable_t::iterator it;
	for (it = dumb_map.begin(); it != dumb_map.end(); it++) {
		std::string dumbnode = it->second.addr + ":" + std::to_string(it->second.port);

		if(first_entry == 1)
			CreateIfNotExists("/con-map/" + proxy + "/" + dumbnode, "M");
		else
			CreateIfNotExists("/con-map/" + proxy + "/" + dumbnode, "S");

		first_entry++;
	}

	// Update topology
	//UpdateTopology(proxy, dumb_map);

}
**/

std::vector<std::string> ZooKeeper::GetAllMastersFromMap() {
	std::vector<std::string> dumbnodes = GetDumbNodesAll(true);
	std::vector<std::string> masters;

	for (std::vector<std::string>::const_iterator i = dumbnodes.begin(); i != dumbnodes.end(); i++) {
		std::string master = Get(*i, false);
		if (master.compare("M"))
			masters.push_back(*i);
	}
	return masters;
}

std::vector<std::string> ZooKeeper::GetAllSlavesFromMap() {
	std::vector<std::string> dumbnodes = GetDumbNodesAll(true);
	std::vector<std::string> slaves;

	for (std::vector<std::string>::const_iterator i = dumbnodes.begin(); i != dumbnodes.end(); i++) {
		std::string slave = Get(*i, false);
		if (!slave.compare("M"))
			slaves.push_back(*i);
	}
	return slaves;
}

/**
void ZooKeeper::CreateTopology(std::string proxy, DumbNodeTable_t dumb_map) {

	// First entry in dumb list file is treated as Master at the moment
	int first_entry = 1;

	std::vector<std::string> = GetAllMastersFromMap();

	for (std::vector<std::string>::const_iterator i = dumbnodes.begin(); i != dumbnodes.end(); i++) {
		CreateIfNotExists("/con-top/" + *i);
	}

	std::vector<std::string> = GetAllSlavesFromMap();


}

void ZooKeeper::UpdateTopology(std::string proxy, DumbNodeTable_t dumb_map) {

	for (std::vector<std::string>::const_iterator i = dumb_map.begin(); i != dumb_map.end(); i++) {

		std::string master = Get(*i, false);

		// Add master node
		if (master.compare("M"))
			CreateIfNotExists("/con-top/" + *i);
		else {
			// Assign a master to rest of the slaves
			std::vector<std::string> masters = GetAllMastersFromMap();

			// Find a master with < 3 slaves and other than the one in this dumb_map
			for (std::vector<std::string>::const_iterator im = masters.begin(); im != masters.end(); im++) {
				std::vector<std::string> children = GetChildren("con-top"  "/" + *im, false);
				if (children.size() < 3 && )
					CreateIfNotExists("/con-top/" + *im + "/" + *i);
			}
			continue;
		}
	}

	std::vector<std::string> = GetAllSlavesFromMap();


}
**/
}
