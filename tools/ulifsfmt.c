/*	ulifsfmt.c for ulios tools
	作者：孙亮
	功能：格式化分区成为ULIFS分区
	最后修改日期：2010-03-05
	备注：使用Turbo C TCC编译器编译成DOS平台COM或EXE文件
*/

#include <stdio.h>
#include <time.h>

typedef unsigned short	BOOL;
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

typedef struct _BOOT_SEC_PART
{
	BYTE boot;	/*引导指示符活动分区为0x80*/
	BYTE beg[3];/*开始磁头/扇区/柱面*/
	BYTE fsid;	/*文件系统ID*/
	BYTE end[3];/*结束磁头/扇区/柱面*/
	DWORD fst;	/*从磁盘开始到分区扇区数*/
	DWORD cou;	/*总扇区数*/
}BOOT_SEC_PART;	/*分区表项*/

typedef struct _BOOT_SEC
{
	BYTE code[446];	/*引导程序*/
	BOOT_SEC_PART part[4];	/*分区表*/
	WORD aa55;		/*启动标志*/
}BOOT_SEC;	/*引导扇区结构*/

typedef struct _PART_INF
{
	BYTE fsid;	/*文件系统ID*/
	BYTE boot;	/*启动标示*/
	BYTE drv;	/*磁盘号*/
	BYTE lev;	/*层级*/
	DWORD fst;	/*起始扇区号*/
	DWORD cou;	/*扇区数*/
	DWORD idsec;/*FSID所在扇区号*/
	WORD pid;	/*FSID所在分区表ID*/
}PART_INF;	/*分区信息*/

typedef struct _ULIFS_BOOTSEC
{
	DWORD	jmpi;	/*跳转指令*/
	BYTE	oem[8];	/*OEM ID(tian&uli2k_X)*/
	DWORD	fsid;	/*文件系统标志"ULTN",文件系统代码:0x7C*/
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
	BYTE	code[382];	/*引导代码*/
	WORD	aa55;	/*引导标志*/
}ULIFS_BOOTSEC;	/*ULIFS引导扇区数据*/

typedef struct _BLKID
{
	DWORD fst;	/*首簇*/
	DWORD cou;	/*数量*/
}BLKID;	/*块索引节点*/

#define ULIFS_FILE_NAME_SIZE	80
#define ULIFS_FILE_ATTR_RDONLY	0x00000001	/*只读*/
#define ULIFS_FILE_ATTR_HIDDEN	0x00000002	/*隐藏*/
#define ULIFS_FILE_ATTR_SYSTEM	0x00000004	/*系统*/
#define ULIFS_FILE_ATTR_LABEL	0x00000008	/*卷标*/
#define ULIFS_FILE_ATTR_DIREC	0x00000010	/*目录(只读)*/
#define ULIFS_FILE_ATTR_ARCH	0x00000020	/*归档*/
#define ULIFS_FILE_ATTR_EXEC	0x00000040	/*可执行*/
#define ULIFS_FILE_ATTR_UNMDFY	0x80000000	/*属性不可修改*/

typedef struct _ULIFS_DIR
{
	BYTE name[ULIFS_FILE_NAME_SIZE];/*文件名*/
	DWORD CreateTime;	/*创建时间1970-01-01经过的秒数*/
	DWORD ModifyTime;	/*修改时间*/
	DWORD AccessTime;	/*访问时间*/
	DWORD attr;			/*属性*/
	DWORD size[2];		/*文件字节数,目录文件的字节数有效*/
	BLKID idx[3];		/*最后有3个自带的索引块,所以一个文件有可能用不到索引簇*/
}ULIFS_DIR;	/*目录项结构*/

#define FALSE		0
#define TRUE		1

#define NO_ERROR	0
#define ERROR_DISK	1
#define ERROR_FILE	2

