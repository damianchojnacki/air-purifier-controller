#include <ESP8266WiFi.h>
#include <ArduinoMqttClient.h>

int fan_manual = 0;

WiFiClient client;
MqttClient mqttClient(client);

const char broker[] = "192.168.0.100";
int port = 1883;

void handleMQTT() {
    int messageSize = mqttClient.parseMessage();

    if (messageSize) {
        Serial.print("Message arrived on topic: ");
        Serial.print(mqttClient.messageTopic());
        Serial.print(" : ");

        char messageTemp[messageSize];

        for (int i = 0; mqttClient.available(); i++) {
            messageTemp[i] = (char) mqttClient.read();
        }

        Serial.print(atoi(messageTemp));
        Serial.println();

        // Changes the fan speed according to the message
        fan_manual = atoi(messageTemp);
    }
}

void watchMQTT() {
    if (!mqttClient.connect(broker, port)) {
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(mqttClient.connectError());

        while (1);
    }

    char topic[255];

    sprintf(topic, "/air-purifiers/%d", ESP.getChipId());

    mqttClient.subscribe(topic);

    Serial.println("Watching for changes...");
}
