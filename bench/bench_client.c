#define _GNU_SOURCE

#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sched.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>


#define SLAP_CKV    // yue: conkv datalet
//#define SLAP_MC   // yue: memcached datalet
//#define SLAP_RDS  // yue: redis datalet
//#define SLAP_LDB  // yue: leveldb datalet

#ifdef SLAP_CKV
//#include <Client.h>
#include <c_client.h>
#elif defined SLAP_MC  
#include <libmemcached/memcached.h>
#elif defined SLAP_RDS
#include <hiredis/hiredis.h>
#elif defined SLAP_LDB
#include <ldb_client.h>
#endif

#include "bench_common.h"

static char text_buf[1024] = "copied_from_https://bitbucket.org/atte/ocaml-memcached/src/74cc78d9a6f3b937ffda5a530b045c8d22ef30b4/test/data/crc32.txt?at=default";

pthread_mutex_t printmutex;
double avg_latency = .0;

typedef struct {
  size_t tid;
#ifdef WRITE_TEXT
  str_query *queries;
#else
  query *queries;
#endif
  size_t num_ops;
  size_t num_puts;
  size_t num_gets;
  size_t num_miss;
  size_t num_hits;
  double tput;
  double time;
  double avg_exectime;
	double avg_lat;
} thread_param;

/* default parameter settings */
static size_t key_len;
static size_t val_len;
static size_t num_queries;
static size_t num_threads = 1;
static size_t num_mget = 1;
static float duration = 10.0;
static char* serverip = NULL;
static char* inputfile = NULL;
static int num_hosts = 1;
static int num_replicas = 1;
static char *host_config_file = NULL;
static int get_from_which = 0;
static int set_to_which = 0;

static int slap_done = 0;

/* Calculate the second difference*/
static double timeval_diff(struct timeval *start, 
                           struct timeval *end)
{
  double r = end->tv_sec - start->tv_sec;

  /* Calculate the microsecond difference */
  if (end->tv_usec > start->tv_usec)
    r += (end->tv_usec - start->tv_usec)/1000000.0;
  else if (end->tv_usec < start->tv_usec)
    r -= (start->tv_usec - end->tv_usec)/1000000.0;
  return r;
}

/* 
 * yue: 
 * Calculate the millisecond difference
 **/
static double ms_timeval_diff(struct timeval *start, 
                           struct timeval *end)
{
  double r = (end->tv_sec - start->tv_sec) * 1000.0;

  /* Calculate the microsecond difference */
  if (end->tv_usec > start->tv_usec)
    r += (end->tv_usec - start->tv_usec)/1000.0;
  else if (end->tv_usec < start->tv_usec)
    r -= (start->tv_usec - end->tv_usec)/1000.0;
  return r;
}

#ifdef SLAP_CKV

/* yue: create a conkv instance */
static void *conkv_new() {
	const char *hosts[] = {"127.0.0.1", "127.0.0.1", "127.0.0.1", "127.0.0.1"};
	const uint32_t ports[] = {7777, 12346, 12347, 12348};
	void *ckvc = NULL;

	pthread_mutex_lock(&printmutex);
	ckvc = client_create();
	if (ckvc == NULL) {
		fprintf(stderr, "Failed to create conkv instance\n");
		exit(1);
	}
	int i;
	fprintf(stderr, "Connect to: ");
	for (i = 0; i < num_hosts; i++) {
		fprintf(stderr, "%s:%d ", hosts[i], ports[i]);
	}
	fprintf(stderr, "\n");
	client_config(ckvc, CFG_HASH_FUNCTION, OPT_HASH_MD5);
	client_init(ckvc, hosts, ports, num_hosts, NULL, 0);
  pthread_mutex_unlock(&printmutex);

  return ckvc;
}

