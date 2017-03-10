/*	f32ldr.c for ulios
	作者：孙亮
	功能：从FAT32文件系统磁盘中载入内核
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

typedef struct _BPB0
{
	BYTE	DRV_num;	/*驱动器号*/
	BYTE	DRV_count;	/*驱动器数*/
	BYTE	DRV_res;	/*保留*/
	BYTE	BS_OEMName[8];	/*OEM ID(tian&uli2k_X)*/
	WORD	bps;	/*每扇区字节数*/
	BYTE	spc;	/*每簇扇区数*/
	WORD	res;	/*保留扇区数*/
	BYTE	nf;		/*FAT数*/
	WORD	nd;		/*根目录项数*/
	WORD	sms;	/*小扇区数(FAT32不用)*/
	BYTE	md;		/*媒体描述符*/
	WORD	spf16;	/*每FAT扇区数(FAT32不用)*/
	WORD	spt;	/*每道扇区数*/
	WORD	nh;		/*磁头数*/
	DWORD	hs;		/*隐藏扇区数*/
	DWORD	ls;		/*总扇区数*/
	DWORD	spf;	/*每FAT扇区数(FAT32专用)*/
	WORD	ef;		/*扩展标志(FAT32专用)*/
	WORD	fv;		/*文件系统版本(FAT32专用)*/
	DWORD	rcn;	/*根目录簇号(FAT32专用)*/
	WORD	fsis;	/*文件系统信息扇区号(FAT32专用)*/
	WORD	backup;	/*备份引导扇区(FAT32专用)*/
	DWORD	res1[3];/*保留(FAT32专用)*/

	BYTE	pdn;	/*物理驱动器号*/
	BYTE	res2;	/*保留*/
	BYTE	ebs;	/*扩展引导标签*/
	DWORD	vsn;	/*分区序号*/
	BYTE	vl[11];	/*卷标*/
	BYTE	sid[8];	/*系统ID*/

	WORD	ldroff;	/*ulios载入程序所在扇区偏移*/
	DWORD	secoff;	/*分区在磁盘上的起始扇区偏移*/
	BYTE	BootPath[32];/*启动列表文件路径*/
}BPB0;	/*FAT32引导记录原始数据*/

typedef struct _BPB
{
	BYTE	DRV_num;	/*驱动器号*/
	BYTE	DRV_count;	/*驱动器数*/
	WORD	bps;	/*每扇区字节数*/
	WORD	spc;	/*每簇扇区数*/
	WORD	res;	/*保留扇区数,包括引导记录*/
	DWORD	secoff;	/*分区在磁盘上的起始扇区偏移*/
	DWORD	cluoff;	/*分区内首簇开始扇区偏移*/
}BPB;	/*FAT32引导记录数据*/

typedef struct _DIR
{
	BYTE name[11];	/*文件名*/
	BYTE attr;		/*属性*/
	BYTE reserved;	/*保留*/
	BYTE crtmils;	/*创建时间10毫秒位*/
	WORD crttime;	/*创建时间*/
	WORD crtdate;	/*创建日期*/
	WORD acsdate;	/*访问日期*/
	WORD idxh;		/*首簇高16位*/
	WORD chgtime;	/*修改时间*/
	WORD chgdate;	/*修改日期*/
	WORD idxl;		/*首簇低16位*/
	DWORD len;		/*长度*/
}DIR;	/*FAT32目录项结构*/

#define BUF_COU		0x4000							/*数据缓存上限*/
#define IDX_COU		(BUF_COU / sizeof(DWORD))		/*索引缓存上限*/

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

WORD namecmp(BYTE *name, BYTE *str)
{
	BYTE newnam[11], *namep;

	for (namep = newnam; namep < newnam + 11; namep++)
		*namep = ' ';
	for (namep = newnam; *str != '.' && *str; namep++, str++)
	{
		if (namep >= newnam + 8)
			return 1;
		if (*str == 0xE5 && namep == newnam)
			*namep = 0x05;
		else if (*str >= 'a' && *str <= 'z')
			*namep = *str - 0x20;
		else
			*namep = *str;
	}
	if (*str == '.')
		str++;
	for (namep = newnam + 8; *str; namep++, str++)
	{
		if (namep >= newnam + 11)
			return 1;
		if (*str >= 'a' && *str <= 'z')
			*namep = *str - 0x20;
		else
			*namep = *str;
	}
	for (namep = newnam; namep < newnam + 11; namep++, name++)
		if (*namep != *name)
			return 1;
	return 0;
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
				DWORD *idx,			/*修改：索引缓存*/
				DIR *SrcDir,		/*输入：文件目录项*/
				DWORD BufferAddr)	/*输出：存放数据远地址*/
{
	DWORD prei, clui, brd;	/*簇号计数,上一簇号,已读取字节数*/

	if (SrcDir->len == 0)	/*空文件*/
		return BufferAddr;
	prei = 0xFFFFFFFF;
	clui = ((DWORD)(SrcDir->idxh) << 16) | SrcDir->idxl;
	brd = 0;
	for (;;)
	{
		DWORD i, fstblk;

		fstblk = bpb->secoff + bpb->cluoff + bpb->spc * clui;	/*取得要读取的首扇区号*/
		for (i = 0; i < bpb->spc; i++)
		{
			ReadSector(bpb->DRV_num, fstblk, 1, BufferAddr);	/*读取数据扇区*/
			PutChar('.');
			fstblk++;
			BufferAddr += bpb->bps;
			brd += bpb->bps;
			if (brd >= SrcDir->len)	/*读取完成*/
				return BufferAddr;
		}
		if ((i = clui / (bpb->bps >> 2)) != prei / (bpb->bps >> 2))	/*下一簇号不在缓冲中*/
			ReadSector(bpb->DRV_num, bpb->secoff + bpb->res + i, 1, FAR2LINE((DWORD)((void far *)idx)));
		prei = clui;
		clui = idx[clui % (bpb->bps >> 2)];
	}
}

