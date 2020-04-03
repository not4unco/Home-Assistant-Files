/*                                      

                  Home Assistant Garden Sensor
                              By
                         Martin Gundel
                 thegadgetdoctor2014@outlook.com
                 
I have created this project on a whim to use the ESP32 I had laying 
around collecting dust as my first sensor integration into my 
Home Assistant. I have tweaked the code from my esp8266 garden sensor 
that I used with Blynk. Hope you all enjoy.


*/
// Required libraries (sketch -> include library -> manage libraries)
// - PubSubClient by Nick ‘O Leary
// - DHT sensor library by Adafruit


#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"          

#define wifi_ssid "xxxxxxxxxx"                                                                  // Replace x's with your info
#define wifi_password "xxxxxxxxx"                                                               // Replace x's with your info

#define mqtt_server "xxx.xxx.xxx.xxx"                                                           // Replace x's with your info
#define mqtt_user "xxxxxxxxxxx"                                                                 // Replace x's with your info
#define mqtt_password "xxxxxxxxx"                                                               // Replace x's with your info

//################  VERSION  ##########################
String version = "Garden Sensor Version 1.0";      // Version of this program

#define version_topic "sensor/garden/version"                                                   // MQTT Firmware version
#define wifi_topic "sensor/garden/wifi"                                                         // MQTT WiFi Signal Strength
#define battery_topic "sensor/garden/battery"                                                   // MQTT Battery Voltage
#define bpercent_topic "sensor/garden/bpercent"                                                 // MQTT Battery Percentage
#define humidity_topic "sensor/garden/humidity"                                                 // MQTT Humidity
#define temperature_topic "sensor/garden/temperature"                                           // MQTT Temperature
#define soilmoisture_topic "sensor/garden/soilmoisture"                                         // MQTT Soil Moisture Percentage
#define debug_topic "debug"                                                                     // MQTT Topic for debugging

/* definitions for deepsleep */
#define uS_TO_S_FACTOR 1000000        /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 600              /* Time ESP32 will go to sleep for 10 minutes (in seconds) */
#define TIME_TO_SLEEP_ERROR 3600       /* Time to sleep in case of error (1 hour) */

bool debug = false;             //Display log message if True

#define DHTPIN 22                                                                                // DHT Pin used
// Uncomment depending on your sensor type:
#define DHTTYPE DHT11           // DHT 11 
//#define DHTTYPE DHT22         // DHT 22  (AM2302)

int wifi_signal;