static void parse_host_file(const char *config_file, const size_t num_hosts, const size_t num_replicas,
		char ****hosts, uint32_t ***ports) {
	*hosts = (char ***)malloc(sizeof(char **)*num_hosts);
	assert(*hosts);
	*ports = (uint32_t **)malloc(sizeof(uint32_t *)*num_hosts);
	assert(*ports);
	int i, j;
	for (i = 0; i < num_hosts; i++) {
		(*hosts)[i] = (char **)malloc(sizeof(char *)*num_replicas);
		assert((*hosts)[i]);
		(*ports)[i] = (uint32_t *)malloc(sizeof(uint32_t)*num_replicas);
		assert((*ports)[i]);
		for (j = 0; j < num_replicas; j++) {
			(*hosts)[i][j] = (char *)malloc(128);
			assert((*hosts)[i][j]);
		}
	}

	FILE *fp = fopen(config_file, "rb");
	if (!fp) {
		fprintf(stderr, "Failed to read %s\n", config_file);
		exit(1);
	}
	const size_t row_sz = 0x500;
	char row[row_sz], *r = NULL;
	const char *major_delim = " ";
	const char *sub_delim = ":";
	int row_num = 0;
	while ((r = fgets(row, row_sz, fp))) {
		char *pch, *saveptr;
		int col_num = 0;
		for (pch = strtok_r(row, major_delim, &saveptr); (pch!=NULL && col_num<num_replicas); 
				pch = strtok_r(NULL, major_delim, &saveptr)) {
			char *sub_pch, *sub_saveptr;
			sub_pch = strtok_r(pch, sub_delim, &sub_saveptr);
			assert(sub_pch != NULL);
			memcpy((*hosts)[row_num][col_num], sub_pch, strlen(sub_pch));
			((*hosts)[row_num][col_num])[strlen(sub_pch)] = '\0'; // yue: terminate the hostname str
			sub_pch = strtok_r(NULL, sub_delim, &sub_saveptr);
			assert(sub_pch != NULL);
			(*ports)[row_num][col_num] = atoi(sub_pch);
			col_num++;
		}
		row_num++;
	}
}

static void *conkv_new_replica(
		const char *host_file, const size_t num_hosts, const size_t num_replicas,
		const int get_from_which, const int set_to_which) {
	assert(get_from_which >= -1 && get_from_which < (int)num_replicas);
	char ***hosts;
	uint32_t **ports;
	parse_host_file(host_file, num_hosts, num_replicas, &hosts, &ports);

#if 0
	pthread_mutex_lock(&printmutex);
	fprintf(stderr, "Parsed hosts:\n");
	for (i = 0; i < num_hosts; i++) {
		for (j = 0; j < num_replicas; j++) {
			fprintf(stderr, "%s:%d ", (*hosts)[i][j], (*ports)[i][j]);
		}
		fprintf(stderr, "\n");
	}
	pthread_mutex_unlock(&printmutex);
#endif
	
	void *ckvc = NULL;
	pthread_mutex_lock(&printmutex);
	ckvc = client_create();
	if (ckvc == NULL) {
		fprintf(stderr, "Failed to create conkv instance\n");
		exit(1);
	}
	int i, j;
	fprintf(stderr, "Connect to:\n");
	for (i = 0; i < num_hosts; i++) {
		for (j = 0; j < num_replicas; j++)
			fprintf(stderr, "%s:%d ", hosts[i][j], ports[i][j]);
		fprintf(stderr, "\n");
	}
	client_config(ckvc, CFG_HASH_FUNCTION, OPT_HASH_MD5);
	// yue:                                           which replica to serve GET   which to serve SET
	//                                                                     |             |
	//                                                                    \|/           \|/
	client_init_replica(ckvc, hosts, ports, num_hosts, num_replicas, get_from_which, set_to_which, 0);
	//client_init_replica(ckvc, hosts, ports, num_hosts, num_replicas, num_replicas-1, 0);
	//client_init_replica(ckvc, hosts, ports, num_hosts, num_replicas, -1, 0);
  pthread_mutex_unlock(&printmutex);

  return ckvc;
}

/* yue: wrapper of set command for conkv */
static int conkv_put(void *ckvc, const char *key, const size_t _key_len, const char *val) {
  err_code_t rc;
	flags_t flags[] = {0};
	message_result_t **m_res = NULL;
	size_t n_res = 0;
  rc = client_set(ckvc, &key, (const size_t *)&_key_len, flags, 0, NULL, 0, &val, &val_len, 1, &m_res, &n_res);
  //rc = client_set(ckvc, &key, &key_len, flags, 0, NULL, 0, &val, &_val_len, 1, &m_res, &n_res);
  if (rc != RET_OK) {
		fprintf(stderr, "failed to put: %d\n", rc);
		//exit(EXIT_FAILURE);
    return 1;
  }
  return 0;
}

