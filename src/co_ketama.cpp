#include "server.h"
#include "co_ketama.h"
#include "net.h"
#include "link.h"
#include "md5.h"

struct _continuumCompare {
	bool operator() (const std::shared_ptr<ContinuumItem_t> &lhs, 
			const std::shared_ptr<ContinuumItem_t> &rhs) const 
	{
		return lhs->point < rhs->point;
	}
} continuumCompare;

#ifdef DEBUG
static void dump_continuum(std::vector<std::shared_ptr<ContinuumItem_t> > array) {
	LOG(INFO) << "Dump continuum array content:";
	uint32_t count = 0;
	std::vector<std::shared_ptr<ContinuumItem_t> >::iterator it;
	for (it = array.begin(); it != array.end(); it++) {
		LOG(INFO) << "[" << count++ << "]: item.point=" << (*it)->point << ", item.addr=" << (*it)->conn->dumb_addr;
	}
}
#endif

Continuum::Continuum(DumbConnTable_t &dumb_map, int32_t total_weight) {
	DumbConnTable_t::iterator it;
	int32_t num_dumbnodes = dumb_map.size();
	CHECK(num_dumbnodes > 0);
	for (it = dumb_map.begin(); it != dumb_map.end(); it++) {
		double pct = (double)it->second->weight / (double)total_weight;
		uint32_t ks = (uint32_t)floor(pct * 40.0 * (double)num_dumbnodes);

		for (uint32_t k = 0; k < ks; k++) {
			char ss[30];
			unsigned char digest[16];

			sprintf(ss, "%s-%d", it->first.c_str(), k);
			ketama_md5_digest(ss, digest);

			for (int h = 0; h < 4; h++) {
				array.emplace_back(std::make_shared<ContinuumItem_t>());
				array.back()->point = (uint32_t)(digest[3+h*4]<<24) 
					| (digest[2+h*4]<<16) 
					| (digest[1+h*4]<<8)
					| (digest[h*4]) ;
				array.back()->conn = it->second;
			}
		}
	}

	std::sort(array.begin(), array.end(), continuumCompare);
	LOG(INFO) << "Continuum array size=" << array.size();
	//dump_continuum(array);
}

void Continuum::ketama_md5_digest(const char *str, unsigned char md5pwd[16]) {
	md5_state_t md5state;

	md5_init(&md5state);
	md5_append(&md5state, (unsigned char *)str, strlen(str));
	md5_finish(&md5state, md5pwd);
}

uint32_t Continuum::ketama_hashi(const char *str) {
	unsigned char digest[16];

	ketama_md5_digest(str, digest);
	return (uint32_t)((digest[3]<<24)
			| (digest[2]<<16)
			| (digest[1]<<8)
			| (digest[0]));
}

Link *Continuum::ketama_get_conn(const std::string &key) {
	uint32_t h = ketama_hashi(key.c_str());
	int32_t high = this->array.size(), low = 0, mid;
	int32_t sz = high;
	uint32_t midv, midv1;

	std::shared_ptr<ContinuumItem_t> item;
	while (true) {
		mid = (int32_t)((low+high) / 2);
		//if (mid == sz)	return this->array[0]->conn;
		if (mid == sz) {
			item = this->array[0];
			break;
		}

		midv = this->array[mid]->point;
		midv1 = mid==0 ? 0 : this->array[mid-1]->point;

		//if (h<=midv && h>midv1)	return this->array[mid]->conn;
		if (h<=midv && h>midv1)	{
			item = this->array[mid];
			break;
		}

		if (midv < h)	low = mid + 1;
		else	high = mid - 1;

		//if (low > high)	return this->array[0]->conn;
		if (low > high)	{
			item = this->array[0];
			break;
		}
	}

	CHECK(item != NULL);
	return item->conn;
}
