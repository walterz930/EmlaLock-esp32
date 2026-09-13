#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>

extern DNSServer dnsServer;

static void stopSetupAccessPoint(){
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
}

static void onWiFiEvent(WiFiEvent_t event){
  if(event==ARDUINO_EVENT_WIFI_STA_GOT_IP) stopSetupAccessPoint();
}

struct ApCleanupInit{
  ApCleanupInit(){ WiFi.onEvent(onWiFiEvent); }
};

static ApCleanupInit apCleanupInit;
