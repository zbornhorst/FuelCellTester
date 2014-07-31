#include <DHT.h>

/*
Fuel Cell Test
*/
#include <SD.h>
#include <File.h> 
#include "Adafruit_MAX31855.h"
#include <Time.h>

#define DHTPIN 2
#define DHTTYPE DHT22

#define THERMODO 3
#define THERMOCS 4
#define THERMOCLK 5

#define SDCS 10

DHT dht(DHTPIN, DHTTYPE);

boolean useInternalReference = true;
double Vref;

File dataFile;
int rawV;
double finalV;

double vcc;
double current;
double x2; //FC Vcurrent HI
double x1; //FC Vcurrent LO

Adafruit_MAX31855 thermocouple(THERMOCLK, THERMOCS, THERMODO);
double temp;
float humidity;

boolean isInitialized = false;
time_t t;
char fn[100];

void setup() {  
  Serial.begin (9600);
  
  
  dht.begin();
  initializeSDCard();  
  //Initialize the time
  Serial.println("Please set the Arduino clock to GMT (MMDDYYYYHHMMSS)");
  
  //Set the voltage reference
  if (useInternalReference) {
    analogReference(INTERNAL);
    Vref = 1.1;
  }
  else {
    Vref = 5;
  }
}

void loop() {
  if (!isInitialized) {
    //setTime(13, 39, 0, 30, 7, 2014);
    while (!setArduinoClock()) {
      ;
    }
    delay(1000);
    t = now();
    Serial.println(t);
    sprintf(fn, "FCtest_%lu.txt", (unsigned long)t);
    Serial.print("Opening file: ");
    Serial.println(fn);
    dataFile = SD.open ("FCtest.txt", FILE_WRITE);
    dataFile.println ("Time, FC Voltage Raw (V), FC Voltage Calc (V), FC Vcurrent LO (V), FC Vcurrent HI (V), FC Current Calc (A), Thermocouple Calc (C), Rel Humidity Calc (%)");
    dataFile.close();

    isInitialized = true;
  }
  
  finalV = getVoltage ();
  current = getCurrent ();
  temp = getTemp ();
  humidity = getHumidity ();
  dataFile = SD.open ("FCtest.txt", FILE_WRITE); 
  
  if (dataFile){ 
    t = now();
    dataFile.print (t);
    dataFile.print (", ");
    dataFile.print (rawV);
    dataFile.print (", ");
    dataFile.print (finalV);
    dataFile.print (", ");
    dataFile.print (x1);
    dataFile.print (", ");
    dataFile.print (x2);
    dataFile.print (", ");
    dataFile.print (current);
    dataFile.print (", ");
    dataFile.print (temp);
    dataFile.print (", ");
    dataFile.println (humidity);
    dataFile.close();
    
    Serial.print ("Voltage (V): ");
    Serial.println (finalV);
    Serial.print ("Current (A): ");
    Serial.println (current);
    Serial.print ("Thermocouple Calc (C): ");
    Serial.println (temp);
    Serial.print ("Rel Humidity Calc (%): ");
    Serial.println (humidity);
  }
  else {
    Serial.println ("error opening FCtest.txt");
  }
  delay (2000);
}

double getVoltage (){
  rawV = analogRead(0);
  
  if (useInternalReference) {
    finalV = (35.0 * (double)rawV / 1023.0 * Vref);
  }
  else {
    //Correct the reference voltage with readVcc
    //ZB:  vcc was a long, so it was either =5 or =4!!
    vcc = ((double)readVcc()) / 1000.0;
    finalV = (35.0 * ((double)rawV) / 1023.0 * vcc);
    
    //Calibrated value after Andy S turned up the "volume"
//    finalV = ((double)analogRead(0)) * .05;
  }
  
  return finalV;
}

double getCurrent (){
  double v; //voltage drop
  double r = .1;
  //double Vref;
  x1 = analogRead (1);
  x2= analogRead (2);
 
  //ZB:  needed to multiply by full scale voltage
  v = ((double)x2-(double)x1)/1023.0 * Vref;
  
  current = v/r;
  
  return current;
}

double getTemp (){
double c = thermocouple.readCelsius();
   if (isnan(c)) {
     Serial.println("Something wrong with thermocouple!");
   }
     return c;
}

float getHumidity (){
float h = dht.readHumidity();
   if (isnan(h)) {
     Serial.println("Failed to read from DHT sensor!");
   }
     return h;
}

long readVcc() {
   long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}

void initializeSDCard ()
//initializes SD card
{
  Serial.print ("Initializing SD card...");
  pinMode (SDCS, OUTPUT);
   if (!SD.begin(SDCS)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
}

boolean setArduinoClock() {
  int n = 0;
  int myYear = 0;
  int myMonth = 0;
  int myDay = 0;
  int myHour = 0;
  int myMinute = 0;
  int mySecond = 0;
  if (Serial.available()) {
    while ( (n<14) ) {
      byte inByte = Serial.read();
      if ( (inByte > 46) && (inByte < 57)) {
        int val = (int)(inByte - '0');
        switch (n) {
          case 0:
            myMonth += (val)*10;
            break;
          case 1:
            myMonth += val;
            break;
          case 2:
            myDay += (val)*10;
            break;
          case 3:
            myDay += val;
            break;
          case 4:
            myYear += (val)*1000;
            break;
          case 5:
            myYear += (val)*100;
            break;
          case 6:
            myYear += (val)*10;
            break;
          case 7:
            myYear += val;
            break;
          case 8:
            myHour += (val)*10;
            break;
          case 9:
            myHour += val;
            break;
          case 10:
            myMinute += (val)*10;
            break;
          case 11:
            myMinute += val;
            break;
          case 12:
            mySecond += (val)*10;
            break;
          case 13:
            mySecond += val;
            break;
          default:
            Serial.println("ERROR: Should not reach this line of code.");
            break;
        } // \switch
        n++;
      } // \if input is an integer
    } // \while n<14 loop
    if ((myHour > 23) ||
        (myMinute > 59) ||
        (mySecond > 59) ||
        (myDay > 31) ||
        (myMonth > 12) ||
        (myYear < 2014) ) {
          Serial.print(myHour);
          Serial.print(" ");
          Serial.print(myMinute);
          Serial.print(" ");
          Serial.print(mySecond);
          Serial.print(" ");
          Serial.print(myDay);
          Serial.print(" ");
          Serial.print(myMonth);
          Serial.print(" ");
          Serial.print(myYear);
          Serial.print(" ");
          Serial.println("Clock set failed");
          return false;
    }
    else {
      setTime(myHour, myMinute, mySecond, myDay, myMonth, myYear);
      Serial.println("Set time to ");
      Serial.print(month());
      Serial.print(" ");
      Serial.print(day());
      Serial.print(" ");
      Serial.print(year());
      Serial.print(" ");
      Serial.print(hour());
      Serial.print(":");
      Serial.print(minute());
      Serial.print(":");
      Serial.print(second());
      Serial.print(" ");
      Serial.println(now());
      return true;
    }
  }
}
