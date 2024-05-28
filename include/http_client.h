// -*- c++ -*-
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "pico/stdlib.h"

#include "lwip/altcp.h"
#include "lwip/pbuf.h"

#include "http_request.h"

namespace http {

class client
{
public:
  // Functions
  void request(http::request *req);

private:
  static err_t recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err);
  static err_t altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err);
  static void dns_resolve_callback(const char* hostname, const ip_addr_t *ipaddr, void *arg);
  static void tls_tcp_setup(http::request *req, const ip_addr_t &addr);
};

} // namespace http

#endif /* HTTP_CLIENT_H */
