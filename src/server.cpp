#include "server.h"
#include "net.h"
#include "co_zk.h"

// yue: initialize gflags command line parameters
DEFINE_string(config, "conf.json", "json format config file");
DEFINE_string(proxyAddr, "127.0.0.1", "proxy's address");
DEFINE_int32(proxyClientPort, 12345, "proxy's listen port on client event");
DEFINE_int32(proxyPeerPort, 12350, "proxy's listen port on peer event");
DEFINE_string(storeAddr, "127.0.0.1", "dumb node's address");
DEFINE_int32(storePort, 12346, "dumb node's port");
DEFINE_bool(doBatch, true, "whether to do batch event processing");
DEFINE_bool(doKetama, false, "whether to do ketama conn mapping");
DEFINE_string(datalets, "dl.cfg", "datalet hosts config file");
DEFINE_bool(h, false, "whether to print help info");
DEFINE_int32(backConn, 250, "maximum number of backend connections with dumb node");
DEFINE_string(shard, "ClusterOn", "shard name for each M/S or M/M combo");
DEFINE_bool(recovery, false, "starting in recover more?");

void print_usage(char **argv) {
	printf("Usage: %s \n", argv[0]);
	printf("  --config: config file\n");
	printf("  --proxyAddr: proxy's address\n");
	printf("  --proxyClientPort: proxy's listen port on client events\n");
	printf("  --proxyPeerPort: proxy's listen port on peer proxy events\n");
	printf("  --doBatch: whether to do batch event processing\n");
	printf("  --doKetama: whether to do ketama conn mapping\n");
	printf("  --backConn: number of backend connections with dumb node\n");
	printf("  --h: print usage info\n");
	printf("  --datalets: datalet hosts config file\n");
	printf("  --shard: shard name - different number for each M/S or M/M combo");
	printf("  --recovery: starting in recover more - for fault tolernace");
	printf("\n");
}

std::vector<std::string> proxies;
std::vector<std::string> dumbnodes;

// yue: FIXME
void co_stop() {
}

/*
 * yue: single event trigger loop
 */
void co_run(NetworkServer *proxy) {
	rstatus_t status;

	for ( ;; ) {
		status = proxy->core_loop();
		if (status != CO_OK) {
			break;
		}
	}

	co_stop();
}

