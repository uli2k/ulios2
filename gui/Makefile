######################
#	Makefile for ulios graphical user interface
#	作者：孙亮
#	功能：图形用户界面组建脚本
#	最后修改日期：2010-11-09
######################

# Entry point of ulios graphical user interface
ENTRYPOINT	= 0x08000000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f aout
CFLAGS		= -c -O1 -Wall -m32 -fno-builtin -fleading-underscore -fno-stack-protector
LDFILE		= ../MkApi/app.ld
LDFLAGS		= -T $(LDFILE) -mi386go32 -Map ../objs/guimap.txt
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
HEADS		= ../MkApi/ulimkapi.h ../driver/basesrv.h gui.h guiapi.h
OBJS		= ../objs/apphead.o ../objs/guiapi.o ../objs/guiobj.o ../objs/guirect.o ../objs/guikbdmus.o ../objs/guilib.o
GUIBIN		= ../out/ulios/gui.bin

# All Phony Targets
.PHONY : all redo clean

# Default starting position

all : $(GUIBIN)

redo : clean all

clean :
	rm -f $(OBJS) $(GUIBIN)

$(GUIBIN) : $(OBJS) $(LDFILE)
	$(LD) $(LDFLAGS) -o $(GUIBIN) $(OBJS)

../objs/apphead.o : ../MkApi/apphead.c
	$(CC) $(CFLAGS) -o $@ $<

../objs/guiapi.o : guiapi.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/guiobj.o : guiobj.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/guirect.o : guirect.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/guikbdmus.o : guikbdmus.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/guilib.o : guilib.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<
