#include "wifi_config.h"
#include "pico/cyw43_arch.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "netif/etharp.h"
#include "lwip/tcpip.h"

void print_ip_address() {
  struct netif *netif = &cyw43_state.netif[CYW43_ITF_AP];
  if (!netif_is_up(netif))
    {
      printf("Unable to get IP address, net interface is down\n");
      return;
    }

  ip4_addr_t ip = netif->ip_addr;
  printf("AP IP Address is %s\n", ip4addr_ntoa(&ip));
}

void Wifi_Config::setup_access_point() {
  if (cyw43_arch_init()) {
    printf("Wifi init failed\n");
    return;
  }
  
  cyw43_arch_enable_ap_mode(WIFI_AP_SSID, WIFI_AP_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
  printf("Access Point set with name: %s\n", WIFI_AP_SSID);

  // print_ip_address();

  setting_netif();
}


void Wifi_Config::setting_netif()
{
  struct netif *netif = &cyw43_state.netif[CYW43_ITF_AP];
  ip4_addr_t ipaddr, netmask, gw;

  // Set up IP address, netmask, and gateway for the AP
  //IP4_ADDR(&ipaddr, 192, 168, 4, 1);
  IP4_ADDR(&ipaddr, 169, 254, 141, 158);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 169, 254, 141, 158);
  // IP4_ADDR(&gw, 192, 168, 4, 1);

  netif_set_addr(netif, &ipaddr, &netmask, &gw);

  // Start the DHCP server
  // dhcp_server_start(netif);
  printf("Server started with IP: %s\n", ipaddr_ntoa(&ipaddr));
}


static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    if (result) {
        printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
            result->ssid, result->rssi, result->channel,
            result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
            result->auth_mode);
    }
    return 0;
}

void Wifi_Config::scan_networks() {
    cyw43_wifi_scan_options_t scan_options = {0};
    
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result);
    if (err == 0)
    {
      printf("\nPerforming wifi scan\n");
    } else {
      printf("Failed to start scan: %d\n", err);
    }
    
}

void Wifi_Config::connect_to_network(const char* ssid, const char* password) {
  cyw43_arch_disable_ap_mode();
  printf("Connection to network: %s\n", ssid);
  if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000))
  {
    printf("Failed to connect \n");
  }
  
  printf("Connected to %s\n", ssid);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}