/*
 * co_kafka.cpp
 *
 *  Created on: Aug 1, 2016
 *      Author: ali
 */

#include "co_kafka.h"

#define _OTYPE_TAB      0x1  /* tabular format */
#define _OTYPE_SUMMARY  0x2  /* summary format */
#define _OTYPE_FORCE    0x4  /* force output regardless of interval timing */

#ifdef USE_C_RDKAFKA
static long int msgs_wait_cnt = 0;
static rd_ts_t t_end;
static long int msgs_wait_cnt = 0;
static rd_ts_t t_end;
static rd_kafka_t *global_rk;
char errstr[512];
static int dispintvl = 1000;
static int dr_disp_div;
static int msgcnt = 1000000;

//TDOD: Fix error to string conversion and make it dynamic
int _error_ = 3;
#endif


namespace conkafka {

#ifdef USE_C_RDKAFKA
static void err_cb_c (rd_kafka_t *rk, int err, const char *reason, void *opaque) {
	printf("%% ERROR CALLBACK: %s: %s: %s\n",
	       rd_kafka_name(rk), _error_, reason);
}

static void throttle_cb (rd_kafka_t *rk, const char *broker_name,
			 int32_t broker_id, int throttle_time_ms,
			 void *opaque) {
	printf("%% THROTTLED %dms by %s (%"PRId32")\n", throttle_time_ms,
	       broker_name, broker_id);
}

static void msg_delivered (rd_kafka_t *rk,
                           const rd_kafka_message_t *rkmessage, void *opaque) {
	static rd_ts_t last;
	rd_ts_t now = rd_clock();
	static int msgs;

	msgs++;

	msgs_wait_cnt--;

	if (rkmessage->err)
                cnt.msgs_dr_err++;
        else {
                cnt.msgs_dr_ok++;
                cnt.bytes_dr_ok += rkmessage->len;
        }


	if ((rkmessage->err &&
	     (cnt.msgs_dr_err < 50 ||
              !(cnt.msgs_dr_err % (dispintvl / 1000)))) ||
	    !last || msgs_wait_cnt < 5 ||
	    !(msgs_wait_cnt % dr_disp_div) ||
	    (int)(now - last) >= dispintvl * 1000) {
		if (rkmessage->err)
			printf("%% Message delivery failed: %s (%li remain)\n",
			       rd_kafka_err2str(rkmessage->err),
			       msgs_wait_cnt);
		else
			printf("%% Message delivered (offset %"PRId64"): "
                               "%li remain\n",
                               rkmessage->offset, msgs_wait_cnt);
		printf(" --> \"%.*s\"\n",
                               (int)rkmessage->len,
                               (const char *)rkmessage->payload);
		last = now;
	}

	cnt.last_offset = rkmessage->offset;

	if (msgs_wait_cnt == 0) {
		printf("All messages delivered!\n");
		t_end = rd_clock();
	}
}

COnKafka_Producer::COnKafka_Producer(std::string brokers, std::string topic) {

	const char *compression = "no";
	int64_t start_offset = 0;
	int batch_size = 0;
	int idle = 0;
	const char *stats_cmd = NULL;
	char *stats_intvlstr = NULL;
	char tmp[128];
	char *tmp2;
	int otype = _OTYPE_SUMMARY;
	double dtmp;
	int rate_sleep = 0;

	/* Kafka topic configuration */
	topic_conf_ = rd_kafka_topic_conf_new();

	/* Kafka configuration */
	conf_ = rd_kafka_conf_new();

	/* Quick termination */
	snprintf(tmp, sizeof(tmp), "%i", SIGIO);
	rd_kafka_conf_set(conf_, "internal.termination.signal", tmp, NULL, 0);

	/* Producer config */
	rd_kafka_conf_set(conf_, "queue.buffering.max.messages", "500000",
			  NULL, 0);
	rd_kafka_conf_set(conf_, "message.send.max.retries", "3", NULL, 0);
	rd_kafka_conf_set(conf_, "retry.backoff.ms", "500", NULL, 0);

	/* Consumer config */
	/* Tell rdkafka to (try to) maintain 1M messages
	 * in its internal receive buffers. This is to avoid
	 * application -> rdkafka -> broker  per-message ping-pong
	 * latency.
	 * The larger the local queue, the higher the performance.
	 * Try other values with: ... -X queued.min.messages=1000
	 */
	rd_kafka_conf_set(conf_, "queued.min.messages", "1000000", NULL, 0);

	/* Kafka topic configuration */
	topic_conf_ = rd_kafka_topic_conf_new();

	/* Create Kafka handle */
	if (!(rk_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf_, errstr, sizeof(errstr)))) {
		fprintf(stderr, "%% Failed to create Kafka producer: %s\n",	errstr);
		exit(1);
	}


