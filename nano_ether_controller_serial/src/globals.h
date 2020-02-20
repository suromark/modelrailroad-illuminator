#include "TM1637Display.h"

const char page[] PROGMEM =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Example: use GET request with parameters in URL: /cmd?T44,P8,L0";

const char page_ok[] PROGMEM =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Got Command";

uint8_t t_pos[4] = {
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, // A
    SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,         // b
    SEG_D | SEG_E | SEG_G,                         // c
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_G          // d
};

uint8_t t_boot[] = {
    SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, /* b*/
    SEG_D | SEG_E | SEG_C | SEG_G,         /* o */
    SEG_D | SEG_E | SEG_C | SEG_G,         /* o */
    SEG_D | SEG_E | SEG_F | SEG_G          /* t */
};
uint8_t t_dhcp[] = {
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_G, /* d */
    SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,         /* H */
    SEG_A | SEG_D | SEG_E | SEG_F,                 /* C */
    SEG_A | SEG_B | SEG_E | SEG_F | SEG_G  /* p */
};
uint8_t t_lost[] = {
    SEG_D | SEG_E | SEG_F,                         /* L */
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_C | SEG_F, /* O */
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,         /* S */
    SEG_D | SEG_E | SEG_F | SEG_G                  /* t */
};
uint8_t t_conn[] = {
    SEG_D | SEG_E | SEG_G,         /* c */
    SEG_D | SEG_E | SEG_C | SEG_G, /* o */
    SEG_C | SEG_E | SEG_G,         /* n */
    SEG_C | SEG_E | SEG_G          /* n */
};
uint8_t t_err[] = {
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G, /* E */
    SEG_E | SEG_G,                         /* r */
    SEG_E | SEG_G,                         /* r */
    0};
uint8_t t_call[] = {
    SEG_A | SEG_D | SEG_E | SEG_F,                 /* C */
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, /* A */
    SEG_D | SEG_E | SEG_F,                         /* L */
    SEG_D | SEG_E | SEG_F                          /* L */
};

uint8_t t_pjon[] = {
    SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,                 /* P */
    SEG_B | SEG_C | SEG_D | SEG_E, /* J */
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, /* O */
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F          /* N */
};
uint8_t t_sose[] = {
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                 /* S */
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, /* O */
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                 /* S */
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G /* E */
};
uint8_t t_ethe[] = {
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G, /* E */
    SEG_D | SEG_E | SEG_F | SEG_G,                  /* t */
    SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,         /* H */
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G /* E */
};

unsigned long theTick = 0;
