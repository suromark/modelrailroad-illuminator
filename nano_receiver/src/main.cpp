#include <Arduino.h>
#include "SoftwareSerial.h"
#define BUSPIN 12
#define MYID 13 // the chip's designated address, usually each receiver gets a different one on your local installation

// PWM PINS: Uno, Nano, Mini : 3, 5, 6, 9, 10, 11; 490 Hz (pins 5 and 6: 980 Hz)
uint8_t pins[] = {A3, A2, A1, A0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
// board output channel assignments from top to bottom

uint8_t pinfx[sizeof pins];  // Pin Effects assignment, e.g. declare a pin to be flashing every 1 second automatically, or to do a random fire flicker, or to be the top light of a traffic signal ...
uint8_t pinlvl[sizeof pins]; // Pin Signal level assignment (storage), 0 = off, 255 = max. (non-PWM pins will be off for lvl < 128 and on for lvl > 128)

void setSignal();                // activate builtin LED  as data-in indicator
void doSignalLoop();             // takes care of disabling builtin LED after it's been on for some 100ms
void checkSerial();              // see if softserial incoming buffer processor
void parseBuffer(char *payload); // processes the message received
void performanceCheck();

void fx1_blink();         // A 1:1 ratio flashing light with 500 ms per phase
void fx2_randomflicker(); // change the value every 70 ms
void fx3_flash();         // a quick walking light along the pins, on = 90 ms, off = 90 ms * (pins -1), runs from pin 0 to pin 13 and repeats
void fx4_fluor();         // a flickering/defective fluorescent light effect that happens at random intervals
void fx5_twinflash();     // a quick twin walking light along the pins, on = 90 ms, off = 90 ms * ( (pins-1) / 2), runs two LEDs 7 steps apart from pin 0 to pin 6, pin 7 to pin 13 and repeats
void fx6_antiblink();     // a 1:1 ration flashing light, 300 ms per phase, alternating between adjacent pins (i.e. pin X = on when pin X+1 = off)
void fx7_breathe();       // soft blink, for PWM outputs (no effect on digital pins)

volatile unsigned long rightNow = 0;
volatile unsigned long didSignal = 0;
char sbuf[160];            // for serial data received
uint16_t sbuf_next = 0;    // pointer where to put next character (i.e. length of received data)
uint16_t is_receiving = 0; // if > 0, we're still tracking incoming serial data
uint8_t is_valid = 0;      // if > 0, serial data has ended with \n and we have buffer

SoftwareSerial softser(BUSPIN, 13); // TX, RX ; RX is not used

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Toytrain Serial Receiver"));
  Serial.print(F("Compiled "));
  Serial.println(__TIMESTAMP__);

  pinMode(LED_BUILTIN, OUTPUT);
  for (uint8_t i = 0; i < sizeof pins; i++)
  {
    pinfx[i] = 0;  /* init for all pins fx = none */
    pinlvl[i] = 0; /* Level = 0 = OFF */
    digitalWrite(pins[i], 0);
    pinMode(pins[i], OUTPUT);
  }
  setSignal();
  softser.begin(9600);
  Serial.println(F("System ready"));
  Serial.print(F("I am Number "));
  Serial.println(MYID);
}  

/* 





Mainloop
*/
void loop()
{

  rightNow = millis();
  doSignalLoop();

  checkSerial();

  /* effects module calls - since every effect has a different (in-coded) loop time base, they can't be unified easily */

  fx1_blink();
  fx2_randomflicker();
  fx3_flash();
  fx4_fluor();
  fx5_twinflash();
  fx6_antiblink();
  fx7_breathe();
  
  // performanceCheck(); /* a diagnostic output of "how many loops per second" to serial */"
}

/*
clear the signal LED after a certain time
*/
void doSignalLoop()
{
  if (didSignal > 0 && rightNow - didSignal > 100)
  {
    digitalWrite(LED_BUILTIN, LOW);
    didSignal = 0;
  }
}

