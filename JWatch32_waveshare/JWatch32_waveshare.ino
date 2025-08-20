#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include <Wire.h>

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);

Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST /* RST */,0 /* rotation */, true /* IPS */, LCD_WIDTH, LCD_HEIGHT, 0, 20, 0, 0);

void setup(void) {
  Serial.begin(115200);
  Wire.begin(IIC_SDA, IIC_SCL);

  gfx->begin();
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  gfx->fillScreen(BLACK);

  char todisp[] = "JWatchOS";

  gfx->Display_Brightness(255);
  gfx->setCursor(20, 100);
  gfx->setTextColor(CYAN);
  gfx->setTextSize(4);

  for(int i = 0; i < strlen(todisp); i++) {
    gfx->print(todisp[i]);
    delay(50);
  }

  delay(1000);

  gfx->setCursor(90, 140);
  gfx->setTextSize(2);
  gfx->setTextColor(RED);
  gfx->print("v");
  gfx->setTextColor(CYAN);
  gfx->print("0.1");

  delay(500);
}

void loop() {
  char todisp[] = "JWatchOS";

  for(int i = 0; i <= strlen(todisp); i++) {
    gfx->Display_Brightness(255);
    gfx->setCursor(20, 100);
    gfx->setTextSize(4);
    for(int j = 0; j < strlen(todisp); j++) {
      if (j == i) {
        gfx->setTextColor(RED, BLACK);
        gfx->print(todisp[j]);
      } else {
        gfx->setTextColor(CYAN, BLACK);
        gfx->print(todisp[j]);
      }
    }

    
    delay(200);
  }

  delay(500);
  
  digitalWrite(LCD_BL, LOW);

  delay(500);
  digitalWrite(LCD_BL, HIGH);
  delay(500);
}