/*读写扇区*/
int RwSector(	BYTE	DrvNum,		/*输入：驱动器号*/
				BOOL	isWrite,	/*输入：是否写入*/
				DWORD	BlockAddr,	/*输入：起始扇区号*/
				WORD	BlockCount,	/*输入：扇区数*/
				DWORD	BufferAddr)	/*输出：存放数据线性地址*/
{
	DAP dap;

	dap.PacketSize = 0x10;
	dap.Reserved = 0;
	dap.BlockCount = BlockCount;
	dap.BufferAddr = LINE2FAR(BufferAddr);
	dap.BlockAddr[0] = BlockAddr;
	dap.BlockAddr[1] = 0;

	if (isWrite)
		_AX = 0x4300;
	else
		_AX = 0x4200;
	_DL = DrvNum;
	_SI = (WORD)(&dap);
	asm int 13h;
	asm jc short DiskError;
	return NO_ERROR;
DiskError:
	return ERROR_DISK;
}

/*取得分区信息*/
void ReadPart(PART_INF *part)
{
	BYTE i, j;
	for (i = 0; i < 0x7F; i++)	/*每个硬盘*/
	{
		BOOT_SEC mbr;
		if (RwSector(i + 0x80, FALSE, 0, 1, FAR2LINE((DWORD)((void far *)&mbr))) != NO_ERROR)
			continue;
		if (mbr.aa55 != 0xAA55)	/*bad MBR*/
			continue;
		for (j = 0; j < 4; j++)	/*4个基本分区*/
		{
			BOOT_SEC_PART *CurPart = &mbr.part[j];
			if (CurPart->cou == 0)
				continue;	/*空分区*/
			if (CurPart->fsid == 0x05 || CurPart->fsid == 0x0F)	/*扩展分区*/
			{
				DWORD fst = CurPart->fst;
				for (;;)	/*扩展分区的每个逻辑驱动器*/
				{
					BOOT_SEC ebr;
					if (RwSector(i + 0x80, FALSE, fst, 1, FAR2LINE((DWORD)((void far *)&ebr))) != NO_ERROR)
						break;
					if (ebr.aa55 != 0xAA55)	/*bad EBR*/
						break;
					if (ebr.part[0].cou)	/*不是空扩展分区*/
					{
						part->fsid = ebr.part[0].fsid;
						part->boot = ebr.part[0].boot;
						part->drv = i + 0x80;
						part->lev = 1;
						part->fst = fst + ebr.part[0].fst;
						part->cou = ebr.part[0].cou;
						part->idsec = fst;
						part->pid = 0;
						part++;
					}
					if (ebr.part[1].cou == 0)	/*最后一个逻辑驱动器*/
						break;
					fst = CurPart->fst + ebr.part[1].fst;
				}
			}
			else	/*普通主分区*/
			{
				part->fsid = CurPart->fsid;
				part->boot = CurPart->boot;
				part->drv = i + 0x80;
				part->lev = 0;
				part->fst = CurPart->fst;
				part->cou = CurPart->cou;
				part->idsec = 0;
				part->pid = j;
				part++;
			}
		}
	}
	part->fsid = 0;
}

