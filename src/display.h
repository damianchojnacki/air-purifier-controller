#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define FONT_HEIGHT 9 // font display height, in pixels

#define OLED_RESET LED_BUILTIN  //4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setupDisplay(){
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // Don't proceed, loop forever
    }

    // Clear the buffer.
    display.clearDisplay();
    display.display();

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(WHITE);
    display.setTextSize(2);
}

void displayDust(int dust) {
    int offsetX = 18;

    if (dust > 99) {
        offsetX = 9;
    }

    display.clearDisplay();
    display.setCursor(offsetX, SCREEN_HEIGHT - 2 * FONT_HEIGHT);
    display.print(dust);
    display.print("/12");
    display.display();
}
