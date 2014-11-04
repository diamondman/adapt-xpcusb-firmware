#include <stdio.h>
#include <stdint.h>
#include <fx2regs.h>
#include <fx2macros.h>
//#include <serial.h>
#include <delay.h>
#include <autovector.h>
#include <setupdat.h>
#include <eputils.h>
#include <delay.h>

#include "gpif.h"

#define VC_XILINX 0xB0

#define XC_DEVICE_DISABLE 0x10
#define XC_DEVICE_ENABLE 0x18
#define XC_REVERSE_WINDEX 0x20
#define XC_SET_JTAG_SPEED 0x28
#define XC_WRITE_JTAG_SINGLE 0x30
#define XC_READ_JTAG_SINGLE 0x38
#define XC_RET_CONSTANT 0x40
#define XC_GET_VERSION 0x50
#define XC_SET_CPLD_UPGRADE 0x52
//#define XC_UNKNOWN 0x68
//#define XC_JTAG_COMPLEX 0x70
//#define XC_MAGIC 0xA0
#define XC_JTAG_TRANSFER 0xA6

#define VER_FX2_FW 0
#define VER_CPLD_FW 1
#define VER_0x0400 2

#define SET_TRANSFER_COUNT(bak0, bak1, bak2, bak3)	\
  SYNCDELAY4;						\
  GPIFTCB0 = bak0;					\
  SYNCDELAY4;						\
  GPIFTCB1 = bak1;					\
  SYNCDELAY4;						\
  GPIFTCB2 = bak2;					\
  SYNCDELAY4;						\
  GPIFTCB3 = bak3;					\
  SYNCDELAY4;
#define WAIT_EP2FIFO_NOT_EMPTY() \
  SYNCDELAY4;		         \
  while (EP2FIFOFLGS & 2){}      \
  SYNCDELAY4;
#define WAIT_GPIF_DONE()      \
  while(!(GPIFTRIG & 0x80)){} \
  SYNCDELAY;

#define SYNCDELAY SYNCDELAY4;

/* IOA -- Assign names to the bits of this register for ease of use. */
__sbit __at 0x80 + 0 STATUS_GREEN;
__sbit __at 0x80 + 1 STATUS_RED;
__sbit __at 0x80 + 2 STATUS_POWER;
__sbit __at 0x80 + 3 UNKNOWN_1;
__sbit __at 0x80 + 4 CPLD_TMS_LEVEL;
__sbit __at 0x80 + 5 CPLD_ENABLE;
__sbit __at 0x80 + 6 CPLD_LAST_WORD;
__sbit __at 0x80 + 7 CPLD_EXTEND;

volatile __bit got_sud;
volatile __bit gpif_interrupted;
volatile uint8_t jtag_speed_mode;
volatile uint32_t __data transfer_bit_count;
volatile uint8_t __data gpiftcb0_bak;
volatile uint8_t __data gpiftcb1_bak;
volatile uint8_t __data gpiftcb2_bak;
volatile uint8_t __data gpiftcb3_bak;
volatile uint8_t __data low2bits;
volatile uint8_t __data jtag_action;

void process_xc_get_version(){
  switch (SETUPDAT[4]){
  case VER_FX2_FW:
    {
      EP0BUF[0] = 0x04;
      EP0BUF[1] = 0x04;
      EP0BCH=0;
      EP0BCL=2;
      break;
    }
  case VER_CPLD_FW:
    {
      //TODO make actually get CPLD FW version
      EP0BUF[0] = 0x00;
      EP0BUF[1] = 0x00;
      EP0BCH=0;
      EP0BCL=2;
      break;
    }
  case VER_0x0400:
    {
      EP0BUF[0] = 0x04;
      EP0BUF[1] = 0x00;
      EP0BCH=0;
      EP0BCL=2;
      break;
    }
  default:
    {
      EP0BUF[0] = 0x05;
      EP0BUF[1] = 0x06;
      EP0BCH=0;
      EP0BCL=2;
      break;
    }
  }
}

