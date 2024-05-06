#include <map>
#include <string>
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT _H

#define BUF_SIZE 4096

#include "pico/stdlib.h"

#include "lwip/altcp.h"
#include "lwip/pbuf.h"

class Http_client
{

private:
  int buffer_available;
  char myBuff[BUF_SIZE];
  char header[BUF_SIZE / 2];
  bool ip_resolved = false;
  ip_addr_t resolved_ip;

  unsigned int http_status;
  std::string http_version;
  unsigned int http_content_length;;
  std::map<std::string, std::string> http_response_headers;
  bool response_headers_complete;
  int http_body_rx;

public:
  // Functions
  Http_client();
  void http_get(char* hostname, char* path);

private:
  static err_t recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err);
  static err_t altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err);
  static void dns_resolve_callback(const char* hostname, const ip_addr_t *ipaddr, void *arg);
  void resolve_dns(const char *hostname);
  void tls_tcp_setup(const char *website);

  bool received_status() {
    return http_status > 0;
  }

  bool received_headers() {
    return response_headers_complete;
  }

  void add_header(std::string header);
};

#endif /* HTTP_CLIENT_H */
