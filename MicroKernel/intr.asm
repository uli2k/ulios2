;intr.asm for ulios
;作者：孙亮
;功能：中段/异常/系统调用发生时的汇编堆栈操作
;最后修改日期：2009-06-30

[BITS 32]
global _AsmIsr00
global _AsmIsr01
global _AsmIsr02
global _AsmIsr03
global _AsmIsr04
global _AsmIsr05
global _AsmIsr06
global _AsmIsr07
global _AsmIsr08
global _AsmIsr09
global _AsmIsr0A
global _AsmIsr0B
global _AsmIsr0C
global _AsmIsr0D
global _AsmIsr0E
global _AsmIsr0F
global _AsmIsr10
global _AsmIsr11
global _AsmIsr12
global _AsmIsr13
extern _IsrCallTable

_AsmIsr00:	;除法错(故障)
	push	byte	0
	push	byte	0
	jmp	short	AsmIsrProc
_AsmIsr01:	;调试异常(故障或陷阱)
	push	byte	0
	push	byte	1
	jmp	short	AsmIsrProc
_AsmIsr02:	;不可屏蔽中断
	push	byte	0
	push	byte	2
	jmp	short	AsmIsrProc
_AsmIsr03:	;单字节INT3调试断点(陷阱)
	push	byte	0
	push	byte	3
	jmp	short	AsmIsrProc
_AsmIsr04:	;INTO异常(陷阱)
	push	byte	0
	push	byte	4
	jmp	short	AsmIsrProc
_AsmIsr05:	;边界检查越界(故障)
	push	byte	0
	push	byte	5
	jmp	short	AsmIsrProc
_AsmIsr06:	;非法操作码(故障)
	push	byte	0
	push	byte	6
	jmp	short	AsmIsrProc
_AsmIsr07:	;协处理器不可用(故障)
	push	byte	0
	push	byte	7
	jmp	short	AsmIsrProc
_AsmIsr08:	;双重故障(异常中止)

	push	byte	8
	jmp	short	AsmIsrProc
_AsmIsr09:	;协处理器段越界(异常中止)
	push	byte	0
	push	byte	9
	jmp	short	AsmIsrProc
_AsmIsr0A:	;无效TSS异常(故障)

	push	byte	10
	jmp	short	AsmIsrProc
_AsmIsr0B:	;段不存在(故障)

	push	byte	11
	jmp	short	AsmIsrProc
_AsmIsr0C:	;堆栈段异常(故障)

	push	byte	12
	jmp	short	AsmIsrProc
_AsmIsr0D:	;通用保护异常(故障)

	push	byte	13
	jmp	short	AsmIsrProc
_AsmIsr0E:	;页异常(故障)

	push	byte	14
	jmp	short	AsmIsrProc
_AsmIsr0F:	;Intel保留
	push	byte	0
	push	byte	15
	jmp	short	AsmIsrProc
_AsmIsr10:	;协处理器浮点运算出错(故障)
	push	byte	0
	push	byte	16
	jmp	short	AsmIsrProc
_AsmIsr11:	;对齐检验(486+)(故障)

	push	byte	17
	jmp	short	AsmIsrProc
_AsmIsr12:	;Machine Check(奔腾+)(异常中止)
	push	byte	0
	push	byte	18
	jmp	short	AsmIsrProc
_AsmIsr13:	;SIMD浮点异常(奔腾III+)(故障)
	push	byte	0
	push	byte	19

AsmIsrProc:
	push	ds
	push	es
	push	fs
	push	gs
	pushad

	mov	ax,	ss
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	eax,	[esp + 48]
	call	[_IsrCallTable + eax * 4]

	popad
	pop	gs
	pop	fs
	pop	es
	pop	ds
	add	esp,	8
	iretd

global _AsmIrq0
global _AsmIrq1
global _AsmIrq2
global _AsmIrq3
global _AsmIrq4
global _AsmIrq5
global _AsmIrq6
global _AsmIrq7
global _AsmIrq8
global _AsmIrq9
global _AsmIrqA
global _AsmIrqB
global _AsmIrqC
global _AsmIrqD
global _AsmIrqE
global _AsmIrqF
extern _IrqProc

_AsmIrq0:	;时钟
	push	byte	0
	jmp	short	AsmIrqProc
_AsmIrq1:	;键盘
	push	byte	1
	jmp	short	AsmIrqProc
_AsmIrq2:	;从8259A
	push	byte	2
	jmp	short	AsmIrqProc
_AsmIrq3:	;串口2
	push	byte	3
	jmp	short	AsmIrqProc
_AsmIrq4:	;串口1
	push	byte	4
	jmp	short	AsmIrqProc
_AsmIrq5:	;LPT2
	push	byte	5
	jmp	short	AsmIrqProc
_AsmIrq6:	;软盘
	push	byte	6
	jmp	short	AsmIrqProc
_AsmIrq7:	;LPT1
	push	byte	7
	jmp	short	AsmIrqProc
_AsmIrq8:	;实时钟
	push	byte	8
	jmp	short	AsmIrqProc
_AsmIrq9:	;重定向IRQ2
	push	byte	9
	jmp	short	AsmIrqProc
_AsmIrqA:	;保留
	push	byte	10
	jmp	short	AsmIrqProc
_AsmIrqB:	;保留
	push	byte	11
	jmp	short	AsmIrqProc
_AsmIrqC:	;PS2鼠标
	push	byte	12
	jmp	short	AsmIrqProc
_AsmIrqD:	;FPU异常
	push	byte	13
	jmp	short	AsmIrqProc
_AsmIrqE:	;AT温盘
	push	byte	14
	jmp	short	AsmIrqProc
_AsmIrqF:	;保留
	push	byte	15

AsmIrqProc:
	push	ds
	push	es
	push	fs
	push	gs
	pushad

	mov	ax,	ss
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	al,	0x20	;主片发送EOI
	out	0x20,	al
	cmp	byte[esp + 48],	8
	jb	CallIrqProc	;主片的IRQ信号，不给从片发送EOI
	out	0xA0,	al	;从片发送EOI
CallIrqProc:
	call	_IrqProc

	popad
	pop	gs
	pop	fs
	pop	es
	pop	ds
	add	esp,	4
	iretd

global _AsmApiCall
extern _ApiCall

_AsmApiCall:	;系统调用接口
	push	ds
	push	es
	push	fs
	push	gs
	pushad

	mov	ax,	ss
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	call	_ApiCall

	popad
	pop	gs
	pop	fs
	pop	es
	pop	ds
	iretd