BOOL handle_xilinxcommand(){
  switch (SETUPDAT[2]) {
  case XC_DEVICE_DISABLE:
    {
      CPLD_ENABLE = FALSE;
      return TRUE;
    }
  case XC_DEVICE_ENABLE:
    {
      CPLD_ENABLE = TRUE;
      return TRUE;
    }
  case XC_REVERSE_WINDEX:
    {
      char tmp = SETUPDAT[4];
      EP0BUF[0] = ((tmp&0x1)<<7)|((tmp&0x2)<<5)|((tmp&0x4)<<3)|((tmp&0x8)<<1)|
        ((tmp&0x10)>>1)|((tmp&0x20)>>3)|((tmp&0x40)>>5)|((tmp&0x80)>>7);
      EP0BCH = 0;
      EP0BCL = 1;
      return TRUE;
    }
  case XC_SET_JTAG_SPEED:
    {
      jtag_speed_mode = SETUPDAT[4];
      return TRUE;
    }
/*case XC_WRITE_JTAG_SINGLE:
    {
      return TRUE;
    }
  case XC_READ_JTAG_SINGLE:
    {
      return TRUE;
    }*/
  case XC_RET_CONSTANT:
    {
      EP0BUF[0] = 0xB5;
      EP0BUF[1] = 0x03;
      EP0BCH=0;
      EP0BCL=2;
      return TRUE;
    }
  case XC_GET_VERSION:
    {
      process_xc_get_version();
      return TRUE;
    }
/*case XC_SET_CPLD_UPGRADE:
    {
      return TRUE;
    }
  case XC_UNKNOWN:
    {}
  case XC_JTAG_COMPLEX:
    {}
  case XC_MAGIC:
    {}*/
  case XC_JTAG_TRANSFER:
    {
      jtag_action=1;
      //TODO review to see if any endian magic messes this up
      //if done with a normal assignment.
      transfer_bit_count = ((((uint32_t)SETUPDAT[3])<<16)|
        (((uint32_t)SETUPDAT[5])<<8)|
        (uint32_t)SETUPDAT[4])+1;

      low2bits = transfer_bit_count & 3;
      transfer_bit_count = transfer_bit_count>>2;
      if (low2bits == 0){
        low2bits = 4;
        transfer_bit_count = transfer_bit_count-1;
      }

      EP0BCH=0;
      EP0BCL=4;
      return TRUE;
    }
  default:
    {
      EP0BCH=0;
      EP0BCL=0;
      return FALSE;
    }
  }
}



void gpifwf_isr() __interrupt GPIFWF_ISR {
  gpif_interrupted = TRUE;
  CLEAR_GPIFWF();
  gpiftcb0_bak = GPIFTCB0;
  gpiftcb1_bak = GPIFTCB1;
  gpiftcb2_bak = GPIFTCB2;
  gpiftcb3_bak = GPIFTCB3; //always 0
  GPIFABORT = 0xFF;
}

// copied routines from setupdat.h
BOOL handle_get_descriptor() {
  return FALSE;
}

// copied usb jt routines from usbjt.h
// this firmware only supports 0,0
BOOL handle_get_interface(uint8_t ifc, uint8_t* alt_ifc) {
  //printf ( "Get Interface\n" );
  if (ifc==0) {*alt_ifc=0; return TRUE;} else { return FALSE;}
}
BOOL handle_set_interface(uint8_t ifc, uint8_t alt_ifc) {
  //printf ( "Set interface %d to alt: %d\n" , ifc, alt_ifc );

  if (ifc==0&&alt_ifc==0) {
    // SEE TRM 2.3.7
    // reset toggles
    RESETTOGGLE(0x02);
    RESETTOGGLE(0x86);
    // restore endpoints to default condition
    RESETFIFO(0x02);
    EP2BCL=0x80;
    SYNCDELAY;
    EP2BCL=0X80;
    SYNCDELAY;
    RESETFIFO(0x86);
    return TRUE;
  } else
    return FALSE;
}

BOOL handle_vendorcommand(uint8_t cmd) {

  switch ( cmd ) {
  case VC_XILINX:
    {
      if(handle_xilinxcommand()){
        return TRUE;
      }
    }
  }
  return FALSE;
}

void sudav_isr() __interrupt SUDAV_ISR {
  got_sud=TRUE;
  CLEAR_SUDAV();
}

void sof_isr () __interrupt SOF_ISR __using 1 {
  CLEAR_SOF();
}

// get/set configuration
uint8_t handle_get_configuration() {
  return 1;
}
BOOL handle_set_configuration(uint8_t cfg) {
  return cfg == 1 ? TRUE : FALSE; // we only handle cfg 1
}

//Add something for suspend
//revisit these two functions to check for compliance
void usbreset_isr() __interrupt USBRESET_ISR {
  handle_hispeed(FALSE);
  CLEAR_USBRESET();
}
void hispeed_isr() __interrupt HISPEED_ISR {
  handle_hispeed(TRUE);
  CLEAR_HISPEED();
}

