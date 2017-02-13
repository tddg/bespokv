/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef NET_REDIS_LINK_H_
#define NET_REDIS_LINK_H_

#include <vector>
#include <string>

#include "server.h"
#include "util/bytes.h"

struct RedisRequestDesc
{
	int strategy;
	std::string redis_cmd;
	std::string ssdb_cmd;
	int reply_type;
};

class RedisLink
{
private:
	std::string cmd;
	RedisRequestDesc *req_desc;

	std::vector<Bytes> recv_bytes;
	std::vector<std::string> recv_string;
#ifdef USE_SMART_PTR
	int parse_req(std::shared_ptr<Buffer> input);
#else
	int parse_req(Buffer *input);
#endif
	int convert_req();
	
public:
	RedisLink(){
		req_desc = NULL;
	}
	
#ifdef USE_SMART_PTR
	const std::vector<Bytes>* recv_req(std::shared_ptr<Buffer> input);
#else
	const std::vector<Bytes>* recv_req(Buffer *input);
#endif
	int send_resp(Buffer *output, const std::vector<std::string> &resp);
};

#endif

