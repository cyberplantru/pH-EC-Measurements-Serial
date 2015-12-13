/*
  Example code for pH and E.C. measurements together 
  
  http://www.cyber-plant.com
  by CyberPlant LLC, 13 December 2015
  This example code is in the public domain.


*/
#include <Wire.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DS18B20.h>

#define X0 0.0
#define X1 2.0
#define X2 12.88
#define X3 80.0
#define pHtoI2C 0x48
#define T 273.15 // degrees Kelvin
#define alphaLTC 0.022 // The linear temperature coefficient

float voltage, data;
byte highbyte, lowbyte, configRegister;

float A;
float B;
float C;

long pulseCount = 0;  //a pulse counter variable
long pulseCal;
byte ECcal = 0;

unsigned long pulseTime,lastTime, duration, totalDuration;


unsigned int Interval=1000;
long previousMillis = 0;
unsigned long Time;

float pH;
float EC;
float temp;
float tempManual = 25.0;

int sequence = 0;

const byte ONEWIRE_PIN = 3; // temperature sensor ds18b20, pin D3
byte address[8];
OneWire onewire(ONEWIRE_PIN);
DS18B20 sensors(&onewire);

int incomingByte = 0;  
 
long Y0;
long Y1;
long Y2;
long Y3;
float IsoP;
float AlphaL;
float AlphaH;
  
void setup()
{

  
  Serial.begin(9600);
  Wire.begin();
  Time=millis();
  pinMode(2, INPUT_PULLUP); //  An internal 20K-ohm resistor is pulled to 5V. If you use hardware pull-up delete this
  sensors.begin();
  Search_sensors();
  Read_EE();
  
  Serial.println("Calibrate commands:");
  Serial.println("E.C. :");
  Serial.println("      Cal. 0,00 uS ---- 0");
  Serial.println("      Cal. 2,00 uS ---- 1");
  Serial.println("      Cal. 12,88 uS --- 2");
  Serial.println("      Cal. 80,00 uS --- 3");
  Serial.println("      Reset E.C. ------ 5");
  Serial.println("pH :");
  Serial.println("      Cal. pH 4.00 ---- 4");
  Serial.println("      Cal. pH 7.00 ---- 7");
  Serial.println("      Cal. pH 10.00 --- 9");
  Serial.println("      Reset pH ---------8");
  Serial.println("  ");
  delay(250);

}

 struct MyObject {
long Y0;
long Y1;
long Y2;
long Y3;
float IsoP;
float AlphaL;
float AlphaH;
};

void Read_EE()
{
  int eeAddress = 0; 

  MyObject customVar;
  EEPROM.get(eeAddress, customVar);
  
  Y0 = (customVar.Y0);
  Y1 = (customVar.Y1);
  Y2 = (customVar.Y2);
  Y3 = (customVar.Y3);
  IsoP = (customVar.IsoP);
  AlphaL = (customVar.AlphaL);
  AlphaH = (customVar.AlphaH);
}

void SaveSet()
{
  int eeAddress = 0;
  MyObject customVar = {
    Y0,
    Y1,
    Y2,
    Y3,
    IsoP,
    AlphaL,
    AlphaH
  };
  EEPROM.put(eeAddress, customVar);
}

float temp_read() // calculate pH
{
      sensors.request(address);
  
  // Waiting (block the program) for measurement reesults
  while(!sensors.available());
  
    temp = sensors.readTemperature(address);
    return temp;
}


void ECread()  //graph function of read EC
{
     if (pulseCal>Y0 && pulseCal<Y1 )
      {
        A = (Y1 - Y0) / (X1 - X0);
        B = Y0 - (A * X0);
        C = (pulseCal - B) / A;
      }
      
      if (pulseCal > Y1 && pulseCal<Y2 )
      {
        A = (Y2-Y1) / (X2 - X1);
        B = Y1 - (A * X1);
        C = (pulseCal - B) / A;
      }
      if (pulseCal > Y2)
      {
        A = (Y3-Y2) / (X3 - X2);
        B = Y2 - (A * X2);
        C = (pulseCal - B) / A;
      }
      
    EC = (C / (1 + alphaLTC * (temp-25.00)));
}

void onPulse() // EC pulse counter
{
  pulseCount++;
}

void pH_read() // read ADS
{
  Wire.requestFrom(pHtoI2C, 3);
  while(Wire.available())
  {
    highbyte = Wire.read();
    lowbyte = Wire.read();
    configRegister = Wire.read();
  }
  data = highbyte * 256;
  data = data + lowbyte;
  voltage = data * 2.048 ;
  voltage = voltage / 32768;
}

void pH_calculate() // calculate pH
{
  if (voltage > 0)
  pH = IsoP - AlphaL * (T + tempManual) * voltage;
  else if (voltage < 0)
  pH = IsoP - AlphaH * (T + tempManual) * voltage;
}

