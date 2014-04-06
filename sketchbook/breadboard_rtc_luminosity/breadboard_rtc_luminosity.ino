/* vim: set ft=cpp: */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "FifoMessageBuffer.h"
#include "RTClib.h"
#include <SFE_TSL2561.h>
#include <SimpleTimer.h>
#include <Wire.h>

// the timer object
SimpleTimer timer;

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
 
RTC_DS1307 RTC;
SFE_TSL2561 light;
 
int adjusted = 0;

boolean lum_gain = 0;     // Gain setting, 0 = X1, 1 = X16;
// If lum_time = 0, integration will be 13.7ms
// If lum_time = 1, integration will be 101ms
// If lum_time = 2, integration will be 402ms
// If lum_time = 3, use manual start / stop to perform your own integration
unsigned char lum_time = 1;
unsigned int lum_ms;  // Set by light.setTiming(); Integration ("shutter") time in milliseconds

void setup () {
  Serial.begin(57600);
  Wire.begin();
  RTC.begin();
  setup_TSL2561();

  Serial.println("Startup");
  //  if (! RTC.isrunning()) {
  //    Serial.println("RTC is NOT running!");
  //    // following line sets the RTC to the date & time this sketch was compiled
  //    RTC.adjust(DateTime(__DATE__, __TIME__));
  //    ++adjusted;
  //  }

  timer.setInterval((8*1000), sampleLux);
}
void setup_TSL2561() {
  light.begin();
  lum_gain = 0;
  lum_time = 1;
  light.setTiming(lum_gain,lum_time,lum_ms);
  light.setPowerUp();
}
 
void loop() {
  timer.run();
}

void sampleLux () {
  char dateString[80];
  char formated_double[16];
  char message[200];
  DateTime now = NULL;

  now = RTC.now();
  sprintf(dateString,"%04d-%02d-%02d %02d:%02d:%02d",now.year(),now.month(),now.day(),now.hour(),now.minute(),now.second());
  sprintf(message,"%s sampleLux()",dateString);
  Serial.println(message);

  unsigned int data0, data1;
  if (light.getData(data0,data1)) {
    double lux;    // Resulting lux value
    boolean good;  // True if neither sensor is saturated

    sprintf(message,"%s light.getData() success. lum_gain %d lum_ms %d data0 %d data1 %d",dateString,(lum_gain ? 1 : 0),lum_ms,data0,data1);
    Serial.println(message);
    good = light.getLux(lum_gain,lum_ms,data0,data1,lux);
    dtostrf(lux, 6, 4, formated_double);
    sprintf(message,"%s lux %s good %d",dateString,formated_double,(good ? 1 : 0));
    Serial.println(message);
  }
  else {
    sprintf(message,"%s sampleLux() light.getData() error",dateString);
    Serial.println(message);
  }
}
