#include <stdint.h>
#include <delay.h>

#include "debug.h"

__sbit __at 0x80 + 0 STATUS_GREEN;
__sbit __at 0x80 + 1 STATUS_RED;

void debugblink(Debug_LED_Sequences type){
  STATUS_GREEN = FALSE;
  STATUS_RED = FALSE;

  switch(type){
  case BLINK_GREEN:
    STATUS_GREEN = TRUE;
    delay(500);
    break;
  case BLINK_RED:
    STATUS_RED = TRUE;
    delay(500);
    break;
  case BLINK_GREEN_RED:
    STATUS_GREEN = TRUE;
    delay(500);
    STATUS_GREEN = FALSE;
    STATUS_RED = TRUE;
    delay(500);
    break;
  case BLINK_GREEN_GREEN:
    STATUS_GREEN = TRUE;
    delay(500);
    STATUS_GREEN = FALSE;
    delay(250);
    STATUS_GREEN = TRUE;
    delay(500);
    break;
  case BLINK_RED_RED:
    STATUS_RED = TRUE;
    delay(500);
    STATUS_RED = FALSE;
    delay(250);
    STATUS_RED = TRUE;
    delay(500);
    break;
  case BLINK_RED_GREEN:
    STATUS_RED = TRUE;
    delay(500);
    STATUS_RED = FALSE;
    STATUS_GREEN = TRUE;
    delay(500);
    break;
  }
  STATUS_GREEN = FALSE;
  STATUS_RED = FALSE;

  delay(1000);
}
