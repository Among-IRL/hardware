#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include <ArduinoJson.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#define USE_SERIAL Serial

WiFiMulti WiFiMulti;
SocketIOclient socketIO;

const int cableIn1 = 13;
const int cableIn2 = 15;
const int cableIn3 = 2;
const int red1 = 12;
const int green1 = 14;
const int red2 = 26;
const int green2 = 25;
const int red3 = 32;
const int green3 = 35;
bool taskEnabled = false;
const char* ssid_board = "CABLE";
const char* password_board = "12345678";
const char* ssid = "SFR_45EF";
const char* password = "d9byza2yhvc92dfebfi7";
const char* host = "192.168.1.149";
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
            if (eventName == "enableTaskCable") {
                taskEnabled = true;
                USE_SERIAL.printf("task enabled\n");
            } else if (eventName == "disableTaskCable") {
                taskEnabled = false;
                initTask();
                USE_SERIAL.printf("task disabled\n");
            } else if (eventName == "taskCompletedCable") {
                taskEnabled = false;
                USE_SERIAL.printf("task disabled\n");
            } else if (eventName == "taskValidCable") {
              String led1 = doc[1]["led1"];
              String led2 = doc[1]["led2"];
              String led3 = doc[1]["led3"];
              taskLed(led1, led2, led3);
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
  taskCable();
  delay(500);
}

void setupPin() {
  pinMode(cableIn1, INPUT);
  pinMode(cableIn2, INPUT);
  pinMode(cableIn3, INPUT);
  pinMode(red1, OUTPUT);
  pinMode(green1, OUTPUT);
  pinMode(red2, OUTPUT);
  pinMode(green2, OUTPUT);
  pinMode(red3, OUTPUT);
  pinMode(green3, OUTPUT);
}

void initTask() {
  digitalWrite(red1, HIGH);
  digitalWrite(green1, LOW);
  digitalWrite(red2, HIGH);
  digitalWrite(green2, LOW);
  digitalWrite(red3, HIGH);
  digitalWrite(green3, LOW);
}

void taskCable() {
  if(taskEnabled) {
    // creat JSON message for Socket.IO (event)
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    // add evnet name
    // Hint: socket.on('event_name', ....
    array.add("taskCable");

    // add payload (parameters) for the event
    JsonObject param1 = array.createNestedObject();
    
    if (digitalRead(cableIn1) == HIGH) {
        param1["cable1"] = digitalRead(cableIn1);
    }
    if (digitalRead(cableIn2) == HIGH) {
        param1["cable2"] = digitalRead(cableIn2);
    }
    if (digitalRead(cableIn3) == HIGH) {
        param1["cable3"] = digitalRead(cableIn3);
    }
    
    // JSON to String (serializion)
    String output;
    serializeJson(doc, output);

    // Send event
    socketIO.sendEVENT(output);

    // Print JSON for debugging
    USE_SERIAL.println(output);
  }
}

void taskLed(String led1, String led2, String led3) {
  if (led1 == "green") {
    digitalWrite(green1, HIGH);
    digitalWrite(red1, LOW);
  } else {
    digitalWrite(red1, HIGH);
    digitalWrite(green1, LOW);
  }
  if (led2 == "green") {
    digitalWrite(green2, HIGH);
    digitalWrite(red2, LOW);
  } else {
    digitalWrite(red2, HIGH);
    digitalWrite(green2, LOW);
  }
  if (led3 == "green") {
    digitalWrite(green3, HIGH);
    digitalWrite(red3, LOW);
  } else {
    digitalWrite(red3, HIGH);
    digitalWrite(green3, LOW);
  }
}