// Create objects
DHT dht(DHTPIN, DHTTYPE);     
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);     
  setup_wifi();                           
  wifi_signal = WiFi_Signal();
  
  client.setServer(mqtt_server, 1883);                                                            // Configure MQTT connection, change port if needed.

  if (!client.connected()) {
    reconnect();
  }
  dht.begin();
  
  // Read temperature in Celcius
    float t = dht.readTemperature(true);                                                          // Remove true if you need degrees in Celcius
  // Read humidity
    float h = dht.readHumidity();
  // Read Soil Moisture
    float asoilmoist=analogRead(32);                                                              // Replace 32 with pin you are using
    float asoilmoist1;
    asoilmoist=0.95*asoilmoist+0.05*analogRead(32);                                               // Replace 32 with pin you are using
    asoilmoist1=map(asoilmoist,0,2400,0,100);
    float voltage = analogRead(35) / 4096.0 * 7.485;  // DO NOT REPLACE PIN ASSIGNMENT. Will need a voltage divider in order to use correctly.
    float bpercent = map(voltage,3.2,4.2,0,100);
      //if (voltage > 1 ) { // Only display if there is a valid reading
      //if (voltage >= 4.19) percentage = 100;
        //else if (voltage < 3.20) percentage = 0;
        //else percentage = voltage / 4.2 * 100;

  // Nothing to send. Warn on MQTT debug_topic and then go to sleep for longer period.
    if ( isnan(t) || isnan(h)) {
      Serial.println("[ERROR] Please check the DHT sensor !");
      client.publish(debug_topic, "[ERROR] Please check the DHT sensor !", true);                   // Publish DHT Sensor Error
      esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);                                //go to sleep
      Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
      Serial.println("Going to sleep now because of ERROR");
      esp_deep_sleep_start();
      return;
    }
  
    if ( debug ) {
      Serial.print("Temperature : ");
      Serial.print(t);
      Serial.print(" | Humidity : ");
      Serial.println(h);
      Serial.print("Battery Voltage : ");
      Serial.print(voltage);
     
    } 
    // Publish values to MQTT topics  version_topic
    client.publish(version_topic, String(version).c_str(), true);                                     // Publish Firmware Version
    if ( debug ) {    
      Serial.println("Version sent to MQTT.");
    }
    delay(100); //some delay is needed for the mqtt server to accept the message
    client.publish(temperature_topic, String(t).c_str(), true);                                       // Publish temperature 
    if ( debug ) {    
      Serial.println("Temperature sent to MQTT.");
    }
    delay(100); //some delay is needed for the mqtt server to accept the message
    client.publish(humidity_topic, String(h).c_str(), true);                                          // Publish humidity
    if ( debug ) {
    Serial.println("Humidity sent to MQTT.");
    }
    delay(100); //some delay is needed for the mqtt server to accept the message
    client.publish(soilmoisture_topic, String(asoilmoist1).c_str(), true);                            // Publish Soil Moisture
    if ( debug ) {
    Serial.println("Soil Moisture sent to MQTT.");
    }
    delay(100); //some delay is needed for the mqtt server to accept the message
    client.publish(battery_topic, String(voltage).c_str(), true);                                     // Publish Battery Voltage
    if ( debug ) {
    Serial.println("Battery Voltage sent to MQTT.");
    }
    delay(100); //some delay is needed for the mqtt server to accept the message
    client.publish(bpercent_topic, String(bpercent).c_str(), true);                                   // Publish Battery Percentage
    if ( debug ) {
    Serial.println("Battery % sent to MQTT.");
    }
    delay(100); //some delay is needed for the mqtt server to accept the message
    client.publish(wifi_topic, String(WiFi_Signal()).c_str(), true);                                  // Publish WiFI Signal Strength
    if ( debug ) {
    Serial.println("WiFi Signal Strength sent to MQTT.");
    }   

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);                                      //go to sleep
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  Serial.println("Going to sleep as normal now.");
  esp_deep_sleep_start();
}

//Setup Wifi Signal
int WiFi_Signal() {
  return WiFi.RSSI();
}
/*
 *  Serial.print("Signal strength: ");
  int bars;
//  int bars = map(RSSI,-80,-44,1,6); // this method doesn't refelct the Bars well
  // simple if then to set the number of bars
  
  if (RSSI > -55) { 
    bars = 5;
  } else if (RSSI < -55 & RSSI > -65) {
    bars = 4;
  } else if (RSSI < -65 & RSSI > -70) {
    bars = 3;
  } else if (RSSI < -70 & RSSI > -78) {
    bars = 2;
  } else if (RSSI < -78 & RSSI > -82) {
    bars = 1;
  } else {
    bars = 0;
  }
 */
 
//Setup connection to wifi
void setup_wifi() {
  delay(20);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

 Serial.println("");
 Serial.println("WiFi is OK ");
 Serial.print("=> ESP32 new IP address is: ");
 Serial.print(WiFi.localIP());
 Serial.println("");
}

//Reconnect to wifi if connection is lost
void reconnect() {

  while (!client.connected()) {
    Serial.print("Connecting to MQTT broker ...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("OK");
    } else {
      Serial.print("[Error] Not connected: ");
      Serial.print(client.state());
      Serial.println("Wait 5 seconds before retry.");
      delay(5000);
    }
  }
}



void loop() { 
}
