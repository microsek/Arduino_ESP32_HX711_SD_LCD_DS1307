/****************************
ESP32       |    Device
----------------------------
GPIO5       |   SD SS(D3)
GPIO23      |   SD MOSI(CMD)
GPIO18      |   SD SCK (CLK)
GPIO19      |   SD MOSO(DO)
----------------------------
GPIO33      |   HX711 DOUT
GPIO32      |   HX711 CLK
----------------------------
GPIO22      |   LCD and DS1307 I2C SCL
GPIO21      |   LCD and DS1307 I2C SDA
----------------------------
GPIO25      |   KEY_Auto
GPIO26      |   KEY_Meno
GPIO27      |   KEY_Left
GPIO14      |   KEY_Right
GPIO4       |   KEY_Select
GPIO15      |   KEY_On 
****************************/
#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>          //https://github.com/esp8266/Arduino
#endif
//needed for library
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"
#include "HX711.h"
#include "FS.h"
#include <SD.h>
#include <SPI.h>

#define zero_factor 93618
#define DOUT  33
#define CLK   32
#define DEC_POINT  2

#define KEY_Auto 25
#define KEY_Menu 26
#define KEY_Left 27
#define KEY_Right 14
#define KEY_Select 4
#define KEY_On 15
//----------- weight ---------------------
float calibration_factor =179642.00; 
float offset=0;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
int ID=1;
int plant=1;
bool savedata=false;
int weight_zero;
String weight;
//------------------------------------
HX711 scale(DOUT, CLK);
LiquidCrystal_I2C lcd(0x27, 20, 4);// Set the LCD address to 0x27 for a 20 chars and 4 line display //esp32 scl22 sda21
RTC_DS1307 rtc;
//------------ define function --------------------
float get_units_kg();
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void set_zero();
void increase_id();
void decrease_id();
void increase_plant();
void decrease_plant();
void save();
//------------------------------------------------

