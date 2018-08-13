#include <PubSubClient.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

 
const char* ssid = "AlphaDevs";
const char* password =  "0777117477";
const char* mqttServer = "m21.cloudmqtt.com";
const int mqttPort = 10488;
const char* mqttUser = "xzmdihpb";
const char* mqttPassword = "p013ayxc-3FB";
 
WiFiClient espClient;
PubSubClient client(espClient);
 
void setup() {
 
  Serial.begin(115200);
  pinMode(2,OUTPUT);
  pinMode(0,OUTPUT);
  digitalWrite(0,HIGH);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2,HIGH);
    delay(500);
    Serial.print(".");
    digitalWrite(2,LOW);
  }
  Serial.println("Connected");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("MQTT.. Connecting");
 
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
 
      Serial.println("MQTT connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
  client.publish("esp/test", "Hello from ESP8266 V7");
  client.subscribe("esp/test");
 
}
 
void callback(char* topic, byte* payload, unsigned int length) {
  String receivedMessage = "";
  Serial.print("Topic :");
  Serial.println(topic);
 
  Serial.print("Message :");

   
  for (int i = 0; i < length; i++) {
    receivedMessage = receivedMessage + ((char)payload[i]);
  }
 
  Serial.println(receivedMessage);
  Serial.println("---");

  if(receivedMessage == "off"){
    Serial.println("Light : OFF");
    digitalWrite(0,HIGH);
    
  }else if(receivedMessage == "on"){
    digitalWrite(0,LOW);
    Serial.println("Light : ON");
  }else if(receivedMessage == "update"){
    Serial.println("Update Trigered");
    OverTheAirUpdate();
  }
 
}
 
void loop() {
  client.loop();
}

void OverTheAirUpdate(){
  // wait for WiFi connection
    if((WiFi.status() == WL_CONNECTED)) {
        Serial.println("Update Started.......");
        client.publish("esp/test", "Update Started.......");
        t_httpUpdate_return ret = ESPhttpUpdate.update("http://www.shopobag.com/Arduino/PubSubWifiMQTT.ino.bin");
        //t_httpUpdate_return  ret = ESPhttpUpdate.update("https://server/file.bin");
        Serial.println("Update Download.......[COMPLETED]");
        client.publish("esp/test", "Update Download.......[COMPLETED]");
        switch(ret) {
            case HTTP_UPDATE_FAILED:
                Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                client.publish("esp/test", "Update Failed");
                break;

            case HTTP_UPDATE_NO_UPDATES:
                Serial.println("HTTP_UPDATE_NO_UPDATES");
                client.publish("esp/test", "HTTP_UPDATE_NO_UPDATES");
                break;

            case HTTP_UPDATE_OK:
                Serial.println("HTTP_UPDATE_OK");
                client.publish("esp/test", "HTTP_UPDATE_OK");
                break;
        }
    }
}

