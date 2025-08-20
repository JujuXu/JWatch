// I2C addresses

// RTC 0x68
// OLED 0x3C
// BPM 0x57
// PRESSURE 0x77
// LIGHT 0x76
// MPU 0x68 0x69 => AD0 HIGH

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <AiEsp32RotaryEncoder.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <WiFi.h>
#include <time.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <BH1750.h>
#include <Adafruit_MPU6050.h>

const char* ssid_list[] {
  "THORGAL",
  "Robotix2.4",
  "JiPhone 15"
};
const int NB_WIFI = sizeof(ssid_list) / sizeof(ssid_list[0]);

const char* pswd_list[] {
  "57xwkppgkr",
  "FreddieLeSang",
  "julien1234"
};

const char* ap_ssid = "JWatch";
const char* ap_pswd = "Ben10CMoi";

#define ENC_CLK 34
#define ENC_DT 35
#define ENC_SW 32
#define ENC_VPP -1
#define ENC_STEPS 4

RTC_DS3231 rtc;

AiEsp32RotaryEncoder ENC = AiEsp32RotaryEncoder(ENC_DT, ENC_CLK, ENC_SW, ENC_VPP, ENC_STEPS);
void IRAM_ATTR readEncoderISR()
{
	ENC.readEncoder_ISR();
}

bool circleValues = true;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

// ⏰ Serveur NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;  // Fuseau horaire (ex: GMT+1 = 3600 sec)
const int   daylightOffset_sec = 3600;  // Heure d'été

long irValue;
float beatsPerMinute;
int beatAvg;

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BMP3XX bmp;
BH1750 lightMeter;
Adafruit_MPU6050 mpu;

const char* menu_txt[] = {
  "Home",
  "BPM",
  "WiFi Client",
  "Wifi AP",
  "Sync RTC",
  "Measures",
  "Gyro"
};
const int NB_MENU = sizeof(menu_txt) / sizeof(menu_txt[0]);

void (*fonctionPtr)(bool button, int value);

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
    rtc.adjust(DateTime(2025, 2, 6, 23, 44, 0));
  }

  ENC.begin();
  ENC.setup(readEncoderISR);
  circleValues = true;
	ENC.setBoundaries(0, 1000, circleValues);
  ENC.setAcceleration(200);
  ENC.setEncoderValue(0);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  if (!bmp.begin_I2C()) {
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    while (1);
  }

  // Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  if (!lightMeter.begin()) {
    Serial.println("Could not find a valid BH1750 sensor, check wiring!");
    while (1);
  }

  if (!mpu.begin(0x69)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("JWatchOS started!");

  startupDisplay();
  fonctionPtr = home;

  // PERFORMANCE
  btStop();
}

void loop() {
  stateMachine();
  adjustBrightness();
}

void stateMachine() {
  bool button = false;
  static int value = 0;
	if (ENC.isEncoderButtonClicked())
	{
		static unsigned long lastTimePressed = 0;
    
    if (millis() - lastTimePressed > 500)
    {
      lastTimePressed = millis();
      button = true;
      Serial.println("Button pressed");
    }
	}

  if (ENC.encoderChanged())
	{
		Serial.print("Value: ");
    value = ENC.readEncoder();
		Serial.println(value);
	}

  if (fonctionPtr) {
    fonctionPtr(button, value);
  }
}

void menu(bool button, int value){
  showMenu(value);
  ENC.setBoundaries(0, NB_MENU-1, circleValues);

  if (button){
    switch (value) {
      case 0:
        fonctionPtr = home;
        break;
      case 1:
        fonctionPtr = bpmRead;
        break;
      case 2:
        fonctionPtr = wifi_client;
        ENC.setBoundaries(0, NB_WIFI, circleValues);
        break;
      case 3:
        fonctionPtr = wifi_ap;
        ENC.setBoundaries(0, 2, circleValues);
        break;
      case 4:
        fonctionPtr = syncRTC;
        break;
      case 5:
        fonctionPtr = measures;
        break;
      case 6:
        fonctionPtr = mpuRead;
        break;
      default:
        break;
    }

    ENC.setEncoderValue(0);
  }
}

void showMenu(int value) {
  display.clearDisplay();
  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  for(int i=0; i<NB_MENU;i++) {
    if (value == i) display.print("> ");

    display.println(menu_txt[i]);
  }

  display.display();
}

void selectMenu(){
  fonctionPtr = menu;
  ENC.setEncoderValue(0);
}