void init(){
  RENUMERATE_UNCOND();

  SETCPUFREQ(CLK_48M);
  SETIF48MHZ();
  //sio0_init(57600); //Enable serial. Has been useful.
  CKCON &= 0xF8; //Make memory access 2 cycles TRM page 272

  USE_USB_INTS();
  ENABLE_SUDAV();
  USE_GPIF_INTS();
  ENABLE_HISPEED();
  ENABLE_USBRESET();
  ENABLE_GPIFWF();

  //REVCTL = 0x03; //They say this is necessary but it just breaks all out endpoints even with priming.
  SYNCDELAY;
  GpifInit();
  SYNCDELAY;

  //Can't seem to get the GPIF editor to set these correctly. Overriding.
  PORTACFG = 0;
  OEA = 0xFB;
  IOA = 8;
  PORTCCFG = 0; //Wrong default. Overriding. C is the address.
  //OEC = 0xFF; //Same as GpifInit
  IOC = 0;
  PORTECFG = 0;
  OEE = 0xD8;

  //BIT 6 0 = OUT, 1 = IN. This is same direction as USB. IN means to PC
  EP2CFG &= 0xF4; //EP2 is READ FROM USB. 512byte OUT BULK set QUAD BUFF
  SYNCDELAY;
  EP4CFG &= 0x7F; //Mark EP4 invalid
  SYNCDELAY;
  EP8CFG &= 0x7F; //Mark EP8 invalid
  SYNCDELAY;

  //reset_FIFO_setup_ep
  SYNCDELAY;
  FIFORESET = 0x80;
  SYNCDELAY;
  FIFORESET = 0x02;
  SYNCDELAY;
  FIFORESET = 0x06;
  SYNCDELAY;
  FIFORESET = 0x00;
  SYNCDELAY;

  EP2FIFOCFG = 0x11;//0x15; //AUTO OUT, ZEROLEN=1???, WORDWIDE=1
  //If the revctl bits were wet like Cypress suggests, we should 
  //'prime the pump' here. Once per out buffer (4 times)
  //SYNCDELAY; OUTPKTEND = 0x82;

  //AUTO IN, WORDWIDE=1
  EP6FIFOCFG = 9;
  SYNCDELAY;

  // Auto-commit 512-byte packets
  EP6AUTOINLENH = 0x02; 
  SYNCDELAY;
  EP6AUTOINLENL = 0x00;
  SYNCDELAY;

  EA=1; // global interrupt enable
}

void main() {
  got_sud=FALSE;
  jtag_speed_mode = 0;
  jtag_action = 0;

  init();

  while(TRUE) {
    IOA = (IOA&0xFC)|(IOA&4 ? 1:2); // set leds
    //make this reject control messages if in bulk wait
    if ( got_sud ) {
      if (jtag_action != 0)
        STALLEP0();
      handle_setupdata();
      got_sud = FALSE;
    }

    if (jtag_action == 1){// && !(EP2FIFOFLGS & 2)){
      uint8_t cpld_trans_len = 4;
      __bit ran_final_write = FALSE;
      jtag_action = 0;

      SET_TRANSFER_COUNT((uint8_t)transfer_bit_count, 
			 (uint8_t)(transfer_bit_count>>8),
			 (uint8_t)(transfer_bit_count>>16),
			 (uint8_t)(transfer_bit_count>>24));

      WAIT_EP2FIFO_NOT_EMPTY();

      CPLD_LAST_WORD = FALSE;

      while (TRUE){
	//BREAKPOINT(BLINK_RED);
        if ((GPIFTCB0|GPIFTCB1|GPIFTCB2|GPIFTCB3) != 0){
	  //BREAKPOINT(BLINK_GREEN_RED);
	  CPLD_EXTEND = TRUE;
          IOC = cpld_trans_len|0x20;

	  SYNCDELAY;
          GPIFTRIG = 0x00;
	  SYNCDELAY;

	  WAIT_GPIF_DONE();

          if (gpif_interrupted){
	    //BREAKPOINT(BLINK_RED_GREEN);
            gpif_interrupted = FALSE;
            IOC = 0x41;

	    SET_TRANSFER_COUNT(1, 0, 0, 0);

            SYNCDELAY;
            GPIFTRIG = 6;
            SYNCDELAY;

	    WAIT_GPIF_DONE();

	    SET_TRANSFER_COUNT(gpiftcb0_bak, gpiftcb1_bak, gpiftcb2_bak, gpiftcb3_bak);
          }
        }else{
          if (ran_final_write){
	    //BREAKPOINT(BLINK_GREEN_GREEN);
	    SYNCDELAY;
            INPKTEND = 6;
	    SYNCDELAY;
            break;
          }else{
	    //BREAKPOINT(BLINK_RED_RED);
	    ran_final_write = TRUE;
	    cpld_trans_len = low2bits;
	    CPLD_LAST_WORD = TRUE;
	    SET_TRANSFER_COUNT(1, 0, 0, 0);
	  }
	}
      }
    }
  }
}
