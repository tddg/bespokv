#include "hiredis/hiredis.h"

using namespace std;

class ConLock {
public:
	ConLock(const char *ip, const int port);
	~ConLock();
	redisContext *c;

    bool lock(const char *key, int ttl);
    bool unlock(const char *key);
};