void startupDisplay() {
  display.clearDisplay();  // Efface l’écran
  display.setRotation(3);  // Rotation de l’écran (3 = 270°)
  display.setTextSize(1);  // Taille du texte
  display.setTextColor(SSD1306_WHITE); // Texte en blanc
  display.setCursor(0, 20); // Position initiale du texte

  char todisp[] = "JWatchOS";

  // Animation lettre par lettre
  for(int i = 0; i < strlen(todisp); i++) {
    display.print(todisp[i]);  // Afficher la lettre actuelle
    display.display();  // Mettre à jour l’écran
    delay(50);  // Petite pause pour l'effet d'apparition
  }

  delay(1000); // Pause avant d'afficher la version

  // Affichage de la version en dessous du titre
  display.setCursor(10, 35);
  display.println("v0.1");
  
  display.display();  // Mettre à jour l'écran avec la version

  Serial.println("startup display done");

  delay(500);
}

void home(bool button, int value){

  if (button){
    selectMenu();
    return;
  }

  display.clearDisplay();
  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 20);

  char rtcbuff[20];

  DateTime now = rtc.now();

  display.setTextSize(4);
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

  getBPM();
  display.print("BPM: ");
  display.println(beatAvg);

  display.display();
}

void bpmRead(bool button, int value){
  if (button){
    selectMenu();
    return;
  }

  getBPM();

  display.clearDisplay();
  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  display.print("IR: ");
  display.println(irValue);

  display.print("BPM: ");
  display.println(beatsPerMinute);

  display.print("AVG: ");
  display.println(beatAvg);

  display.display();
}

void getBPM(){
  irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }

    if (irValue < 50000) {
      beatAvg = 0;
    }
  }
}

void wifi_client(bool button, int value) {
  static int wifi_connected = -1;
  if (WiFi.status() != WL_CONNECTED && wifi_connected != -1) wifi_connected = -1;

  display.clearDisplay();
  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  for(int i=0; i<NB_WIFI+1; i++) {
    if (value == i) display.print("> ");

    if (i == 0) display.println("back");
    else {
      if (wifi_connected == i-1) display.print("*");
      display.print(ssid_list[i-1]);
    }

    display.println();
  }

  display.display();

  if (button) {
    if (value == 0) {
      selectMenu();
      return;
    }

    if (wifi_connected == -1) {
      display.clearDisplay();
      display.setRotation(3);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);

      display.print("Connecting");

      display.display();

      WiFi.softAPdisconnect(true);
      WiFi.disconnect();
      WiFi.begin(ssid_list[value-1], pswd_list[value-1]);

      long start = millis();

      while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= 5000) {
          display.clearDisplay();
          display.setRotation(3);
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.setCursor(0, 0);

          display.println("Failed!");
          display.display();
          delay(1000);

          return;
        }

        display.print(".");
        display.display();
        delay(500);
      }

      wifi_connected = value-1;
      display.println("\nConnected!");
      display.display();
      delay(1000);
    } else if (wifi_connected == value-1) {
      WiFi.disconnect();

      display.clearDisplay();
      display.setRotation(3);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);

      display.println("WiFi disconnected!");
      display.display();

      wifi_connected = -1;

      delay(1000);
      return;
    }
  }
}

void wifi_ap(bool button, int value) {
  display.clearDisplay();
  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  for (int i=0; i<3;i++) {
    if (value == i) display.print("> ");

    if (i==0) display.println("back");
    if (i==1) display.println("ON");
    if (i==2) display.println("OFF");
  }

  display.println("\n");

  display.println("SSID:");
  display.println(ap_ssid);
  display.println("PSWD:");
  display.println(ap_pswd);

  display.display();

  if (button) {
    if (value == 0) {
      selectMenu();
      return;
    }
    else if (value == 1) {
      display.clearDisplay();
      display.setRotation(3);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);

      display.println("Starting AP");
      display.display();

      WiFi.disconnect();
      WiFi.softAP(ap_ssid, ap_pswd);

      display.println("SSID:");
      display.println(ap_ssid);
      display.println("IP:");
      display.println(WiFi.softAPIP());

      display.println("\n Done !");
      display.display();
      delay(3000);
    } else {
      display.clearDisplay();
      display.setRotation(3);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);

      display.println("Stopping AP");
      display.display();

      WiFi.softAPdisconnect(true);

      display.println("\n Done !");
      display.display();
      delay(1000);
    }
  }
}

