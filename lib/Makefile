######################
#	Makefile for lib
#	作者：孙亮
#	功能：库文件组建脚本
#	最后修改日期：2010-06-10
######################

# Entry point of ulios application
ENTRYPOINT	= 0x08000000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f aout
CFLAGS		= -c -O1 -Wall -m32 -fno-builtin -fleading-underscore -fno-stack-protector
LDFILE		= ../MkApi/app.ld
LDFLAGS		= -T $(LDFILE) -mi386go32
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
HEADS		= ../MkApi/ulimkapi.h ../driver/basesrv.h

# All Phony Targets
.PHONY : all redo clean

# Default starting position

all : ../objs/malloc.o ../objs/gdi.o ../objs/gclient.o

redo : clean all

clean :
	rm -f ../objs/malloc.o ../objs/gdi.o ../objs/gclient.o

../objs/malloc.o : ../lib/malloc.c ../lib/malloc.h $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/gdi.o : ../lib/gdi.c ../lib/gdi.h $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/gclient.o : ../lib/gclient.c ../lib/gclient.h ../lib/gcres.h ../lib/malloc.h ../gui/guiapi.h $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<
