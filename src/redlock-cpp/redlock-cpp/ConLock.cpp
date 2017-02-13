
#include "ConLock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

ConLock::ConLock(const char *ip,const int port) {
	c = redisConnect(ip, port);
	if (c == NULL || c->err) {
	    if (c) {
	        printf("Error: %s\n", c->errstr);
	        // handle error
	    } else {
	        printf("Can't allocate redis context\n");
	    }
	}
}

ConLock::~ConLock() {
	redisFree(c);
}

bool ConLock::lock(const char *key, int ttl) {
	redisReply *reply;

//	  reply = (redisReply *)redisCommand(c, "SETNX %s 1", key);
//    if (reply && reply->integer && reply->integer == 1) {
//        freeReplyObject(reply);
//        return true;
//    }

    reply = (redisReply *)redisCommand(c, "set %s %s px %d nx",
                                       key, "1", 500);
    if (reply && reply->str && strcmp(reply->str, "OK") == 0) {
        freeReplyObject(reply);
        return true;
    }

    return false;
}

bool ConLock::unlock(const char *key) {
	redisReply *reply;
	reply = (redisReply *)redisCommand(c, "DEL %s", key);
    if (reply && reply->integer && reply->integer == 1) {
        freeReplyObject(reply);
        return true;
    }
    return false;
}

