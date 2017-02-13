#include <cstdlib>
#include <iostream>
#include <openssl/sha.h>
#include <cstdio>
#include <cstring>
#include <cstdint>

#include "bench_common.h"

using namespace std;

static void sha1(char hash[20], const char* buf, size_t count)
{
	SHA1(reinterpret_cast<const unsigned char*>(buf), count, reinterpret_cast<unsigned char*>(hash));
}

int main(int argc, char **argv) {

	if (argc <= 1) {
		cout << "usage: ./bench_trace_gen  output_filename < input_filename" 
			<< endl;
		exit (1);
	}

	//size_t val_len = static_cast<size_t>(-1);
	//size_t val_len = atoi(argv[1]);
	size_t key_len = NKEY;
	size_t val_len = NVAL;
	size_t num_queries = 0;

	FILE *fp = fopen(argv[1], "w");

#ifdef WRITE_TEXT
	fprintf(fp, "%zu\n", key_len);
	fprintf(fp, "%zu\n", val_len);
#else
	fwrite(&key_len, sizeof(size_t), 1, fp);
	fwrite(&val_len, sizeof(size_t), 1, fp);
#endif

	const size_t tmp_size = 1048576;
	char* tmp = new char[tmp_size];

	while (fgets(tmp, tmp_size, stdin)) {
		char buf[20];
		char rawkey[1024];
#ifdef WRITE_TEXT
		str_query q;
#else
		query q;
#endif
		if (sscanf(tmp, "\"operationcount\"=\"%zu\"", &num_queries)) {
#ifdef WRITE_TEXT
			fprintf(fp, "%zu\n", num_queries);
#else
			fwrite(&num_queries, sizeof(num_queries), 1, fp);
#endif
			continue;
		} else if (sscanf(tmp, "INSERT usertable %s [ field", rawkey)) {
			q.type = query_put;
#ifdef WRITE_TEXT
			q.key_len = strlen(rawkey);
			memcpy(q.key, rawkey, q.key_len+1);
#else
			sha1(buf, rawkey, strlen(rawkey));
			memcpy(q.hashed_key, buf, key_len);
#endif
		} else if (sscanf(tmp, "UPDATE usertable %s [ field", rawkey)) { 
			q.type = query_put;
#ifdef WRITE_TEXT
			q.key_len = strlen(rawkey);
			memcpy(q.key, rawkey, q.key_len+1);
#else
			sha1(buf, rawkey, strlen(rawkey));
			memcpy(q.hashed_key, buf, key_len);
#endif
		} 
		else if (sscanf(tmp, "READ usertable %s [", rawkey)) {
			q.type = query_get;
#ifdef WRITE_TEXT
			q.key_len = strlen(rawkey);
			memcpy(q.key, rawkey, q.key_len+1);
#else
			sha1(buf, rawkey, strlen(rawkey));
			memcpy(q.hashed_key, buf, key_len);
#endif
		} else 
			continue;

#ifdef WRITE_TEXT
		fprintf(fp, "%s %d %zu\n", q.key, q.type, q.key_len);
#else
		fwrite(&q, sizeof(q), 1, fp);
#endif
	}
	delete tmp;
	fclose(fp);
}
