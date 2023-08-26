#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ETHClass.h>           //Is to use the modified ETHClass
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <SoftwareSerial.h>
#include <NTPServer.h>
#include <GPSManager.h>
#include "pindefinitions.h"     //Board PinMap
#include "secrets.h"            // OTA_USERNAME and OTA_PASSWORD definitions

#define GPS_PPS_PIN 12
#define GPS_RX_PIN 14
#define GPS_TX_PIN 15

#define MONITOR_UART_BPS 115200
#define GPS_UART_BPS 115200

// Define OTA_USERNAME and OTA_PASSWORD in "secrets.h"
#define HOSTNAME "esp32-ntpserver-1"

void wifiEvent(WiFiEvent_t event);

AsyncWebServer server(80);
static bool eth_connected = false;
EspSoftwareSerial::UART gpsSerial;

GPSManager* gpsManager;
NTPServer* ntpServer;

void setup() {
  Serial.begin(MONITOR_UART_BPS);
  Serial.println("Booting " HOSTNAME "...");

  gpsSerial.begin(GPS_UART_BPS, SWSERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN, false);
  if (!gpsSerial) {
    Serial.println("Invalid GPS serial config");
  }

  WiFi.onEvent(wifiEvent);

#ifdef ETH_POWER_PIN
  pinMode(ETH_POWER_PIN, OUTPUT);
  digitalWrite(ETH_POWER_PIN, HIGH);
#endif

#if CONFIG_IDF_TARGET_ESP32
  if (!ETH.begin(ETH_ADDR, ETH_RESET_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE)) {
    Serial.println("Ethernet failed to start!");
  }
#else
  if (!ETH.beginSPI(ETH_MISO_PIN, ETH_MOSI_PIN, ETH_SCLK_PIN, ETH_CS_PIN, ETH_RST_PIN, ETH_INT_PIN)) {
    Serial.println("Ethernet failed to start!");
  }
#endif

  AsyncElegantOTA.begin(&server, OTA_USERNAME, OTA_PASSWORD);

  while (!eth_connected) {
    Serial.println("Wait for network to connect...");
    delay(500);
  }

  if (MDNS.begin(HOSTNAME)) {
    Serial.println("mDNS responder started");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", "<!DOCTYPE html><title>" HOSTNAME "</title><h1>" HOSTNAME "</h1><p><a href='/update'>Update</a></p><p><a href='/status'>Status</a></p>");
    });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    String timestr = "No Fix";
    if (gpsManager->validFix()) {
      uint32_t micros = 0;
      time_t time = now(micros);
      timestr =  String(day(time)) + "-" + String(month(time)) + "-" + String(year(time)) + " " + String(hour(time)) + ":" + String(minute(time)) + ":" + String(second(time)) + "." + (micros % 1000000);
    }
    request->send(200, "text/html", "<!DOCTYPE html><title>" HOSTNAME "</title><h1>" HOSTNAME " Status</h1><p>GPS Time: " + timestr + "</p>");
    });

  server.onNotFound([](AsyncWebServerRequest* request) {
    String message = "URL: ";
    message += request->url();
    message += "<br />Method: ";
    message += (request->method() == HTTP_GET) ? "GET" : "POST";
    message += "<br />Arguments: ";
    message += request->args();
    message += "<br />";
    for (uint8_t i = 0; i < request->args(); i++) {
      message += " " + request->argName(i) + ": " + request->arg(i) + "<br />";
    }
    request->send(404, "text/html", "<!DOCTYPE html><title>" HOSTNAME "</title><h1>404 - File Not Found</h1><p>" + message + "</p>");
    });

  server.begin();
  Serial.println("HTTP server started");
  gpsManager = new GPSManager(gpsSerial, GPS_PPS_PIN);
  Serial.println("GPS manager started");
  ntpServer = new NTPServer(Serial, *gpsManager);
  Serial.println("NTP server started");

  Serial.println("Ready");
}

void loop() {
  gpsManager->loop();
}

void wifiEvent(WiFiEvent_t event) {
  switch (event) {
  case ARDUINO_EVENT_ETH_START:
    Serial.println("Ethernet started");
    //set eth hostname here
    ETH.setHostname(HOSTNAME);
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    Serial.println("Ethernet connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    Serial.print("Ethernet MAC: ");
    Serial.print(ETH.macAddress());
    Serial.print(", IPv4: ");
    Serial.print(ETH.localIP());
    if (ETH.fullDuplex()) {
      Serial.print(", FULL_DUPLEX");
    }
    Serial.print(", ");
    Serial.print(ETH.linkSpeed());
    Serial.print("Mbps");
    Serial.print(", ");
    Serial.print("Gateway IP:");
    Serial.println(ETH.gatewayIP());
    eth_connected = true;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    Serial.println("Ethernet disconnected");
    eth_connected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    Serial.println("Ethernet stopped");
    eth_connected = false;
    break;
  default:
    break;
  }
}