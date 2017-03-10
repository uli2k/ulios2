;head.asm for ulios
;作者：孙亮
;功能：内核镜像入口代码,调用内核C程序main函数
;最后修改日期：2009-05-28

[BITS 32]
global _start
extern _main
_start:		;初始化段寄存器
	mov	ax,	0x10	;数据段选择子
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	ss,	ax
	mov	esp,	0x00090000

	call	_main
HltLoop:
	hlt
	jmp	short	HltLoop