void syncRTC(bool button, int value) {
  display.clearDisplay();
  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  if (WiFi.status() != WL_CONNECTED) {
    display.println("No WiFi !");
    display.display();
    delay(1000);

    selectMenu();
    return;
  }

  display.println("Syncing");

  display.display();

  int attempts = 0;

  struct tm timeinfo;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  while (!getLocalTime(&timeinfo) && attempts < 5) {
      display.print(".");
      display.display();
      delay(500);
      attempts++;
  }

  if (attempts >= 5) {
    Serial.println("Erreur : Impossible de récupérer l'heure NTP !");

    display.println("NTP Error!");
    display.display();
    delay(1000);

    selectMenu();
    return;
  }
  
  DateTime now(
      timeinfo.tm_year + 1900, 
      timeinfo.tm_mon + 1, 
      timeinfo.tm_mday, 
      timeinfo.tm_hour, 
      timeinfo.tm_min, 
      timeinfo.tm_sec
  );

  rtc.adjust(now);
  Serial.println("✅ RTC mise à jour avec l'heure NTP !");

  display.println("RTC synced!");

  display.print(now.hour());
  display.print(":");
  display.print(now.minute());
  display.print(":");
  display.println(now.second());

  display.print(now.day());
  display.print(":");
  display.print(now.month());
  display.print(":");
  display.println(now.year());

  display.display();
  delay(5000);

  selectMenu();
  return;
}

void measures(bool button, int value) {
  if (button) {
    selectMenu();
    return;
  }

  display.clearDisplay();
  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  double temp = 0;
  double press = 0;
  float alt = 0;
  float lux = 0;

  getBMP(temp, press, alt);
  getLight(lux);

  display.println("Temp: ");
  display.print(temp);
  display.println(" *C");

  display.println("Pressure: ");
  display.print(press);
  display.println(" hPa");

  display.println("ALT: ");
  display.print(alt);
  display.println(" m");

  display.println("Light: ");
  display.print(lux);
  display.println(" lux");

  display.display();
}

void getBMP(double &temp, double &press, float &alt) {
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  temp = bmp.temperature;
  press = bmp.pressure /100;
  alt = bmp.readAltitude(SEALEVELPRESSURE_HPA);
}

void getLight(float &lux) {
  lux = lightMeter.readLightLevel();
}

void mpuRead(bool button, int value) {
  if (button) {
    selectMenu();
    return;
  }

  display.clearDisplay();
  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  display.println("ACC:");
  display.print("X: ");
  display.println(a.acceleration.x);
  display.print("Y: ");
  display.println(a.acceleration.y);
  display.print("Z: ");
  display.println(a.acceleration.z);

  display.println("\nGYRO:");
  display.print("X: ");
  display.println(g.gyro.x);
  display.print("Y: ");
  display.println(g.gyro.y);
  display.print("Z: ");
  display.println(g.gyro.z);

  display.println("\nTemp: ");
  display.print(temp.temperature);
  display.println("oC");

  display.display();
}

int currentBrightness = 128; // Valeur initiale de la luminosité
const uint8_t brightnessStep = 35; // Pas d'incrémentation pour lisser le changement

void adjustBrightness() {
  static unsigned long lastAdjust = 0;

  if (millis() - lastAdjust >= 400) {
    float lux = lightMeter.readLightLevel(); // Lire la lumière ambiante en lux
    
    // Mapper la valeur de luminosité (0 à 255) en fonction de la lumière ambiante
    lux = constrain(lux, 0, 400);
    int targetBrightness = map(lux, 0, 400, 10, 255); // Garder targetBrightness en int
    
    // Ajustement progressif pour éviter des changements brusques
    if (currentBrightness < targetBrightness) {
        currentBrightness += brightnessStep;
        if (currentBrightness > targetBrightness) currentBrightness = targetBrightness;
    } else if (currentBrightness > targetBrightness) {
        currentBrightness -= brightnessStep;
        if (currentBrightness < targetBrightness) currentBrightness = targetBrightness;
    }
    
    // Assurer que currentBrightness reste bien entre 0 et 255
    currentBrightness = constrain(currentBrightness, 0, 255);
    
    // Appliquer la luminosité à l'écran (SSD1306 n'a pas de fonction de réglage direct, on peut jouer sur le contraste)
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(currentBrightness);
    
    // Serial.print("Brightness: ");
    // Serial.print(currentBrightness);
    // Serial.print(" LUX: ");
    // Serial.print(lux);
    // Serial.print(" target: ");
    // Serial.println(targetBrightness);

    lastAdjust = millis();
  }
}