void Search_sensors() // search ds18b20 temperature sensor
{
  address[8];
  
  onewire.reset_search();
  while(onewire.search(address))
  {
    if (address[0] != 0x28)
      continue;
      
    if (OneWire::crc8(address, 7) != address[7])
    {
      Serial.println(F("temp sensor connection error!"));
      temp = 25.0;
      break;
    }
   /*
    for (byte i=0; i<8; i++)
    {
      Serial.print(F("0x"));
      Serial.print(address[i], HEX);
      
      if (i < 7)
        Serial.print(F(", "));
    }
    
    */
  }

}

void cal_sensors()
{
  Serial.println(" ");
  
 if (incomingByte == 53) // press key "5"
 {
  Reset_EC();
 }
  else if (incomingByte == 48) // press key "0"
 {
  ECcal = 1;
  Serial.print("Cal. 0,00 uS ...");  
  Y0 = pulseCal;
  SaveSet();
  ECcal = 0;
 }
 
 else if (incomingByte == 49) // press key "1"
 {
  ECcal = 1;
  Serial.print("Cal. 2,00 uS ...");  
  Y1 = pulseCal / (1 + alphaLTC * (temp-25.00));
  ECread();
  while (EC > X1)
    {
    Y1++;
    ECread();
    }
  SaveSet();
  ECcal = 0;
 }
 
 else if (incomingByte == 50) // press key "2"
 {
  ECcal = 1;
  Serial.print("Cal. 12,88 uS ...");  
  Y2 = pulseCal / (1 + alphaLTC * (temp-25.00));
  ECread();
  while (EC > X2)
    {      
    Y2++;
    ECread();
    }
  SaveSet();
  ECcal = 0;
 }
 
  else if (incomingByte == 51) // press key "3"
 {
  ECcal = 1;
  Serial.print("Cal. 80,00 uS ..."); 
  Y3 = pulseCal / (1 + alphaLTC * (temp-25.00));
  ECread(); 
  while (EC > X3)
    { 
    Y3++;
    ECread();
    }
  SaveSet();
  ECcal = 0;
 }
 
 if (incomingByte == 56) // press key "8"
 {
  Reset_pH();
 }
 else if (incomingByte == 52) // press key "4"
 {
  Serial.print("Cal. pH 4.00 ...");
  AlphaL = (IsoP - 4) / voltage / (T + tempManual);
  SaveSet();
 }
 else if (incomingByte == 55) // press key "7"
 {
  Serial.print("Cal. pH 7.00 ...");
  IsoP = (IsoP - pH + 7.00);
  SaveSet();
 }
  else if (incomingByte == 57) // press key "9"
 {
  Serial.print("Cal. pH 10.00 ...");
  AlphaH = (IsoP - 10) / voltage / (T + tempManual); 
  SaveSet();
 }
   Serial.println(" complete");
}

void Reset_EC()
{
  ECcal = 1; 
  Serial.print("Reset EC ..."); 
  Y0 = 220;
  Y1 = 1245;
  Y2 = 5282;
  Y3 = 17255;
  SaveSet();
  ECcal = 0;
}

void Reset_pH()
{
  Serial.print("Reset pH ...");

  IsoP = 7.5099949836;
  AlphaL = 0.0778344535;
  AlphaH = 0.0850976657;
  SaveSet();
}

void loop()
{
  
if (ECcal == 0)
{
  if (millis()-Time>=Interval)
{  
  Time = millis();
  
  sequence ++;
  if (sequence > 1)
  sequence = 0;


   if (sequence==0)
  {
    pulseCount=0; //reset the pulse counter
    attachInterrupt(0,onPulse,RISING); //attach an interrupt counter to interrupt pin 1 (digital pin #3) -- the only other possible pin on the 328p is interrupt pin #0 (digital pin #2)
  }
  
   if (sequence==1)
  {
    detachInterrupt(0);
    pulseCal = pulseCount;
    temp_read();
    if (temp > 200 || temp < -20 )
  { 
    temp = tempManual;
    Serial.println("temp sensor connection error!");
    Search_sensors();
  }


  ECread();
  pH_read();
  pH_calculate();
  
  // Prints measurements on Serial Monitor
  Serial.println("  ");
  Serial.print("t ");
  Serial.print(temp);
  Serial.print(F(" *C"));
  Serial.print("    pH ");
  Serial.print(pH); // uS/cm
  Serial.print("    E.C. ");
  Serial.print(EC); // uS/cm
  Serial.print("    pulses/sec = ");
  Serial.print(pulseCal);
  Serial.print("    C = ");
  Serial.println(C); // Conductivity without temperature compensation
  }
}

    if (Serial.available() > 0) //  function of calibration E.C.
    {  
        incomingByte = Serial.read();
        cal_sensors();
    }
}
}

/*-----------------------------------End loop---------------------------------------*/

