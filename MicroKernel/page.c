/*	page.c for ulios
	作者：孙亮
	功能：分页内存管理
	最后修改日期：2009-05-29
*/

#include "knldef.h"

/*分配地址映射结构*/
MAPBLK_DESC *AllocMap()
{
	MAPBLK_DESC *map;

	cli();
	if (FstMap == NULL)
	{
		sti();
		return NULL;
	}
	FstMap = (map = FstMap)->nxt;
	sti();
	return map;
}

/*释放地址映射结构*/
void FreeMap(MAPBLK_DESC *map)
{
	cli();
	map->nxt = FstMap;
	FstMap = map;
	sti();
}

/*映射进程取得地址映射结构,并锁定目标进程*/
static MAPBLK_DESC *GetMap(PROCESS_DESC *proc, void *addr)
{
	PROCESS_DESC *DstProc;
	MAPBLK_DESC *PreMap, *CurMap, *map;

	for (PreMap = NULL, CurMap = proc->map; CurMap; CurMap = (PreMap = CurMap)->nxt)
		if (CurMap->addr == addr)
		{
			map = CurMap;
			if (PreMap)	/*从映射进程的映射结构链表中删除*/
				PreMap->nxt = map->nxt;
			else
				proc->map = map->nxt;
			DstProc = pmt[map->ptid2.ProcID];
			for (PreMap = NULL, CurMap = DstProc->map2; CurMap; CurMap = (PreMap = CurMap)->nxt2)
				if (CurMap == map)
					break;
			if (PreMap)	/*从被映射进程的映射结构链表中删除*/
				PreMap->nxt2 = map->nxt2;
			else
				DstProc->map2 = map->nxt2;
			return map;
		}
	return NULL;
}

/*被映射进程取得地址映射结构,并锁定目标进程*/
static MAPBLK_DESC *GetMap2(PROCESS_DESC *proc, void *addr)
{
	PROCESS_DESC *DstProc;
	MAPBLK_DESC *PreMap, *CurMap, *map;

	for (PreMap = NULL, CurMap = proc->map2; CurMap; CurMap = (PreMap = CurMap)->nxt2)
		if (CurMap->addr2 == addr)
		{
			map = CurMap;
			if (PreMap)	/*从被映射进程的映射结构链表中删除*/
				PreMap->nxt2 = map->nxt2;
			else
				proc->map2 = map->nxt2;
			DstProc = pmt[map->ptid.ProcID];
			for (PreMap = NULL, CurMap = DstProc->map; CurMap; CurMap = (PreMap = CurMap)->nxt)
				if (CurMap == map)
					break;
			if (PreMap)	/*从映射进程的映射结构链表中删除*/
				PreMap->nxt = map->nxt;
			else
				DstProc->map = map->nxt;
			return map;
		}
	return NULL;
}

/*检查地址块是否是映射区*/
static MAPBLK_DESC *CheckInMap(MAPBLK_DESC *map, void *fst, void *end)
{
	while (map)
	{
		if ((DWORD)end > ((DWORD)map->addr & 0xFFFFF000) && (DWORD)fst < ((DWORD)map->addr & 0xFFFFF000) + (map->siz & 0xFFFFF000))
			return map;
		map = map->nxt;
	}
	return NULL;
}

/*检查地址块是否已被映射*/
static MAPBLK_DESC *CheckInMap2(MAPBLK_DESC *map2, void *fst, void *end)
{
	while (map2)
	{
		if ((DWORD)end > ((DWORD)map2->addr2 & 0xFFFFF000) && (DWORD)fst < ((DWORD)map2->addr2 & 0xFFFFF000) + (map2->siz & 0xFFFFF000))
			return map2;
		map2 = map2->nxt2;
	}
	return NULL;
}

/*分配物理页*/
DWORD AllocPage()
{
	DWORD pgid;

	if (PmpID >= PmmLen)	/*物理页不足*/
		return 0;
	pgid = PmpID;
	pmmap[pgid >> 5] |= (1ul << (pgid & 0x0000001F));	/*标志为已分配*/
	RemmSiz -= PAGE_SIZE;
	for (PmpID++; PmpID < PmmLen; PmpID = (PmpID + 32) & 0xFFFFFFE0)	/*寻找下一个未用页*/
	{
		DWORD dat;	/*数据缓存*/

		dat = pmmap[PmpID >> 5];
		if (dat != 0xFFFFFFFF)
		{
			for (;; PmpID++)
				if (!(dat & (1ul << (PmpID & 0x0000001F))))
					return (pgid << 12) + UPHYMEM_ADDR;	/*页号换算为物理地址*/
		}
	}
	return (pgid << 12) + UPHYMEM_ADDR;	/*页号换算为物理地址*/
}

/*回收物理页*/
void FreePage(DWORD pgaddr)
{
	DWORD pgid;

	pgid = (pgaddr - UPHYMEM_ADDR) >> 12;	/*物理地址换算为页号*/
	if (pgid >= PmmLen)	/*非法地址*/
		return;
	pmmap[pgid >> 5] &= (~(1ul << (pgid & 0x0000001F)));
	RemmSiz += PAGE_SIZE;
	if (PmpID > pgid)
		PmpID = pgid;
}

