/* vim: set ft=cpp: */

#include <stdio.h>
#include <string.h>

#include "RTClib.h"
#include <SimpleTimer.h>
#include <Wire.h>

// the timer object
SimpleTimer timer;

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
 
RTC_DS1307 RTC;
 
int adjusted = 0;
void setup () {
  Serial.begin(57600);
  Wire.begin();
  RTC.begin();

  Serial.println("Startup");
  //  if (! RTC.isrunning()) {
  //    Serial.println("RTC is NOT running!");
  //    // following line sets the RTC to the date & time this sketch was compiled
  //    RTC.adjust(DateTime(__DATE__, __TIME__));
  //    ++adjusted;
  //  }

  timer.setInterval((8*1000), sampleLux);
}
 
void loop() {
  timer.run();
}

void sampleLux () {
  char dateString[80];
  DateTime now = NULL;

  now = RTC.now();
  sprintf(dateString,"%04d-%02d-%02d %02d:%02d:%02d",now.year(),now.month(),now.day(),now.hour(),now.minute(),now.second());
  Serial.println(dateString);
}