int FormatUlifs(PART_INF *part, WORD spc, WORD res, BYTE *label)
{
	WORD i, spbm;
	BYTE *p;
	ULIFS_BOOTSEC dbr;
	ULIFS_DIR dir;
	BYTE buf[512];

	printf("Writing... boot sector");
	memset(&dbr, 0, sizeof(ULIFS_BOOTSEC));
	memcpy(dbr.oem, "TUX soft", 8);
	dbr.fsid = 0x4E544C55;
	dbr.bps = 512;
	dbr.spc = 1 << spc;
	dbr.res = res;
	dbr.secoff = part->fst;
	dbr.seccou = part->cou;
	for (;;)
	{
		dbr.clucou = (dbr.seccou - res - dbr.spbm - dbr.spbm) >> spc;
		spbm = (dbr.clucou + 0xFFF) >> 12;
		if (spbm == dbr.spbm)
			break;
		dbr.spbm = spbm;
	}
	dbr.cluoff = res + spbm + spbm;
	dbr.aa55 = 0xAA55;
	if (RwSector(part->drv, TRUE, dbr.secoff, 1, FAR2LINE((DWORD)((void far *)&dbr))) != NO_ERROR)
		return ERROR_DISK;

	printf("\nWriting... bad sector bmp");
	memset(buf, 0, sizeof(buf));
	for (i = 0; i < spbm; i++)
	{
		if (RwSector(part->drv, TRUE, dbr.secoff + res + i, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
			return ERROR_DISK;
		printf(".");
	}

	printf("\nWriting... used sector bmp");
	buf[0] = 1;
	if (RwSector(part->drv, TRUE, dbr.secoff + res + spbm, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
		return ERROR_DISK;
	printf(".");
	buf[0] = 0;
	for (i = 1; i < spbm; i++)
	{
		if (RwSector(part->drv, TRUE, dbr.secoff + res + spbm + i, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
			return ERROR_DISK;
		printf(".");
	}

	printf("\nWriting... root dir");
	memset(&dir, 0, sizeof(ULIFS_DIR));
	p = label;
	for (p++; *p; p++)
		if (*p == '/' || (p - label) >= ULIFS_FILE_NAME_SIZE - 1)
			break;
	memcpy(dir.name, label, p - label);
	time((time_t*)&dir.CreateTime);
	dir.AccessTime = dir.ModifyTime = dir.CreateTime;
	dir.attr = ULIFS_FILE_ATTR_LABEL | ULIFS_FILE_ATTR_DIREC;
	dir.size[0] = sizeof(ULIFS_DIR);
	dir.idx[0].cou = 1;
	memcpy(buf, &dir, sizeof(ULIFS_DIR));
	if (RwSector(part->drv, TRUE, dbr.secoff + dbr.cluoff, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
		return ERROR_DISK;

	printf("\nSetting... ulifs id\n");
	if (RwSector(part->drv, FALSE, part->idsec, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
		return ERROR_DISK;
	((BOOT_SEC*)buf)->part[part->pid].fsid = 0x7C;
	if (RwSector(part->drv, TRUE, part->idsec, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
		return ERROR_DISK;

	return NO_ERROR;
}

int main()
{
	PART_INF part[32], *partp;
	WORD partn, sel;
	BYTE buf[ULIFS_FILE_NAME_SIZE];
	int res;
	printf("Welcome to ulifs format program!\nWARNING! I will clean up all of the data on partition you selected.\n\n");
	printf("Checking partition information...");
	ReadPart(part);
	printf("Done\n");
	printf("\tFS_TYPE\tACTIVE\tDRV_ID\tTYPE\tSTART\tCOUNT\n");
	for (partp = part, partn = 0; partp->fsid; partp++)
	{
		printf("%u:\t", ++partn);
		switch (partp->fsid)
		{
		case 0x01:
		case 0x0B:
		case 0x0C:
			printf("fat32");
			break;
		case 0x7C:
			printf("ulifs");
			break;
		default:
			printf("0x%02X", partp->fsid);
		}
		printf("\t%s\t0x%X\t%s\t%lu\t%lu\n", partp->boot ? "yes" : "no", partp->drv, partp->lev ? "Logical" : "Primary", partp->fst, partp->cou);
	}
	printf("Which partition you want to format?\n[0:Exit, 1 to %u:Select partition]:", partn);
	for (;;)
	{
		gets(buf);
		sel = atoi(buf);
		if (sel == 0)
			return NO_ERROR;
		if (sel > partn)
		{
			printf("Select 0 to %u:", partn);
			continue;
		}
		partp = &part[--sel];
		break;
	}
	printf("How many bytes per cluster?\n[0:Exit, 1:512 byte, 2:1KB(default), 3:2KB, 4:4KB, 5:8KB, 6:16KB, 7:32KB, 8:64KB]:");
	for (;;)
	{
		gets(buf);
		if (buf[0] == '\0')
			sel = 1;
		else
		{
			sel = atoi(buf);
			if (sel == 0)
				return NO_ERROR;
			if (sel > 8)
			{
				printf("Select 0 to 8:");
				continue;
			}
			sel--;
		}
		break;
	}
	printf("Volume label?\n[78 characters, / for none, ENTER for Exit]:");
	gets(buf + 1);
	if (buf[1] == '\0')
		return NO_ERROR;
	buf[0] = '/';
	res = FormatUlifs(partp, sel, 8, buf);
	switch (res)
	{
	case NO_ERROR:
		printf("OK!\n");
		break;
	case ERROR_DISK:
		printf("Disk Error!\n");
		break;
	case ERROR_FILE:
		printf("File Error!\n");
		break;
	}
	return NO_ERROR;
}
