;	uliboot.asm for ulios
;	作者：孙亮
;	功能：从ULIFS文件系统磁盘保留扇区载入ulildr并执行
;	最后修改日期：2009-05-19
;	备注：使用NASM编译

;常数及变量地址
BaseOfLoader	equ	0x9000	;loader段地址
OffsetOfLoader	equ	0x1000	;loader段内偏移
DRV_num		equ	0x0000	;1字节驱动器号
DRV_count	equ	0x0001	;1字节驱动器数
OffsetOfBPB	equ	0x0004	;BPB数据

[BITS 16]
[ORG 0x7C00]
	jmp	short	start
	times	4 - ($ - $$)	DB	0
;----------------------------------------
;ULIFS引导记录数据(格式化时自动生成)
BPB:
BS_OEMName	DB	'TUX soft'	;0004h OEM ID(tian&uli2k_X)
BPB_fsid	DB	'ULTN'		;文件系统标志"ULTN",文件系统代码:0x7C
BPB_ver		DW	0		;版本号0,以下为0版本的BPB
BPB_bps		DW	512		;每扇区字节数512
BPB_spc		DW	2		;每簇扇区数
BPB_res		DW	8		;保留扇区数,包括引导记录
BPB_secoff	DD	3459519		;分区在磁盘上的起始扇区偏移
BPB_seccou	DD	774081		;分区占总扇区数,包括剩余扇区
BPB_spbm	DW	95		;每个位图占扇区数
BPB_cluoff	DW	198		;分区内首簇开始扇区偏移
BPB_clucou	DD	386941		;数据簇数目
BPB_BootPath	DB	'ulios/bootlist';启动列表文件路径
	times	128 - ($ - $$)	DB	0
SizeOfBPB	equ	$ - BPB
;----------------------------------------
;扩展INT13H磁盘地址数据包
DAP		equ	OffsetOfBPB + SizeOfBPB
DAP_size	equ	DAP		;数据包尺寸=16
DAP_res		equ	DAP_size + 1	;0
DAP_count	equ	DAP_res + 1	;要传输的扇区个数
DAP_BufAddr	equ	DAP_count + 2	;传输缓冲地址(segment:offset)
DAP_BlkAddr	equ	DAP_BufAddr + 4	;磁盘起始绝对块地址
;----------------------------------------
start:	;设置段
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	0x7C00

	;显示信息
	mov	si,	LdrMsg
	call	Print

	;复制BPB数据
	mov	si,	BPB		;源偏移
	mov	ax,	BaseOfLoader	;目的段
	mov	es,	ax
	mov	di,	OffsetOfBPB	;目的偏移
	mov	cx,	SizeOfBPB / 2
	rep
	movsw

	;再设数据段
	mov	ds,	ax

	;取得驱动器参数
	mov	[DRV_num],	dl	;保存驱动器号
	mov	ah,	8
	int	0x13
	jc	short	ShowDrvErr
	mov	[DRV_count],	dl	;驱动器数

	;读取ulildr
	mov	byte	[DAP_size],	0x10
	mov	byte	[DAP_res],	0
	mov	ax,	[BPB_res - $$]
	dec	ax			;减去引导扇区的1个
	mov	word	[DAP_count],	ax
	mov	dword	[DAP_BufAddr],	BaseOfLoader * 0x10000 + OffsetOfLoader
	mov	eax,	[BPB_secoff - $$]
	inc	eax			;加上引导扇区的1个
	mov	dword	[DAP_BlkAddr],	eax
	mov	dword	[DAP_BlkAddr + 4],	0
	mov	ah,	0x42
	mov	dl,	[DRV_num]
	mov	si,	DAP
	int	0x13
	jc	short	ShowDrvExtErr

	;显示Done信息
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	si,	DoneMsg
	call	Print

	;跳转到ulildr执行
	jmp	BaseOfLoader:OffsetOfLoader
;----------------------------------------
ShowDrvErr:	;显示磁盘错误
	mov	ax,	cs
	mov	ds,	ax
	mov	si,	DrvErrMsg
	call	Print
	jmp	short	$
DrvErrMsg	DB	" Disk Error!", 0
;----------------------------------------
ShowDrvExtErr:	;显示磁盘扩展INT13读错误
	mov	ax,	cs
	mov	ds,	ax
	mov	si,	DrvExtErrMsg
	call	Print
	jmp	short	$
DrvExtErrMsg	DB	" Disk Ext INT 13 read Error!", 0
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
LdrMsg		DB	"Loading ulildr...", 0
DoneMsg		DB	" Done", 13, 10, 0
;----------------------------------------
	times	510 - ($ - $$)	DB	0
	DW	0xAA55