/*填充连续的页地址*/
long FillConAddr(PAGE_DESC *FstPg, PAGE_DESC *EndPg, DWORD PhyAddr, DWORD attr)
{
	PAGE_DESC *TstPg;
	BOOL isRefreshTlb;

	for (TstPg = (PAGE_DESC*)(*((DWORD*)&FstPg) & 0xFFFFF000); TstPg < EndPg; TstPg += 0x400)	/*调整地址到页表边界*/
	{
		if (!(pt[(DWORD)TstPg >> 12] & PAGE_ATTR_P))	/*页表不存在*/
		{
			DWORD PtAddr;	/*页表的物理地址*/

			if ((PtAddr = LockAllocPage()) == 0)
				return KERR_OUT_OF_PHYMEM;
			pt[(DWORD)TstPg >> 12] = attr | PtAddr;
			memset32(TstPg, 0, 0x400);	/*清空页表*/
		}
	}
	isRefreshTlb = FALSE;
	PhyAddr |= attr;
	for (; FstPg < EndPg; FstPg++)	/*修改页表,映射地址*/
	{
		DWORD PgAddr;	/*页的物理地址*/

		if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*物理页存在*/
		{
			LockFreePage(PgAddr);	/*释放物理页*/
			isRefreshTlb = TRUE;
		}
		*FstPg = PhyAddr;
		PhyAddr += PAGE_SIZE;
	}
	if (isRefreshTlb)
		RefreshTlb();
	return NO_ERROR;
}

/*填充页内容*/
long FillPage(EXEC_DESC *exec, void *addr, DWORD ErrCode)
{
	PAGE_DESC *CurPt, *CurPt0, *CurPg, *CurPg0;

	if (ErrCode & PAGE_ATTR_W)	/*检查是否要写只读的映射区*/
	{
		MAPBLK_DESC *TmpMap;

		cli();
		TmpMap = CheckInMap(CurPmd->map, (void*)((DWORD)addr & 0xFFFFF000), (void*)(((DWORD)addr + PAGE_SIZE) & 0xFFFFF000));
		sti();
		if (TmpMap && !(TmpMap->siz & PAGE_ATTR_W))
			return KERR_WRITE_RDONLY_ADDR;
	}
	CurPg = &pt[(DWORD)addr >> 12];
	CurPg0 = &pt0[(DWORD)addr >> 12];
	CurPt = &pt[(DWORD)CurPg >> 12];
	CurPt0 = &pt[(DWORD)CurPg0 >> 12];
	if (!(ErrCode & PAGE_ATTR_P) && !(*CurPt & PAGE_ATTR_P))	/*页表不存在*/
	{
		if (*CurPt0 & PAGE_ATTR_P)	/*副本中存在*/
			*CurPt = *CurPt0;	/*映射副本页表*/
		else	/*副本中不存在*/
		{
			DWORD PtAddr;	/*页表的物理地址*/
			void *fst, *end;

			if ((PtAddr = LockAllocPage()) == 0)	/*申请新页表*/
				return KERR_OUT_OF_PHYMEM;
			PtAddr |= PAGE_ATTR_P | PAGE_ATTR_U;	/*设为只读*/
			*CurPt = PtAddr;	/*修改页目录表*/
			memset32((void*)((DWORD)CurPg & 0xFFFFF000), 0, 0x400);	/*清空页表*/
			fst = (void*)((DWORD)addr & 0xFFC00000);	/*计算缺页地址所在页表的覆盖区*/
			end = (void*)((DWORD)fst + PAGE_SIZE * 0x400);
			if ((exec->CodeOff < exec->CodeEnd && exec->CodeOff < end && exec->CodeEnd > fst) ||	/*代码段与缺页表覆盖区有重合*/
				(exec->DataOff < exec->DataEnd && exec->DataOff < end && exec->DataEnd > fst))		/*数据段与缺页表覆盖区有重合*/
				*CurPt0 = PtAddr;
		}
	}
	if ((ErrCode & PAGE_ATTR_W) && !(*CurPt & PAGE_ATTR_W))	/*页表写保护*/
	{
		if (*CurPt0 & PAGE_ATTR_P)	/*副本页表存在*/
		{
			DWORD PtAddr;	/*页表的物理地址*/

			if ((PtAddr = LockAllocPage()) == 0)	/*申请新页表*/
				return KERR_OUT_OF_PHYMEM;
			*CurPt = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr;	/*修改页目录表,开启写权限*/
			RefreshTlb();
			memcpy32((void*)((DWORD)CurPg & 0xFFFFF000), (void*)((DWORD)CurPg0 & 0xFFFFF000), 0x400);	/*页表写时复制*/
		}
		else	/*副本页表不存在*/
		{
			*CurPt |= PAGE_ATTR_W;	/*直接开启写权限*/
			RefreshTlb();
		}
	}
	if (!(ErrCode & PAGE_ATTR_P) && !(*CurPg & PAGE_ATTR_P))	/*页不存在*/
	{
		if ((*CurPt0 & PAGE_ATTR_P) && (*CurPg0 & PAGE_ATTR_P))	/*副本页表和副本页存在*/
			*CurPg = *CurPg0;	/*映射副本页*/
		else	/*副本页表或副本页不存在*/
		{
			DWORD PgAddr;	/*页的物理地址*/
			void *fst, *end;

			if ((PgAddr = LockAllocPage()) == 0)	/*申请新页*/
				return KERR_OUT_OF_PHYMEM;
			PgAddr |= PAGE_ATTR_P | PAGE_ATTR_U;	/*设为只读*/
			*CurPg = PgAddr;	/*修改页表*/
			memset32((void*)((DWORD)addr & 0xFFFFF000), 0, 0x400);	/*清空页*/
			fst = (void*)((DWORD)addr & 0xFFFFF000);	/*计算缺页地址所在页的覆盖区*/
			end = (void*)((DWORD)fst + PAGE_SIZE);
			if (exec->CodeOff < exec->CodeEnd && exec->CodeOff < end && exec->CodeEnd > fst)	/*代码段与缺页覆盖区有重合*/
			{
				void *buf;
				DWORD siz;
				THREAD_ID ptid;
				DWORD data[MSG_DATA_LEN];

				buf = (fst < exec->CodeOff ? exec->CodeOff : fst);
				siz = (end > exec->CodeEnd ? exec->CodeEnd : end) - buf;
				data[MSG_API_ID] = FS_API_READPAGE;	/*向文件系统发出消息读取可执行文件页*/
				data[3] = buf - exec->CodeOff + exec->CodeSeek;
				ptid = kpt[FS_KPORT];
				if ((data[MSG_API_ID] = MapProcAddr(buf, siz, &ptid, TRUE, FALSE, data, FS_OUT_TIME)) != NO_ERROR)
					return data[MSG_API_ID];
				if (data[MSG_RES_ID] != NO_ERROR)
					return data[MSG_RES_ID];
				*CurPg0 = PgAddr;
				*CurPg &= (~PAGE_ATTR_W);	/*写权限已在MapProcAddr中被打开,关闭页写权限*/
			}
			if (exec->DataOff < exec->DataEnd && exec->DataOff < end && exec->DataEnd > fst)	/*数据段与缺页覆盖区有重合*/
			{
				void *buf;
				DWORD siz;
				THREAD_ID ptid;
				DWORD data[MSG_DATA_LEN];

				buf = (fst < exec->DataOff ? exec->DataOff : fst);
				siz = (end > exec->DataEnd ? exec->DataEnd : end) - buf;
				data[MSG_API_ID] = FS_API_READPAGE;	/*向文件系统发出消息读取可执行文件页*/
				data[3] = buf - exec->DataOff + exec->DataSeek;
				ptid = kpt[FS_KPORT];
				if ((data[MSG_API_ID] = MapProcAddr(buf, siz, &ptid, TRUE, FALSE, data, FS_OUT_TIME)) != NO_ERROR)
					return data[MSG_API_ID];
				if (data[MSG_RES_ID] != NO_ERROR)
					return data[MSG_RES_ID];
				*CurPg0 = PgAddr;
				*CurPg &= (~PAGE_ATTR_W);	/*写权限已在MapProcAddr中被打开,关闭页写权限*/
			}
		}
	}
	if ((ErrCode & PAGE_ATTR_W) && !(*CurPg & PAGE_ATTR_W))	/*页写保护*/
	{
		if ((*CurPt0 & PAGE_ATTR_P) && (*CurPg0 & PAGE_ATTR_P))	/*副本页存在*/
		{
			DWORD PgAddr;	/*页的物理地址*/

			if ((PgAddr = LockAllocPage()) == 0)	/*申请新页*/
				return KERR_OUT_OF_PHYMEM;
			*CurPg = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PgAddr;	/*修改页表,开启写权限*/
			RefreshTlb();
			pt[(PT_ID << 10) | PG0_ID] = *CurPt0;	/*设置临时映射*/
			memcpy32((void*)((DWORD)addr & 0xFFFFF000), &pg0[(DWORD)addr & 0x003FF000], 0x400);	/*页写时复制*/
			pt[(PT_ID << 10) | PG0_ID] = 0;	/*解除临时映射*/
		}
		else	/*副本页表或副本页不存在*/
		{
			*CurPg |= PAGE_ATTR_W;	/*直接开启写权限*/
			RefreshTlb();
		}
	}
	return NO_ERROR;
}

