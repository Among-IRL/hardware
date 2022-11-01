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

const int button1 = 13;
const int button2 = 15;
const int button3 = 2;
const int button4 = 0;
const int led1 = 12;
const int led2 = 14;
const int led3 = 27;
const int led4 = 26;
String ledRobot = "";
bool taskEnabled = false;
bool reset = false;
bool send1 = true;
bool send2 = true;
bool send3 = true;
bool send4 = true;
const char* ssid_board = "SIMON";
const char* password_board = "12345678";
const char* ssid = "ROUTER";
const char* password = "12345678";
const char* host = "192.168.4.2";
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
            DynamicJsonDocument doc(102400);           
            DeserializationError error = deserializeJson(doc, payload, length);
            if(error) {
                USE_SERIAL.print(F("deserializeJson() failed: "));
                USE_SERIAL.println(error.c_str());
                return;
            }


            String eventName = doc[0];
            if (eventName == "enableTaskSimon") {
                taskEnabled = true;
                USE_SERIAL.printf("task enabled\n");
            } else if (eventName == "disableTaskSimon") {
                taskEnabled = false;
                initTask();
                USE_SERIAL.printf("task disabled\n");
            } else if (eventName == "taskCompletedSimon") {
                taskEnabled = false;
                initTask();
                USE_SERIAL.printf("task disabled\n");
            } else if (eventName == "taskLedSimon") {
                ledRobot = doc[1]["led"].as<String>();
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
  if(taskEnabled) {    
      taskLed(ledRobot);
      checkConnection();
      taskSimon();
   }
}

void setupPin() {
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);
  pinMode(button4, INPUT_PULLUP);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
}

void initTask() {
  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);
  digitalWrite(led4, HIGH);
  delay(500);
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  digitalWrite(led4, LOW);
  delay(500);
  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);
  digitalWrite(led4, HIGH);
  delay(500);
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  digitalWrite(led4, LOW);
  send1 = true;
  send2 = true;
  send3 = true;
  send4 = true;
}

void taskSimon() {

  // creat JSON message for Socket.IO (event)
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();

  // add evnet name
  // Hint: socket.on('event_name', ....
  array.add("taskSimon");

  // add payload (parameters) for the event
  JsonObject param1 = array.createNestedObject();
  

    if (digitalRead(button1) == LOW) {
        Serial.println("Push led1");
        param1["led"] = "led1";
        if (send1) {
          sendData(doc);       
        }   
        send1 = false; 
    } else {
      send1 = true;
    }
    if (digitalRead(button2) == LOW) {
        Serial.println("Push led2");
        param1["led"] = "led2";
        if (send2) {
          sendData(doc);       
        }   
        send2 = false; 
    } else {
        send2 = true;
    }
    if (digitalRead(button3) == LOW) {
        Serial.println("Push led3");
        param1["led"] = "led3";
        if (send3) {
          sendData(doc);       
        }   
        send3 = false; 
    } else {
        send3 = true;
    }
      if (digitalRead(button4) == LOW) {
        Serial.println("Push led4");
        param1["led"] = "led4";
        if (send4) {
          sendData(doc);       
        }   
        send4 = false; 
    } else {
        send4 = true;
    }

    // JSON to String (serializion)

  

}

void taskLed(String led) {
  Serial.println(led);
  if (led == "led1") {
    digitalWrite(led1, HIGH);
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);
    digitalWrite(led4, LOW);
  } else if (led == "led2") {
    digitalWrite(led1, LOW);
    digitalWrite(led2, HIGH);
    digitalWrite(led3, LOW);
    digitalWrite(led4, LOW);
  } else if (led == "led3") {
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    digitalWrite(led3, HIGH);
    digitalWrite(led4, LOW);
  } else if (led == "led4") {
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);
    digitalWrite(led4, HIGH);
  } else {
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);
    digitalWrite(led4, LOW);
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
      param1["module"] = "SIMON";
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

void sendData(ArduinoJson6194_F1::DynamicJsonDocument doc) {
    String output;
    serializeJson(doc, output);
    
    // Send event
    Serial.println("SEND");
    socketIO.sendEVENT(output);
    // Print JSON for debugging
    USE_SERIAL.println(output);
}