int main(int argc, char **argv) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	google::InitGoogleLogging(argv[0]);

	if (FLAGS_h) {
		print_usage(argv);
		return 0;
	}

	NetworkServer *proxy = new NetworkServer(FLAGS_proxyAddr, FLAGS_proxyClientPort,
			FLAGS_proxyPeerPort, FLAGS_config, FLAGS_datalets, FLAGS_shard, FLAGS_recovery);

	/*
	 * -- Zookeeper will contain following trees:
	 * 1) physical_map (proxy -> (datalet, role) [k,v])
	 * 2) reverse_physical_map (datalet -> proxy)
	 * 3) shard_info (proxy, shard#) [key, value]
	 * 4) shard_top (shard# -> proxies)
	 * 5) heart_beat (proxy, exhale/inhale [key, value] value is changed after each minute)
	 * 6) chain (Master -> slave-1 -> slave-2 -> slave-n)
	 * 7) hybrid_chain (Proxy/Master -> Proxy/slave-1 -> Proxy/slave-n -> Proxy/Master)
	 *
	 * Note: Cleanup is the responsibility of coordinator each time it is launched.
	 */
	proxy->zkc = new zookeeper::ZooKeeper(proxy->zk_host);

	if (proxy->topology == ms ) {
		// 1) populate physical_map (proxy -> (datalet, role) [k,v])
		// 2) populate reverse_physical_map (datalet -> proxy)
		// 5) heart_beat (proxy, exhale/inhale [key, value] value is changed after each minute)

		// 3) populate shard_info (proxy, shard#) [key, value]
		// 4) populate shard_top (shard# -> proxies)
		std::pair <std::string, std::string> shard_info;

		// 6) chain (Master -> slave-1 -> slave-2 -> slave-n)
		// populate m-s topology
		std::pair <std::string, std::vector<std::string>> chain;

		DumbNodeTable_t::iterator it;
		for (it = proxy->dumb_map.begin(); it != proxy->dumb_map.end(); it++) {
			if (it->second.replica_role == master_t) {

				proxy->physical_map.first = FLAGS_proxyAddr.c_str();
				proxy->physical_map.first.append(":");
				proxy->physical_map.first.append(to_string(FLAGS_proxyClientPort));

				proxy->physical_map.second = it->second.addr.c_str();
				proxy->physical_map.second.append(":");
				proxy->physical_map.second.append(std::to_string(it->second.port));

				shard_info.first = FLAGS_proxyAddr.c_str();
				shard_info.first.append(":");
				shard_info.first.append(std::to_string(FLAGS_proxyClientPort));
				shard_info.second = proxy->kafka_topic;

				chain.first =  it->second.addr.c_str();
				chain.first.append(":");
				chain.first.append(std::to_string(it->second.port));
			}
			else {
				std::string slave = it->second.addr.c_str();
				slave.append(":");
				slave.append(std::to_string(it->second.port));
				chain.second.push_back(slave);
			}
		}

		proxy->zkc->SetPhysicalMap(proxy->physical_map, true); // 1)..
		proxy->zkc->SetReversePhysicalMap(proxy->physical_map, true); // 2)..
		proxy->zkc->SetShardInfo(shard_info); // 3)..
		proxy->zkc->SetShardTop(shard_info); // 4)..
		proxy->zkc->RegisterHeartBeat(proxy->physical_map, true); // 5)..
		proxy->zkc->SetChain(chain); // 6)..

	} else if (proxy->topology == slave) {

		// 1) populate physical_map (proxy -> (datalet, role) [k,v])
		// 2) populate reverse_physical_map (datalet -> proxy)
		// 5) heart_beat (proxy, exhale/inhale [key, value] value is changed after each minute)

		// 3) populate shard_info (proxy, shard#) [key, value]
		// 4) populate shard_top (shard# -> proxies)
		std::pair <std::string, std::string> shard_info;

		int count = 0; // check that we only have one slave in cfg file
		DumbNodeTable_t::iterator it;
		for (it = proxy->dumb_map.begin(); it != proxy->dumb_map.end(); it++) {
			if (it->second.replica_role == slave_t) {

				proxy->physical_map.first = FLAGS_proxyAddr.c_str();
				proxy->physical_map.first.append(":");
				proxy->physical_map.first.append(to_string(FLAGS_proxyClientPort));

				proxy->physical_map.second = it->second.addr.c_str();
				proxy->physical_map.second.append(":");
				proxy->physical_map.second.append(std::to_string(it->second.port));

				shard_info.first = FLAGS_proxyAddr.c_str();
				shard_info.first.append(":");
				shard_info.first.append(std::to_string(FLAGS_proxyClientPort));
				shard_info.second = proxy->kafka_topic;
			}
			count++;
		}
		CHECK(count < 2);
		proxy->zkc->SetPhysicalMap(proxy->physical_map, false); // 1)..
		proxy->zkc->SetReversePhysicalMap(proxy->physical_map, false); // 2)..
		proxy->zkc->SetShardInfo(shard_info); // 3)..
		proxy->zkc->SetShardTop(shard_info); // 4)..
		proxy->zkc->RegisterHeartBeat(proxy->physical_map, false); // 5)..

	} else if (proxy->topology == aa) {

		if (proxy->consistency_model == eventual) {
			// 3) populate shard_info (proxy, shard#) [key, value]
			// 4) populate shard_top (shard# -> proxies)
			std::pair <std::string, std::string> shard_info;
			DumbNodeTable_t::iterator it;
			for (it = proxy->dumb_map.begin(); it != proxy->dumb_map.end(); it++) {
					if (it->second.replica_role == master_t) {

						proxy->physical_map.first = FLAGS_proxyAddr.c_str();
						proxy->physical_map.first.append(":");
						proxy->physical_map.first.append(to_string(FLAGS_proxyClientPort));

						proxy->physical_map.second = it->second.addr.c_str();
						proxy->physical_map.second.append(":");
						proxy->physical_map.second.append(std::to_string(it->second.port));

						shard_info.first = FLAGS_proxyAddr.c_str();
						shard_info.first.append(":");
						shard_info.first.append(std::to_string(FLAGS_proxyClientPort));
						shard_info.second = proxy->kafka_topic;

					}
					else {
						CHECK(0);
					}

					proxy->zkc->SetPhysicalMap(proxy->physical_map, true); // 1)..
					proxy->zkc->SetReversePhysicalMap(proxy->physical_map, true); // 2)..
					proxy->zkc->SetShardInfo(shard_info); // 3)..
					proxy->zkc->SetShardTop(shard_info); // 4)..
					proxy->zkc->RegisterHeartBeat(proxy->physical_map, true); // 5)..
			}
		}
		else if (proxy->consistency_model == strong) {
			// 3) populate shard_info (proxy, shard#) [key, value]
			// 4) populate shard_top (shard# -> proxies)
			std::pair <std::string, std::string> shard_info;
			DumbNodeTable_t::iterator it;
			for (it = proxy->dumb_map.begin(); it != proxy->dumb_map.end(); it++) {
				if (it->second.replica_role == master_t) {

					proxy->physical_map.first = FLAGS_proxyAddr.c_str();
					proxy->physical_map.first.append(":");
					proxy->physical_map.first.append(to_string(FLAGS_proxyClientPort));

					proxy->physical_map.second = it->second.addr.c_str();
					proxy->physical_map.second.append(":");
					proxy->physical_map.second.append(std::to_string(it->second.port));

					shard_info.first = FLAGS_proxyAddr.c_str();
					shard_info.first.append(":");
					shard_info.first.append(std::to_string(FLAGS_proxyClientPort));
					shard_info.second = proxy->kafka_topic;
				}
			}
			proxy->zkc->SetPhysicalMap(proxy->physical_map, true); // 1)..
			proxy->zkc->SetReversePhysicalMap(proxy->physical_map, true); // 2)..
			proxy->zkc->SetShardInfo(shard_info); // 3)..
			proxy->zkc->SetShardTop(shard_info); // 4)..
			proxy->zkc->RegisterHeartBeat(proxy->physical_map, true); // 5)..

		}
	} else if (proxy->topology == no) {
		// 3) populate shard_info (proxy, shard#) [key, value]
		// 4) populate shard_top (shard# -> proxies)
		std::pair <std::string, std::string> shard_info;
		DumbNodeTable_t::iterator it;
		for (it = proxy->dumb_map.begin(); it != proxy->dumb_map.end(); it++) {
				if (it->second.replica_role == master_t) {

					proxy->physical_map.first = FLAGS_proxyAddr.c_str();
					proxy->physical_map.first.append(":");
					proxy->physical_map.first.append(to_string(FLAGS_proxyClientPort));

					proxy->physical_map.second = it->second.addr.c_str();
					proxy->physical_map.second.append(":");
					proxy->physical_map.second.append(std::to_string(it->second.port));

					shard_info.first = FLAGS_proxyAddr.c_str();
					shard_info.first.append(":");
					shard_info.first.append(std::to_string(FLAGS_proxyClientPort));
					shard_info.second = proxy->kafka_topic;

				}
				else {
					CHECK(0);
				}

				proxy->zkc->SetPhysicalMap(proxy->physical_map, true); // 1)..
				proxy->zkc->SetReversePhysicalMap(proxy->physical_map, true); // 2)..
				proxy->zkc->SetShardInfo(shard_info); // 3)..
				proxy->zkc->SetShardTop(shard_info); // 4)..
				proxy->zkc->RegisterHeartBeat(proxy->physical_map, true); // 5)..
		}
	}


	if (FLAGS_doBatch) {
		proxy->batch_loop();
	} else {
		co_run(proxy);
	}

	return 0;
}

