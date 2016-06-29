/*
  Example code for pH and E.C. mini Kit
  http://www.cyber-plant.com
  by CyberPlant LLC, 13 December 2015
  This example code is in the public domain.
  upd. 15 March 2016
*/
#include <SimpleTimer.h>
#include "Wire.h"
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define X0 0.0
#define X1 2.0
#define X2 12.88
#define X3 80.0
#define pHtoI2C 0x48
#define T 273.15 // degrees Kelvin
#define alphaLTC 0.022 // The linear Temperature coefficient
#define ONE_WIRE_BUS 3 // Connect DS18B20 Temp sensor to pin D3


float voltage, pHvoltage, pH, pHlcd, pHStateL, pHStateH, Temp, TempStateL, TempStateH;
float EC;
float TempManual = 25.0;

int sequence = 0;

volatile bool counting;
volatile unsigned long events;
unsigned long total;
int incomingByte = 0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
SimpleTimer timer;

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

  // pinMode(2, INPUT_PULLUP); //  An internal 20K-ohm resistor is pulled to 5V. If you use hardware pull-up delete this
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

  Read_EE();
  timer.setInterval(1000L, TotalEvents);
  attachInterrupt (0, eventISR, FALLING);
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

void eventISR ()
{
  if (counting == true)
    events++;
}

void TotalEvents()
{
  if (counting == true) {
    counting = false;
    total = events;
    TempRead();
  }
  else if (counting == false) {
    noInterrupts ();
    events = 0;
    EIFR = bit (INTF0);
    counting = true;
    interrupts ();
  }
}

void TempRead()
{
  sensors.requestTemperatures();
  Temp = sensors.getTempCByIndex(0);
  if (-20 > Temp || Temp > 200) {
    Temp = TempManual;
  }
  ECcalculate();
}

void ECcalculate()
{
  float A;
  float B;
  float C;

  if (total < Y0)
    C = 0;
  else if (total >= Y0 && total < (Y0 + Y1))
  {
    A = (Y1 - Y0) / (X1 - X0);
    B = Y0 - (A * X0);
    C = (total  - B) / A;
  }
  else if (total >= (Y0 + Y1) && total < (Y2 + Y1 + Y0))
  {
    A = (Y2 - Y1) / (X2 - X1);
    B = Y1 - (A * X1);
    C = (total  - B) / A;
  }
  else if (total >= (Y2 + Y1 + Y0))
  {
    A = (Y3 - Y2) / (X3 - X2);
    B = Y2 - (A * X2);
    C = (total - B) / A;
  }

  EC = (C / (1 + alphaLTC * (Temp - 25.00)));

  cicleRead();
}

void cicleRead()
{
  ADSread(0);
  pHvoltage = voltage;
  if (pHvoltage > 0)
    pH = IsoP - AlphaL * (T + Temp) * pHvoltage;
  else if (pHvoltage < 0)
    pH = IsoP - AlphaH * (T + Temp) * pHvoltage;
  showResults ();

}

void ADSread(int rate) // read ADS
{

  byte highbyte, lowbyte, configRegister;
  float data;

  Wire.requestFrom(pHtoI2C, 3);
  //Wire.requestFrom(pHtoI2C, 3, sizeof(byte) * 3);
  if (Wire.available())
  {
    highbyte = Wire.read();
    lowbyte = Wire.read();
    configRegister = Wire.read();

    data = highbyte * 256;
    data = data + lowbyte;
    voltage = data * 2.048 ;


    switch (rate)
    {
      case 0:
        voltage = voltage / 32768;
        break;
      /*
          case 1:
            voltage = voltage / 3276.8;
            break;
      */
      case 2:
        voltage = voltage / 327.68;
        break;

      case 3:
        voltage = voltage / 32.768;
        break;
    }
  }
}

void cal_sensors()
{
  Serial.println(" ");

  if (incomingByte == 53) // press key "5"
  {
    Serial.print("Reset EC ...");
    Y0 = 220;
    Y1 = 1245;
    Y2 = 5282;
    Y3 = 17255;
  }
  else if (incomingByte == 48) // press key "0"
  {
    Serial.print("Cal. 0,000 uS ...");
    Y0 = total;
  }

  else if (incomingByte == 49) // press key "1"
  {
    Serial.print("Cal. 2,000 uS ...");
    Y1 = total / (0.995 + alphaLTC * (Temp - 25.00));
  }

  else if (incomingByte == 50) // press key "2"
  {
    Serial.print("Cal. 12,880 uS ...");
    Y2 = total / (0.997 + alphaLTC * (Temp - 25.00));
  }

  else if (incomingByte == 51) // press key "3"
  {
    Serial.print("Cal. 80,000 uS ...");
    Y3 = total / (0.995 + alphaLTC * (Temp - 25.00));
  }

  if (incomingByte == 56) // press key "8"
  {
    Serial.print("Reset pH ...");
    IsoP = 7.5099949836;
    AlphaL = 0.0778344535;
    AlphaH = 0.0850976657;
  }
  else if (incomingByte == 52) // press key "4"
  {
    Serial.print("Cal. pH 4.00 ...");
    AlphaL = (IsoP - 4) / voltage / (T + TempManual);
  }
  else if (incomingByte == 55) // press key "7"
  {
    Serial.print("Cal. pH 7.00 ...");
    IsoP = (IsoP - pH + 7.00);
  }
  else if (incomingByte == 57) // press key "9"
  {
    Serial.print("Cal. pH 10.00 ...");
    AlphaH = (IsoP - 10) / voltage / (T + TempManual);
  }

  SaveSet();
  Serial.println(" complete");

}

void showResults ()
{
  Serial.println("  ");
  Serial.print("  Temp ");
  Serial.print(Temp, 2);
  Serial.print(" *C ");
  Serial.print("  pH ");
  Serial.print(pH);
  Serial.print("  E.C. ");
  Serial.println(EC);

}

void loop()
{

  if (Serial.available() > 0)
  {
    incomingByte = Serial.read();
    cal_sensors();
  }
  timer.run();

}


