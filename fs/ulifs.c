/*	ulifs.c for ulios file system
	作者：孙亮
	功能：ULIFS文件系统实现
	最后修改日期：2009-06-06
*/

#include "fs.h"

/*ulios分区结构:
|引导|保留|数据|剩余
|记录|扇区|簇区|扇区
*/
typedef struct _ULIFS_BPB
{
	DWORD	INS_JMP;/*跳转指令*/
	BYTE	OEM[8];	/*OEM ID(tian&uli2k_X)*/
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
	char	BootPath[88];/*启动列表文件路径*/
	BYTE	code[382];	/*引导代码*/
	WORD	aa55;	/*引导标志*/
}ULIFS_BPB;	/*ulifs BIOS Parameter Block*/

typedef struct _ULIFS
{
	DWORD	spc;	/*每簇扇区数*/
	DWORD	res;	/*保留扇区数(位图之前的扇区)*/
	DWORD	ubm;	/*使用位图开始扇区*/
	DWORD	spbm;	/*每个位图占扇区数*/
	DWORD	CluID;	/*首簇扇区号*/
	DWORD	CuCou;	/*簇数量*/
	DWORD	FstCu;	/*第一个空簇号*/
	DWORD	RemCu;	/*剩余簇数量*/
	DWORD	ubml;	/*使用位图锁*/
}ULIFS;	/*运行时ulifs信息结构*/

/*ulifs文件系统的核心结构:块索引节点(非常简单^_^)
fst是被指向的连续簇块的首簇号,从0开始计数,
cou是被索引的相连数据簇的个数,但它的值可以是0,
是0表示这个节点指向下一个索引节点簇,而不是数据簇*/
typedef struct _BLKID
{
	DWORD	fst;	/*首簇*/
	DWORD	cou;	/*数量*/
}BLKID;	/*块索引节点*/

/*ulifs文件系统目录项结构:
注:目录中没有.和..目录项
根目录0号目录项为根目录的目录项,名称为分区卷标,以'/'开头,但不包括'/'*/
#define ULIFS_FILE_NAME_SIZE	80
#define ULIFS_FILE_ATTR_RDONLY	0x00000001	/*只读*/
#define ULIFS_FILE_ATTR_HIDDEN	0x00000002	/*隐藏*/
#define ULIFS_FILE_ATTR_SYSTEM	0x00000004	/*系统*/
#define ULIFS_FILE_ATTR_LABEL	0x00000008	/*卷标(只读)*/
#define ULIFS_FILE_ATTR_DIREC	0x00000010	/*目录(只读)*/
#define ULIFS_FILE_ATTR_ARCH	0x00000020	/*归档*/
#define ULIFS_FILE_ATTR_EXEC	0x00000040	/*可执行*/
#define ULIFS_FILE_ATTR_DIRTY	0x40000000	/*数据不一致*/
#define ULIFS_FILE_ATTR_UNMDFY	0x80000000	/*属性不可修改*/
#define ULIFS_FILE_IDX_LEN		3

typedef struct _ULIFS_DIR	/*目录项结构*/
{
	char name[ULIFS_FILE_NAME_SIZE];/*文件名*/
	DWORD CreateTime;	/*创建时间1970-01-01经过的秒数*/
	DWORD ModifyTime;	/*修改时间*/
	DWORD AccessTime;	/*访问时间*/
	DWORD attr;			/*属性*/
	QWORD size;			/*文件字节数,目录文件的字节数有效*/
	BLKID idx[ULIFS_FILE_IDX_LEN];	/*自带索引块*/
}ULIFS_DIR;	/*目录项结构*/

#define ULIFS_ID		0x7C		/*ULIFS文件系统ID*/
#define ULIFS_FLAG		0x4E544C55	/*ULTN文件系统标识*/
#define ULIFS_BPS		0x200		/*允许的每扇区字节数*/
#define ULIFS_MAX_SPC	0x80		/*每簇扇区数极限*/
#define ULIFS_MAX_DIR	0x10000		/*目录中目录项数量极限*/

static inline long RwClu(PART_DESC *part, BOOL isWrite, DWORD clu, DWORD cou, void *buf)	/*读写分区簇*/
{
	ULIFS *data = (ULIFS*)part->data;
	return RwPart(part, isWrite, data->CluID + data->spc * clu, data->spc * cou, buf);
}

