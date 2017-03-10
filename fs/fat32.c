/*	ulifs.c for ulios file system
	作者：孙亮
	功能：FAT32文件系统简单支持
	最后修改日期：2009-06-06
*/

#include "fs.h"

typedef struct _FAT32_BPB
{
	BYTE	INS_JMP[3];/*跳转指令*/
	BYTE	OEM[8];	/*OEM ID(tian&uli2k_X)*/
	WORD	bps;	/*每扇区字节数512*/
	BYTE	spc;	/***每簇扇区数*/
	WORD	ressec;	/***保留扇区数(第一个FAT开始之前的扇区数)*/
	BYTE	fats;	/***FAT数一般为2*/
	WORD	rtents;	/*根目录项数(FAT32为0)*/
	WORD	smlsec;	/*小扇区数(FAT32为0)*/
	BYTE	media;	/*媒体描述符硬盘0xF8*/
	WORD	spf;	/*每FAT扇区数(FAT32为0)*/
	WORD	spt;	/*每道扇区数*/
	WORD	heads;	/*磁头数*/
	DWORD	relsec;	/*盘上引导扇区以前所有扇区数*/
	DWORD	totsec;	/***总扇区数*/
	DWORD	spfat;	/***每FAT扇区数FAT32使用*/
	WORD	exflg;	/*扩展标志*/
	WORD	fsver;	/*文件系统版本*/
	DWORD	rtclu;	/***根目录簇号*/
	WORD	fsinfo;	/***文件系统信息扇区号一般为1*/
	WORD	bkbot;	/*备份引导扇区6*/
	BYTE	reser[12];	/*保留12字节*/
	/*以下为扩展BPB*/
	BYTE	pdn;	/*物理驱动器号,第一个驱动器为0x80*/
	BYTE	exres;	/*保留*/
	BYTE	exbtsg;	/*扩展引导标签为0x29*/
	DWORD	volume;	/*分区序号,用于区分磁盘*/
	char	vollab[11];/*卷标*/
	BYTE	fsid[8];/*系统ID*/
	BYTE	code[420];	/*引导代码*/
	WORD	aa55;	/*引导标志*/
}__attribute__((packed)) FAT32_BPB;	/*FAT32的BPB*/

typedef struct _FAT32
{
	DWORD	spc;	/*每簇扇区数*/
	DWORD	fats;	/*fat数目*/
	DWORD	res;	/*保留扇区数(FAT之前的扇区)*/
	DWORD	fsinfo;	/*文件系统信息扇区号一般为1*/
	DWORD	spfat;	/*每FAT扇区数*/
	DWORD	clu0;	/*0簇扇区号*/
	DWORD	clus;	/*簇数量*/
	DWORD	rtclu;	/*根目录簇号*/
	DWORD	fstec;	/*第一个空簇号*/
	DWORD	rescc;	/*剩余簇数量*/
	DWORD	fatl;	/*FAT锁*/
}FAT32;	/*内存中的文件系统信息结构*/

typedef struct _FAT32_FSI
{
	DWORD	RRaA;
	BYTE	res0[480];
	DWORD	rrAa;
	DWORD	frecou;	/*剩余簇数量*/
	DWORD	nxtfre;	/*第一个空簇号*/
	BYTE	res1[12];
	DWORD	end;
}FAT32_FSI;	/*文件信息扇区*/

#define FAT32_FILE_MAIN_SIZE	8		/*主文件名长度*/
#define FAT32_FILE_NAME_SIZE	11		/*文件名总长*/
#define FAT32_FILE_ATTR_RDONLY	0x01	/*只读*/
#define FAT32_FILE_ATTR_HIDDEN	0x02	/*隐藏*/
#define FAT32_FILE_ATTR_SYSTEM	0x04	/*系统*/
#define FAT32_FILE_ATTR_LABEL	0x08	/*卷标(只读)*/
#define FAT32_FILE_ATTR_DIREC	0x10	/*目录(只读)*/
#define FAT32_FILE_ATTR_ARCH	0x20	/*归档*/
#define FAT32_FILE_ATTR_LONG	(FAT32_FILE_ATTR_RDONLY | FAT32_FILE_ATTR_HIDDEN | FAT32_FILE_ATTR_SYSTEM | FAT32_FILE_ATTR_LABEL)	/*长文件名*/

typedef struct _FAT32_DIR
{
	char name[FAT32_FILE_NAME_SIZE];	/*文件名*/
	BYTE attr;		/*属性*/
	BYTE reserved;	/*保留*/
	BYTE crtmils;	/*创建时间10毫秒位*/
	WORD crttime;	/*创建时间*/
	WORD crtdate;	/*创建日期*/
	WORD acsdate;	/*访问日期*/
	WORD idxh;		/*首簇高16位*/
	WORD mdftime;	/*修改时间*/
	WORD mdfdate;	/*修改日期*/
	WORD idxl;		/*首簇低16位*/
	DWORD size;		/*文件字节数*/
}FAT32_DIR;	/*FAT32目录项结构*/

