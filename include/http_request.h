// -*- c++ -*-
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <functional>
#include <map>
#include <vector>
#include <string>
#include <cstdint>

#include "lwip/ip_addr.h"
#include "lwip/altcp.h"

struct altcp_pcb;
struct pbuf;

namespace http {

typedef std::map<std::string, std::string>  headers_t;
typedef std::vector<uint8_t> body_t;
typedef std::string url;

enum class state {
  STATUS,
  HEADERS,
  BODY,
  DONE,
  FAILED,
};


class request {
public:
  request();
  ~request();

  std::string hostname;
  std::string method = "GET";
  std::string path = "/";
  static constexpr char version[] = "HTTP/1.1";
  http::headers_t headers;
  http::body_t body;
  void *arg;

  std::function<size_t(http::request *req, std::vector<uint8_t> part)> callback_body;
  std::function<void(http::request *req)> callback_result;
  std::function<void(http::request *req, std::string name, std::string value)> callback_header;

  int status;
  headers_t response_headers;
  volatile http::state state;
  int content_length;

  void abort();

  // body functions
  size_t read(uint8_t *dst, size_t size);
  size_t peek(uint8_t *dst, size_t size);
  size_t skip(size_t size);

  static void send(http::request *req);

private:
  int body_rx;
  pbuf *buffer;
  int chunk_size;
  struct altcp_pcb *pcb;

  void add_header(std::string header);
  size_t transfer_chunked(int offset);
  size_t transfer_body(int offset, size_t size);

  static err_t recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err);
  static err_t altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err);
  static void dns_resolve_callback(const char* hostname, const ip_addr_t *ipaddr, void *arg);
  static void tls_tcp_setup(http::request *req, const ip_addr_t *addr);
};

} // namespace http

#endif /* HTTP_REQUEST_H */
