#include "HX711.h"
#include <Wire.h> 

#define DOUT  33
#define CLK   32

#define DEC_POINT  2
#define STABLE  1

float offset=0;
float calibration_factor = 1;
float real_weight = 1.56;//kg

HX711 scale(DOUT, CLK);

unsigned char state=0;
long  FindZeroFactor();
float get_units_kg();
void  ReadWeight();
void  FindCalibrationFactor();

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Auto Calibrate Program");
  Serial.println("Send 'a' to Find Zero Factor (Please Remove all weight from scale)");
  Serial.println("Send 'b' to Find Calibration Factor (Please insert know the weight on the scales)");
  Serial.println("Send 'c' Show weight on the scales");
}
void loop() 
{
  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp=='a')
      state=1;
    if(temp=='b')
      state=2;   
    if(temp=='c')
      state=3;
  }

  switch(state)
  {
    case 0:
    break;
    case 1:
      FindZeroFactor();
   //   ReadWeight();
      state=0;
    break;
    case 2:
     FindCalibrationFactor();
     state=0;
    break;
    case 3:
      ReadWeight();
      delay(200);
    break;
    case 4:
     
    break;
    
  }
}

long FindZeroFactor()
{
     Serial.println("Find Zero Factor");
     Serial.println("Please wait .....");
     scale.set_scale();
     scale.tare(); 
     long zero_factor = scale.read_average(20); 
     Serial.print("Zero factor: ");
     Serial.println(zero_factor);
     return(zero_factor);
}

void FindCalibrationFactor()
{
  unsigned char flag_stable=0;
  unsigned int decpoint=1;
  for(unsigned char i=0;i<DEC_POINT+1;i++ )
    decpoint = decpoint*10;
  while(1)
  {
      scale.set_scale(calibration_factor); //Adjust to this calibration factor
      Serial.print("Reading: ");
      float read_weight = get_units_kg();
      String data = String(read_weight, DEC_POINT);
      Serial.print(data);
      Serial.print(" kg"); 
      Serial.print(" calibration_factor: ");
      Serial.print(calibration_factor);
      Serial.println();
      long r_weight      = (real_weight*decpoint);
      long int_read_weight = read_weight*decpoint;
      Serial.print(r_weight);
      Serial.print(" , ");
      Serial.println(int_read_weight);
      long x;
      if(r_weight == int_read_weight)
      {
        flag_stable++;
        if(flag_stable>=STABLE)
        {
          Serial.print("Calibration Factor is = ");
          Serial.println(calibration_factor);
          break;
         }
        }
       if(r_weight > int_read_weight)
          {
            x = r_weight - int_read_weight;
            if(x > 100)
              calibration_factor -= 1000; 
            else if(x > 100)
              calibration_factor -= 10;
            else
              calibration_factor -= 1;
            flag_stable=0;
          }
          if(r_weight < int_read_weight)
          {
            x =  int_read_weight-r_weight;
            if(x > 100)
              calibration_factor += 1000; 
            else if(x > 10)
              calibration_factor += 10;
            else
              calibration_factor += 1; 
            flag_stable=0; 
           }  
  }
}

float get_units_kg()
{
  return(scale.get_units()*0.453592);
}
void ReadWeight()
{
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  Serial.print("Reading: ");
  String data = String(get_units_kg()+offset, DEC_POINT);
  Serial.print(data);
  Serial.println(" kg");
}
