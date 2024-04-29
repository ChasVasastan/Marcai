#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwipopts.h"
#include "lwip/altcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"

char ssid[] = "ssid";
char pass[] = "pass";

#define BUF_SIZE 2048

char myBuff[BUF_SIZE];
char header[BUF_SIZE / 2];

ip_addr_t resolved_ip;
bool ip_resolved = false;

void set_http_get(char *website, char *api_endpoint)
{
  snprintf(header, sizeof(header), "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", api_endpoint, website);
  printf("HTTP Header: %s", header);
}

// Function to handle data being transmitted
err_t recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
  if (p != NULL)
  {
    printf("Recv total %d This buffer %d Next %d err %d\n", p->tot_len, p->len, p->next, err);
    // Copies data to the buffer
    pbuf_copy_partial(p, myBuff, p->tot_len, 0);
    myBuff[p->tot_len] = 0;
    printf("Buffer= %s\n", myBuff);
    // Tells the underlying TCP mechanism that the data has been received
    altcp_recved(pcb, p->tot_len);
    pbuf_free(p);
  }
  return ERR_OK;
}

// Function to handle connection events
static err_t altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
  if (err == ERR_OK)
  {
    err = altcp_write(pcb, header, strlen(header), 0);
    err = altcp_output(pcb);
  }
  else
  {
    printf("Error on connect: %d\n", err);
  }
  return ERR_OK;
}

// Function to connec to wifi
bool connect_wifi()
{
  cyw43_arch_enable_sta_mode();

  if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 30000))
  {
    printf("Failed to connect :(\n");

    return false;
  }

  printf("Connected to %s\n", ssid);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  return true;
}

// Callback for DNS lookup
void dns_resolve_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
  if (ipaddr != NULL)
  {
    printf("DNS lookup successful: %s IP address is %s\n", name, ipaddr_ntoa(ipaddr));
    resolved_ip = *ipaddr;
    ip_resolved = true;
  }
  else
  {
    printf("DNS lookup failed for %s\n", name);
  }
}

// Function to get IP adress for hostname
void resolve_dns(const char *hostname)
{
  ip_addr_t addr;
  err_t err = dns_gethostbyname(hostname, &addr, dns_resolve_callback, NULL);
  if (err == ERR_INPROGRESS)
  {
    printf("DNS resolution in progress...\n");
  }
  else if (err == ERR_OK)
  {
    ip_resolved = true;
    printf("Target IP: %s\n", ipaddr_ntoa(&resolved_ip));
  }
  else
  {
    printf("DNS resolve failed/error\n");
  }
}

int main()
{
  stdio_init_all();
  cyw43_arch_init();

  // The pico will start when you start the terminal
  while (!stdio_usb_connected())
  {
  }

  if (!connect_wifi())
  {
    printf("Connect wifi error\n");
  }

  char server_name[] = "catfact.ninja";
  char uri[] = "/fact";
  set_http_get(server_name, uri);

  resolve_dns(server_name);

  while (!ip_resolved)
  {
    printf("Trying to resolve IP...\n");
    sleep_ms(500);
  }

  // Set up TLS
  struct altcp_tls_config *tls_config = altcp_tls_create_config_client(NULL, 0);
  struct altcp_pcb *pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
  altcp_recv(pcb, recv);

  // Connect to host with TLS/TCP
  mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(pcb), server_name);
  err_t err = altcp_connect(pcb, &resolved_ip, 443, altcp_client_connected);

  cyw43_arch_lwip_begin();

  cyw43_arch_lwip_end();
  while (true)
  {
    sleep_ms(500);
  }
}
