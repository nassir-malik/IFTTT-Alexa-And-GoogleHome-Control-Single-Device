/*
 *Netmedias
 *
 *  Created on: 20.08.2015
 *  
 */
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Hash.h>

// @@@@@@@@@@@@@@@ You only need to midify modify wi-fi and domain info @@@@@@@@@@@@@@@@@@@@
const char* ssid     = "enter your ssid"; //enter your ssid/ wi-fi(case sensitiv) router name - 2.4 Ghz only
const char* password = "enter ssid password";     // enter ssid password (case sensitiv)
char host[] = "alexaskillsiot.herokuapp.com"; //enter your Heroku domain name like "espiot.herokuapp.com" 
int sensor_distance_from_door = 5; // enter the distance in inches between you ultrasonic sensor and garage door
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//Firstly the connections of ultrasonic Sensor.Connect 3.9v to +5v and GND. Trigger pin to 5 & echo pin to 4. 
#define trigPin 5
#define echoPin 4
int duration, distance;
const int relayPin1 = 16; //Garage door 1

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

int port = 80;
char path[] = "/ws"; 
ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

DynamicJsonBuffer jsonBuffer;
String currState, oldState, message;
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) { 


    switch(type) {
        case WStype_DISCONNECTED:
            //USE_SERIAL.printf("[WSc] Disconnected!\n");
           Serial.println("Disconnected! ");
            break;
            
        case WStype_CONNECTED:
            {
             Serial.println("Connected! ");
			    // send message to server when Connected
				    webSocket.sendTXT("Connected");
            }
            break;
            
        case WStype_TEXT:
            Serial.println("Got data");
            //Serial.println("looping...");
            digitalWrite(trigPin, HIGH);
            delayMicroseconds(10);
            digitalWrite(trigPin, LOW);
            duration = pulseIn(echoPin, HIGH);
            distance = (duration/2) / 74; //Inches
            distance = (duration/2) / 29.1; //centimeter
            
            if (distance <= sensor_distance_from_door ){
              //Serial.print(distance);
              currState = "open";
              //Serial.println(currState);
            }else{
              //Serial.println(currState);
              currState = "close";
            }
            
            processWebScoketRequest((char*)payload);

            break;
            
        case WStype_BIN:

            hexdump(payload, length);
            Serial.print("Got bin");
            // send data to server
            // webSocket.sendBIN(payload, length);
            break;
    }

}

void setup() {
    Serial.begin(115200);

    pinMode(trigPin, OUTPUT); 
    pinMode(echoPin, INPUT);

    
    Serial.setDebugOutput(true);
    
    pinMode(relayPin1, OUTPUT);

    
      for(uint8_t t = 4; t > 0; t--) {
          delay(1000);
      }
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    
    //Serial.println(ssid);
    WiFiMulti.addAP(ssid, password);

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
        delay(1000);
    }
    Serial.println("Connected to wi-fi");
    webSocket.begin(host, port, path);
    webSocket.onEvent(webSocketEvent);

}

void loop() {

    webSocket.loop();
    //delay(500);
}


void processWebScoketRequest(String data){
            String jsonResponse = "{\"version\": \"1.0\",\"sessionAttributes\": {},\"response\": {\"outputSpeech\": {\"type\": \"PlainText\",\"text\": \"<text>\"},\"shouldEndSession\": true}}";
            JsonObject& req = jsonBuffer.parseObject(data);

            String instance = req["instance"];
            String state = req["state"];
            String query = req["query"];
            String message = "{\"event\": \"OK\"}";
            
            Serial.println("Data2-->"+data);
            Serial.println("State-->" + state);

            if(query == "?"){ //if command then execute
              Serial.println("Recieved query!");
                 if(currState=="open"){
                      message = "open";
                    }else{
                      message = "closed";
                    }
                   jsonResponse.replace("<text>", "Garage door " + instance + " is " + message );
                   webSocket.sendTXT(jsonResponse);
                   
            }else if(query == "cmd"){ //if query check state
              Serial.println("Recieved command!");
                   if(state != currState){
                         if(currState == "close"){
                            message = "opening";
                          }else{
                            message = "closing";
                          }
                          digitalWrite(relayPin1, HIGH);
                          delay(1000);
                          digitalWrite(relayPin1, LOW);
                   }else{
                          if(currState == "close"){
                            message = "already closed";
                          }else{
                            message = "already open";
                          }
                    }
                  jsonResponse.replace("<text>", "Garage door " + instance + " is " + message );
                  webSocket.sendTXT(jsonResponse);

            
            }else{//can not recognized the command
                    Serial.println("Command is not recognized!");
                   jsonResponse.replace("<text>", "Command is not recognized by garage door Alexa skill");
                   webSocket.sendTXT(jsonResponse);
            }
            Serial.print("Sending response back");
            Serial.println(jsonResponse);
                  // send message to server
                  webSocket.sendTXT(jsonResponse);
}



