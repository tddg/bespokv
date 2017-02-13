#ifndef _BENCH_COMMON_H_
#define _BENCH_COMMON_H_

#include "bench_config.h"

#define WRITE_TEXT
#define BUF_SZ 128

/* type of each query */
enum query_types{
    query_put=0,
    query_get,
    query_del,
};

/* 
 * format of each query, it has a key and a type and we don't care
 * the value
 */
typedef struct __attribute__((__packed__)) {
    char hashed_key[NKEY];
    char type;
} query;

/* 
 * yue: format of each query, it has a key, key's length, and a type. We don't
 * care the value
 */
typedef struct __attribute__((__packed__)) {
	char key[BUF_SZ];
	size_t key_len;
	char type;
} str_query;

/* bench result */
typedef struct __attribute__((__packed__)) {
    double total_tput;
    double total_time;
		double avg_latency;
    size_t total_hits;
    size_t total_miss;
    size_t total_gets;
    size_t total_puts;
    size_t num_threads;
} result_t;

#endif