/* yue: wrapper of get command for conkv */
static char* conkv_get(void *ckvc, const char *key, const size_t _key_len) {
  err_code_t rc;
  char *val;
  size_t len;
  uint32_t flag;
	retrieval_result_t **r_res = NULL;
	size_t n_res = 0;
  rc = client_get(ckvc, &key, (const size_t *)&_key_len, 1, &r_res, &n_res);
  if (rc != RET_OK) {
    return NULL;
  }
	if (n_res == 0)	return NULL;
  return r_res[0]->data_block;
}

#else

/* create a memcached structure */
static memcached_st *memc_new()
{
  char config_string[1024];
  memcached_st *memc = NULL;
  unsigned long long getter;

  pthread_mutex_lock (&printmutex);
  sprintf(config_string, "--SERVER=%s --BINARY-PROTOCOL", serverip);
  printf("config_string = %s\n", config_string);
  memc = memcached(config_string, strlen(config_string));

  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NO_BLOCK);
  printf("No block: %lld\n", getter);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE);
  printf("Socket send size: %lld\n", getter);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE);
  printf("Socket recv size: %lld\n", getter);

  pthread_mutex_unlock (&printmutex);
  return memc;
}

/* wrapper of set command */
static int memc_put(memcached_st *memc, char *key, char *val) {
  memcached_return_t rc;
  rc = memcached_set(memc, key, key_len, val, val_len, (time_t) 0, (uint32_t) 0);
  if (rc != MEMCACHED_SUCCESS) {
    return 1;
  }
  return 0;
}

/* wrapper of get command */
static char* memc_get(memcached_st *memc, char *key) {
  memcached_return_t rc;
  char *val;
  size_t len;
  uint32_t flag;
  val = memcached_get(memc, key, key_len, &len, &flag, &rc);
  if (rc != MEMCACHED_SUCCESS) {
    return NULL;
  }
  return val;
}

#endif  // SLAP_CKV

#ifdef WRITE_TEXT
static void fetch_text_trace(str_query **queries, 
		const size_t per_item_sz, const size_t num_queries, FILE *fp) {
	int i, n;
	for (i = 0; i < num_queries; i++) {
		n = fscanf(fp, "%s %d %zu", &((*queries)[i].key), &((*queries)[i].type), &((*queries)[i].key_len));
		if (n != 3) {
			perror("fscanf error");
		}
	}
}
#endif

/* init all queries from the ycsb trace file before issuing them */
#ifdef WRITE_TEXT
static str_query *queries_init(char* filename)
#else
static query *queries_init(char* filename)
#endif
{
  FILE *input;

  input = fopen(filename, "rb");
  if (input == NULL) {
    perror("can not open file");
    perror(filename);
    exit(1);
  }

  int n;
#ifdef WRITE_TEXT
	n = fscanf(input, "%zu", &key_len);
  if (n != 1)
    perror("fscanf error");
	n = fscanf(input, "%zu", &val_len);
  if (n != 1)
    perror("fscanf error");
	n = fscanf(input, "%zu", &num_queries);
  if (n != 1)
    perror("fscanf error");
#else
  n = fread(&key_len, sizeof(key_len), 1, input);
  if (n != 1)
    perror("fread error");
  n = fread(&val_len, sizeof(val_len), 1, input);
  if (n != 1)
    perror("fread error");
  n = fread(&num_queries, sizeof(num_queries), 1, input);
  if (n != 1)
    perror("fread error");
#endif

  printf("trace(%s):\n", filename);
  printf("\tkey_len = %zu\n", key_len);
  printf("\tval_len = %zu\n", val_len);
  printf("\tnum_queries = %zu\n", num_queries);
  printf("\n");

#ifdef WRITE_TEXT
  str_query *queries = malloc(sizeof(str_query) * num_queries);
#else
  query *queries = malloc(sizeof(query) * num_queries);
#endif
  if (queries == NULL) {
    perror("not enough memory to init queries\n");
    exit(-1);
  }

  size_t num_read;
#ifdef WRITE_TEXT
  fetch_text_trace(&queries, sizeof(query), num_queries, input);
#else
  num_read = fread(queries, sizeof(query), num_queries, input);
  if (num_read < num_queries) {
    fprintf(stderr, "num_read: %zu\n", num_read);
    perror("can not read all queries\n");
    fclose(input);
    exit(-1);
  }
#endif

  fclose(input);
  printf("queries_init...done\n");
  return queries;
}