/*搜索目录项*/
WORD SearchDir(	BPB *bpb,			/*输入：分区BPB*/
				DWORD *idx,			/*修改：索引缓存*/
				BYTE *buf,			/*修改：数据缓存*/
				DIR *SrcDir,		/*输入：搜索的目录*/
				BYTE *FileName,		/*输入：被搜索的目录项名称*/
				DIR *DstDir)		/*输出：找到的目录项*/
{
	DWORD prei, clui;/*簇号计数,上一簇号*/

	prei = 0xFFFFFFFF;
	clui = ((DWORD)(SrcDir->idxh) << 16) | SrcDir->idxl;
	for (;;)
	{
		DWORD i;
		DWORD fstblk;

		fstblk = bpb->secoff + bpb->cluoff + bpb->spc * clui;	/*取得要读取的首扇区号*/
		for (i = 0; i < bpb->spc; i++)
		{
			WORD j;
			ReadSector(bpb->DRV_num, fstblk, 1, FAR2LINE((DWORD)((void far *)buf)));	/*读取数据扇区*/
			for (j = 0; j < bpb->bps / sizeof(DIR); j++)
				if (namecmp(((DIR *)buf)[j].name, FileName) == 0)	/*搜索成功*/
				{
					*DstDir = ((DIR *)buf)[j];	/*复制找到的目录项*/
					return NO_ERROR;
				}
			fstblk++;
		}
		if ((i = clui / (bpb->bps >> 2)) != prei / (bpb->bps >> 2))	/*下一簇号不在缓冲中*/
			ReadSector(bpb->DRV_num, bpb->secoff + bpb->res + i, 1, FAR2LINE((DWORD)((void far *)idx)));
		prei = clui;
		if ((clui = idx[clui % (bpb->bps >> 2)] & 0x0FFFFFFF) >= 0x0FFFFFF8)	/*文件结束*/
			return NOT_FOUND;
	}
}

/*按路径读取整个文件*/
DWORD ReadPath(	BPB *bpb,			/*输入：分区BPB*/
				DWORD *idx,			/*修改：索引缓存*/
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

/*Loader程序主函数,参数为自定义参数*/
void main(BPB0 *bpb0)
{
	DWORD idx[IDX_COU];	/*16K字节索引缓存*/
	BYTE buf[BUF_COU];	/*16K字节数据缓存*/
	BPB bpb;
	DIR RootDir, SrcDir, DstDir;	/*目录项缓存*/
	BYTE BootList[4096], *cmd;	/*启动列表*/
	DWORD addr = 0x10000, *BinAddr = BIN_ADDR, end;	/*内核存放位置, 程序段数据指针*/
	WORD *VesaMode = VESA_MODE;

	if (bpb0->bps > BUF_COU)	/*检查每扇区字节数是否合法*/
		goto errip;
	bpb.DRV_num = bpb0->DRV_num;	/*驱动器号*/
	bpb.bps = bpb0->bps;	/*每扇区字节数*/
	bpb.spc = bpb0->spc;	/*每簇扇区数*/
	bpb.res = bpb0->res;	/*保留扇区数,包括引导记录*/
	bpb.secoff = bpb0->secoff;	/*分区在磁盘上的起始扇区偏移*/
	bpb.cluoff = bpb0->res + (bpb0->spf * bpb0->nf) - (bpb0->spc << 1);	/*分区内首簇开始扇区偏移*/
	RootDir.attr = 0x10;
	RootDir.idxh = (bpb0->rcn >> 16);
	RootDir.idxl = (WORD)bpb0->rcn;
	RootDir.len = 0;
	SrcDir = RootDir;
	end = ReadPath(&bpb, idx, buf, &SrcDir, &DstDir, bpb0->BootPath, FAR2LINE((DWORD)((void far *)BootList)));
	if (end == 0)
		goto errfnf;
	if (end == FAR2LINE((DWORD)((void far *)BootList)))
		goto errfie;
	cmd = BootList;
	BootList[DstDir.len] = 0;
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
				end = ReadPath(&bpb, idx, buf, &SrcDir, &DstDir, cmd, addr);
				if (end == 0)
					goto errfnf;
				if (end == addr)
					break;
				end = (end + 0x00000FFF) & 0xFFFFF000;		/*调整到4K边界*/
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
