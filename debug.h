typedef enum {
  BLINK_GREEN,
  BLINK_RED,
  BLINK_GREEN_RED,
  BLINK_GREEN_GREEN,
  BLINK_RED_RED,
  BLINK_RED_GREEN
} Debug_LED_Sequences;

//__xdata __at 0xE164 volatile uint32_t DEBUG_INT;
__xdata __at 0xE162 volatile uint8_t BREAKPOINT_REG;

void debugblink(Debug_LED_Sequences type);

#define BREAKPOINT(type) BREAKPOINT_REG = 0; while(!BREAKPOINT_REG){debugblink(type);}