/* executing queries at each thread */
static void *queries_exec(void *param)
{
#ifdef SLAP_CKV
	void *ckvc;
	//ckvc = conkv_new();
	ckvc = conkv_new_replica(host_config_file, num_hosts, num_replicas, 
			get_from_which, set_to_which);
	// //FILE *random_fp = fopen("/dev/urandom", "r");
	// int random_fd = open("/dev/urandom", O_RDONLY);
	// //if (random_fp == NULL) {
	// if (random_fd == -1) {
	// 	//perror("fopen");
	// 	perror("open");
	// 	return NULL;
	// }
#else
  /* create a memcached structure */
  memcached_st *memc;
  memc = memc_new();
#endif

  struct timeval tv_s, tv_e;
  struct timeval l_tv_s, l_tv_e;

  thread_param* p = (thread_param*) param;

  pthread_mutex_lock (&printmutex);
  printf("start benching using thread%"PRIu64"\n", p->tid);
  pthread_mutex_unlock (&printmutex);

#ifdef WRITE_TEXT
	str_query *queries = p->queries;
#else
  query* queries = p->queries;
#endif
  p->time = 0.0;
  p->avg_exectime = 0.0;

  while (p->time < duration) {
    gettimeofday(&tv_s, NULL);  // start timing
    for (size_t i = 0 ; i < p->num_ops; i++) {
      enum query_types type = queries[i].type;
#ifdef WRITE_TEXT
      char *key = queries[i].key;
			size_t key_len = queries[i].key_len;
#else
      char *key = queries[i].hashed_key;
#endif

			/* yue: Ok I just want to fill it out with some garbage */
			//ssize_t sz_read = read(random_fd, buf, val_len);
			//buf[val_len] = '\0';
			//fprintf(stderr, "key=%s val=%s", key, buf);
			//fprintf(stderr, "key=%s\n", key);
#ifdef WRITE_TEXT
			char *buf = text_buf;// + (rand()%64);
#else
      char buf[val_len];
#endif

			gettimeofday(&l_tv_s, NULL);  // yue: start latency timing
      if (type == query_put) {
				//fprintf(stderr, "put\n");
				int put_ret;
#ifdef SLAP_CKV
				put_ret = conkv_put(ckvc, key, key_len, buf);
				client_destroy_message_result(ckvc);
#else
        put_ret = memc_put(memc, key, buf);
#endif
				if (!put_ret) {
					p->num_puts++;
				}
      } else if (type == query_get) {
				//fprintf(stderr, "get\n");
#ifdef SLAP_CKV
        char *val = conkv_get(ckvc, key, key_len);
				//if (val != NULL)	fprintf(stderr, "get val=%s\n", val);
				//else	fprintf(stderr, "damn get failure\n");
#else
        char *val = memc_get(memc, key);
#endif
        p->num_gets++;
        if (val == NULL) {
          // cache miss, put something (gabage) in cache
//          p->num_miss++;
//#ifdef SLAP_CKV
					client_destroy_retrieval_result(ckvc);
					conkv_put(ckvc, key, key_len, buf);
					client_destroy_message_result(ckvc);
//#else
//          memc_put(memc, key, buf);
//#endif
        } else {
#ifdef SLAP_CKV  
					client_destroy_retrieval_result(ckvc);
#else
          free(val);
#endif
          p->num_hits++;
        }
      } else {
        fprintf(stderr, "unknown query type\n");
      }
			gettimeofday(&l_tv_e, NULL);  // yue: stop latency timing
			p->avg_exectime += ms_timeval_diff(&l_tv_s, &l_tv_e);
    }
    gettimeofday(&tv_e, NULL);  // stop timing
    p->time += timeval_diff(&tv_s, &tv_e);
  }

  size_t nops = p->num_gets + p->num_puts;
  p->tput = nops / p->time;
	p->avg_lat = p->avg_exectime / (double)nops;

#ifdef PRINT_PER_THREAD_STATS
  pthread_mutex_lock(&printmutex);
  printf("thread%"PRIu64" gets %"PRIu64" items in %.2f sec \n",
         p->tid, nops, p->time);
  printf("#put = %zu, #get = %zu\n", p->num_puts, p->num_gets);
  printf("#miss = %zu, #hits = %zu\n", p->num_miss, p->num_hits);
  printf("hitratio = %.4f\n",   (float) p->num_hits / p->num_gets);
  printf("tput = %.2f\n",  p->tput);
  printf("latency = %.4f ms\n",  p->avg_lat);
  printf("\n");
  pthread_mutex_unlock(&printmutex);
#endif // PRINT_PER_THREAD_STATS

#ifdef SLAP_CKV
	client_destroy(ckvc);
	//close(random_fd);
#else
  memcached_free(memc);
#endif

  //printf("queries_exec...done\n");
  pthread_mutex_lock(&printmutex);
	slap_done++;
  pthread_mutex_unlock(&printmutex);
  pthread_exit(NULL);
}