/*挂载ULIFS分区*/
long UlifsMntPart(PART_DESC *pd)
{
	ULIFS *fs;
	DWORD i;
	long res;
	BYTE buf[ULIFS_BPS];

	if (pd->FsID != 0x7C)
		return FS_ERR_WRONG_ARGS;
	if ((res = RwPart(pd, FALSE, 0, 1, buf)) != NO_ERROR)	/*读引导扇区*/
		return res;
	if (((ULIFS_BPB*)buf)->aa55 != 0xAA55 || ((ULIFS_BPB*)buf)->fsid != ULIFS_FLAG || ((ULIFS_BPB*)buf)->bps != ULIFS_BPS)	/*检查引导标志和分区标志,暂不支持不是512字节的扇区*/
		return FS_ERR_WRONG_ARGS;
	if ((fs = (ULIFS*)malloc(sizeof(ULIFS))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	fs->spc = ((ULIFS_BPB*)buf)->spc;
	fs->res = ((ULIFS_BPB*)buf)->res;
	fs->ubm = fs->res + ((ULIFS_BPB*)buf)->spbm;
	fs->spbm = ((ULIFS_BPB*)buf)->spbm;
	fs->CluID = ((ULIFS_BPB*)buf)->cluoff;
	fs->CuCou = ((ULIFS_BPB*)buf)->clucou;
	fs->RemCu = fs->FstCu = 0;
	fs->ubml = FALSE;
	for (i = 0;; i += sizeof(DWORD) * 8)	/*查找空簇*/
	{
		DWORD bit;

		if ((i & 0xFFF) == 0)
		{
			res = RwPart(pd, FALSE, fs->ubm + (i >> 12), 1, buf);	/*读取使用位图扇区*/
			if (res != NO_ERROR)
			{
				free(fs, sizeof(ULIFS));
				return res;
			}
		}
		if ((bit = ((DWORD*)buf)[(i & 0xFFF) >> 5]) != 0xFFFFFFFF)	/*有空簇*/
		{
			DWORD j;

			for (j = 0; j < sizeof(DWORD) * 8; j++)
				if (!(bit & (1ul << j)))	/*空簇*/
				{
					if (i + j >= fs->CuCou)
					{
						RwPart(pd, FALSE, fs->CluID, 1, buf);	/*取得卷标*/
						strncpy(pd->part.label, (const char*)&buf[1], LABEL_SIZE - 1);
						pd->part.label[LABEL_SIZE - 1] = 0;
						pd->part.size = (QWORD)fs->CuCou * fs->spc * ULIFS_BPS;
						pd->part.remain = (QWORD)fs->RemCu * fs->spc * ULIFS_BPS;
						pd->part.attr = 0;
						pd->data = fs;
						return NO_ERROR;
					}
					if (fs->FstCu == 0)
						fs->FstCu = i + j;	/*取得首空簇号*/
					fs->RemCu++;	/*累计空簇数*/
				}
		}
	}
}

/*卸载ULIFS分区*/
void UlifsUmntPart(PART_DESC *pd)
{
	free(pd->data, sizeof(ULIFS));
	pd->data = NULL;
	pd->FsID = ULIFS_ID;
}

/*设置分区信息*/
long UlifsSetPart(PART_DESC *pd, PART_INFO *pi)
{
	long res;
	ULIFS *data;
	ULIFS_DIR dir[ULIFS_BPS / sizeof(ULIFS_DIR)];

	data = (ULIFS*)pd->data;
	if ((res = RwPart(pd, FALSE, data->CluID, 1, dir)) != NO_ERROR)	/*读取数据簇中的第一个扇区*/
		return res;
	memcpy32(pd->part.label, pi->label, LABEL_SIZE / sizeof(DWORD));
	strncpy(&dir->name[1], pi->label, ULIFS_FILE_NAME_SIZE - 1);
	if ((res = RwPart(pd, TRUE, data->CluID, 1, dir)) != NO_ERROR)
		return res;
	return NO_ERROR;
}

/*读写文件,数据不允许超出文件尾*/
long UlifsRwFile(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, void *buf, DWORD *curc)
{
	QWORD dati, end;	/*当前字节位置,字节结尾位置*/
	PART_DESC *CurPart;	/*所在分区结构指针*/
	DWORD bpc;	/*每簇字节数*/
	BLKID *idxp;	/*索引节点索引*/
	long res;
	BLKID blk, idx[ULIFS_BPS * ULIFS_MAX_SPC / sizeof(BLKID)];	/*索引簇*/
	BYTE dat[ULIFS_BPS * ULIFS_MAX_SPC];	/*数据簇*/

	dati = 0;
	end = seek + siz;
	CurPart = fd->part;
	bpc = ((ULIFS*)(CurPart->data))->spc * ULIFS_BPS;
	if ((DWORD)seek % bpc + siz <= bpc && curc && *curc)	/*读取的数据在一个簇中*/
	{
		if ((res = RwClu(CurPart, FALSE, *curc, 1, dat)) != NO_ERROR)
			return res;
		if (isWrite)
		{
			memcpy8(dat + (DWORD)seek % bpc, buf, siz);
			if ((res = RwClu(CurPart, TRUE, *curc, 1, dat)) != NO_ERROR)
				return res;
		}
		else
			memcpy8(buf, dat + (DWORD)seek % bpc, siz);
		if ((DWORD)end % bpc == 0)
			*curc = 0;
		return NO_ERROR;
	}
	idxp = &idx[(bpc / sizeof(BLKID)) - ULIFS_FILE_IDX_LEN];
	memcpy32(idxp, ((ULIFS_DIR*)fd->data)->idx, sizeof(BLKID) * ULIFS_FILE_IDX_LEN / sizeof(DWORD));	/*复制目录项中的索引*/
	for (;;)	/*索引簇内循环*/
	{
		DWORD tmpcou;

		blk = *idxp;
		if (blk.cou == 0)
		{
			if ((res = RwClu(CurPart, FALSE, blk.fst, 1, idx)) != NO_ERROR)	/*读取下一个索引簇*/
				return res;
			idxp = idx;
			continue;
		}
		if (dati + blk.cou * bpc <= seek)	/*未开始*/
		{
			dati += blk.cou * bpc;	/*处理下一块*/
			idxp++;
			continue;
		}
		if (dati < seek)	/*当前块将开始*/
		{
			tmpcou = (DWORD)((seek - dati) / ULIFS_BPS) / (bpc / ULIFS_BPS);	/*tmpcou = (seek - dati) / bpc;*/
			blk.fst += tmpcou;
			blk.cou -= tmpcou;
			tmpcou *= bpc;
			dati += tmpcou;	/*调整数据指针*/
		}
		if (dati < seek)	/*当前簇将开始*/
		{
			if (dati + bpc >= end)	/*开始即完成*/
				tmpcou = siz;
			else
				tmpcou = bpc - (DWORD)seek % bpc;
			if ((res = RwClu(CurPart, FALSE, blk.fst, 1, dat)) != NO_ERROR)
				return res;
			if (isWrite)
			{
				memcpy8(dat + (DWORD)seek % bpc, buf, tmpcou);
				if ((res = RwClu(CurPart, TRUE, blk.fst, 1, dat)) != NO_ERROR)
					return res;
			}
			else
				memcpy8(buf, dat + (DWORD)seek % bpc, tmpcou);
			if (dati + bpc >= end)	/*开始即完成*/
			{
				if (curc)
					*curc = (DWORD)end % bpc ? blk.fst : 0;
				return NO_ERROR;
			}
			else
			{
				blk.fst++;
				blk.cou--;
				dati += bpc;
				buf += tmpcou;
			}
		}
		if (dati + bpc <= end)	/*有整簇可处理*/
		{
			tmpcou = (DWORD)((end - dati) / ULIFS_BPS) / (bpc / ULIFS_BPS);	/*tmpcou = (end - dati) / bpc;*/
			if (tmpcou > blk.cou)
				tmpcou = blk.cou;
			if ((res = RwClu(CurPart, isWrite, blk.fst, tmpcou, buf)) != NO_ERROR)
				return res;
			blk.fst += tmpcou;
			blk.cou -= tmpcou;
			tmpcou *= bpc;
			dati += tmpcou;
			buf += tmpcou;
			if (dati >= end)	/*刚好完成*/
			{
				if (curc)
					*curc = 0;
				return NO_ERROR;
			}
			if (blk.cou == 0)
			{
				idxp++;
				continue;
			}
		}
		if (dati + bpc >= end)	/*将要完成*/
		{
			if ((res = RwClu(CurPart, FALSE, blk.fst, 1, dat)) != NO_ERROR)
				return res;
			if (isWrite)
			{
				memcpy8(dat, buf, (DWORD)end % bpc);
				if ((res = RwClu(CurPart, TRUE, blk.fst, 1, dat)) != NO_ERROR)
					return res;
			}
			else
				memcpy8(buf, dat, (DWORD)end % bpc);
			if (curc)
				*curc = blk.fst;
			return NO_ERROR;
		}
	}
}

/*分配簇*/
static long AllocClu(PART_DESC *pd, BLKID *blk)
{
	ULIFS *data;
	DWORD fst, end;	/*循环,结束位置*/
	DWORD bm[ULIFS_BPS / sizeof(DWORD)];	/*位图*/

	data = (ULIFS*)pd->data;
	lock(&data->ubml);
	blk->fst = fst = data->FstCu;
	if (blk->cou)
		end = fst + blk->cou;
	else
		end = fst + 1;
	RwPart(pd, FALSE, data->ubm + (fst >> 12), 1, bm);
	for (;;)
	{
		if ((fst & 0x1F) == 0 && end - fst >= sizeof(DWORD) * 8 && bm[(fst & 0xFFF) >> 5] == 0)	/*分配一个双字描述的簇*/
		{
			bm[(fst & 0xFFF) >> 5] = 0xFFFFFFFF;
			fst += sizeof(DWORD) * 8;
		}
		else	/*分配一位描述的簇*/
		{
			bm[(fst & 0xFFF) >> 5] |= (1ul << (fst & 0x1F));
			fst++;
		}
		if (fst >= end || (bm[(fst & 0xFFF) >> 5] & (1ul << (fst & 0x1F))))
			break;	/*完成或遇到非空簇*/
		if ((fst & 0xFFF) == 0)	/*切换当前索引簇*/
		{
			RwPart(pd, TRUE, data->ubm + ((fst - 1) >> 12), 1, bm);
			RwPart(pd, FALSE, data->ubm + (fst >> 12), 1, bm);
		}
	}
	RwPart(pd, TRUE, data->ubm + ((fst - 1) >> 12), 1, bm);
	if (blk->cou)
	{
		blk->cou = fst - blk->fst;
		data->RemCu -= blk->cou;
	}
	else
		data->RemCu--;
	for (;; fst += 32)	/*查找空簇*/
	{
		DWORD bit;

		if ((fst & 0xFFF) == 0)
			RwPart(pd, FALSE, data->ubm + (fst >> 12), 1, bm);	/*读取新扇区*/
		if ((bit = bm[(fst & 0xFFF) >> 5]) != 0xFFFFFFFF)	/*有空簇*/
		{
			DWORD j;

			for (j = 0;; j++)
				if (!(bit & (1ul << j)))	/*空簇*/
				{
					data->FstCu = (fst & 0xFFFFFFE0) + j;	/*取得首空簇号*/
					ulock(&data->ubml);
					return NO_ERROR;
				}
		}
	}
}

/*回收簇*/
static long FreeClu(PART_DESC *pd, BLKID *blk)
{
	ULIFS *data;
	DWORD fst, end;	/*循环,结束位置,位图*/
	DWORD bm[ULIFS_BPS / sizeof(DWORD)];	/*位图*/

	data = (ULIFS*)pd->data;
	lock(&data->ubml);
	fst = blk->fst;
	if (blk->cou)
		end = fst + blk->cou;
	else
		end = fst + 1;
	RwPart(pd, FALSE, data->ubm + (fst >> 12), 1, bm);
	for (;;)
	{
		if ((fst & 0x1F) == 0 && end - fst >= sizeof(DWORD) * 8)	/*释放一个双字描述的簇*/
		{
			bm[(fst & 0xFFF) >> 5] = 0;
			fst += sizeof(DWORD) * 8;
		}
		else	/*释放一位描述的簇*/
		{
			bm[(fst & 0xFFF) >> 5] &= (~(1ul << (fst & 0x1F)));
			fst++;
		}
		if (fst >= end)
			break;	/*完成*/
		if ((fst & 0xFFF) == 0)	/*切换当前索引簇*/
		{
			RwPart(pd, TRUE, data->ubm + ((fst - 1) >> 12), 1, bm);
			RwPart(pd, FALSE, data->ubm + (fst >> 12), 1, bm);
		}
	}
	RwPart(pd, TRUE, data->ubm + ((fst - 1) >> 12), 1, bm);
	if (blk->cou)
		data->RemCu += blk->cou;
	else
		data->RemCu++;
	if (blk->fst < data->FstCu)
		data->FstCu = blk->fst;
	ulock(&data->ubml);
	return NO_ERROR;
}

/*设置文件长度*/
long UlifsSetSize(FILE_DESC *fd, QWORD siz)
{
	ULIFS_DIR *data;
	FILE_DESC *par;
	DWORD cun0, cun, clui;/*原簇数量,修改后簇数量,数据簇索引*/
	PART_DESC *CurPart;	/*所在分区结构指针*/
	DWORD bpc;	/*每簇字节数*/
	BLKID *idxp;	/*索引节点索引*/
	long res;
	BLKID idx[ULIFS_BPS * ULIFS_MAX_SPC / sizeof(BLKID)];	/*索引簇*/

	data = (ULIFS_DIR*)fd->data;
	par = fd->par ? fd->par : fd;
	CurPart = fd->part;
	bpc = ((ULIFS*)CurPart->data)->spc * ULIFS_BPS;
	cun0 = (DWORD)((data->size + bpc - 1) / ULIFS_BPS) / (bpc / ULIFS_BPS);	/*cun0 = (data->size + bpc - 1) / bpc;*/
	cun = (DWORD)((siz + bpc - 1) / ULIFS_BPS) / (bpc / ULIFS_BPS);	/*cun = (siz + bpc - 1) / bpc;*/
	if (cun == cun0)	/*不需要修改簇*/
	{
		fd->file.size = data->size = siz;
		return UlifsRwFile(par, TRUE, (QWORD)fd->idx * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), data, NULL);
	}
	if (cun > cun0 && cun - cun0 > ((ULIFS*)CurPart->data)->RemCu)
		return FS_ERR_HAVENO_SPACE;	/*超出剩余簇数量*/
	idxp = &idx[(bpc / sizeof(BLKID)) - ULIFS_FILE_IDX_LEN];
	memcpy32(idxp, data->idx, sizeof(BLKID) * ULIFS_FILE_IDX_LEN / sizeof(DWORD));	/*复制目录项中的索引*/
	clui = 0;
	if (cun < cun0)	/*减小文件*/
	{
		DWORD PreIdx, CurIdx;	/*前索引簇,当前索引簇位置*/
		BLKID blk;	/*块计数*/
		CurIdx = PreIdx = 0;
		for (;;)	/*索引簇内循环,搜索释放位置*/
		{
			blk = *idxp;
			if (blk.cou == 0)
			{
				if ((res = RwClu(CurPart, FALSE, blk.fst, 1, idx)) != NO_ERROR)	/*读取下一个索引簇*/
					return res;
				PreIdx = CurIdx;
				CurIdx = blk.fst;
				idxp = idx;
				continue;
			}
			clui += blk.cou;
			if (clui > cun)	/*开始释放簇*/
				break;
			idxp++;
		}
		blk.cou = clui - cun;
		blk.fst += idxp->cou - blk.cou;
		if ((res = FreeClu(CurPart, &blk)) != NO_ERROR)	/*释放当前索引标记的簇*/
			return res;
		idxp->cou -= blk.cou;
		blk.fst = CurIdx;
		blk.cou = 1;
		if (CurIdx == 0)	/*没有当前索引簇,修改目录项*/
			memcpy32(data->idx, &idx[(bpc / sizeof(BLKID)) - ULIFS_FILE_IDX_LEN], sizeof(BLKID) * ULIFS_FILE_IDX_LEN / sizeof(DWORD));
		else if (idx[0].cou == 0)	/*当前索引簇全空,删除当前索引簇*/
		{
			if ((res = FreeClu(CurPart, &blk)) != NO_ERROR)
				return res;
		}
		else if (idx[1].cou == 0)	/*当前索引簇只剩一个节点,节点写入前索引项*/
		{
			if ((res = FreeClu(CurPart, &blk)) != NO_ERROR)
				return res;
			if (PreIdx == 0)	/*没有前索引簇*/
				data->idx[ULIFS_FILE_IDX_LEN - 1] = idx[0];
			else
			{
				BLKID tmpidx[ULIFS_BPS * ULIFS_MAX_SPC / sizeof(BLKID)];	/*临时索引簇*/
				if ((res = RwClu(CurPart, FALSE, PreIdx, 1, tmpidx)) != NO_ERROR)
					return res;
				tmpidx[bpc / sizeof(BLKID) - 1] = idx[0];
				if ((res = RwClu(CurPart, TRUE, PreIdx, 1, tmpidx)) != NO_ERROR)
					return res;
			}
		}
		else	/*写入当前索引*/
		{
			if ((res = RwClu(CurPart, TRUE, CurIdx, 1, idx)) != NO_ERROR)
				return res;
		}
		if (clui >= cun0)
		{
			fd->file.size = data->size = siz;
			CurPart->part.remain += (cun0 - cun) * bpc;
			return UlifsRwFile(par, TRUE, (QWORD)fd->idx * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), data, NULL);
		}
		idxp++;
		for (;;)	/*索引簇内循环,继续释放*/
		{
			blk = *idxp;
			if (blk.cou == 0)
			{
				if ((res = RwClu(CurPart, FALSE, blk.fst, 1, idx)) != NO_ERROR)	/*读取下一个索引簇*/
					return res;
				idxp = idx;
				if ((res = FreeClu(CurPart, &blk)) != NO_ERROR)
					return res;
				continue;
			}
			if ((res = FreeClu(CurPart, &blk)) != NO_ERROR)
				return res;
			clui += blk.cou;
			if (clui >= cun0)	/*释放完成*/
			{
				fd->file.size = data->size = siz;
				CurPart->part.remain += (cun0 - cun) * bpc;
				return UlifsRwFile(par, TRUE, (QWORD)fd->idx * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), data, NULL);
			}
			idxp++;
		}
	}
	else	/*增大文件*/
	{
		DWORD CurIdx;	/*前索引簇,当前索引簇位置*/
		BLKID blk;	/*当前块计数*/
		CurIdx = 0;
		blk.cou = blk.fst = 0;
		if (cun0)
		{
			for (;;)	/*索引簇内循环,搜索结束位置*/
			{
				blk = *idxp;
				if (blk.cou == 0)
				{
					if ((res = RwClu(CurPart, FALSE, blk.fst, 1, idx)) != NO_ERROR)	/*读取下一个索引簇*/
						return res;
					CurIdx = blk.fst;
					idxp = idx;
					continue;
				}
				clui += blk.cou;
				if (clui >= cun0)	/*到文件尾*/
					break;
				idxp++;
			}
		}
		for (;;)
		{
			BLKID tblk;	/*临时块计数*/
			tblk.fst = blk.fst + blk.cou;
			tblk.cou = cun - clui;
			if ((res = AllocClu(CurPart, &tblk)) != NO_ERROR)
				return res;
			if (clui == 0)	/*文件起始,设置当前簇节点*/
			{
				blk = tblk;
				*idxp = tblk;
			}
			else if (tblk.fst == blk.fst + blk.cou)	/*在当前簇块后面,加入当前簇节点*/
			{
				blk.cou += tblk.cou;
				idxp->cou += tblk.cou;
			}
			else if (idxp < &idx[bpc / sizeof(BLKID) - 1])	/*加入下一个簇节点*/
			{
				blk = tblk;
				*(++idxp) = tblk;
			}
			else	/*加入下一个索引簇*/
			{
				BLKID iblk;	/*申请索引簇BLKID*/
				iblk.cou = 0;
				if ((res = AllocClu(CurPart, &iblk)) != NO_ERROR)
					return res;
				*idxp = iblk;
				if (CurIdx == 0)
					memcpy32(data->idx, &idx[(bpc / sizeof(BLKID)) - ULIFS_FILE_IDX_LEN], sizeof(BLKID) * ULIFS_FILE_IDX_LEN / sizeof(DWORD));
				else
				{
					if ((res = RwClu(CurPart, TRUE, CurIdx, 1, idx)) != NO_ERROR)
						return res;
				}
				idx[0] = blk;
				idx[1] = tblk;
				blk = tblk;
				idxp = &idx[1];
				CurIdx = iblk.fst;
			}
			if ((clui += tblk.cou) >= cun)	/*申请完成*/
			{
				if (CurIdx == 0)
					memcpy32(data->idx, &idx[(bpc / sizeof(BLKID)) - ULIFS_FILE_IDX_LEN], sizeof(BLKID) * ULIFS_FILE_IDX_LEN / sizeof(DWORD));
				else
				{
					if ((res = RwClu(CurPart, TRUE, CurIdx, 1, idx)) != NO_ERROR)
						return res;
				}
				fd->file.size = data->size = siz;
				CurPart->part.remain -= (cun - cun0) * bpc;
				return UlifsRwFile(par, TRUE, (QWORD)fd->idx * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), data, NULL);
			}
		}
	}
}

