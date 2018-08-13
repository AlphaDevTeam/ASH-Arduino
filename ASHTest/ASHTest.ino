#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include "DHT.h"

#define RESET_PIN D7
#define DOOR_PIN D5
#define LED_PIN1 2
#define LED_PIN2 0
#define DHTPIN D6
#define DHTTYPE DHT22     // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

#ifdef MQTT_KEEPALIVE //if the macro MQTT_KEEPALIVE is defined 
#undef MQTT_KEEPALIVE //un-define it
#define MQTT_KEEPALIVE 02//redefine it with the new value
#endif 

WiFiClient espClient;
PubSubClient client(espClient);

boolean resetConfig = false;
boolean sensorStatusUpdated = false;
char* doorStatus = "N/A";
char* sensorStatus = "N/A";

char cTopicTemp[50];
char cTopicHumid[50];
char cTopicDoor[50];

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6] = "1883";
int mqttPort = 1883;
char customer_token[34] = "CUST_CODE";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
   dht.begin();

  pinMode(RESET_PIN, INPUT);
  pinMode(DOOR_PIN, INPUT);
  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  if ( digitalRead(RESET_PIN) == HIGH ) { 
    resetConfig = true;
  }
  //clean FS, for testing
  if(resetConfig){
    SPIFFS.format();
  }
  

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          Serial.println("JSON Received");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(customer_token, json["customer_token"]);

          mqttPort = atoi(mqtt_port);
          

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read



  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_customer_token("blynk", "Customer Token", customer_token, 32);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_customer_token);

  //reset settings - for testing
  if(resetConfig){
    wifiManager.resetSettings();
  }

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(customer_token, custom_customer_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["customer_token"] = customer_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  client.setCallback(callback);
  client.setServer(mqtt_server, mqttPort);
  Serial.println("Starting Connection with Keep Alive set to : " + MQTT_KEEPALIVE);
  digitalWrite(LED_PIN1,LOW); 
  delay(500);
  digitalWrite(LED_PIN1,HIGH);
  delay(500);
  digitalWrite(LED_PIN1,LOW); 
  delay(500);
  digitalWrite(LED_PIN1,HIGH); 
  delay(500);
  digitalWrite(LED_PIN1,LOW); 

  
  strcpy(cTopicTemp,"/home/");
  strcat(cTopicTemp,customer_token);
  strcat(cTopicTemp,"/temp");

  strcpy(cTopicHumid,"/home/");
  strcat(cTopicHumid,customer_token);
  strcat(cTopicHumid,"/humid");

  strcpy(cTopicDoor,"/home/");
  strcat(cTopicDoor,"serverRoom");
  strcat(cTopicDoor,"/door");
 
}

void loop() {
 
  // put your main code here, to run repeatedly:
  int resetCount = 0; 
  while (!client.connected()) {
    digitalWrite(LED_PIN1,HIGH);
    Serial.println("MQTT.. Connecting");
    Serial.println("MQTT.. Auth");
    
    //boolean connect (clientID, username, password, willTopic, willQoS, willRetain, willMessage)
    if (client.connect(customer_token,"","","/home/livingroom/will",2,1,"OFFLINE")) { 
      Serial.println("MQTT connected");  
      digitalWrite(LED_PIN1,LOW); 
    } else { 
      Serial.print("failed with state ");
      Serial.println(client.state());
      digitalWrite(LED_PIN1,HIGH);
      delay(2000);
      resetCount++;
      if(resetCount >10){
        resetCount = 0;
        ESP.reset();
        Serial.println("ESP Reset");
      }
      Serial.println("Reset Count : " + resetCount);
     digitalWrite(LED_PIN1,HIGH);
    } 
  }
   client.loop();
   
   //Check if the Door is Open
   if ( digitalRead(DOOR_PIN) == LOW ) { 
    doorStatus = "OPEN";
   }else{
    doorStatus = "CLOSED";
   }
   delay(1000);
   float fHumidity = dht.readHumidity();
   // Read temperature as Celsius (the default)
   float fTemperature = dht.readTemperature();
   // Read temperature as Fahrenheit (isFahrenheit = true)
   float fTemperatureFahrenheit = dht.readTemperature(true);
   
  //for a float f, f != f will be true only if f is NaN.
   if( fTemperature != fTemperature || fHumidity != fHumidity){
      fTemperature = 0;
      fHumidity = 0;
      //Fix Me - Workaround
      client.publish(cTopicTemp, "0" );
      Serial.println(cTopicTemp);
      client.publish(cTopicHumid, "0");
      updateSensorStatus("SENSOR - ERROR");
   }else{
      updateSensorStatus("ONLINE");
   }

   char cTemperature[10];
   String sTemperature = dtostrf(fTemperature, 5, 2, cTemperature);
   char cHumidity[10];
   String sHumidity = dtostrf(fHumidity, 5, 2, cHumidity);

   Serial.print("Temperature Sensor: ");
   Serial.println(sTemperature + " / " + cTemperature );
   Serial.print("Humidity Sensor: ");
   Serial.println(sHumidity + " / " + cHumidity );
   
   client.publish(cTopicTemp, cTemperature );
   client.publish(cTopicHumid, cHumidity);
   client.publish(cTopicDoor, doorStatus);
   
   if(sensorStatusUpdated){
    client.publish("/home/livingroom/will", sensorStatus);
    Serial.println("Sensor Status sent");
   }
  
   digitalWrite(LED_PIN1,LOW); 
}

void updateSensorStatus(char* newSensorStatus){
  Serial.print("Updating Sensor Status.........." );
  if(sensorStatus != newSensorStatus){
    sensorStatus = newSensorStatus;
    sensorStatusUpdated = true;
    Serial.println(" [DONE]" );
  }else{
    sensorStatusUpdated = false;
    Serial.println(" [SKIPED]" );
  }
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
  
  if(topic == "/home/livingroom/light" && receivedMessage =="ON"){
    Serial.println("Light : ON");
    digitalWrite(LED_PIN1,HIGH);
    digitalWrite(LED_PIN2,HIGH);    
  }else if(topic == "/home/livingroom/light" && receivedMessage =="OFF"){
    Serial.println("Light : OFF");
    digitalWrite(LED_PIN1,LOW); 
    digitalWrite(LED_PIN2,LOW);       
  }  
 
}
 
