######################
#	Makefile for ulios
#	作者：孙亮
#	功能：微内核组建脚本
#	最后修改日期：2009-07-02
######################

# Entry point of ulios MicroKernel
ENTRYPOINT	= 0x10000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f aout
CFLAGS		= -c -O1 -Wall -m32 -fno-builtin -fleading-underscore -fno-stack-protector
LDFILE		= ulios.ld
LDFLAGS		= -T $(LDFILE) -mi386go32 -Map ../objs/knlmap.txt
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
HEADS		= bootdata.h cintr.h error.h exec.h ipc.h kalloc.h knldef.h page.h task.h ulidef.h x86cpu.h
OBJS		= ../objs/head.o ../objs/intr.o ../objs/cintr.o ../objs/exec.o ../objs/global.o ../objs/kalloc.o ../objs/task.o ../objs/ipc.o ../objs/page.o ../objs/ulios.o
KERNELBIN	= ../out/ulios/uliknl.bin

# All Phony Targets
.PHONY : all redo run clean

# Default starting position

all : $(KERNELBIN)

redo : clean all

run : all
	bochs -q -f ulios.bxrc

clean :
	rm -f $(OBJS) $(KERNELBIN)

$(KERNELBIN) : $(OBJS) $(LDFILE)
	$(LD) $(LDFLAGS) -o $(KERNELBIN) $(OBJS)

../objs/head.o : head.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

../objs/intr.o : intr.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

../objs/cintr.o : cintr.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/exec.o : exec.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/global.o : global.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/ipc.o : ipc.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/kalloc.o : kalloc.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/page.o : page.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/task.o : task.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/ulios.o : ulios.c $(HEADS) ulios.h
	$(CC) $(CFLAGS) -o $@ $<