/*比较路径字符串与文件名是否匹配*/
BOOL UlifsCmpFile(FILE_DESC *fd, const char *path)
{
	char *namep = ((ULIFS_DIR*)fd->data)->name;
	while (*namep)
		if (*namep++ != *path++)
			return FALSE;	/*文件名不匹配*/
	if (*path != '/' && *path)
		return FALSE;
	return TRUE;
}

/*data信息拷贝到info*/
static void DataToInfo(FILE_INFO *fi, ULIFS_DIR *ud)
{
	strncpy(fi->name, ud->name, FILE_NAME_SIZE);
	fi->CreateTime = ud->CreateTime;
	fi->ModifyTime = ud->ModifyTime;
	fi->AccessTime = ud->AccessTime;
	fi->attr = ud->attr;
	fi->size = ud->size;
}

/*info信息拷贝到data*/
static void InfoToData(ULIFS_DIR *ud, FILE_INFO *fi)
{
	strncpy(ud->name, fi->name, ULIFS_FILE_NAME_SIZE - 1);
	ud->name[ULIFS_FILE_NAME_SIZE - 1] = 0;
	ud->CreateTime = fi->CreateTime;
	ud->ModifyTime = fi->ModifyTime;
	ud->AccessTime = fi->AccessTime;
	ud->attr = fi->attr;
	ud->size = fi->size;
}

