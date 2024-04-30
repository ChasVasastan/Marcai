#include "http_client.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwipopts.h"
#include "lwip/altcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"

// Global instance
Http_client global_client;

Http_client::Http_client()
{
    memset(global_client.myBuff, 0, BUF_SIZE);
    memset(global_client.header, 0, BUF_SIZE / 2);
}

void Http_client::http_get(char *website, char *api_endpoint)
{
    snprintf(global_client.header, sizeof(global_client.header), "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", api_endpoint, website);
    printf("HTTP Header: %s", global_client.header);
}

// Function to handle data being transmitted
err_t Http_client::recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
    // Refer to the global_client
    Http_client *client = &global_client;

    if (p == NULL)
    {
        printf("No data received or connection closed\n");
        return ERR_OK;
    }

    // TODO: Test this call and try to extract the mp3 file
    // https://storage.googleapis.com/udio-artifacts-c33fe3ba-3ffe-471f-92c8-5dfef90b3ea3/samples/8ecb5ec94c774b04bdb3fd66d39c1b2b/1/Cosmic%20Canine%20Cruise.mp3

    printf("Recv total %d This buffer %d Next %d Error %d\n", p->tot_len, p->len, p->next, err);

    // Copies data to the buffer
    pbuf_copy_partial(p, client->myBuff, p->tot_len, 0);
    client->myBuff[p->tot_len] = 0;
    printf("Buffer= %s\n", client->myBuff);

    // Tells the underlying TCP mechanism that the data has been received
    altcp_recved(pcb, p->tot_len);

    // Free the pbuf from memory
    pbuf_free(p);

    return ERR_OK;
}

// Function to handle connection events
err_t Http_client::altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
    // Refer to the global_client
    Http_client *client = &global_client;

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
    Http_client *client = &global_client;

    // Set up TLS
    struct altcp_tls_config *tls_config = altcp_tls_create_config_client(NULL, 0);
    struct altcp_pcb *pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    altcp_recv(pcb, recv);

    // Connect to host with TLS/TCP
    printf("Attempting to connect to %s with IP address %s\n", website, ipaddr_ntoa(&client->resolved_ip));
    mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(pcb), website);
    err_t err = altcp_connect(pcb, &client->resolved_ip, 443, altcp_client_connected);
    if (err != ERR_OK)
    {
        printf("TLS connection failed, erro: %d\n", err);
    }
}

// Callback for DNS lookup
void Http_client::dns_resolve_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    Http_client *client = &global_client;
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
    err_t err = dns_gethostbyname(hostname, &addr, dns_resolve_callback, NULL);
    if (err == ERR_INPROGRESS)
    {
        printf("DNS resolution in progress...\n");
    }
    else if (err == ERR_OK)
    {
        global_client.ip_resolved = true;
        global_client.resolved_ip = addr;
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
    return global_client.myBuff;
}

char *Http_client::get_header()
{
    return global_client.header;
}

bool Http_client::is_ip_resolved()
{
    return global_client.ip_resolved;
}
