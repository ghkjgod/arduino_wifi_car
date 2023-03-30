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

#define _WEBSOCKETS_LOGLEVEL_ 2


/* Wifi Crdentials */
String sta_ssid = "zhqm";           // set Wifi network you want to connect to
String sta_password = "19870827";  // set password for Wifi network
unsigned long previousMillis = 0;
IPAddress myIP;

AsyncUDP udp;

WebSocketsServer webSocket = WebSocketsServer(WS_PORT);

U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ 21, /* data=*/ 19, /* reset=*/ U8X8_PIN_NONE); 

int LISTEN_PORT = 9567;

void init_wifi();
void init_server();
void handle_controller(char* payload);

void response_broadcast();
// pre for oled
void pre(void);

int heart = 0;
void dispatchHeartBeat(void *pvParameters) {
    int heart_flag = (int) pvParameters;
    int count = 0;
    while(1) {
         Serial.printf("heart %d!\n",heart);
        if(heart == 0){
          count++;
        }else{
          count = 0;
          heart = 0;
        }
        if(count >= 3)
        {
          count = 0;
          Serial.printf("Disconnected!\n");
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
  const uint8_t* src = (const uint8_t*)mem;

  Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);

  for (uint32_t i = 0; i < len; i++) {
    if (i % cols == 0) {
      Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }

    Serial.printf("%02X ", *src);
    src++;
  }

  Serial.printf("\n");
}


void webSocketEvent(const uint8_t& num, const WStype_t& type, uint8_t* payload, const size_t& length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      front_turn(0, 0);
      u8x8.clearLine(3);
      u8x8.setCursor(0, 3);
      u8x8.print("Disconnected");


      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // send message to client
        webSocket.sendTXT(num, "Connected");
        u8x8.clearLine(3);
        u8x8.setCursor(0, 3);
        u8x8.print("connected");
        

      }
      break;

    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      // send message to client
      handle_controller((char*)payload);
      webSocket.sendTXT(num, "OK");
      // send data to all connected clients

      break;

    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      if (payload[0] == 0xAA && payload[1] == 0xAA) {
        // 心跳包
        Serial.println("heart beat");
        heart = 1;
       
      }
      hexdump(payload, length);

      // send message to client
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
  Serial.begin(115200);  // set up seriamonitor at 115200 bps
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("*ESP32  Remote Control Car");
  Serial.println("--------------------------------------------------------");

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
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    udp.onPacket([](AsyncUDPPacket packet) {
      Serial.print("UDP Packet Type: ");
      Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast"
                                                                             : "Unicast");
      Serial.print(", From: ");
      Serial.print(packet.remoteIP());
      Serial.print(":");
      Serial.print(packet.remotePort());
      Serial.print(", To: ");
      Serial.print(packet.localIP());
      Serial.print(":");
      Serial.print(packet.localPort());
      Serial.print(", Length: ");
      Serial.print(packet.length());
      Serial.print(", Data: ");
      Serial.write(packet.data(), packet.length());
      Serial.println();

      if (strcmp((char*)packet.data(), "scanCar") == 0) {
        Serial.print(myIP.toString());
        packet.print(myIP.toString());
      }
    });
  }
}

void init_server() {
  Serial.println("*init websocket*");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.print("WebSockets Server started @ IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print(", port: ");
  Serial.println(WS_PORT);
}

void handle_controller(char* payload) {

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.println("deserializeJson failed");
    return;
  }

  int LF = doc["LF"];
  int RF = doc["RF"];
  int LB = doc["LB"];
  int RB = doc["RB"];

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
  Serial.println();
  Serial.println("Hostname: " + hostname);

  WiFi.mode(WIFI_STA);
  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
  Serial.println("");
  Serial.print("Connecting to: ");
  Serial.println(sta_ssid);
  Serial.print("Password: ");
  Serial.println(sta_password);

  // try to connect with Wifi network about 10 seconds
  unsigned long currentMillis = millis();
  previousMillis = currentMillis;
  while (WiFi.status() != WL_CONNECTED && currentMillis - previousMillis <= 10000) {
    delay(500);
    Serial.print(".");
    currentMillis = millis();
  }

  // if failed to connect with Wifi network set NodeMCU as AP mode

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("*WiFi-STA-Mode*");
    Serial.print("IP: ");
    myIP = WiFi.localIP();
    Serial.println(myIP);
    pre();
    u8x8.setCursor(0, 0);
    u8x8.clearLine(0);
    u8x8.print("*WiFi-STA-Mode*");
    delay(2000);
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostname.c_str());
    myIP = WiFi.softAPIP();
    Serial.println("");
    Serial.println("WiFi failed connected to " + sta_ssid);
    Serial.println("");
    Serial.println("*WiFi-AP-Mode*");
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    pre();
    u8x8.setCursor(0, 0);
    u8x8.clearLine(0);
    u8x8.print("*WiFi-AP-Mode*");
    delay(2000);
  }
  u8x8.setCursor(0, 1);
  u8x8.clearLine(1);
  u8x8.print(myIP);
  u8x8.setCursor(0, 2);
  u8x8.clearLine(2);
  u8x8.print("aleady...");
}



void loop() {

  webSocket.loop();

}
