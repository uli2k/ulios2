/*	ulildr.c for ulios
	作者：孙亮
	功能：从ULIFS文件系统磁盘中载入内核
	最后修改日期：2009-10-30
	备注：使用Turbo C TCC编译器编译成16位COM文件
*/
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;

#define FAR2LINE(addr)	((WORD)(addr) + (((addr) & 0xFFFF0000) >> 12))
#define LINE2FAR(addr)	((WORD)(addr) | (((addr) & 0xFFFF0000) << 12))

typedef struct _DAP
{
	BYTE	PacketSize;	/*数据包尺寸=16*/
	BYTE	Reserved;	/*0*/
	WORD	BlockCount;	/*要传输的扇区数*/
	DWORD	BufferAddr;	/*传输缓冲地址(segment:offset)*/
	DWORD	BlockAddr[2];/*磁盘起始绝对块地址*/
}DAP;	/*磁盘地址数据包*/

typedef struct _BPB
{
	BYTE	DRV_num;	/*驱动器号*/
	BYTE	DRV_count;	/*驱动器数*/
	BYTE	DRV_res[2];	/*保留*/
	BYTE	BS_OEMName[8];	/*OEM ID(tian&uli2k_X)*/
	BYTE	fsid[4];/*文件系统标志"ULTN",文件系统代码:0x7C*/
	WORD	ver;	/*版本号0,以下为0版本的BPB*/
	WORD	bps;	/*每扇区字节数*/
	WORD	spc;	/*每簇扇区数*/
	WORD	res;	/*保留扇区数,包括引导记录*/
	DWORD	secoff;	/*分区在磁盘上的起始扇区偏移*/
	DWORD	seccou;	/*分区占总扇区数,包括剩余扇区*/
	WORD	spbm;	/*每个位图占扇区数*/
	WORD	cluoff;	/*分区内首簇开始扇区偏移*/
	DWORD	clucou;	/*数据簇数目*/
	BYTE	BootPath[88];/*启动列表文件路径*/
}BPB;	/*ULIFS引导记录数据*/

typedef struct _BLKID
{
	DWORD fst;	/*首簇*/
	DWORD cou;	/*数量*/
}BLKID;	/*块索引节点*/

typedef struct _DIR
{
	BYTE name[80];	/*utf8字符串*/
	DWORD CreateTime;	/*创建时间1970-01-01经过的秒数*/
	DWORD ModifyTime;	/*修改时间*/
	DWORD AccessTime;	/*访问时间*/
	DWORD attr;		/*属性*/
	DWORD len[2];	/*文件长度,与FAT32不同目录文件的长度有效*/
	BLKID idx[3];	/*最后有3个自带的索引块,所以一个文件有可能用不到索引簇*/
}DIR;	/*ULIFS目录项结构*/

#define BUF_COU		0x4000							/*数据缓存上限*/
#define IDX_COU		(BUF_COU / sizeof(BLKID))		/*索引缓存上限*/

#define SETUP		((void (*)())0x1A00)			/*Setup程序位置*/
#define BIN_ADDR	((DWORD *)0x0080)				/*Bin程序段数据数组*/
#define SYS_DIR		((BYTE *)0x0280)				/*系统目录字符串*/
#define VESA_MODE	((WORD *)0x02FC)				/*启动VESA模式号*/

#define NO_ERROR	0
#define NOT_FOUND	1

void memcpy(BYTE *dst, BYTE *src, WORD size)
{
	while (size--)
		*dst++ = *src++;
}

void strcpy(BYTE *dst, BYTE *src)
{
	while ((*dst++ = *src++) != '\0');
}

WORD strcmp(BYTE *str1, BYTE *str2)
{
	while (*str1 == *str2)
	{
		if (*str1 == 0)
			return 0;
		str1++;
		str2++;
	}
	return 1;
}

WORD atol(BYTE *str)
{
	WORD i;

	for (i = 0; *str >= '0' && *str <= '9'; str++)
		i = i * 10 + *str - '0';
	return i;
}

void PutChar(BYTE c)
{
	_AL = c;
	_AH = 0x0E;
	_BX = 0x0007;
	asm int 10h;
}

void PutS(BYTE *str)
{
	while (*str)
	{
		_AL = *str;
		_AH = 0x0E;
		_BX = 0x0007;
		asm int 10h;
		str++;
	}
}

/*读取扇区*/
void ReadSector(BYTE DrvNum,		/*输入：驱动器号*/
				DWORD BlockAddr,	/*输入：起始扇区号*/
				WORD BlockCount,	/*输入：扇区数*/
				DWORD BufferAddr)	/*输出：存放数据线性地址*/
{
	DAP dap;

	dap.PacketSize = 0x10;
	dap.Reserved = 0;
	dap.BlockCount = BlockCount;
	dap.BufferAddr = LINE2FAR(BufferAddr);
	dap.BlockAddr[0] = BlockAddr;
	dap.BlockAddr[1] = 0;

	_AH = 0x42;
	_DL = DrvNum;
	_SI = (WORD)(&dap);
	asm int 13h;
	asm jc short ReadError;
	return;
ReadError:	/*注意：扩展INT13可能不会处理DMA边界错误，调用本函数时需注意参数*/
	PutS(" INT13 ERROR!");
	for (;;);
}

