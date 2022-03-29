
//  ---------------------- set up WIFI ---------------------- 
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

//for LED status
#include <Ticker.h>
Ticker ticker;
//  ---------------------- set up LED ---------------------- 
#ifndef LED_BUILTIN
#define LED_BUILTIN 4 // LED: 4 (D2) 
#endif
int LED_warm = 5; // LED : 5 (D1)
int LED_Wifi = LED_BUILTIN;
float limit_t = -15;
float limit_h = -15;

 
//  ---------------------- set up LED ---------------------- 
void tick()
{
  //toggle state
  digitalWrite(LED_Wifi, !digitalRead(LED_Wifi));     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}
 


//   ---------------------- set up CLOUD ---------------------- 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

#include "ThingSpeak.h"
// set up thingspeak
unsigned long myChannelNumber = 1282123; // Replace the 0 with your channel number
const char* myWriteAPIkey = "RLUAXBVF5FU94XC8"; // paste your thingspeak write API key


// set up : Temperature and Humidity sensor
int DHTPin = 14;
DHT dht(DHTPin,DHT11); 
// set up photosresistor sensor
int light_pin = 12;
// wifi client
WiFiClient  client_thingspeak;
//   ---------------------- set up CLOUD ---------------------- 

//  ---------------------- set up Control (MQTT - NodeRed-UI) ---------------------- 
#include <PubSubClient.h>
String flag_continue = "0"; // to permit continue update data
const char* mqtt_server = "broker.mqtt-dashboard.com";
// ----------------------- Set up email ------------------
String email = "3600xu.vn@gmail.com";
// ---------------------- Set up email -----------------

WiFiClient espClient;
PubSubClient client(espClient);


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      
      // ... and resubscribe
      client.subscribe("/garden/light");
      client.subscribe("/garden/humidity");
      client.subscribe("/garden/temperature");
      client.subscribe("/garden/mail");
      client.subscribe("/garden/permit");
            Serial.println("ok subcibe");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message; 
  for (int i = 0; i < length; i++) {
      message += (char)payload[i];
  }
  Serial.println();

  Serial.println(message);

  if (String(topic) == "/garden/light") {
    if(message == "ON"){
      digitalWrite(LED_warm, HIGH);  
    }
  
   else {
    digitalWrite(LED_warm, LOW);  // Turn the LED off by making the voltage HIGH
  }
 
  }
   
  if(String(topic) == "/garden/temperature"){
      limit_t = message.toFloat();
  }

  if(String(topic) == "/garden/humidity"){
      limit_h = message.toFloat();
  }
  if(String(topic) == "/garden/mail"){
      email = message;
     Serial.println(email); 
  }
  if(String(topic) == "/garden/permit"){
     flag_continue = message;
     Serial.println(flag_continue); 
  }  

}
//  ---------------------- set up Control (MQTT - NodeRed-UI) ---------------------- 
void setup() {
  manageWifi(); // set up WIFI first
  Serial.println();

  // set up MQTT:
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  //LED - Warm - WIFI : 
    pinMode(LED_warm, OUTPUT);
    pinMode(LED_Wifi,OUTPUT);
  // Tem - Hum 
  pinMode(DHTPin, INPUT);
  dht.begin();  
  // phtoresistor sensor
  pinMode(light_pin, INPUT);  
  // thingspeak: 
    ThingSpeak.begin(client_thingspeak);   
  


 
}

void loop() {
  // put your main code here, to run repeatedly:
 float t = dht.readTemperature(); // Gets the values of the temperature
 float h = dht.readHumidity(); // Gets the values of the humidity 
 int light_value = analogRead(light_pin);

   if (!client.connected()) {
     reconnect();
   }
   client.loop();
   if(flag_continue == "1"){
       update_data_Control(t,h);
        manage_temperature(t,h,limit_t,limit_h);
        uploadData_to_Cloud(t,h);
   }


}



void uploadData_to_Cloud(float t, float h)
{
 
  int light_value = analogRead(light_pin);

   //Write data to fields
  ThingSpeak.setField(1, t); // setField(field, value)
  ThingSpeak.setField(2, h); // setField(field, value)
  

    //check return code
  int returncode = ThingSpeak.writeFields(myChannelNumber,myWriteAPIkey);
  if(returncode == 200){
    Serial.println("Channel update successful");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(returncode));
  }

  Serial.println("Temperature: ");
  Serial.println(t);
  Serial.println("Humidity: ");
  Serial.println(h);
  Serial.println("Light value: ");
  Serial.println(light_value);

  send_email(t,h);
  
  delay(30000);
}
void manage_temperature(float t, float h, float limit_t, float limit_h){
  if(t < limit_t && h < limit_h){
    digitalWrite(LED_warm,HIGH);
  }
  else{
    digitalWrite(LED_warm,LOW);
  }
  
}

void manageWifi(){
WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  //set led pin as output
  pinMode(LED_Wifi, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  //reset settings - for testing
  // wm.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(LED_Wifi, LOW);
}

void send_email(float t, float h){

  const char* APIkey = "dqIQDOHBKobDCafEw_luyW";
  const char * event = "email";
  const char* host = "maker.ifttt.com";
  String url = String("https://maker.ifttt.com") +String("/trigger/")+event
                    + String("/with/key/") + APIkey + String("?value1=") + t
                    + String("&value2=") + h + String("&value3=") + email ;
  const uint16_t port = 80;
      // put your main code here, to run repeatedly:
  Serial.print("connecting to: ");
  Serial.print(host);
  Serial.print(":");
  Serial.print(port); 
  // use WiFIClient class to create TCP connections
  WiFiClient http;
  if(!http.connect(host,port)){
      Serial.println("connection failed");
      delay(5000);
      return;
    }

   // this will send a request to the server
   http.print(String("GET ")+ url +
                " HTTP/1.1\r\n" +
                "Host: " + host + "\r\n" +
                "Connection: close\r\n\r\n"              
   );

   delay(500);

   //Read all responses from server:
   while(http.available()){
    String line = http.readStringUntil('\R');
    Serial.print(line);
    }
  
    Serial.println();

}

// ----------------------- Code MQTT --------------- //




 void update_data_Control(float t,float h)
 {
  if (!client.connected()) {
     reconnect();
  }
   char t_report[15];
   char h_report[15];

   dtostrf(t,7,3,t_report);
   dtostrf(h,7,3,h_report);
   client.publish("/garden/temperature", t_report);
    client.publish("/garden/humidity", h_report);
   
 }
