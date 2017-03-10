;	f32boot.asm for ulios
;	作者：孙亮
;	功能：从FAT32文件系统磁盘保留扇区载入f32ldr并执行
;	最后修改日期：2009-05-19
;	备注：使用NASM编译

;常数及变量地址
BaseOfLoader	equ	0x9000	;loader段地址
OffsetOfLoader	equ	0x1000	;loader段内偏移
DRV_num		equ	0x0000	;1字节驱动器号
DRV_count	equ	0x0001	;1字节驱动器数
OffsetOfBPB	equ	0x0003	;BPB数据

[BITS 16]
[ORG 0x7C00]
	jmp	short	start
	times	3 - ($ - $$)	DB	0
;----------------------------------------
;FAT32引导记录数据(格式化时自动生成)
BPB:
BS_OEMName	DB	'TUX soft'	;0003h OEM ID(tian&uli2k_X)
BPB_bps		DW	512		;每扇区字节数
BPB_spc		DB	4		;每簇扇区数
BPB_res		DW	32		;保留扇区数
BPB_nf		DB	2		;FAT数
BPB_nd		DW	0		;根目录项数(FAT32不用)
BPB_sms		DW	0		;小扇区数(FAT32不用)
BPB_md		DB	248		;媒体描述符
BPB_spf16	DW	0		;每FAT扇区数(FAT32不用)
BPB_spt		DW	63		;每道扇区数
BPB_nh		DW	128		;磁头数
BPB_hs		DD	63		;隐藏扇区数
BPB_ls		DD	774081		;总扇区数
BPB_spf		DD	1508		;每FAT扇区数(FAT32专用)
BPB_ef		DW	0		;扩展标志(FAT32专用)
BPB_fv		DW	0		;文件系统版本(FAT32专用)
BPB_rcn		DD	2226		;根目录簇号(FAT32专用)
BPB_fsis	DW	1		;文件系统信息扇区号(FAT32专用)
BPB_backup	DW	6		;备份引导扇区(FAT32专用)
BPB_res1	DD	0, 0, 0		;保留(FAT32专用)
;FAT32扩展引导记录数据(格式化时自动生成)
BPB_pdn		DB	128		;物理驱动器号
BPB_res2	DB	0		;保留
BPB_ebs		DB	41		;扩展引导标签
BPB_vsn		DD	1256925113	;分区序号
BPB_vl	times	11	DB	0	;卷标
BPB_sid		DB	'FAT32   '	;系统ID
;ULIOS扩展引导记录数据(格式化时自动生成)
BPB_ldroff	DW	8		;ulios载入程序所在扇区偏移
BPB_secoff	DD	2685375		;分区在磁盘上的起始扇区偏移
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
	mov	cx,	SizeOfBPB
	rep
	movsb

	;再设数据段
	mov	ds,	ax

	;取得驱动器参数
	mov	[DRV_num],	dl	;保存驱动器号
	mov	ah,	8
	int	0x13
	jc	short	ShowDrvErr
	mov	[DRV_count],	dl	;驱动器数

	;读取f32ldr
	mov	byte	[DAP_size],	0x10
	mov	byte	[DAP_res],	0
	xor	edx,	edx
	mov	dx,	[BPB_ldroff - $$]
	mov	ax,	[BPB_res - $$]
	sub	ax,	dx	;减去ldr以前的扇区
	mov	word	[DAP_count],	ax
	mov	dword	[DAP_BufAddr],	BaseOfLoader * 0x10000 + OffsetOfLoader
	mov	eax,	[BPB_secoff - $$]
	add	eax,	edx	;加上ldr以前的扇区
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

	;跳转到f32ldr执行
	jmp	BaseOfLoader:OffsetOfLoader
;----------------------------------------
ShowDrvErr:	;显示磁盘错误
	mov	ax,	cs
	mov	ds,	ax
	mov	si,	DrvErrMsg
	call	Print
	jmp	short	$
DrvErrMsg	db	" Disk Error!", 0
;----------------------------------------
ShowDrvExtErr:	;显示磁盘扩展INT13读错误
	mov	ax,	cs
	mov	ds,	ax
	mov	si,	DrvExtErrMsg
	call	Print
	jmp	short	$
DrvExtErrMsg	db	" Disk Ext INT 13 read Error!", 0
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
LdrMsg		DB	"Loading f32ldr...", 0
DoneMsg		DB	" Done", 13, 10, 0
;----------------------------------------
	times	510 - ($ - $$)	DB	0
	DW	0xAA55
