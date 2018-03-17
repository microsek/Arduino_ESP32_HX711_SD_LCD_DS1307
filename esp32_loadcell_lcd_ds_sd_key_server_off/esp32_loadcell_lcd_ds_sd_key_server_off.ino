/****************************
ESP32       |   Device and sensor
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
GPIO14      |   KEY_Auto
GPIO27      |   KEY_Menu
GPIO26      |   KEY_Left
GPIO25      |   KEY_Right
GPIO4       |   KEY_Select
GPIO15      |   KEY_On 
----------------------------
GPIO16(rx2) |   SW ON/OFF WIFI
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

#define zero_factor 46312
#define DOUT  33
#define CLK   32
#define DEC_POINT  2

#define KEY_Auto 14
#define KEY_Menu 27
#define KEY_Left 26
#define KEY_Right 25
#define KEY_Select 4
#define KEY_On 15

#define SW_WIFI 16

#define SERVER_PORT 80 
//----------- weight ---------------------
float calibration_factor =87722.00;
float offset=0;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
int ID=1;
int plant=1;
float weight_zero=0.00;
String weight;
String str_host = "Host: www.100smartfarm.com\r\n\r\n";
String str_get  =" HTTP/1.1\r\n";
const char* host = "100smartfarm.com";
String url;

boolean lastState_KEY_Auto;
boolean lastState_KEY_Menu;
boolean lastState_KEY_Left;
boolean lastState_KEY_Right;
boolean lastState_KEY_Select;
boolean lastState_KEY_On;
//------------------------------------
HX711 scale(DOUT, CLK);
LiquidCrystal_I2C lcd(0x27, 20, 4);// Set the LCD address to 0x27 for a 20 chars and 4 line display //esp32 scl22 sda21
RTC_DS1307 rtc;
WiFiServer server(SERVER_PORT);     //เปิดใช้งาน TCP Port 80
WiFiClient client;              //ประกาศใช้  client 
//------------ define function --------------------
float get_units_kg();
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void Client_Request();
//------------------------------------------------

void setup()
{
  pinMode(KEY_Auto, INPUT_PULLUP);
  pinMode(KEY_Menu, INPUT_PULLUP);
  pinMode(KEY_Left, INPUT_PULLUP);
  pinMode(KEY_Right, INPUT_PULLUP);
  pinMode(KEY_Select, INPUT_PULLUP);
  pinMode(KEY_On, INPUT_PULLUP);
  pinMode(SW_WIFI, INPUT_PULLUP);
  
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
     rtc.adjust(DateTime(2018, 3, 16, 22, 16, 0));
  }
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("Setting SD Card     ");
  if(!SD.begin())
  {
    lcd.print("Card Mount Failed");
    delay(1000);   
    return;
  }
  
  if ( digitalRead(SW_WIFI) == LOW ) 
  {
    lcd.setCursor(0, 0);
    lcd.print("Setting WIFI        ");
    WiFiManager wifiManager;
    wifiManager.autoConnect("100SmartFarm");
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("IP:");
    lcd.print(WiFi.localIP());
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("Offline MODE");
  }

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
//******************** Loop Function *****************************************
void loop()
{ 
//---------------------------- Set sero -------------------------------------
  if ( digitalRead(KEY_Auto) == LOW ) 
  {
    delay (50);
    if ((digitalRead(KEY_Auto) == LOW) &&( lastState_KEY_Auto==HIGH))
    {
      weight_zero=get_units_kg();
    }
  }
  lastState_KEY_Auto=digitalRead(KEY_Auto);
// --------------------------- Save data ----------------------------------- 
  if ( digitalRead(KEY_On) == LOW ) 
  {
    delay (50);
    if ((digitalRead(KEY_On) == LOW)&&( lastState_KEY_On==HIGH))
    { 
      DateTime now = rtc.now();
      if ( digitalRead(SW_WIFI) == LOW ) 
      {
      //---------------------- server ------------------------------------
        url = "GET /admin/savedata.php?p_code=";
        url += plant;
        url += "&w_code=";
        url += ID;
        url += "&w_weight=";
        url += weight;    
        url += "&w_time=";
        url += now.year();
        url += "-";
        url += now.month();
        url += "-";
        url += now.day();
        url += "%20";
        url += now.hour();
        url += ".";
        url += now.minute();
        Client_Request();
      }
      //---------------------- SD Card ----------------------------------
      lcd.setCursor(0, 0);
      lcd.print("Save to SD card");  
      weight = String(get_units_kg(), DEC_POINT);
      
      String data_buffer;
      char date_time_file_name[50];
      char data_save_sd_card[100];
      data_buffer="/";
      data_buffer+=now.year();
      data_buffer+="-";
      data_buffer+=now.month();
      data_buffer+="-";
      data_buffer+=now.day();
      data_buffer+=".txt";
      data_buffer.toCharArray(date_time_file_name, 50);     
      
      data_buffer="Plant=";
      data_buffer+=plant;
      data_buffer+=" ID=";
      data_buffer+=ID;
      data_buffer+=" Time=";
      data_buffer+=now.hour();
      data_buffer+=":";
      data_buffer+=now.minute();
      data_buffer+=" Weight=";
      data_buffer+=weight;
      data_buffer+= "Kg\r\n";
      data_buffer.toCharArray(data_save_sd_card, 100); 
      appendFile(SD, date_time_file_name, data_save_sd_card);

      if ( digitalRead(SW_WIFI) == LOW ) 
      {
        lcd.setCursor(0, 0);
        lcd.print("IP:");
        lcd.print(WiFi.localIP());
      }
      else
      {
        lcd.setCursor(0, 0);
        lcd.print("Offline MODE   ");
      }
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
    
  }
   lastState_KEY_On=digitalRead(KEY_On);
//---------------------------Add ID --------------------------------------
  if (( digitalRead(KEY_Menu) == LOW ) &&( lastState_KEY_Menu==HIGH))
  {
    delay (50);
    if (digitalRead(KEY_Menu) == LOW)
    {
      ID++;
    }
  }
  lastState_KEY_Menu=digitalRead(KEY_Menu);
 //--------------------------Dec ID ---------------------------------------
  if (( digitalRead(KEY_Left) == LOW )&&( lastState_KEY_Left==HIGH) )
  {
    delay (50);
    if (digitalRead(KEY_Left) == LOW)
    {
      if (ID>1)
      {
        ID--;
      }
    }
  }
  lastState_KEY_Left=digitalRead(KEY_Left);
 // ------------------------Add Plant ------------------------------------ 
  if (( digitalRead(KEY_Right) == LOW ) &&( lastState_KEY_Right==HIGH) )
  {
    delay (50);
    if (digitalRead(KEY_Right) == LOW)
    {
      plant++;
    }
  }
  lastState_KEY_Right=digitalRead(KEY_Right);
 // ------------------------Dec Plant ----------------------------------- 
 if ( (digitalRead(KEY_Select) == LOW ) &&( lastState_KEY_Select==HIGH) )
  {
    delay (50);
    if (digitalRead(KEY_Select) == LOW)
    {
      if(plant>1)
      {
        plant--;
      }
    }
  }
    lastState_KEY_Select=digitalRead(KEY_Select);
 // ----------------------------------------- 
  lcd.setCursor(0, 1);
  lcd.print("Plant=");
  lcd.print(plant);
  lcd.print(" ID=");
  lcd.print(ID);
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
  weight = String(get_units_kg(), DEC_POINT);
  lcd.setCursor(0, 3);
  lcd.print("Weight= ");
  lcd.print(weight);
  lcd.print("Kg   "); 
}
//***************************** End loop ****************************
float get_units_kg()
{
  return((scale.get_units()*0.453592)-weight_zero)-0.15;
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
void appendFile(fs::FS &fs, const char * path, const char * message)
{
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

void Client_Request()
{
    lcd.setCursor(0, 0);
    lcd.print("Save to Server   "); 
    int cnt=0;
    while (!client.connect(host,80))  //เชื่อมต่อกับ Server และรอจนกว่าเชื่อมต่อสำเร็จ
    {
          delay(100);
          cnt++;
          if(cnt>50)                 //ถ้าหากใช้เวลาเชื่อมต่อเกิน 5 วินาที ให้ออกจากฟังก์ชั่น
          return;
    } 
    lcd.setCursor(0, 0);
    lcd.print("Save to server OK   "); 
    delay(500);
    client.print(url+str_get+str_host);       //ส่งคำสั่ง HTTP GET ไปยัง Server
    delay(100);
}

