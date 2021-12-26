# ESP8266 Air Purifier Controller

## About
This project was created to control DIY Smart Air Purifier.

- [0. WifiManager, OTA and WebSerial](#0-wifimanager-ota-and-webserial)
- [1. Dust measurement](#1-dust-measurement)
- [2. Fan control](#2-fan-control)
- [3. OLED display](#3-oled-display)
- [4. Remote control](#4-remote-control)

### 0. WifiManager, OTA and WebSerial

For convinience I have implemented this 3 libraries: 
- WifiManager makes possible connecting to any wireless router without hardcoding Wifi credentials: 
[Docs](https://github.com/khoih-prog/ESPAsync_WiFiManager)
- Over The Air Update is available on `/update` route. You are not restricted to wireless firmware updates:
[Docs](https://github.com/ayushsharma82/ElegantOTA)
- WebSerial helps debug and monitor device. It is available on `/webserial`. Serial output will be printed there.
[Docs](https://github.com/ayushsharma82/WebSerial)
<br>Additionaly, I prepared a few commands, that you can send through remote terminal:
1. Enable and disable debug mode:
```
debug=true
debug=false
```
2. Restart ESP
```
restart
```
2. Reset ESP (please use restart if possible for reboot)
```
reset
```

### 1. Dust measurement

It is using Sharp GP2Y1010AU0F sensor. It measures PM 2.5 concentration in the air (ug/m3).

### 2. Fan control

Fan speed is based on current dust concentration. As a threshold I took 12 ug/m3. Fan speed is controlled by PWM signal.

**Speed**

There are 2 modes - auto and manual. More information avaiable in section 4 - Remote control.

Auto mode:

Base fan speed - 40%.
Dust density above thresold - 100%. 
Sleep mode - 30%.

Sleep mode enables automaticly on auto mode, from 11pm to 10am. Time is based on NTPClient and UTC offset is automaticly calculated to match 'Europe/Warsaw' timezone.

### 3. OLED display

Current dust level and threshold is diplayed on 128x64 SSD1306 white OLED display.

### 4. Remote fan control

Remote control is available through MQTT protocol. You can send manual fan speed as a mqtt message.

#### Examples:
**Fan speed change to 100%.**
```
mosquitto_pub -t '/air-purifiers/{esp_chip_id}' -m 100
```

**Fan mode change to auto.**
```
mosquitto_pub -t '/air-purifiers/{esp_chip_id}' -m 0
```

Of course you can automate this process using Web UI or Android/iOS application.

Current dust level can be synchronized through MQTT or HTTP POST request to server.
