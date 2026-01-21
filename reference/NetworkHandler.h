#ifndef NETWORK_HANDLER_H
#define NETWORK_HANDLER_H

#include <WiFi.h>          // ESP32 WiFi library
#include <HTTPClient.h>    // HTTP client library
#include <ArduinoJson.h>   // JSON library

// WiFi credentials and API endpoints (customize as needed)
extern const char* ssid;
extern const char* pwd;
extern String getCustomerUrl;
extern String postOpenDoorUrl;
extern String shelfId;

extern WiFiClient wifiClient;
extern String scannedString;
extern boolean scanComplete;

bool connectToWifi();
void checkWiFiConnection();
void sendGetRequest();
String sendPostRequest(String customerId);
void testJsonString(String customerId);

#endif