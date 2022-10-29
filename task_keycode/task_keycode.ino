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

const int ledValidation = 12;

const byte ROWS = 4; 
const byte COLS = 4; 
const int led = 12;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {13, 15, 2, 0}; 
byte colPins[COLS] = {4, 16, 17, 5}; 

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

bool taskEnabled = false;

const char* ssid_board = "KEYCODE";
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
            if (eventName == "enableTaskKeyCode") {
                taskEnabled = true;
                USE_SERIAL.printf("task enabled\n");
            } else if (eventName == "disableTaskKeyCode") {
                taskEnabled = false;
                initTask();
                USE_SERIAL.printf("task disabled\n");
            } else if (eventName == "taskCompletedTaskKeyCode") {
                taskEnabled = false;
                digitalWrite(ledValidation, HIGH);
                USE_SERIAL.printf("task completed\n");
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
  taskKeycode();
}

void setupPin() {
  pinMode(ledValidation, OUTPUT);
}

void initTask() {
  digitalWrite(ledValidation, HIGH); 
  delay(500);
  digitalWrite(ledValidation, LOW);  
  delay(500);
  digitalWrite(ledValidation, HIGH); 
  delay(500);
  digitalWrite(ledValidation, LOW); 
  delay(500);
  digitalWrite(ledValidation, HIGH); 
  delay(500);
  digitalWrite(ledValidation, LOW); 
}

void taskKeycode() {
  if(taskEnabled) {
    // creat JSON message for Socket.IO (event)
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    // add evnet name
    // Hint: socket.on('event_name', ....
    array.add("taskKeyCode");

    // add payload (parameters) for the event
    JsonObject param1 = array.createNestedObject();

    char customKey = customKeypad.getKey();

    if (customKey){
      USE_SERIAL.println(customKey);

      param1["keyPressed"] = customKey;

      // JSON to String (serializion)
      String output;
      serializeJson(doc, output);

      USE_SERIAL.println(output);

      // Send event
      socketIO.sendEVENT(output);
    }
  }
}