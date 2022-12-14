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

const int led1 = 12;
const int obstacleDetector = 13;

bool taskEnabled = false;
bool reset = false;

const char* ssid_board = "CARDSWIPE";
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
            DynamicJsonDocument doc(102400);             
            DeserializationError error = deserializeJson(doc, payload, length);
            if(error) {
                USE_SERIAL.print(F("deserializeJson() failed: "));
                USE_SERIAL.println(error.c_str());
                return;
            }

            String eventName = doc[0];
            if (eventName == "enableTaskCardSwip") {
                taskEnabled = true;
                USE_SERIAL.printf("task enabled\n");
            } else if (eventName == "disableTaskCardSwip") {
                taskEnabled = false;
                initTask();
                USE_SERIAL.printf("task disabled\n");
            } else if (eventName == "taskCompletedTaskCardSwip") {
                taskEnabled = false;
                initTask();
                USE_SERIAL.printf("task disabled\n");
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
  taskCardSwip();
  delay(500);
}

void setupPin() {
  pinMode(led1, OUTPUT);
  pinMode(obstacleDetector, INPUT);
}

void initTask() {
  digitalWrite(led1,HIGH);
  delay(500);
  digitalWrite(led1, LOW);
  delay(500);
   digitalWrite(led1,HIGH);
  delay(500);
  digitalWrite(led1, LOW);
  delay(500);
  digitalWrite(led1,HIGH);
  delay(500);
  digitalWrite(led1, LOW);
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
      param1["module"] = "CARDSWIPE";
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

void taskCardSwip() {
  if(taskEnabled) {
    // creat JSON message for Socket.IO (event)
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    // add evnet name
    // Hint: socket.on('event_name', ....
    array.add("taskCardSwip");

    // add payload (parameters) for the event
    JsonObject param1 = array.createNestedObject();



    bool detectorVal = digitalRead(obstacleDetector); // Lecture de la valeur du signal
   
    if(detectorVal == HIGH) // Si un signal est d??tect??, la diode s'allume
    {
      // USE_SERIAL.println("Pas d'obstacle");
      digitalWrite(led1, LOW);
    }
    else
    {
      param1["isDetected"] = true;
      USE_SERIAL.println("Obstacle detecte");

     // JSON to String (serializion)
      String output;
      serializeJson(doc, output);
      // Print JSON for debugging
      USE_SERIAL.println(output);

      // Send event
      socketIO.sendEVENT(output);
    }
  }
}