void setup()
{
  pinMode(KEY_Auto, INPUT_PULLUP);
  pinMode(KEY_Menu, INPUT_PULLUP);
  pinMode(KEY_Left, INPUT_PULLUP);
  pinMode(KEY_Right, INPUT_PULLUP);
  pinMode(KEY_Select, INPUT_PULLUP);
  pinMode(KEY_On, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(KEY_Auto), set_zero,  RISING);
  attachInterrupt(digitalPinToInterrupt(KEY_Menu), increase_id,  RISING);
  attachInterrupt(digitalPinToInterrupt(KEY_Left), decrease_id,  RISING);
  attachInterrupt(digitalPinToInterrupt(KEY_Right), increase_plant,  RISING);
  attachInterrupt(digitalPinToInterrupt(KEY_Select), decrease_plant,  RISING);
  attachInterrupt(digitalPinToInterrupt(KEY_On), save,  RISING);
  
  lcd.begin();// initialize the LCD
  lcd.backlight();// Turn on the blacklight and print a message.
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("Setup load cell     ");
  scale.set_scale(calibration_factor); 
  scale.set_offset(zero_factor);  
  delay(1000);
  
  lcd.setCursor(0, 0);
  lcd.print("Setup DS1307     ");
  if (! rtc.begin()) 
  {
    lcd.print("Couldn't find RTC");
    while (1);
  }
  if (! rtc.isrunning()) 
  {
    lcd.print("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
   // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
   //  rtc.adjust(DateTime(2018, 2, 9, 21, 35, 0));
  }
  delay(1000);
     
  lcd.setCursor(0, 0);
  lcd.print("Setting WIFI        ");
  WiFiManager wifiManager;
  wifiManager.autoConnect("SmartFarm");
  delay(1000);
  
  lcd.setCursor(0, 0);
  lcd.print("Setting SD Card     ");
  if(!SD.begin())
  {
    lcd.print("Card Mount Failed");
    delay(1000);   
    return;
  }
  
  lcd.setCursor(0, 0);
  lcd.print("IP:");
  lcd.print(WiFi.localIP());
  uint8_t cardType = SD.cardType();  
  if(cardType == CARD_NONE)
  {
      lcd.print(" NoSD");
      return;
  }
  if(cardType == CARD_MMC)
  {
      lcd.print(" MMC");
  } 
  else if(cardType == CARD_SD)
  {
      lcd.print(" SDSC");
  } 
  else if(cardType == CARD_SDHC)
  {
      lcd.print(" SDHC");
  } 
  else 
  {
      lcd.print(" UNKNOWN");
  }
  
//writeFile(SD, "/log.txt", "Hello ");


   
}  


void loop()
{
  if(savedata)
  {
    DateTime now = rtc.now();
    char cstr[16];
    appendFile(SD, "/log.txt", "ID=");
    itoa(ID, cstr, 10);
    appendFile(SD, "/log.txt", cstr);
    appendFile(SD, "/log.txt", " Plant=");
    itoa(plant, cstr, 10);
    appendFile(SD, "/log.txt", cstr);
    appendFile(SD, "/log.txt", " Date:");
    itoa(now.day(), cstr, 10);
    appendFile(SD, "/log.txt", cstr);
    appendFile(SD, "/log.txt", "/");
    itoa(now.month(), cstr, 10);
    appendFile(SD, "/log.txt", cstr);
    appendFile(SD, "/log.txt", "/");
    itoa(now.year(), cstr, 10);
    appendFile(SD, "/log.txt", cstr);
    appendFile(SD, "/log.txt", " Time:");
    itoa(now.hour(), cstr, 10);
    appendFile(SD, "/log.txt", cstr);
    appendFile(SD, "/log.txt", ":");
    itoa(now.minute(), cstr, 10);
    appendFile(SD, "/log.txt", cstr);
    appendFile(SD, "/log.txt", " Weight:");
    //itoa(weight, cstr, 10);
    //appendFile(SD, "/log.txt", weight);
    appendFile(SD, "/log.txt", " Kg\r\n");
    savedata=false;
    delay(1000);
     lcd.setCursor(15, 0);
    uint8_t cardType = SD.cardType();  
  if(cardType == CARD_NONE)
  {
      lcd.print(" NoSD");
      return;
  }
  if(cardType == CARD_MMC)
  {
      lcd.print(" MMC");
  } 
  else if(cardType == CARD_SD)
  {
      lcd.print(" SDSC");
  } 
  else if(cardType == CARD_SDHC)
  {
      lcd.print(" SDHC");
  } 
  else 
  {
      lcd.print(" UNKNOWN");
  }
  }
  lcd.setCursor(0, 1);
  lcd.print("ID=");
  lcd.print(ID);
  lcd.print(" Plant=");
  lcd.print(plant);
  lcd.print(" ");
  
  DateTime now = rtc.now();
  lcd.setCursor(0, 2);
  lcd.print(now.day(), DEC);
  lcd.print('/');
  lcd.print(now.month(), DEC);
  lcd.print('/');
  lcd.print(now.year(), DEC);
  lcd.print(' ');
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  lcd.print(now.second(), DEC);
  lcd.print(' ');
  weight = String(get_units_kg()+offset, DEC_POINT);
  lcd.setCursor(0, 3);
  lcd.print("Weight= ");
  lcd.print(weight);
  lcd.print("Kg   "); 
}

float get_units_kg()
{
  return(scale.get_units()*0.453592);
}

void writeFile(fs::FS &fs, const char * path, const char * message)
{
  lcd.setCursor(15, 0);
  lcd.print("Save");

  File file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    lcd.setCursor(15, 0);
    lcd.print("Nopen");
    return;
  }
  if(file.print(message))
  {
    lcd.setCursor(15, 0);
    lcd.print("Saved");
  } 
  else 
  {
    lcd.setCursor(15, 0);
    lcd.print("Nsave");
  }
}
void appendFile(fs::FS &fs, const char * path, const char * message){
    lcd.setCursor(15, 0);
  lcd.print("Save");

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        lcd.setCursor(15, 0);
    lcd.print("Nopen");
        return;
    }
    if(file.print(message)){
        lcd.setCursor(15, 0);
    lcd.print("Saved");
    } else {
        lcd.setCursor(15, 0);
    lcd.print("Nsave");
    }
}
void set_zero()
{
 ID=ID+1;
 return;
}
void increase_id()
{
  ID=ID+1;
}
void decrease_id()
{
  if (ID>1)
  {
    ID=ID-1;
  }
}
void increase_plant()
{
  plant=plant+1;
}
void decrease_plant()
{
  if(plant>1)
  {
    plant=plant-1;
  }
}
void save()
{
   savedata=true;
}

