#include "HardwareSerial_NB_BC95.h"
#include <Adafruit_MLX90614.h>
#include "HardwareSerial_NB_BC95.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include <BME280I2C.h>
#include <Wire.h>

#include "Adafruit_SGP30.h"


#include "HardwareSerial_NB_BC95.h"
#include "Logo.h"
#include "lv1.h"
#include "lv2.h"
#include "lv3.h"
#include "lv4.h"
#include "lv5.h"
#include "lv6.h"
//#include "BluetoothSerial.h""
#include "Splash2.h"
#include "NBIOT.h"
#include "HardwareSerial_NB_BC95.h"
#include <TFT_eSPI.h>
#include "FS.h"

#include "RTClib.h"
#include "Free_Fonts.h"

#define CF_OL24 &Orbitron_Light_24
#define CF_OL32 &Orbitron_Light_32

#define title1 "PM2.5" // Text that will be printed on screen in any font
#define title2 "PM1"
#define title3 "PM10"
#define title4 "Co2"
#define title5 "VOC"
#define FILLCOLOR1 0xFFFF

#define TFT_BURGUNDY  0xF1EE

int xpos = 0;
int ypos = 0;
boolean isShowTemp = false;
boolean ready2display = false;

int testNum = 0;
int wtd = 0;
int maxwtd = 10;
int statusLoading = 0;
int tftMax = 160;
//BluetoothSerial SerialBT;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();



int error;

signal meta ;
String json = "";
String attr = "";

HardwareSerial hwSerial(2);
#define SERIAL1_RXPIN 25
#define SERIAL1_TXPIN 17      // for thingcontrol board v1.7
BME280I2C bme;    // Default : forced mode, standby time = 1000 ms



String deviceToken = "xxx";
String serverIP = "103.27.203.83"; // Your Server IP;
String serverPort = "9956"; // Your Server Port;

HardwareSerial_NB_BC95 AISnb;
const long intervalSendAttribute = 600000;  //millisecond
const long interval = 60000;  //millisecond
const long intervalDisplay = 1000;  //millisecond
unsigned long previousMillis = 0;
unsigned long displayPreviousMillis = 0 ;
unsigned long attributePreviousMillis = 0;
float temp(NAN), hum(NAN), pres(NAN);

Adafruit_SGP30 sgp;
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
  return absoluteHumidityScaled;
}

// Update these with values suitable for your network.

#define tvoc_topic "sensor/tvoc"
#define eco2_topic "sensor/eco2"




String imsi = "";
boolean readPMS = false;
TFT_eSPI tft = TFT_eSPI();

TFT_eSprite stringPM25 = TFT_eSprite(&tft);
TFT_eSprite stringPM1 = TFT_eSprite(&tft);
TFT_eSprite stringPM10 = TFT_eSprite(&tft);
TFT_eSprite stringCO2 = TFT_eSprite(&tft);
TFT_eSprite topNumber = TFT_eSprite(&tft);
TFT_eSprite ind = TFT_eSprite(&tft);
TFT_eSprite H = TFT_eSprite(&tft);
TFT_eSprite T = TFT_eSprite(&tft);

struct pms7003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm01_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};


struct pms7003data data;

void _initLCD() {


  tft.fillScreen(TFT_BLACK);
  // TFT

  splash();

  // MLX
  mlx.begin();

}

