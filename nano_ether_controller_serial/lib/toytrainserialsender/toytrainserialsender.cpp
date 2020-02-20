#include "Arduino.h"
#include "SoftwareSerial.h"
#include "toytrainserialsender.h"

toytrainserialsender::toytrainserialsender()
{
}

void toytrainserialsender::setup()
{
  Serial.println( F("SoftSerial using TX pin = 2" ));
  softser = new SoftwareSerial(3, 2);
  softser->begin(9600);
}

void toytrainserialsender::send(byte recipient, char *sometext)
{
  char mybuf[160];
  snprintf(mybuf, sizeof mybuf, "T%i,%s", recipient, sometext);
  softser->println(mybuf);
  softser->println(mybuf); // redundancy cheat
}

void toytrainserialsender::loop()
{
  /* nothing to do yet */
}
