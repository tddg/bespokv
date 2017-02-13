#include <iostream>
#include <fstream>
#include <string>

#include "server.h"
#include "proxy_util.h"

folly::dynamic load_config_file(const std::string &config_file) {
	std::ifstream cfile(config_file);
	if (!cfile) {
		throw("cannot read config file '" + config_file + "'");
	}

	std::string json;
	std::string line;
	while (std::getline(cfile, line)) {
		size_t comment_pos = line.find_first_of('#');
		if (comment_pos != std::string::npos) {
			line.resize(comment_pos);
		}
		json += line;
		json += '\n'; // yue: hence the parser can report line numbers on err
	}
	cfile.close();

	folly::json::serialization_opts opts;
	opts.allow_trailing_comma = true;
	
	return folly::parseJson(json, opts);
}

static parse_err_t parse_each_dumbnode(char *line, std::pair<AddrType, DumbNode> &node) {
	char *pch, *saveptr;
	const char *delim = ":";
	
	if (!line)	return PARSE_ERR_INVALID;
	// yue: ip address
	pch = strtok_r(line, delim, &saveptr);
	if (!pch)	return PARSE_ERR_INVALID;
	node.second.addr = std::string(pch);
	// yue: port number
	pch = strtok_r(NULL, delim, &saveptr);
	if (!pch)	return PARSE_ERR_INVALID;
	node.second.port = atoi(pch);
#ifdef USE_KETAMA
	// yue: weight
	pch = strtok_r(NULL, delim, &saveptr);
	if (!pch)	return PARSE_ERR_INVALID;
	node.second.weight = atoi(pch);
#endif
	// yue: master or slave
	pch = strtok_r(NULL, delim, &saveptr);
	if (!pch)	return PARSE_ERR_INVALID;
	node.second.replica_role = (replica_role_t)atoi(pch);

	LOG(INFO) << "dumbNode.addr=" << node.second.addr << ", dumbNode.port=" << 
		node.second.port << ", dumbNode.weight=" << node.second.weight;
	node.first = node.second.addr + ":" + std::to_string(node.second.port);
	return PARSE_OK;
}

DumbNodeTable_t parse_dumbnodes_std_str(const std::string &filename) {
	std::ifstream fs(filename);
	if (!fs) {
		fprintf(stderr, "failed to read %s\n", filename.c_str());
		exit(EXIT_FAILURE);
	}

	std::string line;
	int32_t line_num = 0;
	DumbNodeTable_t dumb_list;
	while (std::getline(fs, line)) {
		size_t comment_pos = line.find_first_of('#');
		if (comment_pos == 0) {
			continue;
		}
		if (comment_pos != std::string::npos) {
			line.resize(comment_pos);
		} 

		line_num++;
		std::pair<AddrType, DumbNode> node;
		parse_err_t status = parse_each_dumbnode((char *)line.c_str(), node);
		if (status != PARSE_OK) {
			fprintf(stderr, "failed to parse line %d in %s\n", line_num, filename.c_str());
			continue;
		}
		dumb_list.insert(node);
	}
	LOG(INFO) << "dumb_list.size=" << dumb_list.size();

	fs.close();
	return dumb_list;
}

DumbNodeTable_t parse_dumbnodes_c_str(std::string &filename) {
	FILE *fp = NULL;
	fp = fopen(filename.c_str(), "rb");
	if (!fp) {
		fprintf(stderr, "failed to read %s\n", filename.c_str());
		exit(EXIT_FAILURE);
	}

	const int line_size = 0x500;
	char line[line_size], *r = NULL;
	int32_t line_num = 0;
	DumbNodeTable_t dumb_list;
	while ((r = fgets(line, line_size, fp))) {
		line_num++;
		std::pair<AddrType, DumbNode> node;
		parse_err_t status = parse_each_dumbnode(line, node);
		if (status != PARSE_OK) {
			fprintf(stderr, "failed to parse line %d in %s\n", line_num, filename.c_str());
			continue;
		}
		dumb_list.insert(node);
	}
	LOG(INFO) << "dumb_list.size=" << dumb_list.size();

	fclose(fp);
	return dumb_list;
}