void _initSGP30 () {
  if (! sgp.begin()) {
    Serial.println("Sensor not found :(");
    while (1);
  }
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);
}
void _initBME280()
{
  while (!Serial) {} // Wait

  delay(200);
  Wire.begin(21, 22);

  while (!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  // bme.chipID(); // Deprecated. See chipModel().
  switch (bme.chipModel())
  {
    case BME280::ChipModel_BME280:
      //      Serial.println(F("Found BME280 sensor! Success."));
      break;
    case BME280::ChipModel_BMP280:
      Serial.println(F("Found BMP280 sensor! No Humidity available."));
      break;
    default:
      Serial.println(F("Found UNKNOWN sensor! Error!"));
  }
}


void setup() {

  pinMode(4, OUTPUT); // turn on PMS7003
  digitalWrite(4, HIGH); // turn on PMS7003
  delay(1000);
  pinMode(32, OUTPUT); // on BME280
  digitalWrite(32, HIGH); // on BME280
  delay(1000);

  Serial.begin(115200);
  //  SerialBT.begin(deviceToken); //Bluetooth device name
  _initBME280();
  _initLCD();
  _initSGP30();


  AISnb.debug = false;

  AISnb.setupDevice(serverPort);
  //
  String ip1 = AISnb.getDeviceIP();
  imsi = AISnb.getIMSI();
  imsi.trim();
  delay(4000);
  //
  pingRESP pingR = AISnb.pingIP(serverIP);
  previousMillis = millis();
  tft.fillScreen(TFT_BLACK);            // Clear screen
  hwSerial.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);

  t1CallgetProbe();
  t2CallshowEnv() ;

  tft.fillRect(5, 170, tft.width() - 15, 5, TFT_GREEN); // Print the test text in the custom font
  tft.fillRect(63, 170, tft.width() - 15, 5, TFT_YELLOW); // Print the test text in the custom font
  tft.fillRect(113, 170, tft.width() - 15, 5, TFT_ORANGE); // Print the test text in the custom font
  tft.fillRect(166, 170, tft.width() - 15, 5, TFT_RED); // Print the test text in the custom font
  tft.fillRect(219, 170, tft.width() - 15, 5, TFT_PURPLE); // Print the test text in the custom font
  tft.fillRect(272, 170, tft.width() - 15, 5, TFT_BURGUNDY); // Print the test text in the custom font

}

void splash() {
  int xpos =  0;
  int ypos = 40;
  tft.init();
  // Swap the colour byte order when rendering
  tft.setSwapBytes(true);
  tft.setRotation(1);  // landscape

  tft.fillScreen(TFT_BLACK);
  // Draw the icons
  tft.pushImage(tft.width() / 2 - logoWidth / 2, 39, logoWidth, logoHeight, logo);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(TC_DATUM); // Centre text on x,y position

  tft.setFreeFont(FSB9);
  xpos = tft.width() / 2; // Half the screen width
  ypos = 150;
  tft.drawString("Greenio", xpos, ypos, GFXFF);  // Draw the text string in the selected GFX free font

  delay(3000);

  tft.setTextFont(GLCD);
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);
  // Select the font
  ypos += tft.fontHeight(GFXFF);                      // Get the font height and move ypos down
  tft.setFreeFont(FSB9);
  tft.pushImage(tft.width() / 2 - (Splash2Width / 2) - 15, 3, Splash2Width, Splash2Height, Splash2);



  delay(1200);
  tft.setTextPadding(180);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  Serial.println("Start...");
  for ( int i = 0; i < 200; i++)
  {
    tft.drawString(".", 1 + 2 * i, 210, GFXFF);
    delay(10);
    Serial.println(i);
  }
  Serial.println("end");
}

void printBME280Data()
{
  _initBME280();
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  bme.read(pres, temp, hum, tempUnit, presUnit);

}

void composeJson() {
  meta = AISnb.getSignal();
  json = "";
  json.concat(" {\"Tn\":\"");
  json.concat(deviceToken);
  json.concat("\",\"temp\":");
  json.concat(temp);
  json.concat(",\"hum\":");
  json.concat(hum);
  json.concat(",\"pres\":");
  json.concat(pres);
  json.concat(",\"pm1\":");
  json.concat(data.pm01_env);
  json.concat(",\"pm2.5\":");
  json.concat(data.pm25_env);
  json.concat(",\"pm10\":");
  json.concat(data.pm100_env);

  json.concat(",\"pn03\":");
  json.concat(data.particles_03um);
  json.concat(",\"pn05\":");
  json.concat(data.particles_05um);
  json.concat(",\"pn10\":");
  json.concat(data.particles_10um);
  json.concat(",\"pn25\":");
  json.concat(data.particles_25um);
  json.concat(",\"pn50\":");
  json.concat(data.particles_50um);
  json.concat(",\"pn100\":");
  json.concat(data.particles_100um);
  json.concat(",\"rssi\":");
  json.concat(meta.rssi);
  json.concat("}");
  //  SerialBT.println(json);

  if (data.pm25_env > 1000)
    ESP.restart();

}