	/* Add broker(s) */
	if (rd_kafka_brokers_add(rk_, brokers.c_str()) < 1) {
		fprintf(stderr, "%% No valid brokers specified\n");
		exit(1);
	}

	/* Explicitly create topic to avoid per-msg lookups. */
	rkt_ = rd_kafka_topic_new(rk_, topic.c_str(), topic_conf_);

}
#endif

/* Set delivery report callback */
COnDeliveryReportCb ex_dr_cb_;


COnKafka_Producer::COnKafka_Producer(std::string brokers, std::string topic) {

  // error string
  std::string err_str;

  /*
   * Create configuration objects
   */
  RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

  conf->set("queue.buffering.max.messages", "2000000", err_str);
  conf->set("message.send.max.retries", "3", err_str);
  conf->set("retry.backoff.ms", "500", err_str);
  conf->set("queued.min.messages", "100000", err_str);
  conf->set("batch.num.messages",  "1000", err_str);

  /*
   * Set configuration properties
   */
  conf->set("metadata.broker.list", brokers, err_str);

  COnEventCb ex_event_cb;
  conf->set("event_cb", &ex_event_cb, err_str);


  /* Set delivery report callback */
  RdKafka::Conf::ConfResult res = conf->set("dr_cb", &ex_dr_cb_, err_str);

  /*
   * Create producer using accumulated global configuration.
   */
  producer_ = RdKafka::Producer::create(conf, err_str);
  if (!producer_) {
    fprintf(stderr, "Failed to create producer");
    exit(1);
  }

  // Create a topic in constructor as we will only have one topic
  topic_ = create_topic_handle(topic, err_str);

  message_delivered = 0;
}

// TODO: Cleanup properly by deleting producer and topic
COnKafka_Producer::~COnKafka_Producer() {

  /*
   * Wait for RdKafka to decommission.
   * This is not strictly needed (when check outq_len() above), but
   * allows RdKafka to clean up all its resources before the application
   * exits so that memory profilers such as valgrind wont complain about
   * memory leaks.
   */
  RdKafka::wait_destroyed(5000);
}

RdKafka::Topic *COnKafka_Producer::create_topic_handle(std::string topic_str, std::string errstr) {

  std::string err_str;
  RdKafka::Conf *tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
  /*
   * Create topic handle.
   */
  RdKafka::Topic *topic = RdKafka::Topic::create(producer_, topic_str,
						   tconf, err_str);
  if (!topic) {
    std::cerr << "Failed to create topic: " << err_str << std::endl;
    exit(1);
  }
  return topic;
}

RdKafka::ErrorCode COnKafka_Producer::produce(char *data, int size, std::string part) {

  //int32_t partition = std::atoi(part.c_str());
  int32_t partition = RdKafka::Topic::PARTITION_UA;

  /*
   * Produce message
   */
  RdKafka::ErrorCode resp = producer_->produce(topic_, partition,
  		RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
  		data, size, NULL, NULL);

  //fprintf(stderr, "Produce successful: data: %s size: %d Queue size: %d\n", data, size, producer_->outq_len());

  if (resp != RdKafka::ERR_NO_ERROR) {
	  fprintf(stderr, "Produce failed: %s. Out Queue Length: %d\n", RdKafka::err2str(resp).c_str(), producer_->outq_len());
	  exit(1);
  }

  message_delivered++;

  if (message_delivered > 1000) {
	  producer_->poll(0);
	  message_delivered = 0;
  }

  return resp;
}


COnKafka_Consumer::COnKafka_Consumer(std::string brokers, std::string topic) {

  // error string
  std::string errstr;

  /*
   * Create configuration objects
   */
  RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

  /*
   * Set configuration properties
   */
  conf->set("metadata.broker.list", brokers, errstr);

  COnEventCb ex_event_cb;
  conf->set("event_cb", &ex_event_cb, errstr);

  /* Set delivery report callback */
  COnDeliveryReportCb ex_dr_cb;

  /* Set delivery report callback */
  conf->set("dr_cb", &ex_dr_cb, errstr);

  /*
   * Create consumer using accumulated global configuration.
   */
  consumer_ = RdKafka::Consumer::create(conf, errstr);
  if (!consumer_) {
    fprintf(stderr, "Failed to create consumer");
    exit(1);
  }

  // Create a topic in constructor as we will only have one topic
  topic_ = create_topic_handle(topic, errstr);
}

