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

const int manivellePin = 12;
const int manivelleDt = 14;
const int manivelleClk = 27;

const int ledValidation = 26;

const int green = 33;
const int red = 25;
const int blue = 32;

bool taskEnabled = true;

const char* ssid_board = "MANIVELLE";
const char* password_board = "12345678";
const char* ssid = "ldqtheone";
const char* password = "chass6000";
const char* host = "192.168.43.7";
const int port = 3000;
const char* path = "/socket.io/?EIO=4";

// Initialisation des variables nécessaires
int compteur = 0; 
bool direction;
int pinClkLast;  
int pinClkActual;

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
            if (eventName == "enableTaskManivelle") {
                taskEnabled = true;
                USE_SERIAL.printf("task enabled\n");
            } else if (eventName == "disableTaskManivelle") {
                taskEnabled = false;
                initTask();
                USE_SERIAL.printf("task disabled\n");
            } else if (eventName == "taskCompletedManivelle") {
                taskEnabled = false;
                USE_SERIAL.printf("task disabled\n");
            } else if (eventName == "taskLedManivelle") {
              String led = doc[1]["led"];
              taskLed(led);
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
  taskManivelle();
}

void setupPin() {
  pinMode(manivellePin, INPUT);
  pinMode(manivelleClk, INPUT);
  pinMode(manivelleDt, INPUT);

  pinMode(ledValidation, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(blue, OUTPUT);
}

void initTask() {
  digitalWrite(ledValidation, LOW);
  digitalWrite(green, LOW);
  digitalWrite(red, LOW);
  digitalWrite(blue, LOW);

  // ...et activation de leurs résistances de PULL-UP
  digitalWrite(manivelleClk, true);
  digitalWrite(manivelleDt, true);
  digitalWrite(manivellePin, true);

  // Lecture initiale de Pin_CLK
  pinClkLast = digitalRead(manivelleClk);   
}

void taskManivelle() {
  if(taskEnabled) {
    // creat JSON message for Socket.IO (event)
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    // add evnet name
    // Hint: socket.on('event_name', ....
    array.add("taskManivelle");

    // add payload (parameters) for the event
    JsonObject param1 = array.createNestedObject();
    
    // Lecture des statuts actuels
    pinClkActual = digitalRead(manivelleClk);
      
    // Vérification de changement
    if (pinClkActual != pinClkLast)
    {       
      if (digitalRead(manivelleDt) != pinClkActual) 
      {  
        // Pin_CLK a changé en premier
        compteur ++;
        direction = true;
      } else { 
        // Sinon Pin_DT achangé en premier
        direction = false;
        compteur--;
      }

      Serial.println("Rotation detectee: ");
      Serial.print("Sens de rotation: ");
      
      if(direction)
      {
        Serial.println("dans le sens des aiguilles d'une montre");
      } else {
        Serial.println("dans le sens contraire des aiguilles d'une montre");
      }
      
      Serial.print("Position actuelle: ");
      Serial.println(compteur);
      Serial.println("------------------------------");         
    } 
    
    // Préparation de la prochaine exécution:
    pinClkLast = pinClkActual;
    
    // fonction Reset remise à la position actuelle
    // if (!digitalRead(manivellePin) && compteur!= 0)
    // {
    //   compteur = 0;
    //   Serial.println("Position reset");
    // }
    
    // JSON to String (serializion)
    String output;
    serializeJson(doc, output);

    // Send event
    socketIO.sendEVENT(output);
  }
}

void taskLed(String led) {
  if (led == "green") {
    digitalWrite(green, HIGH);
    digitalWrite(red, LOW);
    digitalWrite(blue, LOW);
    digitalWrite(ledValidation, LOW);
  } else if (led == "red") {
    digitalWrite(green, LOW);
    digitalWrite(red, HIGH);
    digitalWrite(blue, LOW);
    digitalWrite(ledValidation, LOW);
  } else if (led == "blue") {
    digitalWrite(green, LOW);
    digitalWrite(red, LOW);
    digitalWrite(blue, HIGH);
    digitalWrite(ledValidation, LOW);
  } else {
    digitalWrite(green, LOW);
    digitalWrite(red, LOW);
    digitalWrite(blue, LOW);
    digitalWrite(ledValidation, HIGH);
  }
}