void printPMS7003() {

  // reading data was successful!
  //  Serial.println();
  //  Serial.println("---------------------------------------");
  //    Serial.println("Concentration Units (standard)");
  //    Serial.print("PM 1.0: "); Serial.print(data.pm10_standard);
  //    Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_standard);
  //    Serial.print("\t\tPM 10: "); Serial.println(data.pm100_standard);
  //    Serial.println("---------------------------------------");
  Serial.println("Concentration Units (environmental)");
  Serial.print("PM1.0:"); Serial.print(data.pm01_env);
  Serial.print("\tPM2.5:"); Serial.print(data.pm25_env);
  Serial.print("\tPM10:"); Serial.println(data.pm100_env);
  Serial.println("---------------------------------------");
  Serial.print("Particles > 0.3um / 0.1L air:"); Serial.println(data.particles_03um);
  Serial.print("Particles > 0.5um / 0.1L air:"); Serial.println(data.particles_05um);
  Serial.print("Particles > 1.0um / 0.1L air:"); Serial.println(data.particles_10um);
  Serial.print("Particles > 2.5um / 0.1L air:"); Serial.println(data.particles_25um);
  Serial.print("Particles > 5.0um / 0.1L air:"); Serial.println(data.particles_50um);
  Serial.print("Particles > 10.0 um / 0.1L air:"); Serial.println(data.particles_100um);
  Serial.println("---------------------------------------");

}



void t2CallshowEnv() {
  //  Serial.print(F("ready2display:"));
  //  Serial.println(ready2display);
  if (ready2display) {

    tft.setTextDatum(MC_DATUM);
    xpos = tft.width() / 2 ; // Half the screen width

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setFreeFont(CF_OL32);
    int mid = (tftMax / 2) - 72;
    tft.setTextPadding(100);
    tft.drawString(title1, xpos - 70, 135, GFXFF); // Print the test text in the custom font

    //  for testing
    //        data.pm25_env = testNum;    //for testing
    //        testNum++;
    drawNumberParticules();
    drawPM2_5(data.pm25_env, mid, 50);

    tft.setTextSize(1);
    tft.setFreeFont(CF_OL32);                 // Select the font

    tft.setTextDatum(BR_DATUM);
    tft.setTextColor(TFT_WHITE);
    drawPM1(data.pm01_env, 30, 190);
    tft.setTextSize(1);

    tft.setFreeFont(FSB9);   // Select Free Serif 9 point font, could use:
    tft.drawString(title2, 45, 230, GFXFF); // Print the test text in the custom font

    drawPM10(data.pm100_env, 95, 190);
    tft.drawString(title3, 110, 230, GFXFF); // Print the test text in the custom font

    drawCO2(sgp.eCO2, 130, 190);
    tft.drawString(title4, 145, 230, GFXFF); // Print the test text in the custom font


    tft.drawString("rH ", xpos + 105, 209, GFXFF); // Print the test text in the custom font
    drawH(hum, xpos + 111, 190);
    tft.drawString("%", xpos + 148, 209, GFXFF);
    tft.drawString("T ", xpos + 105, 230, GFXFF); // Print the test text in the custom font
    drawT(temp, xpos + 111, 209);
    tft.drawString("C", xpos + 148, 230, GFXFF);

    //Clear Stage

    ind.createSprite(320, 10);
    ind.fillSprite(TFT_BLACK);
    if ((data.pm25_env >= 0) && (data.pm25_env < 51)) {
      tft.setWindow(0, 25, 55, 55);
      tft.pushImage(tft.width() - lv1Width - 6, 33, lv1Width, lv1Height, lv1);
      ind.fillTriangle(0, 0, 5, 5, 10, 0, FILLCOLOR1);
    } else if ((data.pm25_env >= 51) && (data.pm25_env < 101)  ) {
      tft.pushImage(tft.width() - lv2Width - 6, 33, lv2Width, lv2Height, lv2);
      ind.fillTriangle(55, 0, 60, 5, 65, 0, FILLCOLOR1);
    } else  if ((data.pm25_env >= 101) && (data.pm25_env < 151)  ) {
      tft.pushImage(tft.width() - lv3Width - 6, 33, lv3Width, lv3Height, lv3);
      ind.fillTriangle(105, 0, 110, 5, 115, 0, FILLCOLOR1);
    } else  if ((data.pm25_env >= 151) && (data.pm25_env < 201)  ) {
      tft.pushImage(tft.width() - lv4Width - 6, 33, lv4Width, lv4Height, lv4);
      ind.fillTriangle(155, 0, 160, 5, 165, 0, FILLCOLOR1);
    } else  if ((data.pm25_env >= 201) && (data.pm25_env < 300)  ) {
      tft.pushImage(tft.width() - lv5Width - 6, 33, lv5Width, lv5Height, lv5);
      ind.fillTriangle(210, 0, 215, 5, 220, 0, FILLCOLOR1);
    } else {
      tft.pushImage(tft.width() - lv6Width - 6, 33, lv6Width, lv6Height, lv6);
      ind.fillTriangle(265, 0, 270, 5, 275, 0, FILLCOLOR1);
    }
    ind.pushSprite(29, 160);
    ind.deleteSprite();
  }
}

