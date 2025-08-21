#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "pin_config.h"
#include <Wire.h>
#include "SensorPCF85063.hpp"

#include "HWCDC.h"
HWCDC USBSerial;

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);

Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST /* RST */,0 /* rotation */, true /* IPS */, LCD_WIDTH, LCD_HEIGHT, 0, 20, 0, 0);

SensorPCF85063 rtc;
uint32_t lastMillis;
char prevHourStr[4] = "";
char prevMinStr[4] = "";
char prevSecStr[4] = "";
char prevDateStr[20] = "";

int16_t getCenteredX(const char *text, uint8_t textSize) {
  int16_t textWidth = strlen(text) * 6 * textSize;
  return (LCD_WIDTH - textWidth) / 2;
}

void setup(void) {
  USBSerial.begin(115200);
  Wire.begin(IIC_SDA, IIC_SCL);

  if (!rtc.begin(Wire, PCF85063_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    Serial.println("Failed to find PCF8563 - check your wiring!");
    while (1) {
      delay(1000);
    }
  }

  uint16_t year = 2003;
  uint8_t month = 1;
  uint8_t day = 31;
  uint8_t hour = 6;
  uint8_t minute = 30;
  uint8_t second = 30;
  rtc.setDateTime(year, month, day, hour, minute, second);

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

  gfx->fillScreen(BLACK);
}

void loop() {
  /*char todisp[] = "JWatchOS";

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
  }*/

  if(millis() - lastMillis > 800) {
    lastMillis = millis();
    RTC_DateTime datetime = rtc.getDateTime();

    char hourStr[4];
    char minStr[4];
    char secStr[4];
    char dateStr[20];
    sprintf(hourStr, "%02d",datetime.hour);
    sprintf(minStr, "%02d",datetime.minute);
    sprintf(secStr, "%02d",datetime.second);
    sprintf(dateStr, "%02d %02d %04d",datetime.day,datetime.month,datetime.year);
    
    if (strcmp(hourStr, prevHourStr) != 0) {
      gfx->fillRect(0, 0, LCD_WIDTH, 130, BLACK);
      gfx->setTextColor(CYAN);
      gfx->setTextSize(13,13,0);
      gfx->setCursor(getCenteredX(hourStr,13)+6, 10);
      gfx->print(hourStr);

      strcpy(prevHourStr, hourStr);
    }

    if (strcmp(minStr, prevMinStr) != 0) {
      gfx->fillRect(0, 120, LCD_WIDTH, 100, BLACK);
      gfx->setTextColor(CYAN);
      gfx->setTextSize(13,13,0);
      gfx->setCursor(getCenteredX(minStr,13)+6, 120);
      gfx->print(minStr);

      strcpy(prevMinStr, minStr);
    }

    if (strcmp(secStr, prevSecStr) != 0) {
      gfx->fillRect(LCD_WIDTH-40, 190, 40, 21, BLACK);
      gfx->setTextColor(RED);
      gfx->setTextSize(3,3,0);
      gfx->setCursor(LCD_WIDTH-40, 190);
      gfx->print(secStr);

      strcpy(prevSecStr, secStr);
    }

    if (strcmp(dateStr, prevDateStr) != 0) {
      gfx->fillRect(0, 260, LCD_WIDTH, 20, BLACK);
      gfx->setTextColor(CYAN);
      gfx->setTextSize(2,2,0);

      gfx->setCursor(getCenteredX(dateStr,2), 260);
      gfx->print(dateStr);
      strcpy(prevDateStr, dateStr);
    }
  }

  delay(500);
}
