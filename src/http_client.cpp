#include "http_client.h"

#include "lwip/pbuf.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "audio.h"

#include "lwipopts.h"
#include "lwip/altcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"
#include <string>

extern Audio g_audio;

Http_client::Http_client()
{
  http_status = 0;
  response_headers_complete = false;
  http_content_length = -1;
  buffer_available = 0;
  http_body_rx = 0;
}

void Http_client::http_get(char *hostname, char *path)
{
    char format[] =  "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n";
    snprintf(header, sizeof(header), format, path, hostname);
    printf("HTTP Request: %s", header);
    resolve_dns(hostname);
}

// Function to handle data being transmitted
err_t Http_client::recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
    Http_client *client = reinterpret_cast<Http_client*>(arg);
    uint16_t offset = 0;

    if (p == NULL)
    {
        printf("No data received or connection closed\n");
        return ERR_OK;
    }

    // Copies data to the buffer
    pbuf_copy_partial(p, client->myBuff + client->buffer_available,
                      p->tot_len - offset, offset);
    client->buffer_available += p->tot_len - offset;

    // TODO: Test this call and try to extract the mp3 file
    // Encode the octect format(binary) or try sending it directly to the encoder
    if (!client->received_status()) {
      uint16_t end = pbuf_memfind(p, "\r\n", 2, 0);
      if (end < 0xffff) {
        std::string status_line(end, '\0');
        pbuf_copy_partial(p, status_line.data(), end, 0);
        if (auto npos = status_line.find(" "); npos != std::string::npos) {
          std::string version = status_line.substr(0, npos);
          int status = std::atoi(status_line.substr(npos).c_str());
          printf("HTTP Version: %s\n", version.c_str());
          printf("HTTP Status:  %d\n", status);
          client->http_status = status;
          client->http_version = version;
        } else {
          panic("Invalid status line\n%s", status_line.c_str());
        }
        offset = end + 2;
      } else {
        // TODO: handle status not in payload
        panic("Status outside this buffer");
      }
    }

    if (client->received_status() && !client->received_headers()) {
      uint16_t end = 0xffff;
      do {
        end = pbuf_memfind(p, "\r\n", 2, offset);
        if (end < 0xffff) {
          int size = end - offset;
          if (client->buffer_available > p->tot_len) {
            // set correct size if headers overlap pbufs
            size += client->buffer_available - p->tot_len;
          }

          // Found HTTP header
          std::string header(client->myBuff + offset, size);
          printf("Raw header: %s\n", header.c_str());
          client->add_header(header);

          if (size == 0) {
            // All headers received
            offset += 2;
            break;
          }

          if (client->buffer_available > p->tot_len) {
            pbuf_copy_partial(p, client->myBuff, p->tot_len, 0);
            client->buffer_available = p->tot_len - end;
            offset = end + 2;
          } else {
            offset += size + 2;
          }
        } else {
          // Move data to beginnig of buffer if buffer does not
          // contain all headers
          memmove(client->myBuff,
                  client->myBuff + offset,
                  client->buffer_available - offset);
          client->buffer_available -= offset;
          printf("Headers bigger buffer, waiting for more...\n");
        }
      } while (end < 0xffff);
    }

    // Stream mp3 data and play it on speaker
    if (client->received_status() && client->received_headers()) {
      // Find next frame in buffer
      int frame_index = MP3FindSyncWord((uint8_t*)client->myBuff + offset,
                                        client->buffer_available - offset);

      while (frame_index < client->buffer_available) {
        int read = g_audio.stream_decode((uint8_t*)client->myBuff + frame_index,
                                         client->buffer_available - frame_index);
        if (read == 0)
          break;

        frame_index += read;
      }

      if (frame_index < client->buffer_available) {
        // Move remaining buffer data to the beginnig of the buffer
        memmove(client->myBuff, client->myBuff + frame_index,
                client->buffer_available - frame_index);
        client->buffer_available -= frame_index;
      }

      client->http_body_rx += frame_index - offset;
    }

    printf("Body= %d bytes of %d\r",
           client->http_body_rx, client->http_content_length);

    // Tells the underlying TCP mechanism that the data has been received
    altcp_recved(pcb, p->tot_len);

    // Free the pbuf from memory
    pbuf_free(p);

    if (client->http_body_rx == client->http_content_length) {
      printf("\nBody transferred, closing connection\n");
      return altcp_close(pcb);
    }

    return ERR_OK;
}

// Function to handle connection events
err_t Http_client::altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
    Http_client *client = reinterpret_cast<Http_client*>(arg);

    if (err == ERR_OK)
    {
        printf("Connection established!\n");
        err = altcp_write(pcb, client->header, strlen(client->header), 0);
        err = altcp_output(pcb);
    }
    else
    {
        printf("Error on connect: %d\n", err);
    }
    return ERR_OK;
}

// Function to set up TLS
void Http_client::tls_tcp_setup(const char *website)
{
    // Set up TLS
    struct altcp_tls_config *tls_config = altcp_tls_create_config_client(NULL, 0);
    struct altcp_pcb *pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    altcp_recv(pcb, recv);
    altcp_arg(pcb, this);

    // Connect to host with TLS/TCP
    printf("Attempting to connect to %s with IP address %s\n", website, ipaddr_ntoa(&resolved_ip));
    mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(pcb), website);
    err_t err = altcp_connect(pcb, &resolved_ip, 443, altcp_client_connected);
    if (err != ERR_OK)
    {
        printf("TLS connection failed, error: %d\n", err);
    }
}

// Callback for DNS lookup
void Http_client::dns_resolve_callback(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    Http_client *client = reinterpret_cast<Http_client*>(arg);
    if (ipaddr != NULL)
    {
        printf("DNS lookup successful: %s, IP address is %s\n", name, ipaddr_ntoa(ipaddr));
        client->ip_resolved = true;
        client->resolved_ip = *ipaddr;
        client->tls_tcp_setup(name);
    }
    else
    {
        printf("DNS lookup failed for %s\n", name);
    }
}

// Function to get IP address from hostname
void Http_client::resolve_dns(const char *hostname)
{
    ip_addr_t addr;
    err_t err = dns_gethostbyname(hostname, &addr, dns_resolve_callback, this);
    if (err == ERR_INPROGRESS)
    {
        printf("DNS resolution in progress...\n");
    }
    else if (err == ERR_OK)
    {
        ip_resolved = true;
        resolved_ip = addr;
        printf("Target IP: %s\n", ipaddr_ntoa(&resolved_ip));
    }
    else
    {
        printf("DNS resolve failed/error\n");
    }
}

void Http_client::add_header(std::string header) {
  if (header.empty()) {
    printf("All headers received %ld\n", http_response_headers.size());
    response_headers_complete = true;
    for (auto [k,v] : http_response_headers) {
      // Print processed headers
      printf("%s: %s\n", k.c_str(), v.c_str());
    }
    putchar('\n');
    return;
  }

  if (auto npos = header.find(": "); npos != std::string::npos) {
    std::string name = header.substr(0, npos);
    std::string value = header.substr(npos + 2);
    for (char &c : name) {
      c = std::tolower(c);
    }
    http_response_headers[name] = value;

    if (name == "content-length") {
      http_content_length = std::atoi(value.c_str());
    }
  } else {
    panic("Invalid header\n%s", header.c_str());
  }
}