static void *stats_loop(void *args) {
  thread_param *p = (thread_param *)args;
	int i;
  struct timeval tv_s, tv_e;
	int32_t thpt = 0, last_nops = 0, get_thpt = 0, get_gthpt = 0, put_thpt = 0;
	int32_t last_num_gets = 0, last_num_puts = 0, last_num_hits = 0;
	double *lat = (double *)malloc(sizeof(double)*num_threads);
	double avg_lat = .0;
	double elapsed = .0, time_diff = .0, last_time = .0;
	gettimeofday(&tv_s, NULL);  // start timing
	while (1) {
		if (slap_done == num_threads)	break;

		uint32_t nops = 0, num_gets = 0, num_puts = 0, num_hits = 0;
		gettimeofday(&tv_e, NULL);  // yue: stop latency timing
		elapsed = ms_timeval_diff(&tv_s, &tv_e); 
		time_diff = elapsed - last_time;
		for (i = 0; i < num_threads; i++) {
			//nops += p[i].num_gets + p[i].num_puts;
			nops += p[i].num_hits + p[i].num_puts;
			num_gets += p[i].num_gets;
			num_hits += p[i].num_hits;
			num_puts += p[i].num_puts;
			//lat[i] = time_diff / (double)(nops-last_nops);
		}
		thpt = (nops - last_nops)*1000 / time_diff;
		get_thpt = (num_gets - last_num_gets)*1000 / time_diff;
		get_gthpt = (num_hits - last_num_hits)*1000 / time_diff;
		put_thpt = (num_puts - last_num_puts)*1000 / time_diff;
		avg_lat = time_diff / (double)(nops-last_nops);
		//fprintf(stderr, "time_diff=%.2f, nops=%d, last_nops=%d\n", time_diff/1000., nops, last_nops);
		last_time = elapsed;
		last_nops = nops;
		last_num_gets = num_gets; last_num_puts = num_puts; last_num_hits = num_hits;
		fprintf(stderr, "time= %.2f (s): thpt= %d (qps), get_thpt= %d (qps), put_thpt= %d (qps), latency= %.4f (ms)\n", 
				elapsed/1000, thpt, get_gthpt, put_thpt, avg_lat);

		if (slap_done == num_threads)	break;
		sleep(5);
	}
  
	printf("benchmarking done\n");
	pthread_exit(NULL);
}

static void usage(char* binname)
{
  printf("%s [-t #] [-l trace] [-d #] [-h] [-m #] [-r #] [-f conf]\n", binname);
  printf("\t-t #: number of working threads, by default %"PRIu64"\n", num_threads);
  //printf("\t-b #: batch size for mget, by default %"PRIu64"\n", num_mget);
  printf("\t-d #: duration of the test in seconds, by default %f\n", duration);
  //printf("\t-s IP: memcached server, by default 127.0.0.1 (localhost), required\n");
  printf("\t-l trace: e.g., /path/to/ycsbtrace, required\n");
  printf("\t-f conf: hosts config file\n");
  printf("\t-m #: number of hosts\n");
  printf("\t-r #: number of replicas\n");
  printf("\t-g #: GET from which replica\n");
  printf("\t-h  : show usage\n");
}


