#include "captive_portal.h"
#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_netif.h>
#include <vector>
namespace {
DNSServer dns;
bool started=false;
}
bool portalBegin() {
  auto* netif=esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  bool dhcpOk=false;
  if(netif) {
    auto stopped=esp_netif_dhcps_stop(netif);
    if(stopped==ESP_OK || stopped==ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
      // IDF 4.x DHCP OFFER_DNS mask: advertise the ESP itself as DNS server.
      uint8_t offerDns=0x02;
      esp_netif_dns_info_t info{};
      info.ip.type=ESP_IPADDR_TYPE_V4;
      info.ip.u_addr.ip4.addr=uint32_t(WiFi.softAPIP());
      auto option=esp_netif_dhcps_option(netif,ESP_NETIF_OP_SET,ESP_NETIF_DOMAIN_NAME_SERVER,&offerDns,sizeof(offerDns));
      auto address=esp_netif_set_dns_info(netif,ESP_NETIF_DNS_MAIN,&info);
      auto resumed=esp_netif_dhcps_start(netif);
      dhcpOk=option==ESP_OK && address==ESP_OK && resumed==ESP_OK;
      Serial.printf("[PORTAL] DHCP DNS option=%d address=%d start=%d\n",int(option),int(address),int(resumed));
    }
  }
  dns.setTTL(1);
  started=dns.start(53,"*",WiFi.softAPIP());
  Serial.printf("[PORTAL] DNS=%d DHCP-DNS=%d address=%s\n",started,dhcpOk,WiFi.softAPIP().toString().c_str());
  return started && dhcpOk;
}
void portalProcess() {if(started)dns.processNextRequest();}
void portalRedirect(WebServer& server) {
  String url="http://"+WiFi.softAPIP().toString()+"/";
  server.sendHeader("Location",url,true);
  server.sendHeader("Cache-Control","no-store, no-cache, must-revalidate");
  server.sendHeader("Pragma","no-cache");
  server.send(302,"text/html; charset=utf-8","<!doctype html><html lang=\"de\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>espmame</title><p><a href=\""+url+"\">Schachoberflaeche oeffnen</a></p></html>");
}
void portalRegister(WebServer& server) {
  // Android, Apple and Windows connectivity probes must see a captive network.
  for(const char* path : {"/generate_204","/gen_204","/hotspot-detect.html",
      "/library/test/success.html","/success.html","/connecttest.txt",
      "/ncsi.txt","/redirect","/fwlink","/canonical.html"}) {
    server.on(path,HTTP_GET,[&server]{portalRedirect(server);});
  }
}
void portalSelfTest() {
  WiFiUDP udp;
  if(!udp.begin(40535)) {Serial.println("[DNS TEST] socket failed");return;}
  std::vector<uint8_t> query(12,0);
  query[0]=0x45;query[1]=0x53;query[2]=1;query[5]=1;
  for(const char* label:{"connectivitycheck","gstatic","com"}) {
    size_t n=strlen(label);query.push_back(uint8_t(n));query.insert(query.end(),label,label+n);
  }
  query.insert(query.end(),{0,0,1,0,1});
  udp.beginPacket(WiFi.softAPIP(),53);udp.write(query.data(),query.size());udp.endPacket();
  uint32_t began=millis();
  while(!udp.parsePacket() && uint32_t(millis()-began)<3000)vTaskDelay(pdMS_TO_TICKS(10));
  uint8_t response[256];int n=udp.read(response,sizeof(response));
  bool good=n>=16 && response[0]==0x45 && response[1]==0x53 && (response[2]&0x80) && response[7]>0;
  if(good) {
    IPAddress resolved(response[n-4],response[n-3],response[n-2],response[n-1]);
    Serial.printf("[DNS TEST] wildcard -> %s %s\n",resolved.toString().c_str(),resolved==WiFi.softAPIP()?"PASS":"FAIL");
  } else Serial.printf("[DNS TEST] FAIL response=%d\n",n);
  udp.stop();
}
