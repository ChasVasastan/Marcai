#include "http_client.h"

#include "lwip/pbuf.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwipopts.h"
#include "lwip/altcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"
#include <map>
#include <string>

Http_client::Http_client()
{
  http_status = 0;
  reply_headers_complete = false;
}

void Http_client::http_get(char *website, char *api_endpoint)
{
    snprintf(header, sizeof(header), "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", api_endpoint, website);
    printf("HTTP Header: %s", header);
}

// Function to handle data being transmitted
err_t Http_client::recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
    Http_client *client = reinterpret_cast<Http_client*>(arg);

    if (p == NULL)
    {
        printf("No data received or connection closed\n");
        return ERR_OK;
    }

    // TODO: Test this call and try to extract the mp3 file
    // Encode the octect format(binary) or try sending it directly to the encoder
    printf("Recv total %d This buffer %d Next %p Error %d\n", p->tot_len, p->len, p->next, err);
    uint16_t offset = 0;
    if (!client->received_status()) {
      uint16_t end = pbuf_memfind(p, "\r\n", 2, 0);
      if (end < 0xffff) {
        std::string status_line(end, 0);
        pbuf_copy_partial(p, status_line.data(), end, 0);
        if (auto npos = status_line.find(" "); npos != std::string::npos) {
          std::string version = status_line.substr(0, npos);
          int status = std::atoi(status_line.substr(npos).c_str());
          printf("HTTP Version: %s\n", version.c_str());
          printf("HTTP Status:  %d\n", status);
          client->set_status(status);
          client->set_version(version);
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
          if (size == 0) {
            // End of HTTP headers
            printf("All headers received\n");
            client->set_received_headers();
            putchar('\n');

            for (auto [k,v] : client->get_reply_headers()) {
              // Print processed headers
              printf("%s: %s\n", k.c_str(), v.c_str());
            }
            putchar('\n');

            offset += 2;
            break;
          }
          // Found HTTP header
          std::string header(size, 0);
          pbuf_copy_partial(p, header.data(), size, offset);

          // Find header name and value delimiter
          if (auto npos = header.find(": "); npos != std::string::npos) {
            std::string key = header.substr(0, npos);
            std::string value = header.substr(npos + 2);
            client->add_header(key, value);
            printf("Raw header: %s: %s\n", key.c_str(), value.c_str());
          } else {
            panic("Invalid header\n%s", header.c_str());
          }
          offset += size + 2;
        } else {
          // TODO: handle headers between pbufs
          panic("Headers bigger than this buffer");
        }
      } while (end < 0xffff);

      if (offset < p->tot_len) {
        // bytes left in payload
        printf("remainging payload: %d\n", p->tot_len - offset);
      }
    }

    if (client->received_status() && client->received_headers()) {
      // Copies data to the buffer
      pbuf_copy_partial(p, client->myBuff, p->tot_len - offset, offset);
      client->myBuff[p->tot_len - offset] = 0;
      printf("Buffer= %d bytes\n", p->tot_len - offset);
    }

    // Tells the underlying TCP mechanism that the data has been received
    altcp_recved(pcb, p->tot_len);

    // Free the pbuf from memory
    pbuf_free(p);

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
        printf("TLS connection failed, erro: %d\n", err);
    }
}

// Callback for DNS lookup
void Http_client::dns_resolve_callback(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    Http_client *client = reinterpret_cast<Http_client*>(arg);
    if (ipaddr != NULL)
    {
        printf("DNS lookup successful: %s, IP address is %s\n", name, ipaddr_ntoa(ipaddr));
        client->set_ip_resolved(true);
        client->resolved_ip = *ipaddr;
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

// Setters
void Http_client::set_my_buff(const char *buffer, size_t length)
{
    memcpy(myBuff, buffer, length);
    myBuff[length] = '\0';
}

void Http_client::set_header(const char *newHeader)
{
    strncpy(header, newHeader, sizeof(header) - 1);
    header[sizeof(header) - 1] = '\0';
}

void Http_client::set_ip_resolved(bool status)
{
    ip_resolved = status;
}

// Getters
char *Http_client::get_my_buff()
{
    return myBuff;
}

char *Http_client::get_header()
{
    return header;
}

bool Http_client::is_ip_resolved()
{
    return ip_resolved;
}
