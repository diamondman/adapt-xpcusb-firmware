FX2LIBDIR=lib/fx2lib/
BASENAME = xpcusb
SOURCES=xpcusb.c gpif.c
A51_SOURCES=dscr.a51
PID=0x1004
SDCCFLAGS=--std-sdcc99
CODE_SIZE?=--code-size 0x3E00
XRAM_LOC?=--xram-loc 0xE000


include $(FX2LIBDIR)lib/fx2.mk
