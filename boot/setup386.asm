;setup386.asm for ulios
;作者：孙亮
;功能：进行保护模式前的准备，设置保护模式最终进入32位内核，本setup版本只基于80386 CPU，没有打开pentium pro高级功能的操作
;最后修改日期：2009-06-25
;备注：使用NASM编译器编译成BIN文件

[BITS 16]
[ORG 0x1A00]
;内核表地址大小
GDTsize		equ	0x0800	;GDT字节数
GDTseg		equ	0	;GDT段
GDToff		equ	0	;GDT偏移
IDTsize		equ	0x0800	;IDT字节数
IDTseg		equ	0	;IDT段
IDToff		equ	0x0800	;IDT偏移
KPDTsize	equ	0x2000	;页目录表字节数(包括一个PDDT)
KPDTseg		equ	0	;页目录表段
KPDToff		equ	0x1000	;页目录表偏移
KNLoff		equ	0x10000	;内核起始地址偏移
;启动数据位置
MemEnd		equ	0x00FC	;4字节内存上限
ARDStruct	equ	0x0100	;20 * 19字节内存结构体
VesaMode	equ	0x02FC	;4字节启动VESA模式号
VbeInfo		equ	0x0300	;512字节VESA信息
ModeInfo	equ	0x0500	;256字节模式信息
HdInfo		equ	0x0600	;32字节两个硬盘参数

setup:
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	0xF000
;----------------------------------------
;取得内存分布表
	xor	ebx,	ebx
	mov	[MemEnd],	ebx
	mov	di,	ARDStruct
MemChkLoop:
	mov	eax,	0xE820
	mov	cx,	20
	mov	edx,	'PAMS'
	int	0x15
	jc	short	MemChkFail
	cmp	dword[di + 16],	1		;检查类型
	jne	short	MenChkGoon
	mov	eax,	[di]
	add	eax,	[di + 8]
	cmp	[MemEnd],	eax		;检查上限
	jae	short	MenChkGoon
	mov	[MemEnd],	eax
MenChkGoon:
	add	di,	20
	cmp	ebx,	0
	jne	short	MemChkLoop
	cmp	dword[MemEnd],	0x00800000	;支持最小内存数8M
	jae	short	MemChkOk
MemTooSmall:
	mov	si,	MemSmallMsg
	call	Print
	jmp	short	$
MemSmallMsg	DB	"OUT OF MEMARY!",0
MemChkFail:
	mov	si,	MemChkMsg
	call	Print
	jmp	short	$
MemChkMsg	DB	"MEMARY CHECK FAIL!",0
MemChkOk:
	mov	dword[di],	0xFFFFFFFF	;ARDStruct结束标志
;----------------------------------------
;初始化VESA
	mov	ax,	0x4F00		;获得VESA信息
	mov	di,	VbeInfo
	int	0x10
	cmp	ax,	0x004F
	jne	VesaFail
	mov	cx,	[VesaMode]	;获得显示模式号
	cmp	cx,	0
	je	VesaOk			;不用设置
	mov	ax,	0x4F01		;获得模式信息
	mov	di,	ModeInfo
	int	0x10
	mov	ax,	0x4F02		;设置显示模式
	mov	bx,	[VesaMode]
	or	bx,	0x4000
	int	0x10
	cmp	ax,	0x004F
	je	VesaOk
VesaFail:
	mov	si,	VesaMsg
	call	Print
	jmp	short	$
VesaMsg	DB	"VESA INIT FAIL!",0
VesaOk:
;----------------------------------------
;取得硬盘参数,参考linux 0.11
	mov	di,	HdInfo
	xor	ax,	ax
	mov	ds,	ax
	lds	si,	[0x41 * 4]	;中断向量0x41是硬盘0参数表的地址
	mov	cx,	4
	rep	movsd
	mov	ds,	ax
	lds	si,	[0x46 * 4]	;中断向量0x46是硬盘1参数表的地址
	mov	cx,	4
	rep	movsd

	mov	ax,	0x1500
	mov	dl,	0x81
	int	0x13
	jc	short	Disk1None
	cmp	ah,	3		;是硬盘吗?
	je	short	Disk1Ok
