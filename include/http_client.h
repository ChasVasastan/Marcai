#include <functional>
#include <map>
#include <vector>
#include <string>
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT _H

#define BUF_SIZE 4096

#include "pico/stdlib.h"

#include "lwip/altcp.h"
#include "lwip/pbuf.h"

class Http_client
{
public:
  typedef std::map<std::string, std::string> headers_t;
  typedef std::vector<uint8_t> body_t;

  class Http_request {
  public:
    Http_request() {
    }
    std::string hostname;
    std::string method = "GET";
    std::string path = "/";
    static constexpr char version[] = "HTTP/1.1";
    headers_t headers;
    body_t body;

    std::function<void(std::vector<uint8_t> part, int left)> callback_body;
    std::function<void(std::string name, std::string value)> callback_header;

    Http_client *client;
  };

  // Functions
  Http_client();
  void request(Http_request *req);

private:
  int buffer_available;
  char myBuff[BUF_SIZE];
  bool ip_resolved = false;
  ip_addr_t resolved_ip;

  unsigned int http_status;
  std::string http_version;
  unsigned int http_content_length;;
  headers_t http_response_headers;
  bool response_headers_complete;
  int http_body_rx;

  static err_t recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err);
  static err_t altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err);
  static void dns_resolve_callback(const char* hostname, const ip_addr_t *ipaddr, void *arg);
  void tls_tcp_setup(Http_request *req);

  bool received_status() {
    return http_status > 0;
  }

  bool received_headers() {
    return response_headers_complete;
  }

  void add_header(std::string header);
};

#endif /* HTTP_CLIENT_H */