void drawNumberParticules() {

  topNumber.createSprite(280, 20);
  //  stringPM1.fillSprite(TFT_GREEN);
  topNumber.setFreeFont(FS9);
  topNumber.setTextColor(TFT_WHITE);
  topNumber.setTextSize(1);           // Font size scaling is x1

  topNumber.drawString(">1.0:", 0, 0, GFXFF); // Print the test text in the custom font
  topNumber.drawNumber(data.particles_10um, 40, 0);   //tft.drawString("0.1L air", 155, 5, GFXFF);
  topNumber.drawString(">2.5:", 95, 0, GFXFF); // Print the test text in the custom font
  topNumber.drawNumber(data.particles_25um, 135, 0);   //tft.drawString("0.1L air", 155, 5, GFXFF);
  topNumber.drawString(">5.0:", 180, 0, GFXFF); // Print the test text in the custom font
  topNumber.drawNumber(data.particles_50um, 220, 0);   //tft.drawString("0.1L air", 155, 5, GFXFF);

  topNumber.pushSprite(5, 5);
  topNumber.deleteSprite();


}


boolean readPMSdata(Stream *s) {
  //  Serial.println("readPMSdata");
  if (! s->available()) {
    Serial.println("readPMSdata.false");
    return false;
  }

  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }

  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  s->readBytes(buffer, 32);

  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }

  memcpy((void *)&data, (void *)buffer_u16, 30);
  // get checksum ready
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }
  if (sum != data.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}

void sendAttribute() {
  attr = "";
  attr.concat("{\"Tn\":\"");
  attr.concat(deviceToken);
  attr.concat("\",\"IMSI\":");
  attr.concat("\"");
  attr.concat(imsi);
  attr.concat("\"}");
  UDPSend udp = AISnb.sendUDPmsgStr(serverIP, serverPort, attr);

}

void t1CallgetProbe() {
  boolean pmsReady = readPMSdata(&hwSerial);

  if ( pmsReady ) {
    ready2display = true;
    wtd = 0;
  } else {
    ready2display = false;

  }


  if (wtd > maxwtd)
    ESP.restart();

  printBME280Data();
  getDataSGP30();
}

void drawPM2_5(int num, int x, int y)
{
  // Create a sprite 80 pixels wide, 50 high (8kbytes of RAM needed)
  stringPM25.createSprite(180, 75);
  //  stringPM25.fillSprite(TFT_YELLOW);
  stringPM25.setTextSize(3);           // Font size scaling is x1
  stringPM25.setFreeFont(CF_OL24);  // Select free font
  stringPM25.setTextColor(TFT_WHITE);
  stringPM25.setTextSize(3);

  int mid = (tftMax / 2) - 1;

  stringPM25.setTextColor(TFT_WHITE);  // White text, no background colour
  // Set text coordinate datum to middle centre
  stringPM25.setTextDatum(MC_DATUM);
  // Draw the number in middle of 80 x 50 sprite
  stringPM25.drawNumber(num, mid, 25);
  // Push sprite to TFT screen CGRAM at coordinate x,y (top left corner)
  stringPM25.pushSprite(x, y);
  // Delete sprite to free up the RAM
  stringPM25.deleteSprite();
}