Disk1None:
	xor	eax,	eax
	mov	di,	HdInfo + 16
	mov	cx,	4
	rep	stosd
Disk1Ok:
;----------------------------------------
;打开A20地址线
	in	al,	0x92		;Fast方法
	or	al,	0x02
	out	0x92,	al
	mov	ax,	0xFFFF		;检测是否已打开
	mov	ds,	ax
	mov	word[0xFFFE],	0xABCD
	xor	ax,	ax
	mov	ds,	ax
	cmp	word[0xFFEE],	0xABCD
	jne	A20LineOk
	call	Wait8042		;8042控制器法
	mov	al,	0xD1
	out	0x64,	al
	call	Wait8042
	mov	al,	0xDF
	out	0x60,	al
	call	Wait8042
A20LineOk:
;----------------------------------------
	cli			;禁止中断，从此不能使用中断，直到内核打开
	cld			;清除方向
;----------------------------------------
;设置GDT
	mov	ax,	cs	;复制GDT
	mov	ds,	ax
	mov	si,	GDT
	mov	ax,	GDTseg
	mov	es,	ax
	mov	di,	GDToff
	mov	cx,	GDTN / 4
	rep	movsd
	xor	eax,	eax	;其他表项清0
	mov	cx,	(GDTsize - GDTN) / 4
	rep	stosd
;----------------------------------------
;设置IDT
	mov	ax,	IDTseg	;所有表项清0
	mov	es,	ax
	mov	di,	IDToff
	xor	eax,	eax
	mov	cx,	IDTsize / 4
	rep	stosd
;----------------------------------------
;设置页目录表
	mov	ax,	KPDTseg
	mov	es,	ax
	mov	di,	KPDToff
	mov	eax,	0x0000D063	;内核页0
	stosd
	mov	eax,	0x0000E063	;内核页1
	stosd
	mov	eax,	KPDTseg * 0x10 + KPDToff + 0x1063	;PDDT(内核访问的4K局部页面)
	stosd
	xor	eax,	eax	;其他表项清0
	mov	cx,	KPDTsize / 4 - 3
	rep	stosd
;----------------------------------------
;设置页表
	mov	di,	0xD000
	mov	cx,	0x800	;共2048页 对等映射
	mov	eax,	0x0063
PteLop:	stosd
	add	eax,	0x1000
	loop	PteLop
;----------------------------------------
;加载idt,gdt,pde
	mov	ax,	cs
	mov	ds,	ax
	lgdt	[GDTaddr]
	lidt	[IDTaddr]
	mov	eax,	KPDTseg * 0x10 + KPDToff
	mov	cr3,	eax
;----------------------------------------
;开启32位分页保护模式
	mov	eax,	cr0
	or	eax,	0x80000001
	mov	cr0,	eax
;----------------------------------------
;进入保护模式
	jmp	dword	8:KNLoff
;----------------------------------------
Print:		;显示信息函数
	mov	ah,	0x0E		;显示模式
	mov	bx,	0x0007		;字体属性
PrintNext:
	lodsb
	or	al,	al
	jz	short	PrintEnd
	int	0x10			;显示
	jmp	short	PrintNext
PrintEnd:
	ret
;----------------------------------------
Wait8042:	;等待键盘控制器空闲的子程序
	in	al,	0x64
	test	al,	0x2
	jnz	short	Wait8042
	ret
;----------------------------------------
;GDT,IDT
GDT:;0
	DD	0
	DD	0
KNLcode:;8
	DW	0x001F	;内核代码段0起128KB
	DW	0
	DB	0
	DB	10011001b
	DB	11000000b
	DB	0
KNLdata:;16
	DW	0xFFFF	;内核数据段0起4GB
	DW	0
	DB	0
	DB	10010011b
	DB	11001111b
	DB	0
GDTN	equ	$ - GDT	;复制GDT项字节数
GDTaddr:
	DW	GDTsize - 1	;GDT大小
	DD	GDTseg * 0x10 +	GDToff	;GDT线性地址
IDTaddr:
	DW	IDTsize - 1	;IDT大小
	DD	IDTseg * 0x10 +	IDToff	;IDT线性地址
