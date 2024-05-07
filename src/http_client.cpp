#include "http_client.h"

#include "lwip/pbuf.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "audio.h"

#include "lwipopts.h"
#include "lwip/altcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"
#include <cstdio>
#include <sstream>
#include <string>

extern Audio g_audio;

Http_client::Http_client()
{
  response_headers_complete = false;
  http_body_rx = 0;
}

void Http_client::request(Http_request *req) {
  // Resolve DNS
  ip_addr_t addr;
  err_t err = dns_gethostbyname(req->hostname.c_str(), &addr, dns_resolve_callback, req);
  if (err == ERR_INPROGRESS) {
    printf("DNS resolution in progress...\n");
  } else if (err == ERR_OK) {
    ip_resolved = true;
    resolved_ip = addr;
    printf("Target IP: %s\n", ipaddr_ntoa(&resolved_ip));
  } else {
    printf("DNS resolve failed/error\n");
  }
}

// Function to handle data being transmitted
err_t Http_client::recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
    Http_request *req = reinterpret_cast<Http_request*>(arg);
    Http_client *client = req->client;
    uint16_t offset = 0;

    if (p == NULL)
    {
        printf("No data received or connection closed\n");
        return ERR_OK;
    }

    // TODO: Test this call and try to extract the mp3 file
    // Encode the octect format(binary) or try sending it directly to the encoder
    if (req->status < 100) {
      uint16_t end = pbuf_memfind(p, "\r\n", 2, 0);
      if (end < 0xffff) {
        std::string status_line(end, '\0');
        pbuf_copy_partial(p, status_line.data(), end, 0);
        if (auto npos = status_line.find(" "); npos != std::string::npos) {
          std::string version = status_line.substr(0, npos);
          int status = std::atoi(status_line.substr(npos).c_str());
          printf("HTTP Version: %s\n", version.c_str());
          printf("HTTP Status:  %d\n", status);
          req->status = status;
          if (version != req->version) {
            panic("Response version != HTTP/1.1");
          }
        } else {
          panic("Invalid status line\n%s", status_line.c_str());
        }
        offset = end + 2;
      } else {
        // TODO: handle status not in payload
        panic("Status outside this buffer");
      }
    }

    if (req->status >= 100 && !client->received_headers()) {
      uint16_t end = 0xffff;
      do {
        end = pbuf_memfind(p, "\r\n", 2, offset);
        if (end < 0xffff) {
          std::string header;
          if (req->buffer.size() > 0) {
            // set correct size if headers overlap pbufs
            header = std::string(req->buffer.begin(), req->buffer.end());
            header.resize(req->buffer.size() + end);
            pbuf_copy_partial(p, header.data() + req->buffer.size(), end, offset);
            offset = end + 2;

            // We consumed whole buffer so we can clear it
            req->buffer.clear();
          } else {
            header.resize(end - offset, '\0');
            pbuf_copy_partial(p, header.data(), header.size(), offset);
            offset = end + 2;
          }

          // Found HTTP header
          req->add_header(header);

          if (header.empty()) {
            // All headers received
            offset += 2;
            break;
          }
        } else {
          // Move data to beginnig of buffer if buffer does not
          // contain all headers
          req->buffer.resize(p->tot_len - offset);
          pbuf_copy_partial(p, req->buffer.data(), p->tot_len - offset, offset);
          printf("Headers bigger buffer, waiting for more... (%ld)\n", req->buffer.size());
        }
      } while (end < 0xffff);
    }

    if (req->status >= 100 && client->received_headers()) {
      int size = req->buffer.size();
      req->buffer.resize(size + p->tot_len);
      pbuf_copy_partial(p, req->buffer.data() + size, p->tot_len - offset, offset);

      // Call the callback body function
      if (req->callback_body) {
        size_t consumed = req->callback_body(req->buffer);
        req->buffer.erase(req->buffer.begin(), req->buffer.begin() + consumed);
        printf("Body= %d bytes of %d\r",
               client->http_body_rx, req->content_length);
      } else {
        req->buffer.clear();
      }
    }

    // Tells the underlying TCP mechanism that the data has been received
    altcp_recved(pcb, p->tot_len);

    // Free the pbuf from memory
    pbuf_free(p);

    if (client->http_body_rx == req->content_length) {
      printf("\nBody transferred, closing connection\n");
      return altcp_close(pcb);
    }

    return ERR_OK;
}

// Function to handle connection events
err_t Http_client::altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
  Http_request *req = reinterpret_cast<Http_request*>(arg);

  if (err == ERR_OK) {
    printf("Connection established!\n");
    std::stringstream ss;

    // Request line
    ss << req->method << " " << req->path << " " << req->version << "\r\n";

    if (req->headers.count("host") == 0)
      req->headers["host"] = req->hostname;

    // Request headers
    for (auto [name,value] : req->headers)
      ss << name << ": " << value << "\r\n";

    // End of request headers
    ss << "\r\n";

    std::string request_string = ss.str();
    printf("Request:\n%s\n%ld\n", request_string.c_str(), request_string.length());
    err = altcp_write(pcb, request_string.c_str(), request_string.length(), 0);
    err = altcp_output(pcb);
  } else {
    printf("Error on connect: %d\n", err);
  }
  return ERR_OK;
}

// Function to set up TLS
void Http_client::tls_tcp_setup(Http_request *req)
{
    // Set up TLS
    struct altcp_tls_config *tls_config = altcp_tls_create_config_client(NULL, 0);
    struct altcp_pcb *pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    altcp_recv(pcb, recv);
    altcp_arg(pcb, req);

    // Connect to host with TLS/TCP
    printf("Attempting to connect to %s with IP address %s\n",
           req->hostname.c_str(), ipaddr_ntoa(&resolved_ip));
    auto ctx = reinterpret_cast<mbedtls_ssl_context*>(pcb);
    mbedtls_ssl_set_hostname(ctx, req->hostname.c_str());
    err_t err = altcp_connect(pcb, &resolved_ip, 443, altcp_client_connected);
    if (err != ERR_OK)
    {
        printf("TLS connection failed, error: %d\n", err);
    }
}

// Callback for DNS lookup
void Http_client::dns_resolve_callback(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    Http_request *req = reinterpret_cast<Http_request*>(arg);
    if (ipaddr != NULL)
    {
        printf("DNS lookup successful: %s, IP address is %s\n", name, ipaddr_ntoa(ipaddr));
        req->client->ip_resolved = true;
        req->client->resolved_ip = *ipaddr;
        req->client->tls_tcp_setup(req);
    }
    else
    {
        printf("DNS lookup failed for %s\n", name);
    }
}

void Http_client::Http_request::add_header(std::string header) {
  if (header.empty()) {
    printf("All headers received %ld\n", response_headers.size());
    client->response_headers_complete = true;
    for (auto [k,v] : response_headers) {
      // Print processed headers
      printf("%s: %s\n", k.c_str(), v.c_str());
    }
    putchar('\n');
    return;
  }

  printf("Raw header: %s\n", header.c_str());

  if (auto npos = header.find(": "); npos != std::string::npos) {
    std::string name = header.substr(0, npos);
    std::string value = header.substr(npos + 2);
    for (char &c : name) {
      c = std::tolower(c);
    }
    response_headers[name] = value;

    if (name == "content-length") {
      content_length = std::atoi(value.c_str());
    }
  } else {
    panic("Invalid header\n%s", header.c_str());
  }
}
