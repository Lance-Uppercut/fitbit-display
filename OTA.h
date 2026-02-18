#ifndef OTA_H_
#define OTA_H_

#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif

#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <TelnetStream.h>

inline void setupOTA(const char* nameprefix, const char* ssid, const char* password) {
  uint16_t maxlen = strlen(nameprefix) + 7;
  char* fullhostname = new char[maxlen];
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(fullhostname, maxlen, "%s-%02x%02x%02x", nameprefix, mac[3], mac[4], mac[5]);
  ArduinoOTA.setHostname(fullhostname);

#if defined(ESP32)
  WiFi.setHostname(fullhostname);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
#endif
  delete[] fullhostname;

  WiFi.mode(WIFI_STA);
  IPAddress noAddress(0U);
  IPAddress primaryDns(1, 1, 1, 1);
  IPAddress secondaryDns(8, 8, 8, 8);
  bool dnsConfigured = WiFi.config(noAddress, noAddress, noAddress, primaryDns, secondaryDns);
  Serial.printf("WiFi DNS override %s (%s, %s)\r\n",
                dnsConfigured ? "enabled" : "not-applied",
                primaryDns.toString().c_str(),
                secondaryDns.toString().c_str());

  WiFi.begin(ssid, password);

  unsigned long startedAt = millis();
  const unsigned long initialConnectTimeoutMs = 12000;
  while (WiFi.status() != WL_CONNECTED && (millis() - startedAt) < initialConnectTimeoutMs) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi not connected yet. Continuing without reboot.");
  }

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
    TelnetStream.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    TelnetStream.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percent = total == 0 ? 0 : static_cast<unsigned int>((progress * 100UL) / total);
    Serial.printf("Progress: %u%%\r", percent);
    TelnetStream.printf("Progress: %u%%\r", percent);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    TelnetStream.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("\nAuth Failed");
      TelnetStream.println("\nAuth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("\nBegin Failed");
      TelnetStream.println("\nBegin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("\nConnect Failed");
      TelnetStream.println("\nConnect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("\nReceive Failed");
      TelnetStream.println("\nReceive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("\nEnd Failed");
      TelnetStream.println("\nEnd Failed");
    }
  });

  ArduinoOTA.begin();
  TelnetStream.begin();
  Serial.println("OTA initialized");
}

#endif
