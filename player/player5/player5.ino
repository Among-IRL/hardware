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

const int red1 = 14;
const int red2 = 26;

const int green1 = 12;
const int green2 = 27;

String deadPlayer = "";
String deadPlayerReported = "";
String ledPlayer = "green";

bool playerEnabled = false;
bool reset = false;

const char* ssid_board = "PLAYER5";
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
             if (eventName == "enablePlayer") {
                playerEnabled = true;
                USE_SERIAL.printf("player enabled\n");
            } else if (eventName == "disablePlayer") {
                playerEnabled = false;
                initPlayer();
                USE_SERIAL.printf("player disabled\n");
            } else if (eventName == "deathPlayer") {
                USE_SERIAL.printf("Death player : \n");
                deadPlayer = doc[1]["mac"].as<String>();

                if (deadPlayer == ssid_board) {
                  ledPlayer = "red";
                  USE_SERIAL.printf("Player as been killed\n");
                }

                USE_SERIAL.println(doc[1].as<String>());
                USE_SERIAL.println(ledPlayer);
            } else if (eventName == "deadReport") {
                USE_SERIAL.printf("Report player : \n");
                deadPlayerReported = doc[1]["macDeadPlayer"].as<String>();

                if (deadPlayerReported == ssid_board) {
                  ledPlayer = "yellow";
                  USE_SERIAL.printf("Player is now a ghost\n");
                }

                USE_SERIAL.println(doc[1].as<String>());
                USE_SERIAL.println(ledPlayer);
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
    initPlayer();
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
  if(playerEnabled) {
    playerLed(ledPlayer);
  }
  delay(500);
}

void setupPin() {
  pinMode(red1, OUTPUT);
  pinMode(red2, OUTPUT);
  pinMode(green1, OUTPUT);
  pinMode(green2, OUTPUT);
}

void initPlayer() {
  digitalWrite(red1, HIGH);
  digitalWrite(red2, HIGH);
  digitalWrite(green1, LOW);
  digitalWrite(green2, LOW);
  delay(500);
  digitalWrite(green1, HIGH);
  digitalWrite(green2, HIGH);
  digitalWrite(red1, LOW);
  digitalWrite(red2, LOW);
  delay(500);
  digitalWrite(red1, HIGH);
  digitalWrite(red2, HIGH);
  digitalWrite(green1, LOW);
  digitalWrite(green2, LOW);
  delay(500);
  digitalWrite(green1, HIGH);
  digitalWrite(green2, HIGH);
  digitalWrite(red1, LOW);
  digitalWrite(red2, LOW);
  delay(500);
}

void playerLed(String led) {
  if (led == "green") {
    digitalWrite(green1, HIGH);
    digitalWrite(green2, HIGH);
    digitalWrite(red1, LOW);
    digitalWrite(red2, LOW);
    USE_SERIAL.printf("Player is alive \n");
  } else if (led == "red") {
    digitalWrite(green1, LOW);
    digitalWrite(green2, LOW);
    digitalWrite(red1, HIGH);
    digitalWrite(red2, HIGH);
    delay(250);
    digitalWrite(red1, LOW);
    digitalWrite(red2, LOW);
    delay(250);
    digitalWrite(red1, HIGH);
    digitalWrite(red2, HIGH);
    delay(250);
    USE_SERIAL.printf("Player is dead : \n");
  } else if (led == "yellow") {
    digitalWrite(green1, LOW);
    digitalWrite(green2, LOW);
    digitalWrite(red1, HIGH);
    digitalWrite(red2, HIGH);
    USE_SERIAL.printf("Player is ghost \n");
  } else {
    digitalWrite(green1, LOW);
    digitalWrite(green2, LOW);
    digitalWrite(red1, LOW);
    digitalWrite(red2, LOW);
    USE_SERIAL.printf("Player is disabled");
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
    param1["module"] = "PLAYER5";
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