int
main(int argc, char **argv)
{
  if (argc <= 1) {
    usage(argv[0]);
    exit(-1);
  }

  char ch;
  while ((ch = getopt(argc, argv, "b:t:d:c:l:m:r:f:R:W:h")) != -1) {
    switch (ch) {
    case 't': num_threads = atoi(optarg); break;
    //case 'b': num_mget    = atoi(optarg); break;
    case 'd': duration    = atof(optarg); break;
    //case 's': serverip    = optarg; break;
    case 'l': inputfile   = optarg; break;
    case 'm': num_hosts   = atoi(optarg); break;
    case 'r': num_replicas   = atoi(optarg); break;
		case 'f': host_config_file = optarg; break;
		case 'R': get_from_which = atoi(optarg); break;
		case 'W': set_to_which = atoi(optarg); break;
    case 'h': usage(argv[0]); exit(0); break;
    default:
      usage(argv[0]);
      exit(-1);
    }
  }

  if (host_config_file == NULL || inputfile == NULL) {
    usage(argv[0]);
    exit(-1);
  }

#ifdef WRITE_TEXT
  str_query *queries = queries_init(inputfile);
#else
  query *queries = queries_init(inputfile);
#endif

  pthread_t threads[num_threads];
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);;

  pthread_mutex_init(&printmutex, NULL);

  size_t t;
  thread_param tp[num_threads];
  for (t = 0; t < num_threads; t++) {
    tp[t].queries = queries + t * (num_queries / num_threads);
    tp[t].tid     = t;
    tp[t].num_ops = num_queries / num_threads;
    tp[t].num_puts = tp[t].num_gets = tp[t].num_miss = tp[t].num_hits = 0;
    tp[t].time = tp[t].tput = tp[t].avg_lat = tp[t].avg_exectime = 0.0;
    int rc = pthread_create(&threads[t], &attr, queries_exec, (void *) &tp[t]);
    if (rc) {
      perror("failed: pthread_create\n");
      exit(-1);
    }
  }

	// yue: create a stats thread to periodically print out stats
	pthread_t stats_printer;
	pthread_attr_t attr_;
	pthread_attr_init(&attr_);
	pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_JOINABLE);
	int rc = pthread_create(&stats_printer, &attr_, stats_loop, (void *)tp);
	if (rc) {
		perror("error, pthread_create for stats printer\n");
		exit(-1);
	}

	// yue: summarize results
  result_t result;
  result.total_time = 0.0;
  result.total_tput = 0.0;
  result.avg_latency = 0.0;
  result.total_hits = 0;
  result.total_miss = 0;
  result.total_gets = 0;
  result.total_puts = 0;
  result.num_threads = num_threads;

  for (t = 0; t < num_threads; t++) {
    void *status;
    int rc = pthread_join(threads[t], &status);
    if (rc) {
      perror("error, pthread_join\n");
      exit(-1);
    }
    result.total_time = (result.total_time > tp[t].time) ? result.total_time : tp[t].time;
		result.avg_latency += tp[t].avg_lat;
    result.total_tput += tp[t].tput;
    result.total_hits += tp[t].num_hits;
    result.total_miss += tp[t].num_miss;
    result.total_gets += tp[t].num_gets;
    result.total_puts += tp[t].num_puts;
  }

	void *status;
	rc = pthread_join(stats_printer, &status);
	if (rc) {
		perror("error, pthread_join for stats printer\n");
		exit(-1);
	}

  printf("total_time = %.2f\n", result.total_time);
  printf("total_tput = %.2f\n", result.total_tput);
  printf("avg_latency = %.4f ms\n", result.avg_latency/(double)num_threads);
  printf("total_hitratio = %.4f\n", (float) result.total_hits / result.total_gets);

  pthread_attr_destroy(&attr);
  printf("bye\n");
  return 0;
}
