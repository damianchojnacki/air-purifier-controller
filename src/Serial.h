#include <Arduino.h>
#include <WebSerial.h>

class MultiSerial
{
    public: 
        WebSerialClass webSerial;

        void attach(WebSerialClass webSerial){
            MultiSerial::webSerial = webSerial;
        }

        void println(){
            Serial.println();
            webSerial.println();
        }

        void println(uint32_t message){
            Serial.println(message);
            webSerial.println(message);
        }

        void println(String message){
            Serial.println(message);
            webSerial.println(message);
        }

        void println(const char *message){
            Serial.println(message);
            webSerial.println(message);
        }

        void print(uint32_t message){
            Serial.print(message);
            webSerial.print(message);
        }

        void print(String message){
            Serial.print(message);
            webSerial.print(message);
        }

        void print(const char *message){
            Serial.print(message);
            webSerial.print(message);
        }
};