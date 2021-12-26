#include <Arduino.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <AsyncElegantOTA.h>
#include <ESPAsync_WiFiManager.h> 
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <GP2YDustSensor.h>
#include <WebSerial.h>
#include "mqtt.h"
#include "display.h"
#include "Serial.h"

#define PIN_DUST_LED 14 // D5
#define PIN_DUST_ANALOG 0 // A0
#define PIN_FAN_PWM 12 //D6

const char *SERVER_URL = "http://192.168.0.100";

int fan_should_change = 0;
int fan_current = 60;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiClient wifiClient;
AsyncWebServer server(80);
DNSServer dnsServer;
GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1010AU0F, PIN_DUST_LED, PIN_DUST_ANALOG);
MultiSerial serial;

bool debug = false;

void recvMsg(uint8_t *data, size_t len){
  serial.println();
  serial.println("Received Data...");

  String d = "";

  for(unsigned int i = 0; i < len; i++){
    d += char(data[i]);
  }

  serial.println(d);

  if(d == "debug=true"){
    debug = true;
  }

  if(d == "debug=false"){
    debug = false;
  }

  if(d == "restart"){
    ESP.restart();
  }

  if(d == "reset"){
    ESP.reset();
  }
}

unsigned int getDust(){
  char logBuffer[255];

  for (int i = 0; i <= 10; i++) {
      if(debug){
        sprintf(logBuffer, "Density: %d ug/m3", dustSensor.getDustDensity());
        serial.println(logBuffer);
      }
      
      delay(580);
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

void updateTime(){
    const int UTC_OFFSET_SUMMER = 7200;
    const int UTC_OFFSET_WINTER = 3600;

    timeClient.update();

    time_t rawtime = timeClient.getEpochTime();
    struct tm *ti = localtime(&rawtime);
  
    int month = ti->tm_mon + 1;
    int day = ti->tm_mday;
    int week_day = timeClient.getDay();

    // determine utc offset
    if(
      ((month == 3) && ((day-week_day) >= 25)) ||
      ((month > 3) && (month < 10)) ||
      ((month == 10) && ((day-week_day)) <= 24)
    ){
      timeClient.setTimeOffset(UTC_OFFSET_SUMMER);
    } else {
      timeClient.setTimeOffset(UTC_OFFSET_WINTER);
    }
}

int getFanSpeed(int dust) {
    const int SPEED_MIN = 30;
    const int SPEED_DEFAULT = 40;

    if (fan_manual >= SPEED_MIN) {
        fan_current = fan_manual;

        return fan_current;
    }

    updateTime();

    const int SLEEP_START = 23;
    const int SLEEP_END = 10;

    if (timeClient.getHours() <= SLEEP_END || timeClient.getHours() >= SLEEP_START) {
        if(debug){
          serial.println("Sleep mode.");
        } 

        fan_current = SPEED_MIN;

        return fan_current;
    }

    const int DUST_THRESHOLD = 12;

    if (dust > DUST_THRESHOLD) {
      fan_current = map(dust, 12, 24, SPEED_DEFAULT, 100);

      if(fan_current > 100){
        fan_current = 100;
      }
    } else {
      fan_current = SPEED_DEFAULT;
    }

    /*
    if (dust > DUST_THRESHOLD) {
        fan_should_change++;
    } else {
        fan_should_change--;
    }

    if (fan_should_change > 5) {
        fan_should_change = 0;

        fan_current = map(dust, 12, 24, 100, SPEED_DEFAULT);

        if(fan_current > 100){
          fan_current = 100;
        }
    } else if (fan_should_change < -5) {
        fan_should_change = 0;

        fan_current = SPEED_DEFAULT;
    }
    */

    return fan_current;
}

void updateDustDensity(int dust){
    HTTPClient http;

    char buffer[255];
    
    // dust level server url
    sprintf(buffer, "%s/api/air-purifiers/%d", SERVER_URL, ESP.getChipId());

    serial.print("Updating dust density... ");

    http.begin(wifiClient, buffer); 

    http.addHeader("Content-Type", "application/json");

    if(debug){
      serial.println();
      serial.print("POST request to: ");
      serial.println(buffer);
    }

    delay(500);

    sprintf(buffer, "{\"dust\":%d}", dust);

    delay(500);

    if(debug){
      serial.println("Request body:");
      serial.println(buffer);

      serial.print("Status code: ");
    }

    int status = http.POST(buffer);        

    http.end();

    serial.print(status);
    serial.println();
}

void setup(void) {
    Serial.begin(9600);

    ESPAsync_WiFiManager wifiManager(&server, &dnsServer);

    char buffer[255];

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

    setupDisplay();

    dustSensor.begin();

    timeClient.begin();

    watchMQTT();
}

int cycles = 10;
unsigned int dust_density = 0;

void loop(void) {
    // handle manual fan speed change
    handleMQTT();
    // adjust fan speed
    analogWrite(PIN_FAN_PWM, getFanSpeed(dust_density));

    if(cycles == 0){
      serial.println();
      serial.println(timeClient.getFormattedTime());
      serial.println();

      unsigned int avg = getDust();
      dust_density = avg;
  
      serial.print("Dust density: ");
      serial.print(avg);
      serial.println("ug/m3");
    
      displayDust(avg);
      updateDustDensity(avg);

      serial.print("Fan speed: ");
      serial.print(fan_current);
      serial.print("%");
      serial.println();

      cycles = 10;
    }

    cycles--;
    
    delay(1000);
}
