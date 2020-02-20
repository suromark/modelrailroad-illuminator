// Pins in use:
// 10,11,12,13 used by Ethernet board ENC28J60 (SS; MOSI, MISO, CLK)
// 6,7 by TMC Display
// 2 by SoftSerial for TX, 3 for RX (but that's not used,
// we're doing half-duplex broadcast only)
//

// Call: http://<assigned-DHCP-IP>/cmd?T<targetBoardID>,P<pinnumber>,L<level0-255>
// T<ID> will be retained and always be prepended to the output.
// T is the start marker, end character is Newline, output is done by softwareserial.println()
// every comma will cause the next serial send of the partial string
// so outgoing line data is always looking like "T<ID>,<command><value><newline>"

#include <Arduino.h>
#include "EtherCard.h"
#include "TM1637Display.h"
#include "globals.h"
#include "toytrainserialsender.h"

#define DEBUG true
#define Serial if(DEBUG)Serial // Netter Trick :-)

static byte mymac[] = {0x74, 0x69, 0x69, 0x2D, 0x30, 0x31};
byte Ethernet::buffer[1024]; // tcp/ip send and receive buffer -- at least size of expected message/response text!

toytrainserialsender toytps;

TM1637Display display(6, 7); // CLK, DIO

void doInfodisplay();
void ethernet_loop();
void doparser(char *dertext);
void (*reboot)(void) = 0; //declare reset function at address 0

void setup()
{
  display.setBrightness(0x03);
  display.setSegments(t_boot, 4, 0);
  delay(1000);

  Serial.begin(115200);
  Serial.println(F("[Toy Train SoftSerial Sender]"));

  display.setSegments(t_sose, 4, 0);
  toytps.setup(); 
  delay(1000);

  display.setSegments(t_ethe, 4, 0);
  delay(1000);

  Serial.println(F("Start Ethernet"));

  if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0)
    Serial.println(F("Controller ERR"));

  display.setSegments(t_dhcp, 4, 0);
  Serial.println(F("Req DHCP"));

  if (!ether.dhcpSetup( "toytrainbox", true ))
  {
    Serial.println(F("DHCP ERR"));
    display.setSegments(t_err);
    delay(5000);
    reboot();
  }

  ether.printIp("IP: ", ether.myip);
  ether.printIp("GW: ", ether.gwip);
  ether.printIp("DNS:", ether.dnsip);
};

void loop()
{
  theTick = millis();
  ethernet_loop();
  toytps.loop();
  doInfodisplay();
};

void doInfodisplay()
{
  static unsigned long lastTick = 0;
  static int loopa = 0;
  if (lastTick + 1000 > theTick)
  {
    return;
  }

  display.showNumberDec(ether.myip[loopa], false, 3, 1);
  display.setSegments(t_pos + loopa, 1, 0);

  loopa++;
  if (loopa == 4)
  {
    loopa = 0;
  }

  lastTick = theTick;
}

void ethernet_loop()
{
  char t_commandstart[] = "GET /cmd?";

  /* Check for incoming requests */
  int payloadStart = ether.packetLoop(ether.packetReceive());
  if (payloadStart)
  {
    display.setSegments(t_call, 4, 0);
    char *payload = (char *)Ethernet::buffer + payloadStart;
    // Serial.println(payload);
    /* find the GET line and search for the keys */
    int compa = strncmp(payload, t_commandstart, sizeof t_commandstart - 1);

    if (0 == compa)
    {
      // Serial.println(__LINE__);
      /* parse the payload */
      doparser(payload + 8); // start at the question mark

      memcpy_P(ether.tcpOffset(), page_ok, sizeof page_ok);
      ether.httpServerReply(sizeof page_ok - 1);
    }
    else
    {
      // response page
      // Serial.println(__LINE__);
      memcpy_P(ether.tcpOffset(), page, sizeof page);
      ether.httpServerReply(sizeof page - 1);
    }
  }
}

void doparser(char *commands) // starts with the last character of the command, which is '?' - will always be skipped
{
  uint8_t target = 0;
  uint8_t ended = 0;
  int upperlimit = 500;
  int cursor = 0;
  int startmark = 0;
  long int intake = 0;
  char *nextpart;
  char outbuff[150]; // maximum permitted length of a single command; typically they'll only be 2 - 4 chars long

  Serial.println(F("PARSE"));

  while (cursor < upperlimit && !ended)
  {
    cursor++;

    if (commands[cursor] == ',') // found a comma, send the stuff that came before
    {
      if (target == 0)
      {
        continue; // do nothing if we don't have a target ID set
      }
      if (startmark == 0)
      {
        continue; // do nothing if the startmark is yet undeclared
      }
      if (startmark == cursor)
      {
        startmark = cursor + 1; // no string between mark+cursor, keep going
        continue;
      }
      if (cursor - startmark >= (int) sizeof outbuff)
      {
        ended = 1;
        continue; // buffer size exceeded, ignore the rest
      }
      // send the text between startmark and (cursor-1) to the target
      strncpy(outbuff, commands + startmark, cursor - startmark);
      outbuff[cursor - startmark] = '\0';
      toytps.send( target, outbuff );
      startmark = cursor + 1;
      continue;
    }

    if (commands[cursor] == ' ')
    {
      ended = 1;
      
      if (startmark < cursor )
      {
        // send final text piece between startmark and cursor - 1
      strncpy(outbuff, commands + startmark, cursor - startmark);
      outbuff[cursor - startmark] = '\0';
      toytps.send( target, outbuff );
      startmark = cursor + 1;
      }
      continue;
    }

    if (commands[cursor] == 'T')
    {
      intake = strtol(commands + cursor + 1, &nextpart, 10); // start processing after T
      if (commands + cursor + 1 == nextpart)
      { // the text processed didn't yield a result, endpoint has not moved
        Serial.println(F("ERR T"));
        return;
      }
      target = (uint8_t)intake;
      Serial.println("");
      Serial.print(F("OK T="));
      Serial.println(target);
      cursor = nextpart - commands; // increment cursor to the char after the number (either comma or blank);
      startmark = cursor + 1;
    }
  }
}