/*
light up the builtin signal LED
*/
void setSignal()
{
  digitalWrite(LED_BUILTIN, HIGH);
  didSignal = rightNow;
}

/* count the loops that happen in one second */
void performanceCheck()
{
  static unsigned long last = 0;
  static unsigned long loopy = 0;

  loopy++;

  if (rightNow < 1000 + last)
  {
    return;
  }
  last = rightNow;

  Serial.println(loopy);
  loopy = 0;
}

/**
 * 
 * 
 * feed the internal buffer from the serial stream if anything's available
 * call the parser if end character is detected
 */
void checkSerial()
{

  if (softser.available() > 0) // option: use while() to process all the buffer in one main loop, or if() to fetch just one char from the buffer per main loop
  {
    byte onechar = softser.read();

    /* T = Startmark? */
    if (onechar == 'T' && is_receiving == 0)
    {
      is_receiving = 65535; // also a timeout counter */
      sbuf_next = 0;
      is_valid = 0;
    }

    sbuf[sbuf_next] = onechar;

    /* Newline = Endmark? */
    if (onechar == '\n' && is_receiving > 0)
    {
      is_valid = 1;
      sbuf[sbuf_next] = 0; /* endmark */
      is_receiving = 0;    /* we're now waiting for new start mark */
      Serial.println(F("Data received"));
      Serial.println(sbuf);
      /* call parser on buffer now */
      parseBuffer(sbuf);
    }
    else
    {

      if (sbuf_next < sizeof sbuf)
      {
        sbuf_next++;
      }
    }
  }
}

/**
 * 
 * 
 * 
 * Analyze the payload value
 * */
void parseBuffer(char *payload)
{
  int hits = 0;
  static unsigned int theTarget = 0; /* intermediate buffer for the ID in the message */
  static uint8_t thePin = 0;         /* container for last pinID received */
  unsigned int theValue = 0;         /* container for value of L<level> */

  setSignal();

  /* Set the level of the active pin: pattern check T<ID>,L<level> */

  hits = sscanf(payload, "T%u,L%u", &theTarget, &theValue);
  if (hits == 2 && theTarget == MYID)
  {
    pinlvl[thePin] = constrain(theValue, 0, 255);
    if (pinlvl[thePin] == 0)
    {
      pinfx[thePin] = 0;
    }
    analogWrite(pins[thePin], pinlvl[thePin]); /* digital pins will also respond to this, they turn on with L255 */
  }

  /* Set the level of all pins: pattern check T<ID>,G<level> */

  hits = sscanf(payload, "T%u,G%u", &theTarget, &theValue);
  if (hits == 2 && theTarget == MYID)
  {
    for (byte i = 0; i < sizeof pins; i++)
    {
      pinlvl[i] = constrain(theValue, 0, 255);
      if (pinlvl[i] == 0)
      {
        pinfx[i] = 0;
      }
      analogWrite(pins[i], pinlvl[i]);
    }
  }

  /* Define the active pin: pattern check T<ID>,P<PinIndex> */

  hits = sscanf(payload, "T%u,P%u", &theTarget, &theValue);
  if (hits == 2 && theTarget == MYID)
  {
    thePin = constrain(theValue, 0, sizeof pins - 1);
  }

  /* Define the effect of the active pin: pattern check T<ID>,F<Effect> */

  hits = sscanf(payload, "T%u,F%u", &theTarget, &theValue);
  if (hits == 2 && theTarget == MYID)
  {
    pinfx[thePin] = constrain(theValue, 0, 255);
  }
};

/*
a timer function that toggles all pins with assigned FX 1 - an automated slow walking blink effect with on:off = 1:1 
*/
void fx1_blink()
{
  static byte stepper = 0;
  static uint8_t blink[sizeof pins];
  static unsigned long last = 0;
  if (rightNow < 50 + last)
  {
    return;
  }
  last = rightNow;
  stepper++;
  if (stepper >= sizeof pins)
  {
    stepper = 0;
  }

  if (pinfx[stepper] == 1)
  {
    blink[stepper] = 1 - blink[stepper];
    analogWrite(pins[stepper], (blink[stepper] ? pinlvl[stepper] : 0));
  }
}

