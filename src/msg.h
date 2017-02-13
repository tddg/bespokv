#pragma once

#include "server.h"
#include "net.h"
#include "link.h"
#include "fde.h"
#include "util/bytes.h"

rstatus_t req_recv(NetworkServer *proxy, Link *conn);
rstatus_t req_send(NetworkServer *proxy, Link *conn);
rstatus_t rsp_recv(NetworkServer *proxy, Link *conn);
rstatus_t rsp_send(NetworkServer *proxy, Link *conn);

rstatus_t replica_req_send(NetworkServer *proxy, Link *conn);
rstatus_t replica_rsp_recv(NetworkServer *proxy, Link *conn);

rstatus_t admin_replica_recv(NetworkServer *proxy, Link *conn);

#ifdef USE_SMART_PTR
Link *req_forward(NetworkServer *proxy, Link *conn, std::shared_ptr<Buffer> msg);
Link *req_mapped_forward(NetworkServer *proxy, Link *conn, std::shared_ptr<Buffer> msg);

Link *rsp_forward(NetworkServer *proxy, Link *conn, std::shared_ptr<Buffer> msg);
Link *rsp_mapped_forward(NetworkServer *proxy, Link *conn, std::shared_ptr<Buffer> msg);
#else
Link *req_forward(NetworkServer *proxy, Link *conn, Buffer *msg);
Link *req_mapped_forward(NetworkServer *proxy, Link *conn, Buffer *msg);

Link *rsp_forward(NetworkServer *proxy, Link *conn, Buffer *msg);
Link *rsp_mapped_forward(NetworkServer *proxy, Link *conn, Buffer *msg);
#endif // USE_SMART_PTR

