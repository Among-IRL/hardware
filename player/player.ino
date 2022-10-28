#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include <ArduinoJson.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include <Keypad.h>

#define USE_SERIAL Serial

WiFiMulti WiFiMulti;
SocketIOclient socketIO;

const int red1 = 12;
const int red2 = 26;

const int green1 = 14;
const int green2 = 25;

String ledPlayer = "";

const char* ssid_board = "PLAYER1";
const char* password_board = "12345678";
const char* ssid = "ldqtheone";
const char* password = "chass6000";
const char* host = "192.168.43.7";
const int port = 3000;
const char* path = "/socket.io/?EIO=4";

void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            USE_SERIAL.printf("[IOc] Disconnected!\n");
            break;
        case sIOtype_CONNECT:
            USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);

            // join default namespace (no auto join in Socket.IO V3)
            socketIO.send(sIOtype_CONNECT, "/");
            break;
        case sIOtype_EVENT:
        {
            char * sptr = NULL;
            int id = strtol((char *)payload, &sptr, 10);
            USE_SERIAL.printf("[IOc] get event: %s id: %d\n", payload, id);
            if(id) {
              payload = (uint8_t *)sptr;
            }
            DynamicJsonDocument doc(1024);           
            DeserializationError error = deserializeJson(doc, payload, length);
            if(error) {
                USE_SERIAL.print(F("deserializeJson() failed: "));
                USE_SERIAL.println(error.c_str());
                return;
            }

            String eventName = doc[0];
            if (eventName == "playerLed") {
                ledPlayer = doc[1]["led"].as<String>();
            }
         } 
            break;
        case sIOtype_ACK:
            USE_SERIAL.printf("[IOc] get ack: %u\n", length);
            break;
        case sIOtype_ERROR:
            USE_SERIAL.printf("[IOc] get error: %u\n", length);
            break;
        case sIOtype_BINARY_EVENT:
            USE_SERIAL.printf("[IOc] get binary: %u\n", length);
            break;
        case sIOtype_BINARY_ACK:
            USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
            break;
    }
}

void setup() {
    setupPin();
    initTask();
    USE_SERIAL.begin(115200);
    USE_SERIAL.setDebugOutput(true);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid_board, password_board);

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    WiFiMulti.addAP(ssid, password);

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }

    String ip = WiFi.localIP().toString();
    USE_SERIAL.printf("[SETUP] WiFi Connected %s\n", ip.c_str());

    // server address, port and URL
    socketIO.begin(host, port, path);

    // event handler
    socketIO.onEvent(socketIOEvent);
}

unsigned long messageTimestamp = 0;
void loop() {
  socketIO.loop();
  playerLed(ledPlayer);
}

void setupPin() {
  pinMode(red1, OUTPUT);
  pinMode(red2, OUTPUT);
  pinMode(green1, OUTPUT);
  pinMode(green2, OUTPUT);
}

void initTask() {
  digitalWrite(green1, HIGH);  
  digitalWrite(green2, HIGH);  
  digitalWrite(red1, LOW);  
  digitalWrite(red2, LOW);  
}

void playerLed(String led) {
  if (led == "green") {
    digitalWrite(green, HIGH);
    digitalWrite(red, LOW);
  } else if (led == "red") {
    digitalWrite(green, LOW);
    digitalWrite(red, HIGH);
  } else if (led == "yellow") {
    digitalWrite(green, HIGH);
    digitalWrite(red, HIGH);
  } else {
    digitalWrite(green, LOW);
    digitalWrite(red, LOW);
  }
}