#include "Parser.h"
#include "Keywords.h"


using douban::mc::io::BufferReader;
using douban::mc::io::TokenData;
using douban::mc::io::parseTokenData;
using douban::mc::io::freeTokenData;
using douban::mc::types::RetrievalResult;
using douban::mc::types::LineResult;

namespace douban {
namespace mc {

PacketParser::PacketParser(BufferReader* reader)
  : m_buffer_reader(NULL), m_state(FSM_START), m_mode(MODE_UNDEFINED),
    m_expectedResultCount(0), mt_kvPtr(NULL) {
  m_buffer_reader = reader;
}

PacketParser::PacketParser()
  : m_buffer_reader(NULL), m_state(FSM_START), m_mode(MODE_UNDEFINED),
    m_expectedResultCount(0), mt_kvPtr(NULL) {
}


PacketParser::~PacketParser() {
}


void PacketParser::setMode(ParserMode md) {
  m_mode = md;
}


void PacketParser::processMessageResult(enum message_result_type tp) {
  m_messageResults.push_back(message_result_t());

  message_result_t* inner_rst = &m_messageResults.back();
#ifdef USE_PROTOBUF
	inner_rst->type_ = tp;
#else
	if (m_requestKeys.empty())	return;
  struct ::iovec iov = m_requestKeys.front();
  m_requestKeys.pop();
  inner_rst->type_ = tp;
  inner_rst->key = static_cast<char*>(iov.iov_base);
  inner_rst->key_len = iov.iov_len;
#endif
}


void PacketParser::processLineResult(err_code_t& err) {
  err = RET_OK;
  LineResult* inner = &(m_lineResults.back());
  inner->line.clear();
  inner->line_len = m_buffer_reader->readUntil(err, '\n', inner->line);
  if (err != RET_OK) {
    return;
  }
  m_buffer_reader->skipBytes(err, 1);
}


void PacketParser::setBufferReader(BufferReader* reader) {
  m_buffer_reader = reader;
}

#ifdef USE_PROTOBUF
void PacketParser::setBufferReader(String *reader) {
  input = reader;
}
#endif


void PacketParser::addRequestKey(const char* const key, const size_t len) {
  // log_info("add request key: %.*s", static_cast<int>(len), key);
  struct ::iovec iov = {const_cast<char*>(key), len};
  m_requestKeys.push(iov);
}

std::queue<struct iovec>* PacketParser::getRequestKeys() {
  return &m_requestKeys;
}

size_t PacketParser::requestKeyCount() {
  return m_requestKeys.size();
}

void PacketParser::process_packets(err_code_t& err) {
  // NOTE: always return with err RET_INCOMPLETE_BUFFER_ERR if not all packets are recved.
  //
  // m_n_active--;
  // m_buffer_reader->print();
  err = RET_OK;

  if (IS_END_STATE(m_state)) {
    m_state = FSM_START;
  }

#define SKIP_BYTES(N) \
  do { \
    m_buffer_reader->skipBytes(err, (N)); \
    if (err != RET_OK) { \
      return; \
    } \
  } while(0)

#define READ_UNSIGNED(N) \
  do { \
    m_buffer_reader->readUnsigned(err, (N)); \
    if (err != RET_OK) { \
      return; \
    } \
  } while (0)

  while (!canEndParse()) {
    switch (m_state) {
      case FSM_START:
        {
          this->start_state(err);
          if (err != RET_OK) {
            return;
          }
        }
        break;
      case FSM_GET_START: // got "VALUE "
        {
          mt_kvPtr = &m_retrievalResults.back();
          mt_kvPtr->key.clear();
          mt_kvPtr->key_len = m_buffer_reader->readUntil(err, ' ', mt_kvPtr->key);
          if (err != RET_OK) {
            return;
          }
          SKIP_BYTES(1);  // " "
          m_state = FSM_GET_KEY;
        }
        break;
      case FSM_GET_KEY: // got "key "
        {
          assert(mt_kvPtr != NULL and mt_kvPtr->key.size() > 0);
          uint64_t flags;
          READ_UNSIGNED(flags);
          mt_kvPtr->flags = static_cast<flags_t>(flags);
          SKIP_BYTES(1);
          m_state = FSM_GET_FLAG;
        }
        break;
      case FSM_GET_FLAG: // got "flag "
        {
          assert(mt_kvPtr != NULL && mt_kvPtr->bytes >= 0 && mt_kvPtr->bytes + 1 >= mt_kvPtr->bytesRemain);
          uint64_t bytes;
          READ_UNSIGNED(bytes);
          mt_kvPtr->bytes = static_cast<uint32_t>(bytes);
          mt_kvPtr->bytesRemain = mt_kvPtr->bytes + 1; // 1 for '\n
          SKIP_BYTES(1);  // " " or "\r"
          m_state = FSM_GET_BYTES_CAS;
        }
        break;
      case FSM_GET_BYTES_CAS: // got "bytes\r" or "bytes cas\r"
        {
          assert(mt_kvPtr != NULL);
          const char c = m_buffer_reader->peek(err, 0);
          if (err != RET_OK) {
            return;
          }
          if (c == '\n') {  // get
            mt_kvPtr->cas_unique = 0;
          } else {  // gets
            READ_UNSIGNED(mt_kvPtr->cas_unique);
            SKIP_BYTES(1); // '\r' after cas
          }
          m_state = FSM_GET_VALUE_REMAINING;
        }
        break;
      case FSM_GET_VALUE_REMAINING: // not got "\n" + all bytes + "\r\n"
        {
          assert(mt_kvPtr != NULL);
          if (mt_kvPtr->bytesRemain == mt_kvPtr->bytes + 1) {
            SKIP_BYTES(1);
            --mt_kvPtr->bytesRemain;
          }

          if (mt_kvPtr->bytesRemain == mt_kvPtr->bytes) {
            mt_kvPtr->data_block.clear();
            if (m_buffer_reader->readLeft() < mt_kvPtr->bytes + 2) {
              m_buffer_reader->setNextPreferedDataBlockSize(mt_kvPtr->bytes + 2 - m_buffer_reader->readLeft());
            }
            m_buffer_reader->readBytes(err, mt_kvPtr->bytes, mt_kvPtr->data_block);
            if (err != RET_OK) {
              return;
            }
            mt_kvPtr->bytesRemain = 0;
          }

          if (mt_kvPtr->bytesRemain == 0) {
            SKIP_BYTES(2); // "\r\n
#ifndef NDEBUG
            char* _k = parseTokenData(mt_kvPtr->key, mt_kvPtr->key_len);
            // debug("got %.*s", static_cast<int>(mt_kvPtr->key_len), _k);
            if (mt_kvPtr->key.size() > 1) {
              delete[] _k;
            }
#endif
            mt_kvPtr = NULL;
            m_state = FSM_START;
          }
        }
        break;
      case FSM_INCR_DECR_START:
        {
          unsigned_result_t* inner_rst = &(m_unsignedResults.back());
          READ_UNSIGNED(inner_rst->value);
          SKIP_BYTES(1);

          struct ::iovec iov = m_requestKeys.front();
          inner_rst->key = static_cast<char*>(iov.iov_base);
          inner_rst->key_len = iov.iov_len;

          m_state = FSM_INCR_DECR_REMAINING;
        }
        break;
      case FSM_INCR_DECR_REMAINING:
        {
          m_buffer_reader->skipUntil(err, '\n');
          if (err != RET_OK) {
            return;
          }
          SKIP_BYTES(1);  // '\n'
          m_requestKeys.pop();
          m_state = FSM_START;
        }
        break;

      case FSM_VER_START:
        {
          processLineResult(err);
          if (err != RET_OK) {
            return;
          }
          m_state = FSM_END;
        }
        break;
      case FSM_STAT_START:
        {
          processLineResult(err);
          if (err != RET_OK) {
            return;
          }
          m_state = FSM_START;
        }
        break;
      default:
        break;
    }
  }
}

#ifdef USE_PROTOBUF

void PacketParser::parse_resp(err_code_t &err) {
	if (input->empty()) {
		err = RET_INCOMPLETE_BUFFER_ERR;
		return;
	}

	int parsed = 0;
	char *ptr = input->data();

	uint32_t *magic;
	magic = (uint32_t *)ptr;
	assert(*magic == MAGIC);
	ptr += sizeof(uint32_t);
	parsed += sizeof(uint32_t);

	size_t *len = (size_t *)ptr;
	size_t totalSize = *len;
	assert(totalSize < 1073741826);
	ptr += sizeof(size_t);
	parsed += sizeof(size_t);

	char *head = input->data();
	size_t typeLen = *((size_t *)ptr);
	ptr += sizeof(size_t);
	assert((size_t)(ptr-head) < totalSize);
	parsed += sizeof(size_t);

	std::string op(ptr, typeLen);
	ptr += typeLen;
	parsed += typeLen;

	size_t dataLen = *((size_t *)ptr);
	ptr += sizeof(size_t);
	assert((size_t)(ptr-head) < totalSize);
	assert((size_t)(ptr+dataLen-head) <= totalSize);
	parsed += sizeof(size_t);
	std::string reply(ptr, dataLen);
	ptr += dataLen;
	parsed += dataLen;
	assert(parsed == (int)totalSize);
	input->discard(parsed);

	static protoSpec::GetMessage get;
	static protoSpec::PutMessage put;
	static protoSpec::DelMessage del;
	protoSpec::Reply response;
	//fprintf(stderr, "op type=%s\n", op.c_str());
	if (get.GetTypeName() == op) { // yue: handles get response
		response.ParseFromString(reply);
		if (response.status() == 0) {
			m_retrievalResults.push_back(types::RetrievalResult());
			mt_kvPtr = &m_retrievalResults.back();
			mt_kvPtr->key.clear();
			mt_kvPtr->value = response.value();
			err = RET_OK;
			//fprintf(stderr, "GET response status=OK\n");
			return;
		} else {
			/* yue: FIXME a proper handler logic required here */
			err = RET_OK;
			return;
		}
	} else if (put.GetTypeName() == op) { // yue: handles put response
		response.ParseFromString(reply);
		int resp_status = response.status();
		if (resp_status == 0) {
			//fprintf(stderr, "PUT response status=STORED\n");
			processMessageResult(MSG_STORED);
			err = RET_OK;
			return;
		} else if (resp_status == -3) {
			//fprintf(stderr, "PUT response status=NOT_STORED\n");
			processMessageResult(MSG_NOT_STORED);
			err = RET_OK;
			return;
		}
	} else if (del.GetTypeName() == op) { // yue: handles del response
		
	}
}

#endif // ifdef USE_PROTOBUF


#ifdef REDIS

void PacketParser::parse_resp_redis(err_code_t &err) {
	err = RET_OK;

#ifndef NDEBUG
#define _EXPECT_BYTES(S, N) \
  do { \
    m_buffer_reader->expectBytes(err, (S), (N)); \
    if (err != RET_OK) { \
      return; \
    } \
  } while (0)
#else
#define _EXPECT_BYTES(S, N) \
  do { \
    m_buffer_reader->skipBytes(err, (N)); \
    if (err != RET_OK) { \
      return; \
    } \
  } while (0)
#endif

#define SKIP_BYTES(N) \
  do { \
    m_buffer_reader->skipBytes(err, (N)); \
    if (err != RET_OK) { \
      return; \
    } \
  } while(0)

#define READ_UNSIGNED(N) \
  do { \
    m_buffer_reader->readUnsigned(err, (N)); \
    if (err != RET_OK) { \
      return; \
    } \
  } while (0)

  while (!canEndParse()) {
		const char status = m_buffer_reader->peek(err, 0);
		/*
		 * yue: so far only the following three parsers supported
		 */
		switch(status) {
			case REDIS_RESP_STR:
				{
					SKIP_BYTES(1); // '+'
					_EXPECT_BYTES("OK\r\n", 4);
					processMessageResult(MSG_OK);
				}
				break;
			//case REDIS_RESP_ERR:
			case REDIS_RESP_BULK:
				{
					const char c = m_buffer_reader->peek(err, 1);
					if (c == '-') {
						uint64_t nil;
						SKIP_BYTES(2); // "$-"
						READ_UNSIGNED(nil);
						assert(nil == 1);
						//if (nil == 1) {
						processMessageResult(MSG_NOT_FOUND);
						m_state = FSM_END;
						break;
						//}
					}
					SKIP_BYTES(1); // '$'
					uint64_t bytes;
					READ_UNSIGNED(bytes);
					//fprintf(stderr, "bulk_str: bytes=%zu\n", bytes);
					if (bytes == 0) {
						processMessageResult(MSG_NOT_FOUND);
						break;
					}
					{
						m_retrievalResults.push_back(types::RetrievalResult());
						mt_kvPtr = &m_retrievalResults.back();
						assert(mt_kvPtr != NULL);
						mt_kvPtr->key.clear();
						mt_kvPtr->bytes = static_cast<uint32_t>(bytes);
						mt_kvPtr->data_block.clear();
						//if (m_buffer_reader->readLeft() < mt_kvPtr->bytes + 2) {
						if (m_buffer_reader->readLeft() < mt_kvPtr->bytes) {
							m_buffer_reader->setNextPreferedDataBlockSize(mt_kvPtr->bytes - m_buffer_reader->readLeft());
						}
						m_buffer_reader->readBytes(err, mt_kvPtr->bytes, mt_kvPtr->data_block);
						if (err != RET_OK) {
							return;
						}
            mt_kvPtr->bytesRemain = 0;
						mt_kvPtr = NULL;
					}
					m_state = FSM_END;
				}
				break;
			default:
				// yue: programming error will trigger disconnect and a later reconnect
				err = RET_PROGRAMMING_ERR;
				return;
		}
	}
}

#endif // ifdef REDIS


int PacketParser::start_state(err_code_t& err) {

  err = RET_OK;
  const char c1 = m_buffer_reader->peek(err, 0);
  if (err != RET_OK) {
    return 0;
  }
  // log_info("start_state with %c", c1);

#ifndef NDEBUG
#define EXPECT_BYTES(S, N) \
  do { \
    m_buffer_reader->expectBytes(err, (S), (N)); \
    if (err != RET_OK) { \
      return 0; \
    } \
  } while (0)
#else
#define EXPECT_BYTES(S, N) \
  do { \
    m_buffer_reader->skipBytes(err, (N)); \
    if (err != RET_OK) { \
      return 0; \
    } \
  } while (0)
#endif

  switch (c1) {
    case 'V':
      {
        const char c2 = m_buffer_reader->peek(err, 1);
        if (err != RET_OK) {
          return 0;
        }
        if (c2 == 'A') {
          // VALUE
          EXPECT_BYTES("VALUE ", 6);
          m_retrievalResults.push_back(types::RetrievalResult());
          m_state = FSM_GET_START;
        } else if (c2 == 'E') {
          // VERSION
          EXPECT_BYTES("VERSION ", 8);
          m_lineResults.push_back(types::LineResult());
          m_state = FSM_VER_START;
        }
      }
      break;
    case 'E':
      {
        const char c2 = m_buffer_reader->peek(err, 1);
        if (err != RET_OK) {
          return 0;
        }
        if (c2 == 'R') {
          // ERROR
          // client side(programmer) should be blame for this error
          TokenData err_td;
          size_t n = m_buffer_reader->readUntil(err, '\n', err_td);
          if (err != RET_OK) {
            return 0;
          }
          m_buffer_reader->skipBytes(err, 1); // '\n'
          assert(err == RET_OK);
          char* ptr = parseTokenData(err_td, n);
          log_err("error: [%.*s]", static_cast<int>(n - 1), ptr); // -1 to ignore '\r'
          freeTokenData(err_td);
          err = RET_PROGRAMMING_ERR;
          m_state = FSM_ERROR;
        } else if (c2 == 'N') {
          // END
          EXPECT_BYTES("END\r\n", 5);
          m_state = FSM_END;
        } else if (c2 == 'X') {
          // EXISTS
          EXPECT_BYTES("EXISTS\r\n", 8);
          processMessageResult(MSG_EXISTS);
        }
      }
      break;
    case 'O':
      {
        // OK
        EXPECT_BYTES("OK\r\n", 4);
        processMessageResult(MSG_OK);
      }
      break;
    case 'S':
      {
        const char c3 = m_buffer_reader->peek(err, 2);
        if (err != RET_OK) {
          return 0;
        }
        const char c2 = m_buffer_reader->peek(err, 1);
        if (err != RET_OK) {
          return 0;
        }
        if (c2 == 'T') {
          if (c3 == 'O') {
            // STORED
            EXPECT_BYTES("STORED\r\n", 8);
            processMessageResult(MSG_STORED);
          } else {
            // STAT
            EXPECT_BYTES("STAT ", 5);
            m_lineResults.push_back(types::LineResult());
            m_state = FSM_STAT_START;
          }
        } else {
          // SERVER_ERROR
          TokenData err_td;
          size_t n = m_buffer_reader->readUntil(err, '\n', err_td);
          if (err != RET_OK) {
            return 0;
          }
          m_buffer_reader->skipBytes(err, 1); // '\n'
          assert(err == RET_OK);
          char* ptr = parseTokenData(err_td, n);
          log_err("server_error: [%.*s]", static_cast<int>(n - 1), ptr); // -1 to ignore '/r'
          freeTokenData(err_td);
          err = RET_MC_SERVER_ERR;
          m_state = FSM_ERROR;
        }
      }
      break;
    case 'D':
      {
        // DELETED
        EXPECT_BYTES("DELETED\r\n", 9);
        processMessageResult(MSG_DELETED);
      }
      break;
    case 'N':
      {
        const char c5 = m_buffer_reader->peek(err, 4);
        if (err != RET_OK) {
          return 0;
        }

        if (c5 == 'F') {
          // NOT_FOUND
          EXPECT_BYTES("NOT_FOUND\r\n", 11);
          processMessageResult(MSG_NOT_FOUND);
        } else if (c5 == 'S') {
          // NOT_STORED
          EXPECT_BYTES("NOT_STORED\r\n", 12);
          processMessageResult(MSG_NOT_STORED);
        }
      }
      break;
    case 'T':
      {
        // TOUCHED
        EXPECT_BYTES("TOUCHED\r\n", 9);
        processMessageResult(MSG_TOUCHED);
      }
      break;
    case 'C':
      {
        // CLIENT_ERROR
        // client side(programmer) should be blame for this error
        TokenData err_td;
        size_t n = m_buffer_reader->readUntil(err, '\n', err_td);
        if (err != RET_OK) {
          return 0;
        }
        m_buffer_reader->skipBytes(err, 1); // '\n'
        assert(err == RET_OK);
        char* ptr = parseTokenData(err_td, n);
        log_err("client_error: [%.*s]", static_cast<int>(n - 1), ptr); // -1 to ignore '/r'
        freeTokenData(err_td);
        err = RET_PROGRAMMING_ERR;
        m_state = FSM_ERROR;
      }
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      {
        m_unsignedResults.push_back(unsigned_result_t());
        m_state = FSM_INCR_DECR_START;
      }
      break;
    default:
      err = RET_PROGRAMMING_ERR;
      log_err("programming error: unexpected leading char '%c' found", c1);
      break;
  }
  return 0;

#undef EXPECT_BYTES
}


void PacketParser::reset() {
  while (!m_requestKeys.empty()) {
    m_requestKeys.pop();
  }

  m_retrievalResults.clear();
  m_messageResults.clear();
  m_lineResults.clear();
  m_unsignedResults.clear();

  m_state = FSM_START;
  m_mode = MODE_UNDEFINED;
  m_expectedResultCount = 0;
}


types::RetrievalResultList* PacketParser::getRetrievalResults() {
  return &m_retrievalResults;
}


types::MessageResultList* PacketParser::getMessageResults() {
  return &m_messageResults;
}


types::LineResultList* PacketParser::getLineResults() {
  return &m_lineResults;
}


types::UnsignedResultList* PacketParser::getUnsignedResults() {
  return &m_unsignedResults;
}

} // namespace mc
} // namespace douban
