#include "NetworkHandler.h"

// WiFi credentials and API endpoints
const char* ssid = "HL7628AP";
const char* pwd = "abc123##";
String getCustomerUrl = "http://192.168.1.80:15000/api/customer";
String postOpenDoorUrl = "http://192.168.1.80:15000/api/Cart/openShelfByQrCode";
String shelfId = "S1";


bool connected = false;

// Global variables
WiFiClient wifiClient;

bool connectToWifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pwd);

  int timeout = 20; // 10 seconds timeout (20 * 500ms)
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(500);
    Serial.print(".");
    timeout--;
  }

  return (WiFi.status() == WL_CONNECTED); // True if connected, false if not
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectToWifi();
  }
}

void sendGetRequest() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(wifiClient, getCustomerUrl.c_str());

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(payload);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  delay(10000); // Delay before next request
}

String sendPostRequest(String customerId) {
  DynamicJsonDocument doc(128);
  customerId.replace("\r", "");
  doc["customerId"] = customerId;
  doc["shelfCode"] = shelfId;

  String jsonString;
  serializeJson(doc, jsonString);

  HTTPClient http;
  http.begin(wifiClient, postOpenDoorUrl.c_str());
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(jsonString);

  if (httpResponseCode > 0) {
    Serial.print("HTTP POST request successful, response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
    return payload;
  } else {
    Serial.print("Error in HTTP POST request, response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.print("Payload: ");
    Serial.println(payload);
    return "2";
  }

  http.end();

  return "1";
}

void testJsonString(String customerId) {
  DynamicJsonDocument doc(128);
  customerId.replace("\r", "");
  doc["customerId"] = customerId;
  doc["shelfCode"] = shelfId;

  String jsonString;
  serializeJson(doc, jsonString);

  Serial.println(jsonString);
}