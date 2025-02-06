#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <AiEsp32RotaryEncoder.h>

#define ENC_CLK 0
#define ENC_DT 2
#define ENC_SW 15
#define ENC_VPP -1
#define ENC_STEPS 4

RTC_DS3231 rtc;

AiEsp32RotaryEncoder ENC = AiEsp32RotaryEncoder(ENC_DT, ENC_CLK, ENC_SW, ENC_VPP, ENC_STEPS);
void IRAM_ATTR readEncoderISR()
{
	ENC.readEncoder_ISR();
}

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    for(;;);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(2025, 2, 6, 23, 44, 0));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  ENC.begin();
  ENC.setup(readEncoderISR);
  bool circleValues = true;
	ENC.setBoundaries(0, 1000, circleValues);
  ENC.setAcceleration(250);

  Serial.println("JWatchOS started!");
}

void loop() {
  home();
  rotary_loop();
}

void home() {
  display.clearDisplay();
  display.setRotation(3);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  // Display static text
  display.println("JWatchOS");
  display.println("v0.1");

  char rtcbuff[20];

  DateTime now = rtc.now();

  display.setTextSize(4.5);
  strcpy(rtcbuff, "hh");
  display.println(now.toString(rtcbuff));

  strcpy(rtcbuff, "mm");
  display.print(now.toString(rtcbuff));

  display.setTextSize(1);
  strcpy(rtcbuff, "ss");
  display.println(now.toString(rtcbuff));

  display.println("\n\n\n");

  strcpy(rtcbuff, "DD MM YYYY");
  display.println(now.toString(rtcbuff));

  display.display();
}

void rotary_onButtonClick()
{
	static unsigned long lastTimePressed = 0;
	//ignore multiple press in that time milliseconds
	if (millis() - lastTimePressed < 500)
	{
		return;
	}
	lastTimePressed = millis();
	Serial.print("button pressed ");
	Serial.print(millis());
	Serial.println(" milliseconds after restart");
}

void rotary_loop()
{
	//dont print anything unless value changed
	if (ENC.encoderChanged())
	{
		Serial.print("Value: ");
		Serial.println(ENC.readEncoder());
	}
	if (ENC.isEncoderButtonClicked())
	{
		rotary_onButtonClick();
	}
}