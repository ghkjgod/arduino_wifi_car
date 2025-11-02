#include <WiFi.h>
#include <WiFiAP.h>
#include <ArduinoJson.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WebSocketsServer_Generic.h>
#include "AsyncUDP.h"
#include "Arduino.h"
#include <U8x8lib.h>
#include "motor.h"

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define WS_PORT 8088
#define _WEBSOCKETS_LOGLEVEL_ 0
#define DEBUG_MODE 0

#if DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#define DEBUG_WRITE(x, len) Serial.write(x, len)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#define DEBUG_WRITE(x, len)
#endif


/* Wifi Credentials */
String sta_ssid = "zhqm";           // set Wifi network you want to connect to
String sta_password = "19870827";  // set password for Wifi network
unsigned long previousMillis = 0;
IPAddress myIP;

AsyncUDP udp;
WebSocketsServer webSocket = WebSocketsServer(WS_PORT);
U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ 21, /* data=*/ 19, /* reset=*/ U8X8_PIN_NONE);

int LISTEN_PORT = 9567;
volatile int heart = 0;

// Pre-allocated JSON document for better performance
StaticJsonDocument<256> jsonDoc;

// Function declarations
void init_wifi();
void init_server();
void handle_controller(const char* payload);
void response_broadcast();
void pre(void);
void dispatchHeartBeat(void *pvParameters) {
    (void)pvParameters; // Suppress unused parameter warning
    int count = 0;
    while(1) {
        DEBUG_PRINTF("heart %d!\n", heart);
        if(heart == 0) {
          count++;
        } else {
          count = 0;
          heart = 0;
        }
        if(count >= 3) {
          count = 0;
          DEBUG_PRINTF("Disconnected!\n");
          front_turn(0, 0);
          u8x8.clearLine(3);
          u8x8.setCursor(0, 3);
          u8x8.print("Disconnected");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}



void checkHeartBeat(){

   xTaskCreatePinnedToCore(
    dispatchHeartBeat
    ,  "dispatchHeartBeat"   // A name just for humans
    ,  2048  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  (void *)heart
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

}

void hexdump(const void* mem, const uint32_t& len, const uint8_t& cols = 16) {
  #if DEBUG_MODE
  const uint8_t* src = (const uint8_t*)mem;
  DEBUG_PRINTF("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);

  for (uint32_t i = 0; i < len; i++) {
    if (i % cols == 0) {
      DEBUG_PRINTF("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    DEBUG_PRINTF("%02X ", *src);
    src++;
  }
  DEBUG_PRINTF("\n");
  #endif
}


void webSocketEvent(const uint8_t& num, const WStype_t& type, uint8_t* payload, const size_t& length) {
  switch (type) {
    case WStype_DISCONNECTED:
      DEBUG_PRINTF("[%u] Disconnected!\n", num);
      front_turn(0, 0);
      u8x8.clearLine(3);
      u8x8.setCursor(0, 3);
      u8x8.print("Disconnected");
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        DEBUG_PRINTF("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.sendTXT(num, "Connected");
        u8x8.clearLine(3);
        u8x8.setCursor(0, 3);
        u8x8.print("connected");
      }
      break;

    case WStype_TEXT:
      DEBUG_PRINTF("[%u] get Text: %s\n", num, payload);
      handle_controller((const char*)payload);
      webSocket.sendTXT(num, "OK");
      break;

    case WStype_BIN:
      DEBUG_PRINTF("[%u] get binary length: %u\n", num, length);
      if (length >= 2 && payload[0] == 0xAA && payload[1] == 0xAA) {
        // 心跳包
        DEBUG_PRINTLN("heart beat");
        heart = 1;
      }
      hexdump(payload, length);
      webSocket.sendBIN(num, payload, length);
      break;

    case WStype_ERROR:
      front_turn(0, 0);
      u8x8.clearLine(3);
      u8x8.setCursor(0, 3);
      u8x8.print("connect error");
      break;

    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;

    default:
      break;
  }
}


void setup() {
  Serial.begin(115200);
  #if DEBUG_MODE
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("*ESP32  Remote Control Car");
  Serial.println("--------------------------------------------------------");
  #endif

  init_motor();
  u8x8.begin();
  init_wifi();
  init_server();
  response_broadcast();
  checkHeartBeat();
}


void pre(void) {
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.clear();
}


void response_broadcast() {
  if (udp.listen(LISTEN_PORT)) {
    DEBUG_PRINT("UDP Listening on IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
    udp.onPacket([](AsyncUDPPacket packet) {
      #if DEBUG_MODE
      DEBUG_PRINT("UDP Packet Type: ");
      DEBUG_PRINT(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
      DEBUG_PRINT(", From: ");
      DEBUG_PRINT(packet.remoteIP());
      DEBUG_PRINT(":");
      DEBUG_PRINT(packet.remotePort());
      DEBUG_PRINT(", To: ");
      DEBUG_PRINT(packet.localIP());
      DEBUG_PRINT(":");
      DEBUG_PRINT(packet.localPort());
      DEBUG_PRINT(", Length: ");
      DEBUG_PRINT(packet.length());
      DEBUG_PRINT(", Data: ");
      DEBUG_WRITE(packet.data(), packet.length());
      DEBUG_PRINTLN();
      #endif

      if (strncmp((const char*)packet.data(), "scanCar", 7) == 0) {
        DEBUG_PRINT(myIP.toString());
        packet.print(myIP.toString());
      }
    });
  }
}

void init_server() {
  DEBUG_PRINTLN("*init websocket*");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  DEBUG_PRINT("WebSockets Server started @ IP address: ");
  DEBUG_PRINT(WiFi.localIP());
  DEBUG_PRINT(", port: ");
  DEBUG_PRINTLN(WS_PORT);
}

void handle_controller(const char* payload) {
  // Clear the static JSON document for reuse
  jsonDoc.clear();
  DeserializationError error = deserializeJson(jsonDoc, payload);

  if (error) {
    DEBUG_PRINTLN("deserializeJson failed");
    return;
  }

  int LF = jsonDoc["LF"];
  int RF = jsonDoc["RF"];
  int LB = jsonDoc["LB"];
  int RB = jsonDoc["RB"];

  // Validate input ranges
  if (LF < -255 || LF > 255 || RF < -255 || RF > 255 ||
      LB < -255 || LB > 255 || RB < -255 || RB > 255) {
    DEBUG_PRINTLN("Invalid motor values");
    return;
  }

  if (LF == LB && RF == RB) {
    // 边轮模式
    if (LF >= 0 && RF >= 0) {
      front_turn(LF, RF);
    } else if (LF <= 0 && RF <= 0) {
      back_turn(abs(LF), abs(RF));
    } else if (LF < 0 && RF > 0) {
      left_tank_turn(abs(LF));
    } else if (LF > 0 && RF < 0) {
      right_tank_turn(abs(LF));
    }
  } else {
    //TODO 麦克纳姆轮模式
  }
}

void init_wifi() {
  char chip_id[15];
  snprintf(chip_id, 15, "%04X", (uint16_t)(ESP.getEfuseMac() >> 32));
  String hostname = "esp32car-" + String(chip_id);
  DEBUG_PRINTLN();
  DEBUG_PRINT("Hostname: ");
  DEBUG_PRINTLN(hostname);

  WiFi.mode(WIFI_STA);
  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
  DEBUG_PRINTLN();
  DEBUG_PRINT("Connecting to: ");
  DEBUG_PRINTLN(sta_ssid);

  // Non-blocking WiFi connection with better performance
  unsigned long startTime = millis();
  const unsigned long timeout = 10000;

  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
    delay(100); // Reduced delay for faster response
    DEBUG_PRINT(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINTLN();
    DEBUG_PRINTLN("*WiFi-STA-Mode*");
    DEBUG_PRINT("IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
    myIP = WiFi.localIP();
    pre();
    u8x8.setCursor(0, 0);
    u8x8.clearLine(0);
    u8x8.print("*WiFi-STA-Mode*");
    delay(1000); // Reduced delay
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostname.c_str());
    myIP = WiFi.softAPIP();
    DEBUG_PRINTLN();
    DEBUG_PRINT("WiFi failed to connect to ");
    DEBUG_PRINTLN(sta_ssid);
    DEBUG_PRINTLN();
    DEBUG_PRINTLN("*WiFi-AP-Mode*");
    DEBUG_PRINT("AP IP address: ");
    DEBUG_PRINTLN(myIP);
    pre();
    u8x8.setCursor(0, 0);
    u8x8.clearLine(0);
    u8x8.print("*WiFi-AP-Mode*");
    delay(1000); // Reduced delay
  }

  u8x8.setCursor(0, 1);
  u8x8.clearLine(1);
  u8x8.print(myIP);
  u8x8.setCursor(0, 2);
  u8x8.clearLine(2);
  u8x8.print("ready...");
}



void loop() {

  webSocket.loop();

}