void fx2_randomflicker()
{
  static unsigned long last = 0;
  if (rightNow < 70 + last)
  {
    return;
  }
  last = rightNow;

  for (uint8_t i = 0; i < sizeof pinfx; i++)
  {
    if (pinfx[i] == 2)
    {
      analogWrite(pins[i], (rand() > RAND_MAX >> 1 ? pinlvl[i] : 0));
    }
  }
}

void fx3_flash() // walking flashing light; on:off ratio = 1:pins, recalculates every 90 ms
{
  static byte stepper = 0;
  static unsigned long last = 0;
  if (rightNow < 90 + last)
  {
    return;
  }
  last = rightNow;
  stepper++;
  if (stepper >= sizeof pins)
  {
    stepper = 0;
  }

  for (uint8_t i = 0; i < sizeof pinfx; i++)
  {
    if (pinfx[i] == 3)
    {
      analogWrite(pins[i], (stepper == i ? pinlvl[i] : 0));
    }
  }
}

void fx4_fluor() // fluorescent lights effect, recalculated every 20ms
{
  static uint16_t blink[sizeof pins]; // stores a countdown value for each channel
  static unsigned long last = 0;

  if (rightNow < 20 + last)
  {
    return;
  }
  last = rightNow;

  for (uint8_t i = 0; i < sizeof pinfx; i++)
  {
    if (pinfx[i] == 4)
    {
      byte lvl = pinlvl[i];
      if (blink[i] == 0)
      {
        blink[i] = random(50, 500); // create a new random countdown until the next flicker
      }
      if (blink[i] < random(30) || blink[i] > 390) // flicker when close to zero or freshly started
      {
        lvl = (byte)(((byte)rand() * pinlvl[i]) >> 8); //
      }
      if (blink[i] > 400) // if the countdown was very high, turn off the light for a bit
      {
        lvl = 0; //
      }
      analogWrite(pins[i], lvl);
      blink[i]--;
    }
  }
}

void fx5_twinflash() // walking lights effect at double speed (two LEDs in 14)
{
  static byte stepper = 0;
  static unsigned long last = 0;
  if (rightNow < 90 + last)
  {
    return;
  }
  last = rightNow;
  stepper++;
  if (stepper >= sizeof pins)
  {
    stepper = 0;
  }

  for (uint8_t i = 0; i < sizeof pinfx; i++)
  {
    if (pinfx[i] == 5)
    {
      analogWrite(pins[i], (stepper % 7 == i % 7 ? pinlvl[i] : 0));
    }
  }
}

/*
blink all channels synchronized, on:off = 1:1, 300 ms/phase, odd pins blink alternating to even pins
*/
void fx6_antiblink()
{
  static byte state = 0;
  static unsigned long last = 0;
  if (rightNow < 300 + last)
  {
    return;
  }
  last = rightNow;
  state++;

  for (byte i = 0; i < sizeof pins; i++)
  {
    if (pinfx[i] == 6)
    {
      analogWrite(pins[i], (((state ^ i) & 1) == 0 ? pinlvl[i] : 0));
    }
  }
}

/* 
cycle through 0-255 every 30 ms and use abs( val-127 ) for analog output.
Best for PWM pins, otherwise it's just another blink effect
*/

void fx7_breathe()
{
  static byte state[sizeof pins];
  static unsigned long last = 0;
  if (rightNow < 30 + last)
  {
    return;
  }
  last = rightNow;
  
  for (byte i = 0; i < sizeof pins; i++)
  {
    if (pinfx[i] == 7)
    {
      state[i] += 4;
      byte v = ( state[i] > 127 ? 127 - state[i] : state[i] ) << 1;
      analogWrite(pins[i], (int) ( pinlvl[i] * v ) >> 8 );
    }
  }
}
