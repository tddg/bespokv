#pragma once

//#define BOOST_NO_CXX11_SCOPED_ENUMS
//#include <boost/filesystem.hpp>
//#undef BOOST_NO_CXX11_SCOPED_ENUMS

#include <glog/logging.h>
#include <gflags/gflags.h>
#include <folly/String.h>
#include <folly/Conv.h>
#include <folly/json.h>
//#include <boost/crc.hpp>
//#include <boost/lexical_cast.hpp>

#include <unordered_map>
#include <deque>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <queue>
#include <string>
#include <memory>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <netdb.h> 

#include "util/bytes.h"

#define USE_SMART_PTR

// yue: status error code
#define CO_OK      0
#define CO_ERR    -1
#define CO_EAGAIN -2
#define CO_ENOMEM -3
#define CO_C_DROP -4
#define CO_PARSE_END -5

//#define RECLAIM_CONN

class Bytes;

typedef int rstatus_t;
typedef std::string AddrType;
struct AddrHash {
	std::string operator()(const AddrType &k) const 
	{
		return k;
	}
};
struct AddrEqual {
	bool operator()(const AddrType &lhs, const AddrType &rhs) const 
	{
		return lhs == rhs;
	}
};

typedef std::vector<Bytes> Request;

extern int ev_counter;
extern int in_ev;

static inline double millitime() {
	struct timeval now;
	gettimeofday(&now, NULL);
	double ret = now.tv_sec + now.tv_usec/1000.0/1000.0;

	return ret;
}

static inline int64_t time_ms() {
	struct timeval now;
	gettimeofday(&now, NULL);

	return now.tv_sec*1000 + now.tv_usec/1000;
}