// TODO: Cleanup properly by deleting consumer and topic
COnKafka_Consumer::~COnKafka_Consumer() {

  /*
   * Wait for RdKafka to decommission.
   * This is not strictly needed (when check outq_len() above), but
   * allows RdKafka to clean up all its resources before the application
   * exits so that memory profilers such as valgrind wont complain about
   * memory leaks.
   */
  RdKafka::wait_destroyed(5000);
}

RdKafka::Topic *COnKafka_Consumer::create_topic_handle(std::string topic_str, std::string errstr) {

  std::string err_str;
  RdKafka::Conf *tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
  /*
   * Create topic handle.
   */
  RdKafka::Topic *topic = RdKafka::Topic::create(consumer_, topic_str,
						   tconf, err_str);
  if (!topic) {
    std::cerr << "Failed to create topic: " << err_str << std::endl;
    exit(1);
  }
  return topic;
}


void COnKafka_Consumer::set_offset(std::string part, int64_t start_offset) {
	// TODO: use following three properly
	int use_cb_set = 0;
	int32_t partition = std::atoi(part.c_str());

	/*
	* Start consumer for topic+partition at start offset
	*/
	RdKafka::ErrorCode resp = consumer_->start(topic_, partition, start_offset);
	if (resp != RdKafka::ERR_NO_ERROR) {
	  fprintf(stderr, "Failed to start consumer");
	  exit(1);
	}
}


RdKafka::Message *COnKafka_Consumer::consume(std::string part, bool use_ccb) {

  // TODO: use following three properly
  int use_cb_set = 0;
  int32_t partition = std::atoi(part.c_str());
  //int64_t start_offset = RdKafka::Topic::OFFSET_BEGINNING;

  ConConsumeCb ex_consume_cb;
  RdKafka::Message *msg;

  //FIXME: use consume_callback to improve perfromance

//  if (use_cb_set) {
//	msg  = consumer_->consume_callback(topic_, partition, 1000,
//                               &ex_consume_cb, &use_cb_set);
//  } else {
    msg = consumer_->consume(topic_, partition, 1000);

    consumer_->poll(0);
    msg_consume(msg, NULL);
//  }

  return msg;
}


void COnKafka_Consumer::msg_consume(RdKafka::Message* message, void* opaque) {
  switch (message->err()) {
    case RdKafka::ERR__TIMED_OUT:
      break;

    case RdKafka::ERR_NO_ERROR:
      /* Real message */
      //std::cout << "Read msg at offset " << message->offset() << std::endl;
      if (message->key()) {
        std::cout << "Key: " << *message->key() << std::endl;
      }
      break;

    case RdKafka::ERR__PARTITION_EOF:
    	fprintf(stderr,"---\nConsumed all messages, hurrah!\n Message offset at: %d\n---\n",  message->offset());
      break;

    case RdKafka::ERR__UNKNOWN_TOPIC:
    case RdKafka::ERR__UNKNOWN_PARTITION:
      std::cerr << "Consume failed: " << message->errstr() << std::endl;
      break;

    default:
      /* Errors */
      std::cerr << "Consume failed: " << message->errstr() << std::endl;
  }
}

void ConConsumeCb::msg_consume(RdKafka::Message* message, void* opaque) {
	  switch (message->err()) {
	    case RdKafka::ERR__TIMED_OUT:
	      break;

	    case RdKafka::ERR_NO_ERROR:
	      /* Real message */
	      std::cout << "Read msg at offset " << message->offset() << std::endl;
	      if (message->key()) {
	        std::cout << "Key: " << *message->key() << std::endl;
	      }
	      printf("%.*s\n",
	        static_cast<int>(message->len()),
	        static_cast<const char *>(message->payload()));
	      break;

	    case RdKafka::ERR__PARTITION_EOF:
	      /* Last message */
	      break;

	    case RdKafka::ERR__UNKNOWN_TOPIC:
	    case RdKafka::ERR__UNKNOWN_PARTITION:
	      std::cerr << "Consume failed: " << message->errstr() << std::endl;
	      break;

	    default:
	      /* Errors */
	      std::cerr << "Consume failed: " << message->errstr() << std::endl;
	  }
}

}