void drawT(int num, int x, int y)
{
  T.createSprite(50, 20);
  //  stringPM1.fillSprite(TFT_GREEN);
  T.setFreeFont(FSB9);
  T.setTextColor(TFT_WHITE);
  T.setTextSize(1);
  T.drawNumber(num, 0, 3);
  T.pushSprite(x, y);
  T.deleteSprite();
}

void drawH(int num, int x, int y)
{
  H.createSprite(50, 20);
  //  stringPM1.fillSprite(TFT_GREEN);
  H.setFreeFont(FSB9);
  H.setTextColor(TFT_WHITE);
  H.setTextSize(1);
  H.drawNumber(num, 0, 3);
  H.pushSprite(x, y);
  H.deleteSprite();
}


void drawPM1(int num, int x, int y)
{
  stringPM1.createSprite(50, 20);
  //  stringPM1.fillSprite(TFT_GREEN);
  stringPM1.setFreeFont(FSB9);
  stringPM1.setTextColor(TFT_WHITE);
  stringPM1.setTextSize(1);
  stringPM1.drawNumber(num, 0, 3);
  stringPM1.pushSprite(x, y);
  stringPM1.deleteSprite();
}

void drawCO2(int num, int x, int y)
{
  stringCO2.createSprite(100, 20);
  stringCO2.fillSprite(TFT_GREEN);
  stringCO2.setFreeFont(FSB9);
  stringCO2.setTextColor(TFT_WHITE);
  stringCO2.setTextSize(1);
  stringCO2.drawNumber(num, 0, 3);
  stringCO2.pushSprite(x, y);
  stringCO2.deleteSprite();
}

void drawPM10(int num, int x, int y)
{
  stringPM10.createSprite(50, 20);
  //  stringPM1.fillSprite(TFT_GREEN);
  stringPM10.setFreeFont(FSB9);
  stringPM10.setTextColor(TFT_WHITE);
  stringPM10.setTextSize(1);
  stringPM10.drawNumber(num, 0, 3);
  stringPM10.pushSprite(x, y);
  stringPM10.deleteSprite();
}

void getDataSGP30 () {
  // put your main code here, to run repeatedly:
  // If you have a temperature / humidity sensor, you can set the absolute humidity to enable the humditiy compensation for the air quality signals
  //float temperature = 22.1; // [°C]
  //float humidity = 45.2; // [%RH]
  //sgp.setHumidity(getAbsoluteHumidity(temperature, humidity));

  if (! sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }
  Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
  Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");


  if (! sgp.IAQmeasureRaw()) {
    Serial.println("Raw Measurement failed");
    return;
  }
  Serial.print("Raw H2 "); Serial.print(sgp.rawH2); Serial.print(" \t");
  Serial.print("Raw Ethanol "); Serial.print(sgp.rawEthanol); Serial.println("");
  delay(2000);


  uint16_t TVOC_base, eCO2_base;
  if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
    Serial.println("Failed to get baseline readings");
    return;
  }
  Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
  Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
}
void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - displayPreviousMillis >= intervalDisplay) {
    displayPreviousMillis = currentMillis;
    t1CallgetProbe();
    t2CallshowEnv();
    printPMS7003() ;
  }
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;



    composeJson();
    tft.pushImage(285, 0, nbiotWidth, nbiotHeight, nbiot);

    UDPSend udp = AISnb.sendUDPmsgStr(serverIP, serverPort, json);
    delay(2000);
    tft.fillRect(285, 0, nbiotWidth, nbiotHeight, TFT_BLACK); // Print the test text in the custom font

    UDPReceive resp = AISnb.waitResponse();
  }

  if (currentMillis - attributePreviousMillis >= intervalSendAttribute) {
    attributePreviousMillis = currentMillis;
    sendAttribute();
  }

}
