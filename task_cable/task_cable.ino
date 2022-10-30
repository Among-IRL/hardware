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
const int red10 = 12;
const int green10 = 14;
const int red20 = 27;
const int green20 = 26;
const int red30 = 25;
const int green30 = 33;
const int red11 = 32;
const int green11 = 5;
const int red21 = 0;
const int green21 = 4;
const int red31 = 16;
const int green31 = 17;
bool taskEnabled = false;
bool reset = false;
bool checkCable = false;
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
            reset = true;
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
                if (digitalRead(cableIn1) == LOW) {
                    checkCable = true;
                } 
                if (digitalRead(cableIn2) == LOW) {
                    checkCable = true;
                }
                if (digitalRead(cableIn3) == LOW) {
                    checkCable = true;                  
                }
            } else if (eventName == "disableTaskCable") {
                taskEnabled = false;
                initTask();
                USE_SERIAL.printf("task disabled\n");
            } else if (eventName == "taskCompletedCable") {
                taskEnabled = false;
                USE_SERIAL.printf("task complete\n");
            } else if (eventName == "taskValidCable") {
              String led10 = doc[1]["led10"];
              String led11 = doc[1]["led11"];
              String led20 = doc[1]["led20"];
              String led21 = doc[1]["led21"];
              String led30 = doc[1]["led30"];
              String led31 = doc[1]["led31"];
              taskLed(led10, led11, led20, led21, led30, led31);
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
  checkConnection();
  taskCable();
  delay(500);
}

void setupPin() {
  pinMode(cableIn1, INPUT_PULLUP);
  pinMode(cableIn2, INPUT_PULLUP);
  pinMode(cableIn3, INPUT_PULLUP);
  pinMode(red10, OUTPUT);
  pinMode(green10, OUTPUT);
  pinMode(red20, OUTPUT);
  pinMode(green20, OUTPUT);
  pinMode(red30, OUTPUT);
  pinMode(green30, OUTPUT);
  pinMode(red11, OUTPUT);
  pinMode(green11, OUTPUT);
  pinMode(red21, OUTPUT);
  pinMode(green21, OUTPUT);
  pinMode(red31, OUTPUT);
  pinMode(green31, OUTPUT);
}

void initTask() {
  digitalWrite(red10, HIGH);
  digitalWrite(green10, HIGH);
  digitalWrite(red20, HIGH);
  digitalWrite(green20, LOW);
  digitalWrite(red30, LOW);
  digitalWrite(green30, HIGH);
  digitalWrite(red11, HIGH);
  digitalWrite(green11, HIGH);
  digitalWrite(red21, HIGH);
  digitalWrite(green21, LOW);
  digitalWrite(red31, LOW);
  digitalWrite(green31, HIGH);
  delay(500);
  digitalWrite(red10, HIGH);
  digitalWrite(green10, LOW);
  digitalWrite(red20, LOW);
  digitalWrite(green20, HIGH);
  digitalWrite(red30, HIGH);
  digitalWrite(green30, HIGH);
  digitalWrite(red11, HIGH);
  digitalWrite(green11, LOW);
  digitalWrite(red21, LOW);
  digitalWrite(green21, HIGH);
  digitalWrite(red31, HIGH);
  digitalWrite(green31, HIGH);
  delay(500);
  digitalWrite(red10, LOW);
  digitalWrite(green10, HIGH);
  digitalWrite(red20, HIGH);
  digitalWrite(green20, HIGH);
  digitalWrite(red30, HIGH);
  digitalWrite(green30, LOW);
  digitalWrite(red11, LOW);
  digitalWrite(green11, HIGH);
  digitalWrite(red21, HIGH);
  digitalWrite(green21, HIGH);
  digitalWrite(red31, HIGH);
  digitalWrite(green31, LOW);
  delay(500);
  digitalWrite(red10, HIGH);
  digitalWrite(green10, HIGH);
  digitalWrite(red20, HIGH);
  digitalWrite(green20, LOW);
  digitalWrite(red30, LOW);
  digitalWrite(green30, HIGH);
  digitalWrite(red11, HIGH);
  digitalWrite(green11, HIGH);
  digitalWrite(red21, HIGH);
  digitalWrite(green21, LOW);
  digitalWrite(red31, LOW);
  digitalWrite(green31, HIGH);
  delay(500);
  digitalWrite(red10, HIGH);
  digitalWrite(green10, LOW);
  digitalWrite(red20, LOW);
  digitalWrite(green20, HIGH);
  digitalWrite(red30, HIGH);
  digitalWrite(green30, HIGH);
  digitalWrite(red11, HIGH);
  digitalWrite(green11, LOW);
  digitalWrite(red21, LOW);
  digitalWrite(green21, HIGH);
  digitalWrite(red31, HIGH);
  digitalWrite(green31, HIGH);
  delay(500);
  digitalWrite(red10, LOW);
  digitalWrite(green10, LOW);
  digitalWrite(red20, LOW);
  digitalWrite(green20, LOW);
  digitalWrite(red30, LOW);
  digitalWrite(green30, LOW);
  digitalWrite(red11, LOW);
  digitalWrite(green11, LOW);
  digitalWrite(red21, LOW);
  digitalWrite(green21, LOW);
  digitalWrite(red31, LOW);
  digitalWrite(green31, LOW);
}

