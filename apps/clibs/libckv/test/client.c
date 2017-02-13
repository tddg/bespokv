#include <stdio.h>
#include <stdlib.h>

#include <libmemcached/memcached.h>

//#ifdef USE_PROTOBUF
#ifdef REDIS
#include "../include/Client.h"
#include "../include/Result.h"
//#include "ckv_proto.pb.h"

using douban::mc::Client;
#endif

int main(int argc, char **argv) {
//#ifdef USE_PROTOBUF
#ifdef REDIS
	Client *client = new Client();
	client->config(CFG_HASH_FUNCTION, OPT_HASH_MD5);
	const char *hosts[] = {"localhost"};
	const uint32_t ports[] = {7777};
	client->init(hosts, ports, 1);

	const char *keys[] = {"foo:user250"};
	size_t key_lens[] = {11};
	const char *vals[] = {"xxxxxxyyyyyyzzzzzz"};
	size_t val_lens[] = {18};
	retrieval_result_t **r_results = NULL;
	size_t n_results = 0;
	//client->get(keys, key_lens, 1, &r_results, &n_results);
	//fprintf(stderr, "n_results=%lu\n", n_results);

	flags_t flags[] = {0};
	//message_result_t **m_results = NULL;
	//for (int i = 0; i < 10000; i++) {
		message_result_t **m_results = NULL;
	err_code_t rc =	client->set(keys, key_lens, flags, 0, NULL, 0, vals, val_lens, 1, &m_results, &n_results);
	if (rc != ERR_OK) {
		fprintf(stderr, "failed to put\n");
	}
	//	client->destroyMessageResult();
	//}
	//	message_result_t **m_results2 = NULL;
	//	client->set(keys, key_lens, flags, 0, NULL, 0, vals, val_lens, 1, &m_results2, &n_results);
	//	client->destroyMessageResult();
#else
	char config_string[1024];
	sprintf(config_string, "--SERVER=%s", "localhost:11211");
	memcached_st *memc = memcached(config_string, strlen(config_string));
	const char *key = "foo:user250";
	const char *val = "xxxxxxx";
	memcached_set(memc, key, 11, val, 8, (time_t)0, (uint32_t)0);
#endif

	return 0;
}

