#include <ETH.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Preferences.h>

/* =========================
   STATIC IP CONFIG
========================= */

// Ethernet
IPAddress eth_IP(192,168,1,69);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
IPAddress dns(8,8,8,8);

/* =========================
   UART CONFIG
========================= */

HardwareSerial UBX(1);

#define UBX_RX 4
#define UBX_TX 2
#define UBX_BAUD 230400
#define SERIAL_BUFFER 4096

/* =========================
   UBX TCP SERVER
========================= */

WiFiServer bridgeServer(9000);
WiFiClient bridgeClient;

/* ========================= */

WebServer web(80);
Preferences prefs;

/* ========================= */

void handleRoot() {

  String page = "<html><body style='font-family:Arial'>";
  page += "<h3>WT32 Network Manager</h3>";

  page += "ETH IP: " + ETH.localIP().toString() + "<br>";
  page += "UBX TCP Port: 9000<br>";
  
  }

 /* ========================= */

void setup() {

  Serial.begin(115200);

  // UART UBX
  UBX.setRxBufferSize(SERIAL_BUFFER);
  UBX.begin(UBX_BAUD, SERIAL_8N1, UBX_RX, UBX_TX);

  ETH.begin();
  ETH.config(eth_IP, gateway, subnet, dns);

  while (!ETH.linkUp()) delay(100);

  // Start TCP server
  bridgeServer.begin();
  bridgeServer.setNoDelay(true);

  Serial.println("READY");
}

/* ========================= */

void loop() {

  web.handleClient();

  // Accept TCP client
  if (!bridgeClient || !bridgeClient.connected()) {
    bridgeClient = bridgeServer.available();
    if (bridgeClient) {
      bridgeClient.setNoDelay(true);
      Serial.println("TCP Client Connected");
    }
  }

  if (bridgeClient && bridgeClient.connected()) {

    uint8_t buf[1024];

    // UBX → TCP
    while (UBX.available()) {
      int len = UBX.readBytes(buf, sizeof(buf));
      if (len > 0) {
        if (bridgeClient.write(buf, len) <= 0) {
          bridgeClient.stop();
          break;
        }
      }
    }

    // TCP → UBX
    while (bridgeClient.available()) {
      int len2 = bridgeClient.read(buf, sizeof(buf));
      if (len2 > 0) {
        UBX.write(buf, len2);
      }
    }
  }

  delay(1);
}