#define FAT32_ID		0x0C		/*FAT32文件系统ID*/
#define FAT32_BPS		0x200		/*允许的每扇区字节数*/
#define FAT32_MAX_SPC	0x80		/*每簇扇区数极限*/
#define FAT32_MAX_DIR	0xFFFF		/*目录中目录项数量极限*/

static inline long RwClu(PART_DESC *part, BOOL isWrite, DWORD clu, DWORD cou, void *buf)	/*读写分区簇*/
{
	FAT32 *data = (FAT32*)part->data;
	return RwPart(part, isWrite, data->clu0 + data->spc * clu, data->spc * cou, buf);
}

/*挂载FAT32分区*/
long Fat32MntPart(PART_DESC *pd)
{
	FAT32 *fs;
	DWORD i;
	long res;
	BYTE buf[FAT32_BPS];

	if (pd->FsID != 0x01 && pd->FsID != 0x0B && pd->FsID != 0x0C)
		return FS_ERR_WRONG_ARGS;
	if ((res = RwPart(pd, FALSE, 0, 1, buf)) != NO_ERROR)	/*读引导扇区*/
		return res;
	if (((FAT32_BPB*)buf)->aa55 != 0xAA55 || ((FAT32_BPB*)buf)->bps != FAT32_BPS)
		return FS_ERR_WRONG_ARGS;
	if ((fs = (FAT32*)malloc(sizeof(FAT32))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	fs->spc = ((FAT32_BPB*)buf)->spc;
	fs->fats = ((FAT32_BPB*)buf)->fats;
	fs->res = ((FAT32_BPB*)buf)->ressec;
	fs->spfat = ((FAT32_BPB*)buf)->spfat;
	fs->clu0 = ((FAT32_BPB*)buf)->ressec + (((FAT32_BPB*)buf)->spfat * ((FAT32_BPB*)buf)->fats) - (((FAT32_BPB*)buf)->spc << 1);
	fs->clus = (pd->SeCou - fs->clu0) / ((FAT32_BPB*)buf)->spc;
	fs->rtclu = ((FAT32_BPB*)buf)->rtclu;
	fs->fsinfo = ((FAT32_BPB*)buf)->fsinfo;
	strncpy(pd->part.label, ((FAT32_BPB*)buf)->vollab, 11);
	pd->part.label[11] = 0;
	if ((res = RwPart(pd, FALSE, fs->fsinfo, 1, buf)) != NO_ERROR)	/*读信息扇区*/
	{
		free(fs, sizeof(FAT32));
		return res;
	}
	fs->rescc=((FAT32_FSI*)buf)->frecou;
	fs->fstec=((FAT32_FSI*)buf)->nxtfre;
	fs->fatl = FALSE;
	if (fs->rescc >= fs->clus || fs->fstec >= fs->clus)	/*簇号错误,重新扫描*/
	{
		fs->rescc = fs->fstec = 0;
		RwPart(pd, FALSE, fs->res, 1, buf);	/*第1个FAT簇*/
		for (i = 2; i < fs->clus; i++)	/*查找空簇*/
		{
			if ((i & 0x7F) == 0)
				RwPart(pd, FALSE, fs->res + (i >> 7), 1, buf);
			if (((DWORD*)buf)[i & 0x7F] == 0)	/*(i%128)是空簇*/
			{
				if (fs->fstec == 0)
					fs->fstec = i;	/*取得首空簇号*/
				fs->rescc++;
			}
		}
	}
	pd->part.size = (QWORD)fs->clus * fs->spc * FAT32_BPS;
	pd->part.remain = (QWORD)fs->rescc * fs->spc * FAT32_BPS;
	pd->part.attr = 0;
	pd->data = fs;
	return NO_ERROR;
}

/*卸载FAT32分区*/
void Fat32UmntPart(PART_DESC *pd)
{
	FAT32_FSI buf;

	RwPart(pd, FALSE, ((FAT32*)pd->data)->fsinfo, 1, &buf);
	buf.frecou = ((FAT32*)pd->data)->rescc;
	buf.nxtfre = ((FAT32*)pd->data)->fstec;
	RwPart(pd, TRUE, ((FAT32*)pd->data)->fsinfo, 1, &buf);
	free(pd->data, sizeof(FAT32));
	pd->data = NULL;
	pd->FsID = FAT32_ID;
}

/*设置分区信息*/
long Fat32SetPart(PART_DESC *pd, PART_INFO *pi)
{
	const char *namep;
	long res;
	FAT32_BPB buf;

	for (namep = pi->label; *namep; namep++)
		if (namep - pi->label >= 11)	/*卷标名超长*/
			return FS_ERR_NAME_TOOLONG;
	if ((res = RwPart(pd, FALSE, 0, 1, &buf)) != NO_ERROR)
		return res;
	memcpy32(pd->part.label, pi->label, 3);
	strncpy(buf.vollab, pi->label, 11);
	if ((res = RwPart(pd, TRUE, 0, 1, &buf)) != NO_ERROR)
		return res;
	return NO_ERROR;
}

/*读写文件,数据不允许超出文件尾*/
long Fat32RwFile(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, void *buf, DWORD *curc)
{
	FAT32_DIR *data;
	DWORD dati, end;	/*当前字节位置,字节结尾位置*/
	PART_DESC *CurPart;	/*所在分区结构指针*/
	DWORD bpc;	/*每簇字节数*/
	DWORD PreIdx, CurIdx;	/*索引节点索引*/
	long res;
	DWORD idx[FAT32_BPS / sizeof(DWORD)];	/*索引扇区*/
	BYTE dat[FAT32_BPS * FAT32_MAX_SPC];	/*数据簇*/

	data = (FAT32_DIR*)fd->data;
	end = (DWORD)seek + siz;
	CurPart = fd->part;
	bpc = ((FAT32*)(CurPart->data))->spc * FAT32_BPS;
	PreIdx = INVALID;
	if (curc && (*curc))
	{
		CurIdx = *curc;
		dati = (DWORD)seek - (DWORD)seek % bpc;
	}
	else
	{
		CurIdx = ((DWORD)data->idxh << 16) | data->idxl;
		dati = 0;
	}
	for (;;)
	{
		if (dati >= (DWORD)seek)	/*已经开始*/
		{
			if (dati + bpc > end)	/*将要完成*/
			{
				if ((res = RwClu(CurPart, FALSE, CurIdx, 1, dat)) != NO_ERROR)
					return res;
				if (isWrite)
				{
					memcpy8(dat, buf, end % bpc);
					if ((res = RwClu(CurPart, TRUE, CurIdx, 1, dat)) != NO_ERROR)
						return res;
				}
				else
					memcpy8(buf, dat, end % bpc);
			}
			else	/*中间*/
			{
				if ((res = RwClu(CurPart, isWrite, CurIdx, 1, buf)) != NO_ERROR)
					return res;
				buf += bpc;
			}
		}
		else if (dati + bpc > (DWORD)seek)	/*将要开始*/
		{
			DWORD tmpcou;
			if (dati + bpc >= end)	/*开始即完成*/
				tmpcou = siz;
			else
				tmpcou = bpc - (DWORD)seek % bpc;
			if ((res = RwClu(CurPart, FALSE, CurIdx, 1, dat)) != NO_ERROR)
				return res;
			if (isWrite)
			{
				memcpy8(dat + (DWORD)seek % bpc, buf, tmpcou);
				if ((res = RwClu(CurPart, TRUE, CurIdx, 1, dat)) != NO_ERROR)
					return res;
			}
			else
				memcpy8(buf, dat + (DWORD)seek % bpc, tmpcou);
			buf += tmpcou;
		}
		dati += bpc;
		if (dati > end)	/*完成*/
		{
			if (curc)
				*curc = CurIdx;
			return NO_ERROR;
		}
		if ((CurIdx >> 7) != (PreIdx >> 7))	/*下一簇号不在缓冲中*/
			if ((res = RwPart(CurPart, FALSE, ((FAT32*)CurPart->data)->res + (CurIdx >> 7), 1, idx)) != NO_ERROR)
				return res;
		PreIdx = CurIdx;
		CurIdx = idx[CurIdx & 0x7F];
		if (dati >= end)	/*完成*/
		{
			if (curc)
				*curc = (CurIdx & 0x0FFFFFFF) != 0x0FFFFFFF ? CurIdx : 0;
			return NO_ERROR;
		}
	}
}

/*分配簇*/
DWORD AllocClu(PART_DESC *pd, DWORD cou)
{
	DWORD CurIdx, PreIdx, *pidx, *cidx, *swp;
	DWORD idx[2][FAT32_BPS / sizeof(DWORD)];

	lock(&((FAT32*)pd->data)->fatl);
	PreIdx = CurIdx = ((FAT32*)pd->data)->fstec;
	pidx = cidx = idx[0];
	swp = idx[1];
	((FAT32*)pd->data)->rescc -= cou;
	cou--;
	RwPart(pd, FALSE, ((FAT32*)pd->data)->res + (CurIdx >> 7), 1, cidx);
	for (CurIdx++;; CurIdx++)
	{
		if ((CurIdx & 0x7F) == 0)	/*到达新FAT扇区*/
		{
			cidx = swp;	/*设置新缓冲*/
			RwPart(pd, FALSE, ((FAT32*)pd->data)->res + (CurIdx >> 7), 1, cidx);
		}
		if (cidx[CurIdx & 0x7F] == 0)	/*找到空簇*/
		{
			if (cou)	/*分配没有完成*/
			{
				pidx[PreIdx & 0x7F] = CurIdx;
				if ((CurIdx >> 7) != (PreIdx >> 7))	/*当前与脏块不同*/
				{
					RwPart(pd, TRUE, ((FAT32*)pd->data)->res + (PreIdx >> 7), 1, pidx);
					RwPart(pd, TRUE, ((FAT32*)pd->data)->res + ((FAT32*)pd->data)->spfat + (PreIdx >> 7), 1, pidx);
					swp = pidx;	/*交换缓冲*/
					pidx = cidx;	/*设置当前块为前一块*/
				}
				PreIdx = CurIdx;
				cou--;
			}
			else	/*全部分配完*/
			{
				pidx[PreIdx & 0x7F] = 0x0FFFFFFF;
				RwPart(pd, TRUE, ((FAT32*)pd->data)->res + (PreIdx >> 7), 1, pidx);
				RwPart(pd, TRUE, ((FAT32*)pd->data)->res + ((FAT32*)pd->data)->spfat + (PreIdx >> 7), 1, pidx);
				PreIdx = ((FAT32*)pd->data)->fstec;
				((FAT32*)pd->data)->fstec = CurIdx;	/*重新设置首空簇*/
				ulock(&((FAT32*)pd->data)->fatl);
				return PreIdx;
			}
		}
	}
}

/*回收簇*/
DWORD FreeClu(PART_DESC *pd, DWORD clu)
{
	DWORD buf[FAT32_BPS / sizeof(DWORD)];	/*索引扇区*/

	lock(&((FAT32*)pd->data)->fatl);
	RwPart(pd, FALSE, ((FAT32*)pd->data)->res + (clu >> 7), 1, buf);
	for (;;)
	{
		DWORD CurIdx;
		if (((FAT32*)pd->data)->fstec > clu)
			((FAT32*)pd->data)->fstec = clu;
		CurIdx = buf[clu & 0x7F];
		buf[clu & 0x7F] = 0;
		((FAT32*)pd->data)->rescc++;
		if ((CurIdx & 0x0FFFFFFF) == 0x0FFFFFFF)
			break;
		if ((CurIdx >> 7) != (clu >> 7))
		{
			RwPart(pd, TRUE, ((FAT32*)pd->data)->res + (clu >> 7), 1, buf);
			RwPart(pd, TRUE, ((FAT32*)pd->data)->res + ((FAT32*)pd->data)->spfat + (clu >> 7), 1, buf);
			RwPart(pd, FALSE, ((FAT32*)pd->data)->res + (CurIdx >> 7), 1, buf);
		}
		clu = CurIdx;
	}
	RwPart(pd, TRUE, ((FAT32*)pd->data)->res + (clu >> 7), 1, buf);
	RwPart(pd, TRUE, ((FAT32*)pd->data)->res + ((FAT32*)pd->data)->spfat + (clu >> 7), 1, buf);
	ulock(&((FAT32*)pd->data)->fatl);
	return NO_ERROR;
}

/*设置文件长度*/
long Fat32SetSize(FILE_DESC *fd, QWORD siz)
{
	FAT32_DIR *data;
	FILE_DESC *par;
	DWORD cun0, cun, clui, CurIdx, PreIdx;/*原簇数量,修改后簇数量,数据簇索引*/
	PART_DESC *CurPart;	/*所在分区结构指针*/
	DWORD bpc;	/*每簇字节数*/
	long res;
	DWORD idx[FAT32_BPS / sizeof(DWORD)];	/*索引扇区*/

	if (siz > 0xFFFFFFFF)
		return FS_ERR_SIZE_LIMIT;
	data = (FAT32_DIR*)fd->data;
	par = fd->par;
	CurPart = fd->part;
	bpc = ((FAT32*)CurPart->data)->spc * FAT32_BPS;
	cun0 = (data->size + bpc - 1) / bpc;
	cun = ((DWORD)siz + bpc - 1) / bpc;
	if (cun == cun0)	/*不需要修改簇*/
	{
		data->size = (data->attr & FAT32_FILE_ATTR_DIREC) ? 0 : siz;
		fd->file.size = siz;
		if (par)
			return Fat32RwFile(par, TRUE, fd->idx * sizeof(FAT32_DIR), sizeof(FAT32_DIR), data, NULL);
		return NO_ERROR;
	}
	if (cun > cun0 && cun - cun0 > ((FAT32*)CurPart->data)->rescc)
		return FS_ERR_HAVENO_SPACE;	/*超出剩余簇数量*/
	CurIdx = ((DWORD)data->idxh << 16) | data->idxl;
	PreIdx = INVALID;
	clui = 0;
	if (cun < cun0)	/*减小文件*/
	{
		for (;;)
		{
			if (clui >= cun)	/*从此开始回收*/
				break;
			clui++;
			if ((CurIdx >> 7) != (PreIdx >> 7))	/*下一簇号不在缓冲中*/
				RwPart(CurPart, FALSE, ((FAT32*)CurPart->data)->res + (CurIdx >> 7), 1, idx);
			PreIdx = CurIdx;
			CurIdx = idx[CurIdx & 0x7F];
		}
		if ((res = FreeClu(CurPart, CurIdx)) != NO_ERROR)
			return res;
		if (cun)	/*设置结束簇*/
		{
			RwPart(CurPart, FALSE, ((FAT32*)CurPart->data)->res + (PreIdx >> 7), 1, idx);
			idx[PreIdx & 0x7F] = 0x0FFFFFFF;
			RwPart(CurPart, TRUE, ((FAT32*)CurPart->data)->res + (PreIdx >> 7), 1, idx);
			RwPart(CurPart, TRUE, ((FAT32*)CurPart->data)->res + ((FAT32*)CurPart->data)->spfat + (PreIdx >> 7), 1, idx);
		}
		else
			data->idxh = data->idxl = 0;
		CurPart->part.remain += (cun0 - cun) * bpc;
		data->size = (data->attr & FAT32_FILE_ATTR_DIREC) ? 0 : siz;
		fd->file.size = siz;
		if (par)
			return Fat32RwFile(par, TRUE, fd->idx * sizeof(FAT32_DIR), sizeof(FAT32_DIR), data, NULL);
		return NO_ERROR;
	}
	else	/*增大文件*/
	{
		DWORD clu;
		for (;;)
		{
			if (cun0 == 0 || CurIdx == 0x0FFFFFFF)
				break;
			clui++;
			if ((CurIdx >> 7) != (PreIdx >> 7))	/*下一簇号不在缓冲中*/
				RwPart(CurPart, FALSE, ((FAT32*)CurPart->data)->res + (CurIdx >> 7), 1, idx);
			PreIdx = CurIdx;
			CurIdx = idx[CurIdx & 0x7F];
		}
		if (cun - clui)
		{
			clu = AllocClu(CurPart, cun - clui);
			if (cun0)	/*设置连接簇*/
			{
				RwPart(CurPart, FALSE, ((FAT32*)CurPart->data)->res + (PreIdx >> 7), 1, idx);
				idx[PreIdx & 0x7F] = clu;
				RwPart(CurPart, TRUE, ((FAT32*)CurPart->data)->res + (PreIdx >> 7), 1, idx);
				RwPart(CurPart, TRUE, ((FAT32*)CurPart->data)->res + ((FAT32*)CurPart->data)->spfat + (PreIdx >> 7), 1, idx);
			}
			else
			{
				data->idxh = clu >> 16;
				data->idxl = clu;
			}
			CurPart->part.remain -= (cun - cun0) * bpc;
		}
		data->size = (data->attr & FAT32_FILE_ATTR_DIREC) ? 0 : siz;
		fd->file.size = siz;
		if (par)
			return Fat32RwFile(par, TRUE, fd->idx * sizeof(FAT32_DIR), sizeof(FAT32_DIR), data, NULL);
		return NO_ERROR;
	}
}

/*文件名填入目录结构*/
static long NameToDir(char DirName[FAT32_FILE_NAME_SIZE], const char *name)
{
	char *namep;

	memset8(DirName, ' ', FAT32_FILE_NAME_SIZE);
	namep = DirName;
	if (*name == (char)0xE5)
	{
		*namep = (char)0x05;
		namep++;
		name++;
	}
	while (*name != '.' && *name != '/' && *name)
	{
		if (namep >= DirName + FAT32_FILE_MAIN_SIZE)
			return FS_ERR_NAME_TOOLONG;
		else if (*name >= 'a' && *name <= 'z')
			*namep = *name - 0x20;
		else
			*namep = *name;
		namep++;
		name++;
	}
	if (*name == '.')
	{
		name++;
		namep = DirName + FAT32_FILE_MAIN_SIZE;
		while (*name != '/' && *name)
		{
			if (namep >= DirName + FAT32_FILE_NAME_SIZE)
				return FS_ERR_NAME_TOOLONG;
			if (*name >= 'a' && *name <= 'z')
				*namep = *name - 0x20;
			else
				*namep = *name;
			namep++;
			name++;
		}
	}
	return NO_ERROR;
}

/*目录结构填入文件名*/
static long DirToName(char *name, char DirName[FAT32_FILE_NAME_SIZE])
{
	char *namep;

	namep = DirName;
	if (*namep == (char)0x05)
	{
		*name = (char)0xE5;
		namep++;
		name++;
	}
	while (namep < DirName + FAT32_FILE_MAIN_SIZE)
	{
		if (*namep == ' ')
			break;
		*name++ = *namep++;
	}
	namep = DirName + FAT32_FILE_MAIN_SIZE;
	if (*namep != ' ')
	{
		*name++ = '.';
		while (namep < DirName + FAT32_FILE_NAME_SIZE)
		{
			if (*namep == ' ')
				break;
			*name++ = *namep++;
		}
	}
	*name = 0;
	return NO_ERROR;
}

/*时间填入目录结构*/
static long TimeToDir(WORD *DirDate, WORD *DirTime, const TM *tm)
{
	if (tm->yer < 1980)
		return TIME_ERR_WRONG_TM;
	if (tm->mon == 0 || tm->mon > 12)
		return TIME_ERR_WRONG_TM;
	if (tm->day == 0 || tm->day > 31)
		return TIME_ERR_WRONG_TM;
	if (tm->hor > 23 || tm->min > 59 || tm->sec > 59)
		return TIME_ERR_WRONG_TM;
	if (DirTime)
		*DirTime = (tm->hor << 11) | (tm->min << 5) | (tm->sec >> 1);
	*DirDate = ((tm->yer - 1980) << 9) | (tm->mon << 5) | tm->day;
	return NO_ERROR;
}

/*目录结构填入时间*/
static long DirToTime(TM *tm, WORD DirDate, WORD DirTime)
{
	tm->sec = (DirTime << 1) & 0x1F;
	tm->min = (DirTime >> 5) & 0x1F;
	tm->hor = DirTime >> 11;
	tm->day = DirDate & 0x1F;
	tm->mon = (DirDate >> 5) & 0x0F;
	tm->yer = (DirDate >> 9) + 1980;
	return NO_ERROR;
}

/*比较路径字符串与文件名是否匹配*/
BOOL Fat32CmpFile(FILE_DESC *fd, const char *path)
{
	char newnam[FAT32_FILE_NAME_SIZE], *name, *namep;

	if (NameToDir(newnam, path) != NO_ERROR)
		return FALSE;
	name = ((FAT32_DIR*)fd->data)->name;
	for (namep = newnam; namep < newnam + FAT32_FILE_NAME_SIZE; namep++, name++)
		if (*namep != *name)
			return FALSE;
	return TRUE;
}

/*data信息拷贝到info*/
static void DataToInfo(FILE_INFO *fi, FAT32_DIR *fd)
{
	TM tm;
	DirToName(fi->name, fd->name);
	DirToTime(&tm, fd->crtdate, fd->crttime);
	TMMkTime(&fi->CreateTime, &tm);
	DirToTime(&tm, fd->mdfdate, fd->mdftime);
	TMMkTime(&fi->ModifyTime, &tm);
	DirToTime(&tm, fd->acsdate, 0);
	TMMkTime(&fi->AccessTime, &tm);
	fi->attr = fd->attr;
	fi->size = fd->size;
}

/*info信息拷贝到data*/
static void InfoToData(FAT32_DIR *fd, FILE_INFO *fi)
{
	TM tm;
	NameToDir(fd->name, fi->name);
	TMLocalTime(fi->CreateTime, &tm);
	TimeToDir(&fd->crtdate, &fd->crttime, &tm);
	TMLocalTime(fi->ModifyTime, &tm);
	TimeToDir(&fd->mdfdate, &fd->mdftime, &tm);
	TMLocalTime(fi->AccessTime, &tm);
	TimeToDir(&fd->acsdate, NULL, &tm);
	fd->attr = fi->attr & 0x3F;
	fd->size = fi->size;
}

/*搜索并设置文件项*/
long Fat32SchFile(FILE_DESC *fd, const char *path)
{
	FILE_DESC *par;
	FAT32_DIR *dir;
	long res;

	if ((dir = (FAT32_DIR*)malloc(sizeof(FAT32_DIR))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	if ((par = fd->par) == NULL)	/*取得根目录项*/
	{
		memset32(dir, 0, sizeof(FAT32_DIR) / sizeof(DWORD));
		dir->idxh = ((FAT32*)fd->part->data)->rtclu >> 16;
		dir->idxl = ((FAT32*)fd->part->data)->rtclu;
		dir->attr = FAT32_FILE_ATTR_DIREC;	/*属性:目录*/
		fd->file.name[0] = 0;
		fd->file.AccessTime = fd->file.ModifyTime = fd->file.CreateTime = INVALID;
		fd->file.attr = FILE_ATTR_DIREC;
		fd->file.size = 0;
		fd->data = dir;
		return NO_ERROR;
	}
	else	/*取得其他目录项*/
	{
		DWORD curc, seek;

		fd->data = dir;
		for (curc = 0, seek = 0;; seek += sizeof(FAT32_DIR))
		{
			if ((res = Fat32RwFile(par, FALSE, seek, sizeof(FAT32_DIR), dir, &curc)) != NO_ERROR)
			{
				fd->data = NULL;
				free(dir, sizeof(FAT32_DIR));
				return res;
			}
			if (dir->attr & FAT32_FILE_ATTR_LABEL)
				continue;
			if (dir->name[0] == 0)
				break;
			if (Fat32CmpFile(fd, path))
			{
				DataToInfo(&fd->file, dir);
				fd->idx = seek / sizeof(FAT32_DIR);
				return NO_ERROR;
			}
			if (curc == 0)
				break;
		}
		fd->data = NULL;
		free(dir, sizeof(FAT32_DIR));
		return FS_ERR_PATH_NOT_FOUND;
	}
}

/*检查文件名正确性*/
static long CheckName(const char *name)
{
	static const char ErrChar[] = {	/*错误字符*/
		0x22, 0x2A, 0x2B, 0x2C, 0x2E, 0x2F, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x5B, 0x5C, 0x5D, 0x7C
	};
	const char *namep, *errcp;
	if (*name == '.')
		return FS_ERR_NAME_FORMAT;
	namep = name;
	while (*namep != '.' && *namep)	/*主名有非法字符或超长*/
	{
		if (namep - name >= FAT32_FILE_MAIN_SIZE)
			return FS_ERR_NAME_TOOLONG;
		if (*namep <= 0x20)
			return FS_ERR_NAME_FORMAT;
		for (errcp = ErrChar; errcp < &ErrChar[sizeof(ErrChar)]; errcp++)
			if (*namep == *errcp)
				return FS_ERR_NAME_FORMAT;
			namep++;
	}
	if (*namep == '.')
	{
		namep++;
		name = namep;
		while (*namep)	/*扩展名有非法字符或超长*/
		{
			if (namep - name >= FAT32_FILE_NAME_SIZE - FAT32_FILE_MAIN_SIZE)
				return FS_ERR_NAME_TOOLONG;
			if (*namep <= 0x20)
				return FS_ERR_NAME_FORMAT;
			for (errcp = ErrChar; errcp < &ErrChar[sizeof(ErrChar)]; errcp++)
				if (*namep == *errcp)
					return FS_ERR_NAME_FORMAT;
				namep++;
		}
	}
	return NO_ERROR;
}

/*创建并设置文件项*/
long Fat32NewFile(FILE_DESC *fd, const char *path)
{
	FILE_DESC *par;
	FAT32_DIR *dir;
	DWORD curc, seek;
	DWORD bpc;
	long res;
	FAT32_DIR dat[FAT32_BPS * FAT32_MAX_SPC / sizeof(FAT32_DIR)];	/*数据簇*/

	if ((res = CheckName(path)) != NO_ERROR)	/*检查文件名*/
		return res;
	if ((res = Fat32SchFile(fd, path)) != FS_ERR_PATH_NOT_FOUND)	/*文件项已经存在了*/
	{
		if (res == NO_ERROR)
		{
			free(fd->data, sizeof(FAT32_DIR));	/*释放SchFile申请的数据*/
			fd->data = NULL;
			return FS_ERR_PATH_EXISTS;
		}
		return res;
	}
	if ((dir = (FAT32_DIR*)malloc(sizeof(FAT32_DIR))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	bpc = ((FAT32*)fd->part->data)->spc * FAT32_BPS;
	/*搜索空位*/
	par = fd->par;
	for (curc = 0, seek = 0;; seek += sizeof(FAT32_DIR))
	{
		DWORD tmpcurc = curc;
		if ((res = Fat32RwFile(par, FALSE, seek, sizeof(FAT32_DIR), dir, &curc)) != NO_ERROR)
		{
			free(dir, sizeof(FAT32_DIR));
			return res;
		}
		if (dir->name[0] == (char)0xE5)
		{
			strcpy(fd->file.name, path);
			fd->file.size = 0;
			InfoToData(dir, &fd->file);
			if ((res = Fat32RwFile(par, TRUE, seek, sizeof(FAT32_DIR), dir, &tmpcurc)) != NO_ERROR)
			{
				free(dir, sizeof(FAT32_DIR));
				return res;
			}
			goto crtdir;
		}
		if (dir->name[0] == 0)
			break;
		if (curc == 0)
		{
			seek += sizeof(FAT32_DIR);
			break;
		}
	}
	if (seek >= sizeof(FAT32_DIR) * FAT32_MAX_DIR)
	{
		free(dir, sizeof(FAT32_DIR));
		return FS_ERR_SIZE_LIMIT;
	}
	((FAT32_DIR*)par->data)->size = seek;
	if ((res = Fat32SetSize(par, seek + sizeof(FAT32_DIR))) != NO_ERROR)	/*增大目录*/
	{
		((FAT32_DIR*)par->data)->size = 0;
		free(dir, sizeof(FAT32_DIR));
		return res;
	}
	strcpy(fd->file.name, path);
	fd->file.size = 0;
	InfoToData(dir, &fd->file);
	memcpy32(&dat[0], dir, sizeof(FAT32_DIR) / sizeof(DWORD));
	memset32(&dat[1], 0, (bpc - seek % bpc - sizeof(FAT32_DIR)) / sizeof(DWORD));
	if ((res = Fat32RwFile(par, TRUE, seek, bpc - seek % bpc, dat, NULL)) != NO_ERROR)
	{
		free(dir, sizeof(FAT32_DIR));
		return res;
	}
crtdir:
	fd->idx = seek / sizeof(FAT32_DIR);
	fd->data = dir;
	if (dir->attr & FAT32_FILE_ATTR_DIREC)	/*目录中创建目录的.和..*/
	{
		dir->size = 0;
		if ((res = Fat32SetSize(fd, sizeof(FAT32_DIR) * 2)) != NO_ERROR)	/*设置初始目录大小*/
		{
			fd->data = NULL;
			free(dir, sizeof(FAT32_DIR));
			return res;
		}
		memcpy32(&dat[0], dir, sizeof(FAT32_DIR) / sizeof(DWORD));
		dat[0].name[0] = '.';
		memset8(&dat[0].name[1], ' ', FAT32_FILE_NAME_SIZE - 1);
		memcpy32(&dat[1], par->data, sizeof(FAT32_DIR) / sizeof(DWORD));
		dat[1].name[1] = dat[1].name[0] = '.';
		memset8(&dat[1].name[2], ' ', FAT32_FILE_NAME_SIZE - 2);
		if (par->par == NULL)
			dat[1].idxl = dat[1].idxh = 0;
		memset32(&dat[2], 0, (bpc - sizeof(FAT32_DIR) * 2) / sizeof(DWORD));
		if ((res = Fat32RwFile(fd, TRUE, 0, bpc, dat, NULL)) != NO_ERROR)
		{
			fd->data = NULL;
			free(dir, sizeof(FAT32_DIR));
			return res;
		}
	}
	return NO_ERROR;
}

/*删除文件项*/
long Fat32DelFile(FILE_DESC *fd)
{
	FAT32_DIR *data;
	FILE_DESC *par;
	DWORD curc, seek;
	long res;

	data = (FAT32_DIR*)fd->data;
	if (data->attr & FAT32_FILE_ATTR_DIREC)
	{
		FAT32_DIR dir;
		for (seek = sizeof(FAT32_DIR) * 2, curc = 0;; seek += sizeof(FAT32_DIR))	/*检查目录是否空*/
		{
			if ((res = Fat32RwFile(fd, FALSE, seek, sizeof(FAT32_DIR), &dir, &curc)) != NO_ERROR)
				return res;
			if (dir.name[0] == 0)
				break;
			if (curc == 0)
			{
				seek += sizeof(FAT32_DIR);
				break;
			}
			if (dir.name[0] != (char)0xE5)	/*不空的目录不能删除*/
				return FS_ERR_DIR_NOT_EMPTY;
		}
		data->size = seek;
		if ((res = Fat32SetSize(fd, 0)) != NO_ERROR)	/*清空目录*/
		{
			data->size = 0;
			return res;
		}
	}
	else if (data->size)
		if ((res = Fat32SetSize(fd, 0)) != NO_ERROR)	/*清除文件占用的空间*/
			return res;
	par = fd->par;
	seek = fd->idx * sizeof(FAT32_DIR);
	curc = 0;
	data->name[0] = (char)0xE5;
	if ((res = Fat32RwFile(par, TRUE, seek, sizeof(FAT32_DIR), data, &curc)) != NO_ERROR)	/*标记为删除并检查是不是最后一项*/
		return res;
	if (curc && (res = Fat32RwFile(par, FALSE, seek + sizeof(FAT32_DIR), sizeof(FAT32_DIR), data, NULL)) != NO_ERROR)
		return res;
	if (curc == 0 || data->name[0] == 0)	/*已是最后一项,清除目录占用的空间*/
	{
		DWORD tmpseek = seek + sizeof(FAT32_DIR);
		while (seek)
		{
			if ((res = Fat32RwFile(par, FALSE, seek - sizeof(FAT32_DIR), sizeof(FAT32_DIR), data, NULL)) != NO_ERROR)
				return res;
			if (data->name[0] != (char)0xE5)
				break;
			seek -= sizeof(FAT32_DIR);
		}
		((FAT32_DIR*)par->data)->size = tmpseek;
		if ((res = Fat32SetSize(par, seek)) != NO_ERROR)
		{
			((FAT32_DIR*)par->data)->size = 0;
			return res;
		}
	}
	free(data, sizeof(FAT32_DIR));
	fd->data = NULL;
	return NO_ERROR;
}

/*设置文件项信息*/
long Fat32SetFile(FILE_DESC *fd, FILE_INFO *fi)
{
	if (fd->par)
	{
		if (fi->name[0])
		{
			long res;
			if ((res = CheckName(fi->name)) != NO_ERROR)
				return res;
			strcpy(fd->file.name, fi->name);
		}
		if (fi->CreateTime != INVALID)
			fd->file.CreateTime = fi->CreateTime;
		if (fi->ModifyTime != INVALID)
			fd->file.ModifyTime = fi->ModifyTime;
		if (fi->AccessTime != INVALID)
			fd->file.AccessTime = fi->AccessTime;
		if (fi->attr != INVALID)
		{
			if ((fi->attr & FAT32_FILE_ATTR_LONG) == FAT32_FILE_ATTR_LONG)
				return FS_ERR_WRONG_ARGS;
			fd->file.attr = fi->attr & 0x3F;
		}
		InfoToData(fd->data, &fd->file);
		if (((FAT32_DIR*)fd->data)->attr & FAT32_FILE_ATTR_DIREC)
			((FAT32_DIR*)fd->data)->size = 0;
		return Fat32RwFile(fd->par, TRUE, fd->idx * sizeof(FAT32_DIR), sizeof(FAT32_DIR), fd->data, NULL);
	}
	return NO_ERROR;
}

/*获取目录列表*/
long Fat32ReadDir(FILE_DESC *fd, QWORD *seek, FILE_INFO *fi, DWORD *curc)
{
	long res;

	for (;;)
	{
		FAT32_DIR dir;
		if ((res = Fat32RwFile(fd, FALSE, *seek, sizeof(FAT32_DIR), &dir, curc)) != NO_ERROR)
			return res;
		*seek += sizeof(FAT32_DIR);
		if (dir.name[0] == 0)
			break;
		if (dir.name[0] != (char)0xE5 && !(dir.attr & FAT32_FILE_ATTR_LABEL))	/*有效的文件名,除去长文件名和卷标*/
		{
			DataToInfo(fi, &dir);
			return NO_ERROR;
		}
		if (*curc == 0)
			break;
	}
	*seek = 0;
	return FS_ERR_END_OF_FILE;
}

/*释放文件描述符中的私有数据*/
void Fat32FreeData(FILE_DESC *fd)
{
	free(fd->data, sizeof(FAT32_DIR));
	fd->data = NULL;
}
