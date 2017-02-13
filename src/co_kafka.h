/*
 * co_kafka.h
 *
 *  Created on: Aug 1, 2016
 *      Author: ali
 */

#ifndef SRC_CO_KAFKA_H_
#define SRC_CO_KAFKA_H_

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cstring>

#ifdef _MSC_VER
#include "../win32/wingetopt.h"
#elif _AIX
#include <unistd.h>
#else
#include <getopt.h>
#endif

//#define USE_C_RDKAFKA

/*
 * Typically include path in a real application would be
 * #include <librdkafka/rdkafkacpp.h>
 */
#include "kafka/src-cpp/rdkafkacpp.h"

#ifdef USE_C_RDKAFKA
#include "kafka/src/rdkafka.h"
#include "kafka/src/rd.h"
#include "kafka/src/rdtime.h"
#endif

namespace conkafka {

#ifdef USE_C_RDKAFKA
static struct {
	rd_ts_t  t_start;
	rd_ts_t  t_end;
	rd_ts_t  t_end_send;
	uint64_t msgs;
	uint64_t msgs_last;
        uint64_t msgs_dr_ok;
        uint64_t msgs_dr_err;
        uint64_t bytes_dr_ok;
	uint64_t bytes;
	uint64_t bytes_last;
	uint64_t tx;
	uint64_t tx_err;
        uint64_t avg_rtt;
        uint64_t offset;
	rd_ts_t  t_fetch_latency;
	rd_ts_t  t_last;
        rd_ts_t  t_enobufs_last;
	rd_ts_t  t_total;
        rd_ts_t  latency_last;
        rd_ts_t  latency_lo;
        rd_ts_t  latency_hi;
        rd_ts_t  latency_sum;
        int      latency_cnt;
        int64_t  last_offset;
} cnt = {};
#endif


class COnKafka_Producer {
public:
	int message_delivered;
	COnKafka_Producer(std::string broker, std::string topic);

#ifdef USE_C_RDKAFKA
	COnKafka_Producer(std::string brokers, std::string topic);
#endif

	~COnKafka_Producer();

#ifdef USE_C_RDKAFKA
	RdKafka::ErrorCode produce(std::string message);
	static void err_cb (rd_kafka_t *rk, int err, const char *reason, void *opaque);
#endif

	RdKafka::Topic *create_topic_handle(std::string topic_str, std::string errstr);
	RdKafka::ErrorCode produce(char *data, int size, std::string part);

private:
	RdKafka::Producer *producer_ = nullptr;
	RdKafka::Topic *topic_ = nullptr;


#ifdef USE_C_RDKAFKA
	// C based implementation
	rd_kafka_t *rk_ = nullptr;
	rd_kafka_topic_t *rkt_ = nullptr;
	rd_kafka_conf_t *conf_ = nullptr;
	rd_kafka_topic_conf_t *topic_conf_ = nullptr;
	rd_kafka_queue_t *rkqu_ = NULL;
#endif

};

//int COnKafka_Producer::message_delivered = 0;

class COnKafka_Consumer {
public:
	COnKafka_Consumer(std::string broker, std::string topic);
	~COnKafka_Consumer();

	RdKafka::Topic *create_topic_handle(std::string topic_str, std::string errstr);
	void set_offset(std::string part, int64_t start_offset);
	RdKafka::Message *consume(std::string part, bool use_ccb);
	void msg_consume(RdKafka::Message* message, void* opaque);

private:
	RdKafka::Consumer *consumer_ = nullptr;
	RdKafka::Topic *topic_ = nullptr;
};

class COnDeliveryReportCb : public RdKafka::DeliveryReportCb {
 public:
  void dr_cb (RdKafka::Message &message) {
  }
};

class COnEventCb : public RdKafka::EventCb {
 public:
  void event_cb (RdKafka::Event &event) {
    switch (event.type())
    {
      case RdKafka::Event::EVENT_ERROR:
        std::cerr << "ERROR (" << RdKafka::err2str(event.err()) << "): " <<
            event.str() << std::endl;
        if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN)
        break;

      case RdKafka::Event::EVENT_STATS:
        std::cerr << "\"STATS\": " << event.str() << std::endl;
        break;

      case RdKafka::Event::EVENT_LOG:
        fprintf(stderr, "LOG-%i-%s: %s\n",
                event.severity(), event.fac().c_str(), event.str().c_str());
        break;

      default:
        std::cerr << "EVENT " << event.type() <<
            " (" << RdKafka::err2str(event.err()) << "): " <<
            event.str() << std::endl;
        break;
    }
  }
};

/* Use of this partitioner is pretty pointless since no key is provided
 * in the produce() call. */
class MyHashPartitionerCb : public RdKafka::PartitionerCb {
 public:
  int32_t partitioner_cb (const RdKafka::Topic *topic, const std::string *key,
                          int32_t partition_cnt, void *msg_opaque) {
    return djb_hash(key->c_str(), key->size()) % partition_cnt;
  }
 private:

  static inline unsigned int djb_hash (const char *str, size_t len) {
    unsigned int hash = 5381;
    for (size_t i = 0 ; i < len ; i++)
      hash = ((hash << 5) + hash) + str[i];
    return hash;
  }
};

class ConConsumeCb : public RdKafka::ConsumeCb {
 public:
  void consume_cb (RdKafka::Message &msg, void *opaque) {
    msg_consume(&msg, opaque);
  }

  void msg_consume(RdKafka::Message* message, void* opaque);
};

}

#endif /* SRC_CO_KAFKA_H_ */
