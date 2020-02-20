#ifndef toytrainserialsender_h
#define toytrainserialsender_h

#include "Arduino.h"
#include "SoftwareSerial.h"

class toytrainserialsender
{
private:
    SoftwareSerial* softser;

public:
    toytrainserialsender();
    void setup( );
    void send(  byte recipient, char *sometext );
    void loop();
};

#endif