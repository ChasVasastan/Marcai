#include "http_client.h"

#include "lwip/debug.h"
#include "lwip/pbuf.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"
#include "lwip/altcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"

#include <cstdio>
#include <sstream>
#include <string>

#include "audio.h"
#include "state.h"

namespace http
{

  void http::client::request(http::request *req)
  {
    // Resolve DNS
    ip_addr_t addr;
    err_t err = dns_gethostbyname(req->hostname.c_str(), &addr, dns_resolve_callback, req);
    if (err == ERR_INPROGRESS)
    {
      printf("DNS resolution in progress...\n");
    }
    else if (err == ERR_OK)
    {
      req->ip_resolved = true;
      req->resolved_ip = addr;
      printf("Target IP: %s\n", ipaddr_ntoa(&req->resolved_ip));
      req->client->tls_tcp_setup(req);
    }
    else
    {
      printf("DNS resolve failed/error\n");
    }
    
  }

  // Function to handle data being transmitted
  err_t http::client::recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
  {
    http::request *req = reinterpret_cast<http::request *>(arg);
    uint16_t offset = 0;

    if (p == NULL)
    {
      printf("No data received or connection closed\n");
      return ERR_OK;
    }

    if (req->buffer == NULL)
      req->buffer = p;
    else
      pbuf_cat(req->buffer, p);

    if (req->state == http::state::STATUS)
    {
      uint16_t end = pbuf_memfind(req->buffer, "\r\n", 2, offset);
      if (end < 0xffff)
      {
        std::string status_line(end, '\0');
        pbuf_copy_partial(req->buffer, status_line.data(), end, offset);
        printf("< %s\n", status_line.c_str());
        if (auto npos = status_line.find(" "); npos != std::string::npos)
        {
          std::string version = status_line.substr(0, npos);
          int status = std::atoi(status_line.substr(npos).c_str());
          req->status = status;
          if (version != req->version)
          {
            panic("Response version %s != HTTP/1.1", version.c_str());
          }
          req->state = http::state::HEADERS;
        }
        else
        {
          panic("Invalid status line\n%s", status_line.c_str());
        }
        offset = end + 2;
      }
      else
      {
        // TODO: handle status not in payload
        panic("Status outside this buffer");
      }
    }

    if (req->state == http::state::HEADERS)
    {
      uint16_t end;
      do
      {
        end = pbuf_memfind(req->buffer, "\r\n", 2, offset);
        if (end < 0xffff)
        {
          // Found HTTP header
          std::string header(end - offset, '\0');
          pbuf_copy_partial(req->buffer, header.data(), header.size(), offset);
          printf("< %s\r\n", header.c_str());
          offset = end + 2;

          req->add_header(header);
        }
      } while (end < 0xffff && req->state == http::state::HEADERS);
    }

    if (req->state == http::state::BODY)
    {
      // Call the callback body function
      LWIP_ASSERT("callback_body fn == NULL", req->callback_body != nullptr);
      if (req->content_length >= 0)
        offset = req->transfer_body(offset, req->buffer->tot_len - offset);
      else
        offset = req->transfer_chunked(offset);
    }

    // Tells the underlying TCP mechanism that the data has been received
    altcp_recved(pcb, offset);
    req->buffer = pbuf_free_header(req->buffer, offset);

    if (req->state == http::state::DONE)
    {
      printf("Body transferred, closing connection (%d bytes)\n",req->body_rx);
      return altcp_close(pcb);
    }

    if (req->state == http::state::FAILED)
    {
      printf("Connection failed, closing...\n");
      pbuf_free(req->buffer);
      return altcp_close(pcb);
    }

/*
    // Get singleton instance
    State &state = State::getInstance();
    if (state.play_next_song_flag || state.play_previous_song_flag)
    {
      printf("Closing connection forcefully, trying to play next or previous song\n");
      altcp_recved(pcb, offset);
      pbuf_free(req->buffer);
      req->state = http::state::DONE;
      return altcp_close(pcb);
    }
    */

    return ERR_OK;
  }

  // Function to handle connection events
  err_t http::client::altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err)
  {
    http::request *req = reinterpret_cast<http::request *>(arg);

    if (err == ERR_OK)
    {
      printf("Connection established!\n");
      std::stringstream ss;

      // Request line
      ss << req->method << " " << req->path << " " << req->version << "\r\n";
      printf("> %s", ss.str().c_str());

      if (req->headers.count("host") == 0)
        req->headers["host"] = req->hostname;

      if (req->method == "POST" || req->method == "PUT" || req->method == "PATCH")
      {
        req->headers["content-length"] = std::to_string(req->body.size());
      }

      // Request headers
      for (auto [name, value] : req->headers)
      {
        printf("> %s: %s\r\n", name.c_str(), value.c_str());
        ss << name << ": " << value << "\r\n";
      }

      // End of request headers
      ss << "\r\n";
      printf(">\r\n");
      std::string request_string = ss.str();
      err = altcp_write(pcb, request_string.c_str(), request_string.length(), 0);
      err = altcp_output(pcb);

      // Request body
      err = altcp_write(pcb, req->body.data(), req->body.size(), 0);
      err = altcp_output(pcb);
    }
    else
    {
      printf("Error on connect: %d\n", err);
    }
    return ERR_OK;
  }

  // Function to set up TLS
  void http::client::tls_tcp_setup(http::request *req)
  {
    // Set up TLS
    struct altcp_tls_config *tls_config = altcp_tls_create_config_client(NULL, 0);
    struct altcp_pcb *pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    altcp_recv(pcb, recv);
    altcp_arg(pcb, req);
    req->pcb = pcb;

    // Connect to host with TLS/TCP
    printf("Attempting to connect to %s with IP address %s\n",
           req->hostname.c_str(), ipaddr_ntoa(&req->resolved_ip));
    auto ctx = reinterpret_cast<mbedtls_ssl_context *>(pcb);
    mbedtls_ssl_set_hostname(ctx, req->hostname.c_str());
    err_t err = altcp_connect(pcb, &req->resolved_ip, 443, altcp_client_connected);
    if (err != ERR_OK)
    {
      printf("TLS connection failed, error: %d\n", err);
    }
  }

  // Callback for DNS lookup
  void http::client::dns_resolve_callback(const char *name, const ip_addr_t *ipaddr, void *arg)
  {
    http::request *req = reinterpret_cast<http::request *>(arg);
    if (ipaddr != NULL)
    {
      printf("DNS lookup successful: %s, IP address is %s\n", name, ipaddr_ntoa(ipaddr));
      req->ip_resolved = true;
      req->resolved_ip = *ipaddr;
      req->client->tls_tcp_setup(req);
    }
    else
    {
      printf("DNS lookup failed for %s\n", name);
    }
  }

} // namespace http
