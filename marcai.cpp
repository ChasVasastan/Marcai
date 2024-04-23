#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"

#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"

char ssid[] = "ssid";
char pass[] = "pass";

#define BUF_SIZE 2048

char myBuff[BUF_SIZE];
char header[] = "GET / HTTP/1.1\r\nHOST: catfact.ninja\r\n\r\n";

err_t recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
    if (p != NULL)
    {
       printf("recv total %dthis buffer %d next %d err %d\n", p->tot_len, p->len, p->next, err);
        pbuf_copy_partial(p, myBuff, p->tot_len, 0);
        myBuff[p->tot_len] = 0;
        printf("Buffer= %s\n", myBuff);
        altcp_recved(pcb, p->tot_len);
        pbuf_free(p);
    }
    return ERR_OK;
}

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

bool connect_wifi()
{
    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("failed to connect :(\n");

        return false;
    }

    printf("Connected :)\n");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    return true;
}

int main()
{
    stdio_init_all();
    cyw43_arch_init();

    while (!stdio_usb_connected())
    {
    }

    if (!connect_wifi())
    {
        printf("connect wifi error\n");
    }

    struct altcp_tls_config *tls_config = altcp_tls_create_config_client(NULL, 0);
    struct altcp_pcb *pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    mbedtls_ssl_set_hostname((mbedtls_ssl_context*)altcp_tls_context(pcb), "catfact.ninja");
    altcp_recv(pcb, recv);
    ip_addr_t ip;
    // implement dynamics dns get otherwise you need to manually change ipv4
    IP4_ADDR(&ip, 104, 131, 8, 184);
    cyw43_arch_lwip_begin();
    err_t err = altcp_connect(pcb, &ip, 443, altcp_client_connected);
    cyw43_arch_lwip_end();
    while (true)
    {
        sleep_ms(500);
    }
}