void taskCable() {
  if(taskEnabled) {
    // creat JSON message for Socket.IO (event)
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    if (checkCable == false) {
      // add evnet name
      // Hint: socket.on('event_name', ....
      array.add("taskCable");

      // add payload (parameters) for the event
      JsonObject param1 = array.createNestedObject();
      
      if (digitalRead(cableIn1) == LOW) {
          param1["cable1"] = true;
          USE_SERIAL.println("Cable 1");
      }
      if (digitalRead(cableIn2) == LOW) {
          param1["cable2"] = true;
          USE_SERIAL.println("Cable 2");
      }
      if (digitalRead(cableIn3) == LOW) {
          param1["cable3"] = true;
          USE_SERIAL.println("Cable 3");
      }      
    } else {
            // add evnet name
      // Hint: socket.on('event_name', ....
      array.add("alreadyConnect");

      // add payload (parameters) for the event
      JsonObject param1 = array.createNestedObject();
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

void taskLed(String led10, String led11, String led20, String led21, String led30, String led31) {
  if (led10 == "green") {
    digitalWrite(green10, HIGH);
    digitalWrite(red10, LOW);
  } else if ("yellow") {
    digitalWrite(green10, HIGH);
    digitalWrite(red10, HIGH);
  } else if ("red") {
    digitalWrite(green10, LOW);
    digitalWrite(red10, HIGH);    
  } else {
    digitalWrite(green10, LOW);
    digitalWrite(red10, LOW);
  }

  if (led11 == "green") {
    digitalWrite(green11, HIGH);
    digitalWrite(red11, LOW);
  } else if ("yellow") {
    digitalWrite(green11, HIGH);
    digitalWrite(red11, HIGH);
  } else if ("red") {
    digitalWrite(green11, LOW);
    digitalWrite(red11, HIGH);
  } else {
    digitalWrite(green11, LOW);
    digitalWrite(red11, LOW);
  }  

  if (led20 == "green") {
    digitalWrite(green20, HIGH);
    digitalWrite(red20, LOW);
  } else if ("yellow") {
    digitalWrite(green20, HIGH);
    digitalWrite(red20, HIGH);
  } else if ("red") {
    digitalWrite(green20, LOW);
    digitalWrite(red20, HIGH);
  } else {
    digitalWrite(green20, LOW);
    digitalWrite(red20, LOW);
  }  

  if (led21 == "green") {
    digitalWrite(green21, HIGH);
    digitalWrite(red21, LOW);
  } else if ("yellow") {
    digitalWrite(green21, HIGH);
    digitalWrite(red21, HIGH);
  } else if ("red") {
    digitalWrite(green21, LOW);
    digitalWrite(red21, HIGH);
  } else {
    digitalWrite(green21, LOW);
    digitalWrite(red21, LOW);
  }  

  if (led30 == "green") {
    digitalWrite(green30, HIGH);
    digitalWrite(red30, LOW);
  } else if ("yellow") {
    digitalWrite(green30, HIGH);
    digitalWrite(red30, HIGH);
  } else if ("red") {
    digitalWrite(green30, LOW);
    digitalWrite(red30, HIGH);
  } else {
    digitalWrite(green30, LOW);
    digitalWrite(red30, LOW);
  }  

  if (led31 == "green") {
    digitalWrite(green31, HIGH);
    digitalWrite(red31, LOW);
  } else if ("yellow") {
    digitalWrite(green31, HIGH);
    digitalWrite(red31, HIGH);
  } else if ("red") {
    digitalWrite(green31, LOW);
    digitalWrite(red31, HIGH);
  } else {
    digitalWrite(green31, LOW);
    digitalWrite(red31, LOW);
  }  
}

void checkConnection() {
      if (reset) {
      DynamicJsonDocument doc(1024);
      JsonArray array = doc.to<JsonArray>();
      // add evnet name
      // Hint: socket.on('event_name', ....
      array.add("connectEsp");

      // add payload (parameters) for the event
      JsonObject param1 = array.createNestedObject();
      param1["module"] = "CABLE";
      // JSON to String (serializion)
      String output;
      serializeJson(doc, output);

      // Print JSON for debugging
      USE_SERIAL.println(output);

      // Send event
      socketIO.sendEVENT(output);
      reset = false;
    }
}
