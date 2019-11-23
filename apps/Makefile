######################
#	Makefile for application
#	作者：孙亮
#	功能：应用程序组建脚本
#	最后修改日期：2009-07-02
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
LDFLAGS		= -T $(LDFILE) -mi386go32 -Map ../objs/test.txt
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
HEADS		= ../MkApi/ulimkapi.h ../driver/basesrv.h
HEADOBJ		= ../objs/apphead.o
OBJS		= ../objs/loader.o ../objs/cmd.o ../objs/3demo.o ../objs/3dline.o ../objs/workout.o ../objs/jpg.o ../objs/clock.o ../objs/desktop.o ../objs/guitest.o ../objs/calc.o ../objs/gmgr.o ../objs/logo.o ../objs/1024.o ../objs/sortdemo.o ../objs/gravit.o ../objs/life.o
BINS		= ../out/ulios/loader.bin ../out/ulios/cmd.bin ../out/apps/3demo.bin ../out/apps/3dline.bin ../out/apps/workout.bin ../out/apps/jpg.bin ../out/apps/clock.bin ../out/ulios/desktop.bin ../out/apps/guitest.bin ../out/apps/calc.bin ../out/ulios/gmgr.bin ../out/apps/logo.bin ../out/apps/1024.bin ../out/apps/sortdemo.bin ../out/apps/gravit.bin ../out/apps/life.bin

# All Phony Targets
.PHONY : all redo clean

# Default starting position

all : $(BINS)

redo : clean all

clean :
	rm -f $(OBJS) $(HEADOBJ) $(BINS)

../out/ulios/loader.bin : ../objs/loader.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/loader.o

../out/ulios/cmd.bin : ../objs/cmd.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/cmd.o ../objs/string.o

../out/apps/3demo.bin : ../objs/3demo.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/3demo.o ../objs/gdi.o

../out/apps/3dline.bin : ../objs/3dline.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/3dline.o ../objs/gdi.o

../out/apps/workout.bin : ../objs/workout.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/workout.o

../out/apps/jpg.bin : ../objs/jpg.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/jpg.o ../objs/gdi.o

../out/apps/clock.bin : ../objs/clock.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/clock.o ../objs/gdi.o

../out/ulios/desktop.bin : ../objs/desktop.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/desktop.o ../objs/malloc.o ../objs/gclient.o

../out/apps/guitest.bin : ../objs/guitest.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/guitest.o ../objs/malloc.o ../objs/gclient.o

../out/apps/calc.bin : ../objs/calc.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/calc.o ../objs/malloc.o ../objs/gclient.o

../out/ulios/gmgr.bin : ../objs/gmgr.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/gmgr.o  ../objs/string.o ../objs/malloc.o ../objs/gclient.o

../out/apps/logo.bin : ../objs/logo.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/logo.o ../objs/gdi.o ../objs/malloc.o

../out/apps/1024.bin : ../objs/1024.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/1024.o ../objs/malloc.o ../objs/gclient.o

../out/apps/sortdemo.bin : ../objs/sortdemo.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/sortdemo.o ../objs/string.o ../objs/malloc.o ../objs/gclient.o

../out/apps/gravit.bin : ../objs/gravit.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/gravit.o ../objs/gdi.o

../out/apps/life.bin : ../objs/life.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/life.o ../objs/gdi.o

../objs/apphead.o : ../MkApi/apphead.c
	$(CC) $(CFLAGS) -o $@ $<

../objs/loader.o : loader.c $(HEADS) ../fs/fsapi.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/cmd.o : cmd.c $(HEADS) ../fs/fsapi.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/3demo.o : 3demo.c $(HEADS) ../lib/gdi.h ../lib/math.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/3dline.o : 3dline.c $(HEADS) ../lib/gdi.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/workout.o : workout.c $(HEADS) ../lib/math.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/jpg.o : jpg.c $(HEADS) ../lib/gdi.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/clock.o : clock.c $(HEADS) ../lib/gdi.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/desktop.o : desktop.c $(HEADS) ../fs/fsapi.h ../lib/malloc.h ../lib/gclient.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/guitest.o : guitest.c $(HEADS) ../lib/malloc.h ../lib/gclient.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/calc.o : calc.c $(HEADS) ../lib/malloc.h ../lib/gclient.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/gmgr.o : gmgr.c $(HEADS) ../fs/fsapi.h ../lib/malloc.h ../lib/gclient.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/logo.o : logo.c $(HEADS) ../fs/fsapi.h ../lib/malloc.h ../lib/math.h ../lib/gdi.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/1024.o : 1024.c $(HEADS) ../lib/malloc.h ../lib/gclient.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/sortdemo.o : sortdemo.c $(HEADS) ../lib/malloc.h ../lib/gclient.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/gravit.o : gravit.c $(HEADS) ../lib/gdi.h ../lib/math.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/life.o : life.c $(HEADS) ../lib/gdi.h
	$(CC) $(CFLAGS) -o $@ $<
