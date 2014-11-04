FX2LIBDIR=../../
BASENAME = xpcusb
SOURCES=xpcusb.c gpif.c
A51_SOURCES=dscr.a51
PID=0x1004
SDCCFLAGS=--std-sdcc99

include $(FX2LIBDIR)lib/fx2.mk
