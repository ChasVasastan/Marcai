#include <map>
#include <string>
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT _H

#define BUF_SIZE 2048

#include "pico/stdlib.h"

#include "lwip/altcp.h"
#include "lwip/pbuf.h"

class Http_client
{

private:
  char myBuff[BUF_SIZE];
  char header[BUF_SIZE / 2];
  bool ip_resolved = false;
  ip_addr_t resolved_ip;

  unsigned int http_status;
  std::string http_version;
  std::map<std::string, std::string> reply_headers;
  bool reply_headers_complete;

public:
  // Setters
  void set_my_buff(const char *buffer, size_t length);
  void set_header(const char *newHeader);
  void set_ip_resolved(bool status);

  // Getters
  char *get_my_buff();
  char *get_header();
  bool is_ip_resolved();

  // Functions
  Http_client();
  void http_get(char *website, char *api_endpoint);
  static err_t recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err);
  static err_t altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err);
  static void dns_resolve_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
  void resolve_dns(const char *hostname);
  void tls_tcp_setup(const char *website);

  bool received_status() {
    return http_status > 0;
  }

  bool received_headers() {
    return reply_headers_complete;
  }

  void set_received_headers() {
    reply_headers_complete = true;
  }

  void set_status(int status) {
    http_status = status;
  }

  void set_version(std::string version) {
    http_version = version;
  }

  std::map<std::string, std::string>& get_reply_headers() {
    return reply_headers;
  }

  void add_header(std::string key, std::string value) {
    // make header key lowercase
    for (char &c : key)
      c = std::tolower(c);
    reply_headers[key] = value;
  }
};

#endif /* HTTP_CLIENT_H */
