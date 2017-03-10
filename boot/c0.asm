;	C0.ASM for ulios
;	作者：孙亮
;	功能：TC编译器预编译头代码
;	最后修改日期：2009-05-19
;	备注：使用TASM编译生成C0T.OBJ，用于用TC编写16位启动程序
;	TASM C0,C0T /D__TINY__ /MX;

	.186
	EXTRN	_main:	NEAR
_TEXT	SEGMENT	BYTE	PUBLIC	'CODE'
_TEXT	ENDS
_DATA	SEGMENT	PARA	PUBLIC	'DATA'
_DATA	ENDS
_BSS	SEGMENT	WORD	PUBLIC	'BSS'
_BSS	ENDS
DGROUP	GROUP	_TEXT,	_DATA,	_BSS	;代码段和数据段构成一个组
ASSUME	CS:_TEXT

_TEXT	SEGMENT
	ORG	100h
START	PROC	NEAR
	jmp	START2
	DB	3837	DUP(0)
START2:
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	-4096		;堆栈设在0xF000
	xor	ax,	ax
	push	ax
	call	_main
START	ENDP
_TEXT	ENDS
	END	START