/*读取整个文件*/
DWORD ReadFile(	BPB *bpb,			/*输入：分区BPB*/
				BLKID *idx,			/*修改：索引缓存*/
				DIR *SrcDir,		/*输入：文件目录项*/
				DWORD BufferAddr)	/*输出：存放数据远地址*/
{
	WORD idxcou, idxi, idxblkcou;	/*每次读取的索引节点数,索引节点索引,每次读取的索引扇区数*/
	DWORD idxfstblk, brd;	/*索引簇首扇区,已读取字节数*/

	if (SrcDir->len[0] == 0)	/*空文件*/
		return BufferAddr;
	idxcou = bpb->bps / sizeof(BLKID) * bpb->spc;
	if (idxcou > IDX_COU)	/*缓冲长度*/
		idxcou = IDX_COU;
	idxi = idxcou - 3;
	idxblkcou = idxcou * sizeof(BLKID) / bpb->bps;
	brd = 0;
	memcpy((BYTE *)(idx + idxi), (BYTE *)(SrcDir->idx), sizeof(BLKID) * 3);	/*复制目录项中的索引*/
	for (;;)
	{
		for (;;)	/*处理索引簇*/
		{
			WORD blkcou, i;
			DWORD fstblk;
			if (idxi >= idxcou)	/*读取下一扇区*/
			{
				idxfstblk += idxblkcou;
				break;
			}
			else if ((blkcou = idx[idxi].cou) == 0)	/*读取下一索引簇*/
			{
				idxfstblk = bpb->secoff + bpb->cluoff + bpb->spc * idx[idxi].fst;
				break;
			}
			blkcou *= bpb->spc;	/*取得要读取的扇区数*/
			fstblk = bpb->secoff + bpb->cluoff + bpb->spc * idx[idxi].fst;	/*取得要读取的首扇区号*/
			for (i = 0; i < blkcou; i++)
			{
				ReadSector(bpb->DRV_num, fstblk, 1, BufferAddr);	/*读取数据扇区*/
				PutChar('.');
				fstblk++;
				BufferAddr += bpb->bps;
				brd += bpb->bps;
				if (brd >= SrcDir->len[0])	/*读取完成*/
					return BufferAddr;
			}
			idxi++;
		}
		ReadSector(bpb->DRV_num, idxfstblk, idxblkcou, FAR2LINE((DWORD)((void far *)idx)));
		idxi = 0;
	}
}

/*搜索目录项*/
WORD SearchDir(	BPB *bpb,			/*输入：分区BPB*/
				BLKID *idx,			/*修改：索引缓存*/
				BYTE *buf,			/*修改：数据缓存*/
				DIR *SrcDir,		/*输入：搜索的目录*/
				BYTE *FileName,		/*输入：被搜索的目录项名称*/
				DIR *DstDir)		/*输出：找到的目录项*/
{
	WORD idxcou, idxi, idxblkcou;	/*每次读取的索引节点数，索引节点索引，每次读取的索引扇区数*/
	DWORD idxfstblk, brd;	/*索引簇首扇区，已读取字节数*/

	if (SrcDir->len[0] == 0)	/*空目录*/
		return NOT_FOUND;
	idxcou = bpb->bps / sizeof(BLKID) * bpb->spc;
	if (idxcou > IDX_COU)	/*缓冲长度*/
		idxcou = IDX_COU;
	idxi = idxcou - 3;
	idxblkcou = idxcou * sizeof(BLKID) / bpb->bps;
	brd = 0;
	memcpy((BYTE *)(idx + idxi), (BYTE *)(SrcDir->idx), sizeof(BLKID) * 3);	/*复制目录项中的索引*/
	for (;;)
	{
		for (;;)	/*处理索引簇*/
		{
			WORD blkcou, i;
			DWORD fstblk;

			if (idxi >= idxcou)	/*读取下一扇区*/
			{
				idxfstblk += idxblkcou;
				break;
			}
			else if ((blkcou = idx[idxi].cou) == 0)	/*读取下一索引簇*/
			{
				idxfstblk = bpb->secoff + bpb->cluoff + bpb->spc * idx[idxi].fst;
				break;
			}
			blkcou *= bpb->spc;	/*取得要读取的扇区数*/
			fstblk = bpb->secoff + bpb->cluoff + bpb->spc * idx[idxi].fst;	/*取得要读取的首扇区号*/
			for (i = 0; i < blkcou; i += idxblkcou)
			{
				WORD j;

				ReadSector(bpb->DRV_num, fstblk, idxblkcou, FAR2LINE((DWORD)((void far *)buf)));	/*读取数据扇区*/
				for (j = 0; j < bpb->bps * idxblkcou / sizeof(DIR); j++)
				{
					if (strcmp(((DIR *)buf)[j].name, FileName) == 0)	/*搜索成功*/
					{
						*DstDir = ((DIR *)buf)[j];	/*复制找到的目录项*/
						return NO_ERROR;
					}
				}
				brd += bpb->bps * idxblkcou;
				if (brd >= SrcDir->len[0])	/*读取完成*/
					return NOT_FOUND;
				fstblk += idxblkcou;
			}
			idxi++;
		}
		ReadSector(bpb->DRV_num, idxfstblk, idxblkcou, FAR2LINE((DWORD)((void far *)idx)));
		idxi = 0;
	}
}

