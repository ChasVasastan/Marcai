// -*- c++ -*-
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <functional>
#include <map>
#include <vector>
#include <string>
#include <cstdint>

#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"

namespace http {

typedef std::map<std::string, std::string> headers_t;
typedef std::vector<uint8_t> body_t;
typedef std::string url;

enum class state {
  STATUS,
  HEADERS,
  BODY,
  DONE,
  FAILED,
};

class client;

class request {
public:
  friend class http::client;
  request();

  http::client *client;
  std::string hostname;
  std::string method = "GET";
  std::string path = "/";
  static constexpr char version[] = "HTTP/1.1";
  http::headers_t headers;
  http::body_t body;
  void *arg;

  std::function<size_t(http::request *req, std::vector<uint8_t> part)> callback_body;
  std::function<void(std::string name, std::string value)> callback_header;

  int status;
  headers_t response_headers;

private:
  http::state state;
  int content_length;
  int body_rx;
  pbuf *buffer;
  bool ip_resolved = false;
  ip_addr_t resolved_ip;
  int chunk_size;

  void add_header(std::string header);
  size_t transfer_chunked(int offset);
  size_t transfer_body(int offset, size_t size);
};

} // namespace http

#endif /* HTTP_REQUEST_H */