/*搜索并设置文件项*/
long UlifsSchFile(FILE_DESC *fd, const char *path)
{
	FILE_DESC *par;
	ULIFS_DIR *dir;
	long res;

	if ((dir = (ULIFS_DIR*)malloc(sizeof(ULIFS_DIR))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	if ((par = fd->par) == NULL)	/*取得根目录项*/
	{
		ULIFS_DIR tmpdir[ULIFS_BPS / sizeof(ULIFS_DIR)];
		if ((res = RwPart(fd->part, FALSE, ((ULIFS*)fd->part->data)->CluID, 1, tmpdir)) != NO_ERROR)	/*读取数据簇中的第一个扇区*/
		{
			free(dir, sizeof(ULIFS_DIR));
			return res;
		}
		*dir = tmpdir[0];
		DataToInfo(&fd->file, dir);
		fd->idx = 0;
		fd->data = dir;
		return NO_ERROR;
	}
	else	/*取得其他目录项*/
	{
		DWORD curc;
		QWORD seek, siz;

		fd->data = dir;
		for (curc = 0, seek = 0, siz = ((ULIFS_DIR*)par->data)->size; seek < siz; seek += sizeof(ULIFS_DIR))
		{
			if ((res = UlifsRwFile(par, FALSE, seek, sizeof(ULIFS_DIR), dir, &curc)) != NO_ERROR)
			{
				fd->data = NULL;
				free(dir, sizeof(ULIFS_DIR));
				return res;
			}
			if (UlifsCmpFile(fd, path))
			{
				DataToInfo(&fd->file, dir);
				fd->idx = seek / sizeof(ULIFS_DIR);
				return NO_ERROR;
			}
		}
		fd->data = NULL;
		free(dir, sizeof(ULIFS_DIR));
		return FS_ERR_PATH_NOT_FOUND;
	}
}

/*检查文件名正确性*/
static long CheckName(const char *name)
{
	const char *namep;
	namep = name;
	while (*namep)	/*文件名超长*/
	{
		if (namep - name >= ULIFS_FILE_NAME_SIZE)
			return FS_ERR_NAME_TOOLONG;
		if (*namep < 0x20)
			return FS_ERR_NAME_FORMAT;
		namep++;
	}
	return NO_ERROR;
}

/*创建并设置文件项*/
long UlifsNewFile(FILE_DESC *fd, const char *path)
{
	FILE_DESC *par;
	ULIFS_DIR *dir;
	DWORD curc;
	QWORD seek, siz;
	long res;

	if ((res = CheckName(path)) != NO_ERROR)	/*检查文件名*/
		return res;
	if ((res = UlifsSchFile(fd, path)) != FS_ERR_PATH_NOT_FOUND)	/*文件项已经存在了*/
	{
		if (res == NO_ERROR)
		{
			free(fd->data, sizeof(ULIFS_DIR));	/*释放SchFile申请的数据*/
			fd->data = NULL;
			return FS_ERR_PATH_EXISTS;
		}
		return res;
	}
	if ((dir = (ULIFS_DIR*)malloc(sizeof(ULIFS_DIR))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	/*搜索空位*/
	par = fd->par;
	for (curc = 0, seek = 0, siz = ((ULIFS_DIR*)par->data)->size; seek < siz; seek += sizeof(ULIFS_DIR))
	{
		if ((res = UlifsRwFile(par, FALSE, seek, sizeof(ULIFS_DIR), dir, &curc)) != NO_ERROR)
		{
			free(dir, sizeof(ULIFS_DIR));
			return res;
		}
		if (dir->name[0] == 0)
			goto crtdir;
	}
	if (seek >= sizeof(ULIFS_DIR) * ULIFS_MAX_DIR)
	{
		free(dir, sizeof(ULIFS_DIR));
		return FS_ERR_SIZE_LIMIT;
	}
	if ((res = UlifsSetSize(par, seek + sizeof(ULIFS_DIR))) != NO_ERROR)
	{
		free(dir, sizeof(ULIFS_DIR));
		return res;
	}
crtdir:	/*创建*/
	strcpy(fd->file.name, path);
	fd->file.size = 0;
	InfoToData(dir, &fd->file);
	if ((res = UlifsRwFile(par, TRUE, seek, sizeof(ULIFS_DIR), dir, &curc)) != NO_ERROR)
	{
		free(dir, sizeof(ULIFS_DIR));
		return res;
	}
	fd->idx = seek / sizeof(ULIFS_DIR);
	fd->data = dir;
	return NO_ERROR;
}

/*删除文件项*/
long UlifsDelFile(FILE_DESC *fd)
{
	ULIFS_DIR *data;
	FILE_DESC *par;
	QWORD seek, siz;
	long res;

	data = (ULIFS_DIR*)fd->data;
	if (data->size)
	{
		if (data->attr & ULIFS_FILE_ATTR_DIREC)	/*不空的目录不能删除*/
			return FS_ERR_DIR_NOT_EMPTY;
		else if ((res = UlifsSetSize(fd, 0)) != NO_ERROR)	/*清除文件占用的空间*/
			return res;
	}
	par = fd->par;
	seek = (QWORD)fd->idx * sizeof(ULIFS_DIR);
	siz = ((ULIFS_DIR*)par->data)->size;
	if (seek + sizeof(ULIFS_DIR) >= siz)	/*已是最后一项*/
	{
		while (seek)
		{
			if ((res = UlifsRwFile(par, FALSE, seek - sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), data, NULL)) != NO_ERROR)
				return res;
			if (data->name[0])
				break;
			seek -= sizeof(ULIFS_DIR);
		}
		if ((res = UlifsSetSize(par, seek)) != NO_ERROR)
			return res;
	}
	else	/*直接标记为删除*/
	{
		data->name[0] = 0;
		if ((res = UlifsRwFile(par, TRUE, seek, sizeof(ULIFS_DIR), data, NULL)) != NO_ERROR)
			return res;
	}
	free(data, sizeof(ULIFS_DIR));
	fd->data = NULL;
	return NO_ERROR;
}

/*设置文件项信息*/
long UlifsSetFile(FILE_DESC *fd, FILE_INFO *fi)
{
	if (fd->par && fi->name[0])
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
		fd->file.attr = fi->attr;
	InfoToData(fd->data, &fd->file);
	return UlifsRwFile(fd->par ? fd->par : fd, TRUE, (QWORD)fd->idx * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), fd->data, NULL);
}

/*获取目录列表*/
long UlifsReadDir(FILE_DESC *fd, QWORD *seek, FILE_INFO *fi, DWORD *curc)
{
	QWORD siz;
	long res;

	for (siz = ((ULIFS_DIR*)fd->data)->size; *seek < siz;)
	{
		ULIFS_DIR dir;
		if ((res = UlifsRwFile(fd, FALSE, *seek, sizeof(ULIFS_DIR), &dir, curc)) != NO_ERROR)
			return res;
		*seek += sizeof(ULIFS_DIR);
		if (dir.name[0] && dir.name[0] != '/')	/*有效的文件名,除去根目录*/
		{
			DataToInfo(fi, &dir);
			return NO_ERROR;
		}
	}
	*seek = 0;
	return FS_ERR_END_OF_FILE;
}

/*释放文件描述符中的私有数据*/
void UlifsFreeData(FILE_DESC *fd)
{
	free(fd->data, sizeof(ULIFS_DIR));
	fd->data = NULL;
}