/*按路径读取整个文件*/
DWORD ReadPath(	BPB *bpb,			/*输入：分区BPB*/
				BLKID *idx,			/*修改：索引缓存*/
				BYTE *buf,			/*修改：数据缓存*/
				DIR *SrcDir,		/*修改：文件目录项*/
				DIR *DstDir,		/*修改：目标目录项*/
				BYTE *path,			/*输入：文件路径*/
				DWORD BufferAddr)	/*输出：存放数据远地址*/
{
	while (*path)
	{
		BYTE *cp = path;

		while (*cp != '/' && *cp != 0)
			cp++;
		if (*cp)
		{
			*cp++ = 0;
			PutS(path);
			if (SearchDir(bpb, idx, buf, SrcDir, path, DstDir) != NO_ERROR)	/*取得目录项*/
				return 0;
			if (!(DstDir->attr & 0x10))	/*检查是否是目录*/
				return 0;
			*SrcDir = *DstDir;
			path = cp;
			PutChar('/');
		}
		else
		{
			DWORD end;

			PutS(path);
			if (SearchDir(bpb, idx, buf, SrcDir, path, DstDir) != NO_ERROR)	/*取得文件目录项*/
				return 0;
			if (DstDir->attr & 0x10)	/*检查是否是文件*/
				return 0;
			end = ReadFile(bpb, idx, DstDir, BufferAddr);	/*读取文件*/
			if (end == BufferAddr)
				PutS("<-File is empty!");
			PutChar(13);
			PutChar(10);
			return end;
		}
	}
}

void memzero(BYTE far *dst, WORD size)
{
	while (size--)
		*dst++ = 0;
}

/*Loader程序主函数,参数为自定义参数*/
void main(BPB *bpb)
{
	BLKID idx[IDX_COU];	/*16K字节索引缓存*/
	BYTE buf[BUF_COU];	/*16K字节数据缓存*/
	DIR RootDir, SrcDir, DstDir;	/*目录项缓存*/
	BYTE BootList[4096], *cmd;	/*启动列表*/
	DWORD addr = 0x10000, *BinAddr = BIN_ADDR, end;	/*内核存放位置, 程序段数据指针*/
	WORD *VesaMode = VESA_MODE;

	if (bpb->bps > BUF_COU)	/*检查每扇区字节数是否合法*/
		goto errip;
	ReadSector(bpb->DRV_num, bpb->secoff + bpb->cluoff, 1, FAR2LINE((DWORD)((void far *)buf)));	/*读取根目录项所在扇区*/
	SrcDir = RootDir = *((DIR*)buf);
	end = ReadPath(bpb, idx, buf, &SrcDir, &DstDir, bpb->BootPath, FAR2LINE((DWORD)((void far *)BootList)));
	if (end == 0)
		goto errfnf;
	if (end == FAR2LINE((DWORD)((void far *)BootList)))
		goto errfie;
	cmd = BootList;
	BootList[DstDir.len[0]] = 0;
	*VesaMode = 0;
	for (;;)
	{
		BYTE *cp = cmd;

		while (*cp != '\n' && *cp != 0)
			cp++;
		if (*cp)
			*cp++ = 0;
		switch (*cmd++)
		{
		case 'F':	/*File*/
		case 'f':
			if (BinAddr < BIN_ADDR + 30)	/*15个加载文件限制*/
			{
				SrcDir = RootDir;
				end = ReadPath(bpb, idx, buf, &SrcDir, &DstDir, cmd, addr);
				if (end == 0)
					goto errfnf;
				if (end == addr)
					break;
				end = (end + 0x00000FFF) & 0xFFFFF000;		/*调整到4K边界*/
				DstDir.len[0] = (0x1000 - (DstDir.len[0] & 0xFFF)) & 0xFFF;
				memzero(LINE2FAR(end - DstDir.len[0]), (WORD)DstDir.len[0]);	/*清空文件尾内存*/
				*BinAddr++ = addr;
				*BinAddr++ = end - addr;
				addr = end;
			}
			break;
		case 'S':	/*SysDir*/
		case 's':
			strcpy(SYS_DIR, cmd);
			break;
		case 'V':	/*VesaMode*/
		case 'v':
			*VesaMode = atol(cmd);
			break;
		}
		if (*cp)	/*还有命令*/
			cmd = cp;
		else
		{
			*BinAddr = 0;
			SETUP();
		}
	}
errip:
	PutS("invalid partition!");
	for (;;);
errfnf:
	PutS("<-File not found!");
errfie:
	for (;;);
}
