#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <patterns.h> 

void wifiServerSetup();
void wifiLoop();
extern uint8_t g_current_pattern_number;
extern int g_Brightness;
extern int g_Speed;

#endif // WIFI_SERVER_H
