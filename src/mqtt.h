#include <ESP8266WiFi.h>
#include <ArduinoMqttClient.h>

extern int fan_manual;
extern MultiSerial serial;
extern bool debug;
extern void updatePreset(char* preset);

WiFiClient client;
MqttClient mqttClient(client);

const char broker[] = "192.168.0.100";
int port = 1883;

template <typename T>
void sendMQTTMessage(char* topic, T message) {
    mqttClient.beginMessage(topic);
    mqttClient.print(message);
    mqttClient.endMessage();
}

void handleMQTT() {
    int messageSize = mqttClient.parseMessage();

    if (messageSize) {
        if(debug){
            serial.print("Message arrived on topic: ");
            serial.print(mqttClient.messageTopic());
            serial.print(" : ");
        }

        char messageTemp[messageSize];

        for (int i = 0; mqttClient.available(); i++) {
            messageTemp[i] = (char) mqttClient.read();
        }

        int speed = atoi(messageTemp);

        if(debug){
            serial.print(speed);
            serial.println();
        }

        // Changes the fan speed according to the message
        fan_manual = speed;

        if(speed == 0){
            updatePreset("auto");
        } else {
            updatePreset("manual");
        }
    }
}

void watchMQTT() {
    if (!mqttClient.connect(broker, port)) {
        serial.print("MQTT connection failed! Error code = ");
        serial.println(mqttClient.connectError());

        while (1);
    }

    char topic[64];

    sprintf(topic, "/air-purifiers/%d", ESP.getChipId());

    mqttClient.subscribe(topic);

    serial.println("Watching for changes...");
}
