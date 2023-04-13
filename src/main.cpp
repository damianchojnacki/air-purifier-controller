#include <Arduino.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <AsyncElegantOTA.h>
#include <ESPAsync_WiFiManager.h> 
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <GP2YDustSensor.h>
#include <WebSerial.h>
#include "Serial.h"
#include "mqtt.h"
#include "timer.h"
#include "display.h"

#define PIN_DUST_LED 14 // D5
#define PIN_DUST_ANALOG 0 // A0
#define PIN_FAN_PWM 12 //D6

int fan_should_change = 0;
int fan_current = 60;
int fan_manual = 0;
bool debug = false;

WiFiClient wifiClient;
AsyncWebServer server(80);
AsyncDNSServer dnsServer;
GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1010AU0F, PIN_DUST_LED, PIN_DUST_ANALOG);
MultiSerial serial;

uint16_t getDustRaw(){
  // Turn on the dust sensor LED by setting digital pin LOW.
  digitalWrite(PIN_DUST_LED, LOW);

  // Wait 0.28ms before taking a reading of the output voltage as per spec.
  delayMicroseconds(280);

  // Record the output voltage. This operation takes around 100 microseconds.
  uint16_t VoRaw = analogRead(PIN_DUST_ANALOG);

  // Turn the dust sensor LED off by setting digital pin HIGH.
  digitalWrite(PIN_DUST_LED, HIGH);

  return VoRaw;
}

unsigned int getDust(){
  char logBuffer[255];

  for (int i = 0; i <= 10; i++) {
      int dust = dustSensor.getDustDensity();

      if(debug){
        sprintf(logBuffer, "Raw dust sensor value: %d, Density: %d ug/m3", getDustRaw(), dust);
        serial.println(logBuffer);
      }
      
      delay(100);
  }

  unsigned int dustAverage = dustSensor.getRunningAverage();
  float newBaseline = dustSensor.getBaselineCandidate();
  
  if(debug){
    sprintf(logBuffer, "10s Avg Dust Density: %d ug/m3; New baseline: %.4f", dustAverage, newBaseline);
    serial.println(logBuffer);
  }

  // compensates sensor drift after 1m
  dustSensor.setBaseline(newBaseline);

  return dustAverage;
}

int getFanSpeed(int dust) {
    const int SPEED_MIN = 30;
    const int SPEED_DEFAULT = 40;

    if (fan_manual >= SPEED_MIN) {
        return fan_manual;
    }

    const int SLEEP_START = 23;
    const int SLEEP_END = 10;

    if (timeClient.getHours() <= SLEEP_END || timeClient.getHours() >= SLEEP_START) {
        if(debug){
          serial.println("Sleep mode.");
        } 

        return SPEED_MIN;
    }

    const int DUST_THRESHOLD = 13;

    if (dust > DUST_THRESHOLD) {
      int speed = map(dust, DUST_THRESHOLD, 55, SPEED_MIN, 100);

      if(speed > 100){
        speed = 100;
      }

      return speed;
    }

    return SPEED_DEFAULT;
}

void updateDustDensity(int dust){
    char topic[64];

    sprintf(topic, "/air-purifiers/%d/dust", ESP.getChipId());

    sendMQTTMessage(topic, dust);
}

void updateFanSpeed(int speed) {
    char topic[64];

    sprintf(topic, "/air-purifiers/%d/speed", ESP.getChipId());

    sendMQTTMessage(topic, speed);
}

void updatePreset(char* preset) {
    char topic[64];

    sprintf(topic, "/air-purifiers/%d/preset", ESP.getChipId());

    sendMQTTMessage(topic, preset);
}

void updateState() {
    char topic[64];

    sprintf(topic, "/air-purifiers/%d/state", ESP.getChipId());

    sendMQTTMessage(topic, "ON");

    if(debug){
      sprintf(topic, "/air-purifiers/%d", ESP.getChipId());

      serial.print("Watching for changes on topic: ");
      serial.print(topic);
      serial.println();
    }
}

void setup(void) {
    Serial.begin(9600);

    ESPAsync_WiFiManager wifiManager(&server, &dnsServer);

    char buffer[64];

    sprintf(buffer, "Air Purifier %d", ESP.getChipId());

    wifiManager.setAPStaticIPConfig(IPAddress(192,168,0,1), IPAddress(192,168,0,1), IPAddress(255,255,255,0));
    wifiManager.autoConnect(buffer);

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print(F("Connected. Local IP: "));
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println(wifiManager.getStatus(WiFi.status()));
        Serial.println("Can't connect! Enter WiFi config mode...");
        Serial.println("Restart...");
        ESP.reset();
    }

    AsyncElegantOTA.begin(&server);  

    WebSerial.begin(&server);
    WebSerial.msgCallback(recvMsg);
    
    serial.attach(WebSerial); 

    server.begin();

    Serial.println("Server started.");

    // fan control
    pinMode(PIN_FAN_PWM, OUTPUT);
    analogWriteRange(100);
    analogWriteFreq(10000);

    // dust sensor
    pinMode(PIN_DUST_LED, OUTPUT);

    //setupDisplay();

    dustSensor.begin();

    timeClient.begin();

    watchMQTT();

    updatePreset("auto");
}

int cycles = 0;
unsigned int dust_density = 0;

void loop(void) {
    // handle manual fan speed change
    handleMQTT();

    int speed = getFanSpeed(dust_density);

    if(speed != fan_current){
      fan_current = speed;
      updateFanSpeed(fan_current);
      analogWrite(PIN_FAN_PWM, speed);
    }

    if(cycles == 0){
      updateTime();

      serial.println();
      serial.println(timeClient.getFormattedTime());
      serial.println();

      unsigned int avg = getDust();
      dust_density = avg;
  
      serial.print("Dust density: ");
      serial.print(avg);
      serial.println("ug/m3");
    
      //displayDust(avg);
      updateDustDensity(avg);

      serial.print("Fan speed: ");
      serial.print(fan_current);
      serial.print("%");
      serial.println();

      cycles = 1000;
    }

    cycles--;
    
    delay(10);
}
