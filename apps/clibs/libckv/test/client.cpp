#include <stdio.h>
#include <stdlib.h>
#include <string>

//#define USE_PROTOBUF
#define REDIS

//#ifndef USE_PROTOBUF
#ifndef REDIS
#include <libmemcached/memcached.h>
#else
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
	//if (r_results != NULL) {
	//	client->destroyRetrievalResult();
	//}
	//fprintf(stderr, "n_results=%lu\n", n_results);

	flags_t flags[] = {0};
	//message_result_t **m_results = NULL;
	for (int i = 0; i < 1; i++) {
		err_code_t rc;
		message_result_t **m_results = NULL;
		rc = client->set(keys, key_lens, flags, 0, NULL, 0, vals, val_lens, 1, &m_results, &n_results);
		if (rc != RET_OK)	fprintf(stderr, "failed to put\n");
		client->destroyMessageResult();
	}
	message_result_t **m_results2 = NULL;
	client->set(keys, key_lens, flags, 0, NULL, 0, vals, val_lens, 1, &m_results2, &n_results);
	client->destroyMessageResult();

	n_results = 0;
	keys[0] = "foo:user250";
	client->get(keys, key_lens, 1, &r_results, &n_results);
	fprintf(stderr, "get %lu vals: %s\n", n_results, (*r_results)->data_block);
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

