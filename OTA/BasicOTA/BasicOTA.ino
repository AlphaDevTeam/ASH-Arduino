#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

const char* ssid = "AlphaDevs";
const char* password = "0777117477";
boolean isUpdate = false;
String version = "V.10";

WiFiClient espClient;
PubSubClient client(espClient);

void configureMQTT(){

  const char* mqttServer = "m21.cloudmqtt.com";
  const int mqttPort = 10488;
  const char* mqttUser = "xzmdihpb";
  const char* mqttPassword = "p013ayxc-3FB";
 
  Serial.begin(115200);
  pinMode(2,OUTPUT);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2,HIGH);
    delay(500);
    Serial.println("Connecting to WiFi..");
    digitalWrite(2,LOW);
  }
  Serial.println("Connected to the WiFi network");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
 
      Serial.println("connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }

  String displayMessage = "Hello from ESP8266 " + version;
  char* displayCharArray;
  displayMessage.toCharArray(displayCharArray,displayMessage.length() );
  client.publish("esp/test",  "Hello from ESP8266 V12");
  Serial.println("Running Version : " + version);
  client.subscribe("#");

  
}

void callback(char* topic, byte* payload, unsigned int length) {
  String receivedMessage = "";
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");

   
  for (int i = 0; i < length; i++) {
    receivedMessage = receivedMessage + ((char)payload[i]);
  }
 
  Serial.println(receivedMessage);
  Serial.println("-----------------------");

  if(receivedMessage == "mobile"){
    Serial.println("trigger : ON");
    digitalWrite(2,HIGH);
  }else if(receivedMessage == "computer"){
    digitalWrite(2,LOW);
    Serial.println("trigger : OFF");
  }else if(receivedMessage == "update"){
    isUpdate = true;
    client.publish("esp/test", "Going to Update Mode ........");
    Serial.println("Going to Update Mode ........");
    delay(1000);
    client.publish("esp/test", "Going to Update Mode ........ configureOTA");
    configureOTA();
    loop();
    client.publish("esp/test", "Going to Update Mode ........ handleUpdate");
    client.publish("esp/test", "Going to Update Mode ........ [DONE]");  
    Serial.println("Going to Update Mode ........[DONE]");
    client.publish("esp/test", "Going to Update Mode ........ [DONE]");
  }
 
}

void configureOTA(){
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
    //client.publish("esp/test", "Start updating ");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    //client.publish("esp/test", "END ");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    //client.publish("esp/test", "progress ");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    //client.publish("esp/test", "error");
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  //client.publish("esp/test", "Going to Update Mode ........ Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //client.publish("esp/test", "Going to Update Mode ........ IP Set To");
 // client.publish("esp/test", WiFi.localIP());
  
}

void setup() {
  Serial.println("Configuring MQTT...");
  configureMQTT();
  Serial.println("Configuring MQTT... [DONE]ss");
}

void loop() {
  if(isUpdate){
     ArduinoOTA.handle();
     //client.publish("esp/test", "waiting for the update");
  }else{
    client.loop();
  }
 
}
