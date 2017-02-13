#pragma once

#include <folly/String.h>
#include <folly/Conv.h>
#include <folly/json.h>

#include "server.h"
#include "net.h"

typedef enum {
	PARSE_OK=0,
	PARSE_ERR_INVALID=-1
} parse_err_t;

folly::dynamic load_config_file(const std::string &filename);

DumbNodeTable_t parse_dumbnodes_c_str(std::string &filename);
DumbNodeTable_t parse_dumbnodes_std_str(const std::string &filename);