/*清除页内容*/
void ClearPage(PAGE_DESC *FstPg, PAGE_DESC *EndPg, BOOL isFree)
{
	BOOL isRefreshTlb;

	isRefreshTlb = FALSE;
	while (FstPg < EndPg)	/*目录项对应的页表对应的每页分别回收*/
	{
		DWORD PtAddr;	/*页表的物理地址*/

		if ((PtAddr = pt[(DWORD)FstPg >> 12]) & PAGE_ATTR_P)	/*页表存在*/
		{
			PAGE_DESC *TstPg;

			TstPg = FstPg;	/*记录开始释放的位置*/
			do	/*删除对应的全部非空页*/
			{
				DWORD PgAddr;	/*页的物理地址*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*物理页存在*/
				{
					if (isFree)
						LockFreePage(PgAddr);	/*释放物理页*/
					*FstPg = 0;
					isRefreshTlb = TRUE;
				}
			}
			while (((DWORD)(++FstPg) & 0xFFF) && FstPg < EndPg);
			while ((DWORD)TstPg & 0xFFF)	/*检查页表是否需要释放*/
				if (*(--TstPg) & PAGE_ATTR_P)
					goto skip;
			while ((DWORD)FstPg & 0xFFF)
				if (*(FstPg++) & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*释放页表*/
			pt[(DWORD)TstPg >> 12] = 0;
skip:		continue;
		}
		else	/*整个页表空*/
			FstPg = (PAGE_DESC*)(((DWORD)FstPg + 0x1000) & 0xFFFFF000);
	}
	if (isRefreshTlb)
		RefreshTlb();
}

/*清除页内容(不清除副本)*/
void ClearPageNoPt0(PAGE_DESC *FstPg, PAGE_DESC *EndPg)
{
	PAGE_DESC *FstPg0;
	BOOL isRefreshTlb;

	FstPg0 = FstPg + (PT0_ID - PT_ID) * 0x100000;
	isRefreshTlb = FALSE;
	while (FstPg < EndPg)	/*目录项对应的页表对应的每页分别回收*/
	{
		DWORD PtAddr;	/*页表的物理地址*/
		DWORD Pt0Addr;	/*副本页表的物理地址*/

		if ((PtAddr = pt[(DWORD)FstPg >> 12]) & PAGE_ATTR_P && (PtAddr >> 12) != ((Pt0Addr = pt[(DWORD)FstPg0 >> 12]) >> 12))	/*页表存在,与副本不重合*/
		{
			PAGE_DESC *TstPg;

			TstPg = FstPg;	/*记录开始释放的位置*/
			do	/*删除对应的全部非空页*/
			{
				DWORD PgAddr;	/*页的物理地址*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*物理页存在*/
				{
					if (!(Pt0Addr & PAGE_ATTR_P) || (PgAddr >> 12) != (*FstPg0 >> 12))	/*与副本不重合*/
						LockFreePage(PgAddr);	/*释放物理页*/
					*FstPg = 0;
					isRefreshTlb = TRUE;
				}
			}
			while (((DWORD)(++FstPg0, ++FstPg) & 0xFFF) && FstPg < EndPg);
			while ((DWORD)TstPg & 0xFFF)	/*检查页表是否需要释放*/
				if (*(--TstPg) & PAGE_ATTR_P)
					goto skip;
			while ((DWORD)FstPg & 0xFFF)
				if ((FstPg0++, *(FstPg++)) & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*释放页表*/
			pt[(DWORD)TstPg >> 12] = 0;
skip:		continue;
		}
		else	/*整个页表空*/
		{
			FstPg = (PAGE_DESC*)(((DWORD)FstPg + 0x1000) & 0xFFFFF000);
			FstPg0 = (PAGE_DESC*)(((DWORD)FstPg0 + 0x1000) & 0xFFFFF000);
		}
	}
	if (isRefreshTlb)
		RefreshTlb();
}

/*映射物理地址*/
long MapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz)
{
	PROCESS_DESC *CurProc;
	BLK_DESC *CurBlk;
	void *MapAddr;
	PAGE_DESC *FstPg, *EndPg;

	CurProc = CurPmd;
	if (CurProc->attr & PROC_ATTR_APPS)
		return KERR_NO_DRIVER_PRIVILEGE;	/*驱动进程特权API*/
	if (siz == 0)
		return KERR_MAPSIZE_IS_ZERO;
	siz = (PhyAddr + siz + 0x00000FFF) & 0xFFFFF000;	/*调整PhyAddr,siz到页边界,siz临时用做结束地址*/
	*((DWORD*)addr) = PhyAddr & 0x00000FFF;	/*记录地址参数的零头*/
	PhyAddr &= 0xFFFFF000;
	if (siz <= PhyAddr)	/*长度越界*/
		return KERR_MAPSIZE_TOO_LONG;
	if (PhyAddr < BOOTDAT_ADDR || (PhyAddr < UPHYMEM_ADDR + (PmmLen << 12) && siz > (DWORD)kdat))	/*与不允许被映射的区域有重合*/
		return KERR_ILLEGAL_PHYADDR_MAPED;
	siz -= PhyAddr;	/*siz恢复为映射字节数*/
	if ((CurBlk = LockAllocUBlk(CurProc, siz)) == NULL)
		return KERR_OUT_OF_LINEADDR;
	MapAddr = CurBlk->addr;
	cli();
	if (CheckInMap2(CurProc->map2, MapAddr, MapAddr + siz))	/*检查区域是否已被映射*/
	{
		sti();
		LockFreeUBlk(CurProc, CurBlk);
		return KERR_PAGE_ALREADY_MAPED;
	}
	lock(&CurProc->Page_l);
	FstPg = &pt[(DWORD)MapAddr >> 12];
	EndPg = &pt[((DWORD)MapAddr + siz) >> 12];
	if (FillConAddr(FstPg, EndPg, PhyAddr, PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U) != NO_ERROR)
	{
		ClearPage(FstPg, EndPg, TRUE);
		ulock(&CurProc->Page_l);
		LockFreeUBlk(CurProc, CurBlk);
		return KERR_OUT_OF_PHYMEM;
	}
	ulock(&CurProc->Page_l);
	*((DWORD*)addr) |= *((DWORD*)&MapAddr);
	return NO_ERROR;
}

/*映射用户地址*/
long MapUserAddr(void **addr, DWORD siz)
{
	BLK_DESC *CurBlk;

	siz = (siz + 0x00000FFF) & 0xFFFFF000;	/*siz调整到页边界*/
	if (siz == 0)
		return KERR_MAPSIZE_IS_ZERO;
	if ((CurBlk = LockAllocUBlk(CurPmd, siz)) == NULL)	/*只申请空间即可*/
		return KERR_OUT_OF_LINEADDR;
	*addr = CurBlk->addr;
	return NO_ERROR;
}

/*解除映射地址*/
long UnmapAddr(void *addr)
{
	PROCESS_DESC *CurProc;
	BLK_DESC *blk;

	CurProc = CurPmd;
	*((DWORD*)&addr) &= 0xFFFFF000;
	if ((blk = LockFindUBlk(CurProc, addr)) == NULL)
		return KERR_ADDRARGS_NOT_FOUND;
	lock(&CurProc->Page_l);
	ClearPage(&pt[(DWORD)addr >> 12], &pt[((DWORD)addr + blk->siz) >> 12], TRUE);
	ulock(&CurProc->Page_l);
	LockFreeUBlk(CurProc, blk);
	return NO_ERROR;
}

/*映射地址块给别的进程,并发送映射消息*/
long MapProcAddr(void *addr, DWORD siz, THREAD_ID *ptid, BOOL isWrite, BOOL isChkExec, DWORD *argv, DWORD cs)
{
	PROCESS_DESC *CurProc, *DstProc;
	THREAD_DESC *CurThed, *DstThed;
	MAPBLK_DESC *map;
	void *MapAddr, *AlgAddr;
	PAGE_DESC *FstPg, *FstPg2, *EndPg;
	DWORD AlgSize;
	MESSAGE_DESC *msg;
	long res;

	if (siz == 0)
		return KERR_MAPSIZE_IS_ZERO;
	AlgAddr = (void*)((DWORD)addr & 0xFFFFF000);
	AlgSize = ((DWORD)addr + siz + 0x00000FFF) & 0xFFFFF000;	/*调整ProcAddr,siz到页边界,AlgSiz临时用做结束地址*/
	if (AlgSize <= (DWORD)AlgAddr)	/*长度越界*/
		return KERR_MAPSIZE_TOO_LONG;
	if (AlgAddr < UADDR_OFF)	/*与不允许被映射的区域有重合*/
		return KERR_ACCESS_ILLEGAL_ADDR;
	if (ptid->ProcID >= PMT_LEN)
		return KERR_INVALID_PROCID;
	if (ptid->ThedID >= TMT_LEN)
		return KERR_INVALID_THEDID;
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (ptid->ProcID == CurThed->id.ProcID)	/*不允许进程自身映射*/
		return KERR_PROC_SELF_MAPED;
	if (isChkExec)
	{
		EXEC_DESC *CurExec;

		CurExec = CurProc->exec;	/*检查映射区域与代码段和数据段的重合*/
		lock(&CurProc->Page_l);
		lockw(&CurExec->Page_l);
		if (CurExec->CodeOff < CurExec->CodeEnd && CurExec->CodeOff < (void*)AlgSize && CurExec->CodeEnd > AlgAddr)	/*代码段与映射区有重合*/
		{
			void *fst, *end;

			fst = (void*)((DWORD)(AlgAddr < CurExec->CodeOff ? CurExec->CodeOff : AlgAddr) & 0xFFFFF000);
			end = ((void*)AlgSize > CurExec->CodeEnd ? CurExec->CodeEnd : (void*)AlgSize);
			while (fst < end)	/*填充被映射的代码段*/
			{
				if ((res = FillPage(CurExec, fst, isWrite ? PAGE_ATTR_W : 0)) != NO_ERROR)
				{
					ulockw(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					return res;
				}
				fst += PAGE_SIZE;
			}
		}
		if (CurExec->DataOff < CurExec->DataEnd && CurExec->DataOff < (void*)AlgSize && CurExec->DataEnd > AlgAddr)	/*数据段与映射区有重合*/
		{
			void *fst, *end;

			fst = (void*)((DWORD)(AlgAddr < CurExec->DataOff ? CurExec->DataOff : AlgAddr) & 0xFFFFF000);
			end = ((void*)AlgSize > CurExec->DataEnd ? CurExec->DataEnd : (void*)AlgSize);
			while (fst < end)	/*填充被映射的数据段*/
			{
				if ((res = FillPage(CurExec, fst, isWrite ? PAGE_ATTR_W : 0)) != NO_ERROR)
				{
					ulockw(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					return res;
				}
				fst += PAGE_SIZE;
			}
		}
		ulockw(&CurExec->Page_l);
		ulock(&CurProc->Page_l);
	}
	cli();	/*要访问其他进程的信息,所以防止任务切换*/
	DstProc = pmt[ptid->ProcID];
	if (DstProc == NULL || (DstProc->attr & PROC_ATTR_DEL))
	{
		sti();
		return KERR_PROC_NOT_EXIST;
	}
	DstThed = DstProc->tmt[ptid->ThedID];
	if (DstThed == NULL || (DstThed->attr & THED_ATTR_DEL))
	{
		sti();
		return KERR_THED_NOT_EXIST;
	}
	DstProc->MapCou++;
	sti();
	AlgSize -= (DWORD)AlgAddr;	/*AlgSize恢复为映射字节数*/
	if ((MapAddr = LockAllocUFData(DstProc, AlgSize)) == NULL)	/*申请用户空间*/
	{
		clisub(&DstProc->MapCou);
		return KERR_OUT_OF_LINEADDR;
	}
	if ((map = AllocMap()) == NULL)	/*申请映射管理结构*/
	{
		LockFreeUFData(DstProc, MapAddr, AlgSize);
		clisub(&DstProc->MapCou);
		return KERR_OUT_OF_LINEADDR;
	}
	map->addr = (void*)((DWORD)MapAddr | ((DWORD)addr & 0x00000FFF));	/*设置映射结构*/
	map->addr2 = addr;
	map->siz = AlgSize | (isWrite ? PAGE_ATTR_W : 0);
	map->ptid = *ptid;
	map->ptid2 = CurThed->id;
	cli();
	if (isWrite && CurProc->map)	/*检查是否要写只读的映射区*/
	{
		MAPBLK_DESC *TmpMap;

		TmpMap = CurProc->map;
		for (;;)
		{
			if ((TmpMap = CheckInMap(TmpMap, AlgAddr, AlgAddr + AlgSize)) == NULL)
				break;
			if (!(TmpMap->siz & PAGE_ATTR_W))
			{
				FreeMap(map);
				LockFreeUFData(DstProc, MapAddr, AlgSize);
				clisub(&DstProc->MapCou);
				return KERR_WRITE_RDONLY_ADDR;
			}
			TmpMap = TmpMap->nxt;
		}
	}
	map->nxt = DstProc->map;
	map->nxt2 = CurProc->map2;
	CurProc->map2 = DstProc->map = map;
	if (isChkExec)
	{
		clilock(CurProc->Page_l || DstProc->Page_l);
		CurProc->Page_l = TRUE;
		DstProc->Page_l = TRUE;
		sti();
	}
	else
		lock(&DstProc->Page_l);
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid->ProcID]);	/*映射关系进程的页表*/
	FstPg = &pt[(DWORD)AlgAddr >> 12];
	EndPg = &pt[((DWORD)AlgAddr + AlgSize) >> 12];
	FstPg2 = &pt2[(DWORD)MapAddr >> 12];
	while (FstPg < EndPg)	/*源地址循环*/
	{
		if (pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P)	/*源页表存在*/
		{
			do
			{
				DWORD PgAddr;	/*源页的物理地址*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*源页存在*/
				{
					PAGE_DESC *CurPt2;
					DWORD PgAddr2;	/*目的页的物理地址*/

					CurPt2 = &pt[(DWORD)FstPg2 >> 12];
					if (!(*CurPt2 & PAGE_ATTR_P))	/*目的页表不存在*/
					{
						DWORD PtAddr2;	/*目的页表的物理地址*/

						if ((PtAddr2 = LockAllocPage()) == 0)	/*申请目的页表*/
						{
							ClearPage(&pt2[(DWORD)MapAddr >> 12], FstPg2, FALSE);
							ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
							ulock(&DstProc->Page_l);
							if (isChkExec)
								ulock(&CurProc->Page_l);
							cli();
							GetMap2(CurProc, addr);
							sti();
							FreeMap(map);
							LockFreeUFData(DstProc, MapAddr, AlgSize);
							clisub(&DstProc->MapCou);
							return KERR_OUT_OF_PHYMEM;
						}
						*CurPt2 = PAGE_ATTR_P | PAGE_ATTR_U | PtAddr2;	/*开启用户权限*/
						memset32((void*)((DWORD)FstPg2 & 0xFFFFF000), 0, 0x400);	/*清空页表*/
					}
					else if ((PgAddr2 = *FstPg2) & PAGE_ATTR_P)	/*目的页存在*/
					{
						cli();
						if (CheckInMap2(DstProc->map2, (void*)((DWORD)FstPg2 << 10), (void*)((DWORD)(FstPg2 + 1) << 10)))	/*已被映射*/
						{
							sti();
							ClearPage(&pt2[(DWORD)MapAddr >> 12], FstPg2, FALSE);
							ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
							ulock(&DstProc->Page_l);
							if (isChkExec)
								ulock(&CurProc->Page_l);
							cli();
							GetMap2(CurProc, addr);
							sti();
							FreeMap(map);
							LockFreeUFData(DstProc, MapAddr, AlgSize);
							clisub(&DstProc->MapCou);
							return KERR_PAGE_ALREADY_MAPED;
						}
						else
							LockFreePage(PgAddr2);	/*释放目的页*/
					}
					if (isWrite)
					{
						*CurPt2 |= PAGE_ATTR_W;	/*开启页表写权限*/
						*FstPg2 = PAGE_ATTR_W | PgAddr;	/*开启页写权限*/
					}
					else
					{
						if (((DWORD)CurPt2 << 20) >= (DWORD)MapAddr && ((DWORD)(CurPt2 + 1) << 20) <= (DWORD)MapAddr + AlgSize)	/*页表覆盖区全部位于映射空间内*/
							*CurPt2 &= (~PAGE_ATTR_W);	/*关闭页表写权限*/
						*FstPg2 = (~PAGE_ATTR_W) & PgAddr;	/*关闭页写权限*/
					}
				}
			}
			while (((DWORD)(++FstPg2, ++FstPg) & 0xFFF) && FstPg < EndPg);
		}
		else	/*源页表不存在*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
	ulock(&DstProc->Page_l);
	if (isChkExec)
		ulock(&CurProc->Page_l);
	clisub(&DstProc->MapCou);	/*到此映射完成*/
	if (!isChkExec)
		lockset((volatile DWORD*)(&CurProc->PageReadAddr), (DWORD)map->addr2);
	if ((msg = AllocMsg()) == NULL)	/*申请消息结构*/
		return KERR_MSG_NOT_ENOUGH;
	msg->ptid = *ptid;	/*设置消息*/
	memcpy32(msg->data, argv, MSG_DATA_LEN);
	msg->data[MSG_ATTR_ID] = (argv[MSG_API_ID] & MSG_API_MASK) | (isWrite ? MSG_ATTR_RWMAP : MSG_ATTR_ROMAP);
	msg->data[MSG_ADDR_ID] = (DWORD)map->addr;
	msg->data[MSG_SIZE_ID] = siz;
	if ((res = SendMsg(msg)) != NO_ERROR)	/*发送映射消息*/
	{
		FreeMsg(msg);
		return res;
	}
	if (cs)	/*等待返回消息*/
	{
		if ((res = RecvProcMsg(&msg, *ptid, cs)) != NO_ERROR)
			return res;
		*ptid = msg->ptid;
		memcpy32(argv, msg->data, MSG_DATA_LEN);
		FreeMsg(msg);
	}
	return NO_ERROR;
}

/*解除映射进程共享地址,并发送解除消息*/
long UnmapProcAddr(void *addr, const DWORD *argv)
{
	PROCESS_DESC *CurProc, *DstProc;
	MAPBLK_DESC *map;
	PAGE_DESC *FstPg, *EndPg, *FstPg2;
	THREAD_ID ptid;
	void *addr2;
	BOOL isRefreshTlb;
	MESSAGE_DESC *msg;
	long res;

	CurProc = CurPmd;
	cli();
	map = GetMap(CurProc, addr);
	if (map == NULL)
	{
		sti();
		return KERR_ADDRARGS_NOT_FOUND;
	}
	ptid = map->ptid2;
	addr2 = map->addr2;
	DstProc = pmt[ptid.ProcID];
	DstProc->MapCou++;
	if (DstProc->PageReadAddr != addr2)
	{
		clilock(DstProc->Page_l || CurProc->Page_l);
		DstProc->Page_l = TRUE;
		CurProc->Page_l = TRUE;
		sti();
	}
	else
		lock(&CurProc->Page_l);
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid.ProcID]);	/*映射关系进程的页表*/
	res = NO_ERROR;
	isRefreshTlb = FALSE;
	FstPg = &pt[(DWORD)addr >> 12];
	EndPg = &pt[((DWORD)addr + (map->siz & 0xFFFFF000)) >> 12];
	FstPg2 = &pt2[(DWORD)addr2 >> 12];
	while (FstPg < EndPg)	/*源地址循环*/
	{
		DWORD PtAddr;	/*源页表的物理地址*/

		if ((PtAddr = pt[(DWORD)FstPg >> 12]) & PAGE_ATTR_P)	/*源页表存在*/
		{
			PAGE_DESC *TstPg;

			TstPg = FstPg;	/*记录开始释放的位置*/
			do
			{
				DWORD PgAddr;	/*源页的物理地址*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*源页存在*/
				{
					DWORD PgAddr2;	/*目的页的物理地址*/

					if (!(pt[(DWORD)FstPg2 >> 12] & PAGE_ATTR_P))	/*目的页表不存在*/
					{
						DWORD PtAddr2;	/*目的页表的物理地址*/

						if ((PtAddr2 = LockAllocPage()) == 0)	/*申请目的页表*/
						{
							res = KERR_OUT_OF_PHYMEM;
							goto skip2;
						}
						pt[(DWORD)FstPg2 >> 12] |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr2;	/*开启用户写权限*/
						memset32((void*)((DWORD)FstPg2 & 0xFFFFF000), 0, 0x400);	/*清空页表*/
					}
					else if ((PgAddr2 = *FstPg2) & PAGE_ATTR_P && (PgAddr2 >> 12) != (PgAddr >> 12))	/*目的页存在,且与源页不重合*/
					{
						cli();
						if (CheckInMap(DstProc->map, (void*)((DWORD)FstPg2 << 10), (void*)((DWORD)(FstPg2 + 1) << 10)) ||
							CheckInMap2(DstProc->map2, (void*)((DWORD)FstPg2 << 10), (void*)((DWORD)(FstPg2 + 1) << 10)))	/*是映射区*/
						{
							sti();
							pt[(PT_ID << 10) | PG0_ID] = pt[(DWORD)FstPg2 >> 12];	/*设置临时映射*/
							memcpy32(&pg0[((DWORD)FstPg2 << 10) & 0x003FF000], (void*)((DWORD)FstPg << 10), 0x400);	/*复制页内容*/
							pt[(PT_ID << 10) | PG0_ID] = 0;	/*解除临时映射*/
							LockFreePage(PgAddr);	/*释放源页*/
							PgAddr = PgAddr2;
						}
						else
							LockFreePage(PgAddr2);	/*释放目的页*/
					}
					*FstPg2 |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | (PgAddr & 0xFFFFF000);	/*开启用户写权限*/
					*FstPg = 0;
					isRefreshTlb = TRUE;
				}
				else
					*FstPg = 0;
			}
			while (((DWORD)(++FstPg2, ++FstPg) & 0xFFF) && FstPg < EndPg);
			while ((DWORD)TstPg & 0xFFF)	/*检查页表是否需要释放*/
				if (*(--TstPg) & PAGE_ATTR_P)
					goto skip;
			while ((DWORD)FstPg & 0xFFF)
				if (*(FstPg++) & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*释放页表*/
			pt[(DWORD)TstPg >> 12] = 0;
skip:		continue;
		}
		else	/*源页表不存在*/
		{
			DWORD step;

			pt[(DWORD)FstPg >> 12] = 0;
			step = 0x1000 - ((DWORD)FstPg & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
skip2:
	if (isRefreshTlb)
		RefreshTlb();
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
	ulock(&CurProc->Page_l);
	if (DstProc->PageReadAddr != addr2)
		ulock(&DstProc->Page_l);
	clisub(&DstProc->MapCou);	/*到此映射完成*/
	if (DstProc->PageReadAddr == addr2)
		DstProc->PageReadAddr = NULL;
	FreeMap(map);
	LockFreeUFData(CurProc, (void*)((DWORD)addr & 0xFFFFF000), map->siz & 0xFFFFF000);
	if ((msg = AllocMsg()) == NULL)	/*申请消息结构*/
		return KERR_MSG_NOT_ENOUGH;
	msg->ptid = ptid;	/*设置消息*/
	memcpy32(msg->data, argv, MSG_DATA_LEN);
	msg->data[MSG_ATTR_ID] = (argv[MSG_API_ID] & MSG_API_MASK) | MSG_ATTR_UNMAP;
	msg->data[MSG_ADDR_ID] = (DWORD)addr2;
	if (res != NO_ERROR)
		msg->data[MSG_RES_ID] = res;
	if ((res = SendMsg(msg)) != NO_ERROR)	/*发送撤销映射消息*/
	{
		FreeMsg(msg);
		return res;
	}
	return NO_ERROR;
}

/*取消映射进程共享地址,并发送取消消息*/
long CnlmapProcAddr(void *addr, const DWORD *argv)
{
	PROCESS_DESC *CurProc, *DstProc;
	MAPBLK_DESC *map;
	PAGE_DESC *FstPg, *EndPg, *FstPg2;
	THREAD_ID ptid;
	void *addr2;
	MESSAGE_DESC *msg;
	long res;

	CurProc = CurPmd;
	cli();
	map = GetMap2(CurProc, addr);
	if (map == NULL)
	{
		sti();
		return KERR_ADDRARGS_NOT_FOUND;
	}
	ptid = map->ptid;
	addr2 = map->addr;
	DstProc = pmt[ptid.ProcID];
	DstProc->MapCou++;
	clilock(CurProc->Page_l || DstProc->Page_l);
	CurProc->Page_l = TRUE;
	DstProc->Page_l = TRUE;
	sti();
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid.ProcID]);	/*映射关系进程的页表*/
	FstPg = &pt[(DWORD)addr >> 12];
	EndPg = &pt[((DWORD)addr + (map->siz & 0xFFFFF000)) >> 12];
	FstPg2 = &pt2[(DWORD)addr2 >> 12];
	while (FstPg < EndPg)	/*源地址循环*/
	{
		DWORD PtAddr2;	/*目的页表的物理地址*/

		if ((PtAddr2 = pt[(DWORD)FstPg2 >> 12]) & PAGE_ATTR_P)	/*页表存在*/
		{
			PAGE_DESC *TstPg2;

			TstPg2 = FstPg2;	/*记录开始释放的位置*/
			do
			{
				DWORD PgAddr2;	/*目的页的物理地址*/

				if ((PgAddr2 = *FstPg2) & PAGE_ATTR_P)	/*物理页存在*/
				{
					cli();
					if ((!(pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P) || (PgAddr2 >> 12) != (*FstPg >> 12)) &&
						!CheckInMap2(DstProc->map2, (void*)((DWORD)FstPg2 << 10), (void*)((DWORD)(FstPg2 + 1) << 10)))	/*(源页不存在或与源页不重合)且没被映射*/
						LockFreePage(PgAddr2);	/*释放物理页*/
					sti();
					*FstPg2 = 0;
				}
			}
			while (((DWORD)(++FstPg, ++FstPg2) & 0xFFF) && FstPg < EndPg);
			while ((DWORD)TstPg2 & 0xFFF)	/*检查页表是否需要释放*/
				if (*(--TstPg2) & PAGE_ATTR_P)
					goto skip;
			while ((DWORD)FstPg2 & 0xFFF)
				if ((FstPg++, *(FstPg2++)) & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr2);	/*释放页表*/
			pt[(DWORD)TstPg2 >> 12] = 0;
skip:		continue;
		}
		else	/*整个页目录表项空*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg2 & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
	ulock(&DstProc->Page_l);
	ulock(&CurProc->Page_l);
	clisub(&DstProc->MapCou);	/*到此解除映射完成*/
	FreeMap(map);
	LockFreeUFData(DstProc, (void*)((DWORD)addr2 & 0xFFFFF000), map->siz & 0xFFFFF000);
	if ((msg = AllocMsg()) == NULL)	/*申请消息结构*/
		return KERR_MSG_NOT_ENOUGH;
	msg->ptid = ptid;	/*设置消息*/
	memcpy32(msg->data, argv, MSG_DATA_LEN);
	msg->data[MSG_ATTR_ID] = (argv[MSG_API_ID] & MSG_API_MASK) | MSG_ATTR_CNLMAP;
	msg->data[MSG_ADDR_ID] = (DWORD)addr2;
	if ((res = SendMsg(msg)) != NO_ERROR)	/*发送取消映射消息*/
	{
		FreeMsg(msg);
		return res;
	}
	return NO_ERROR;
}

/*清除进程的映射队列*/
void FreeAllMap()
{
	PROCESS_DESC *CurProc;
	MAPBLK_DESC *CurMap, *map;
	DWORD data[MSG_DATA_LEN];

	data[MSG_API_ID] = (MSG_ATTR_PROCEXIT >> 16);
	CurProc = CurPmd;
	while (CurProc->MapCou)	/*等待其它进程映射完成*/
		schedul();
	CurMap = CurProc->map;
	while (CurMap)
	{
		map = CurMap->nxt;
		UnmapProcAddr(CurMap->addr, data);
		CurMap = map;
	}
	CurMap = CurProc->map2;
	while (CurMap)
	{
		map = CurMap->nxt2;
		CnlmapProcAddr(CurMap->addr2, data);
		CurMap = map;
	}
}

/*页故障处理程序*/
void PageFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags)
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	EXEC_DESC *CurExec;
	void *addr;
	long res;

	/*进入中断处理程序以前中断已经关闭*/
	addr = GetPageFaultAddr();
	sti();
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	CurThed->attr &= (~THED_ATTR_APPS);	/*进入系统调用态*/
	if (addr < UADDR_OFF)	/*进程非法访问内核空间*/
	{
		MESSAGE_DESC *msg;

		if ((msg = AllocMsg()) != NULL)	/*通知报告服务器异常消息*/
		{
			msg->ptid = kpt[REP_KPORT];
			msg->data[MSG_ATTR_ID] = MSG_ATTR_EXCEP;
			msg->data[MSG_ADDR_ID] = (DWORD)addr;
			msg->data[MSG_SIZE_ID] = eip;
			msg->data[MSG_RES_ID] = KERR_ACCESS_ILLEGAL_ADDR;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		ThedExit(KERR_ACCESS_ILLEGAL_ADDR);
	}
	CurExec = CurProc->exec;
	lock(&CurProc->Page_l);
	lockw(&CurExec->Page_l);
	res = FillPage(CurExec, addr, ErrCode);
	ulockw(&CurExec->Page_l);
	ulock(&CurProc->Page_l);
	if (res != NO_ERROR)
	{
		MESSAGE_DESC *msg;

		if ((msg = AllocMsg()) != NULL)	/*通知报告服务器异常消息*/
		{
			msg->ptid = kpt[REP_KPORT];
			msg->data[MSG_ATTR_ID] = MSG_ATTR_EXCEP;
			msg->data[MSG_ADDR_ID] = (DWORD)addr;
			msg->data[MSG_SIZE_ID] = eip;
			msg->data[MSG_RES_ID] = res;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		ThedExit(res);
	}
	CurThed->attr |= THED_ATTR_APPS;	/*离开系统调用态*/
}
