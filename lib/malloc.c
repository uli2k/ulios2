/*	malloc.c for ulios
	作者：孙亮
	功能：用户内存堆管理实现
	最后修改日期：2009-05-28
*/

#include "malloc.h"

typedef struct _FREE_BLK_DESC
{
	void *addr;					/*起始地址*/
	DWORD siz;					/*字节数*/
	struct _FREE_BLK_DESC *nxt;	/*后一项*/
}FREE_BLK_DESC;	/*自由块描述符*/

#define FBT_LEN		256

FREE_BLK_DESC fbt[FBT_LEN];

/*初始化堆管理表*/
long InitMallocTab(DWORD siz)
{
	FREE_BLK_DESC *fbd;
	void *addr;
	long res;

	if ((res = KMapUserAddr(&addr, siz)) != NO_ERROR)
		return res;
	fbt->addr = &fbt[2];	/*0项不用1项已用2以后的空闲*/
	fbt->siz = siz;
	fbt->nxt = &fbt[1];
	fbt[1].addr = addr;
	fbt[1].siz = siz;
	fbt[1].nxt = NULL;
	for (fbd = &fbt[2]; fbd < &fbt[sizeof(fbt) / sizeof(FREE_BLK_DESC) - 1]; fbd++)
		fbd->nxt = fbd + 1;
	fbd->nxt = NULL;
	return NO_ERROR;
}

/*自由块分配*/
static void *AllocBlk(DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk;

	if (siz > fbt->siz)	/*没有足够空间*/
		return NULL;
	for (CurFblk = (PreFblk = fbt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (CurFblk->siz >= siz)	/*首次匹配法*/
		{
			fbt->siz -= siz;
			if ((CurFblk->siz -= siz) == 0)	/*表项已空*/
			{
				PreFblk->nxt = CurFblk->nxt;	/*去除表项*/
				CurFblk->nxt = (FREE_BLK_DESC*)fbt->addr;	/*释放表项*/
				fbt->addr = (void*)CurFblk;
			}
			return CurFblk->addr + CurFblk->siz;
		}
	return NULL;
}

/*自由块回收*/
static void FreeBlk(void *addr, DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk, *TmpFblk;

	fbt->siz += siz;
	for (CurFblk = (PreFblk = fbt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (addr < CurFblk->addr)
			break;	/*搜索释放位置*/
	if (PreFblk != fbt)
		if (CurFblk)
			if (PreFblk->addr + PreFblk->siz == addr)	/*是否与前空块相接*/
				if (addr + siz == CurFblk->addr)	/*是否与后空块相接*/
					goto link;
				else
					goto addpre;
			else
				if (addr + siz == CurFblk->addr)	/*是否与后空块相接*/
					goto addnxt;
				else
					goto creat;
		else
			if (PreFblk->addr + PreFblk->siz == addr)	/*是否与前空块相接*/
				goto addpre;
			else
				goto creat;
	else
		if (CurFblk && addr + siz == CurFblk->addr)	/*是否与后空块相接*/
			goto addnxt;
		else
			goto creat;
creat:	/*新建描述符*/
	TmpFblk = (FREE_BLK_DESC*)fbt->addr;
	if (TmpFblk == NULL)	/*无空表项,无法释放*/
		return;
	fbt->addr = (void*)TmpFblk->nxt;	/*申请表项*/
	TmpFblk->addr = addr;
	TmpFblk->siz = siz;
	TmpFblk->nxt = PreFblk->nxt;
	PreFblk->nxt = TmpFblk;
	return;
addpre:	/*加入到前面*/
	PreFblk->siz += siz;
	return;
addnxt:	/*加入到后面*/
	CurFblk->addr = addr;
	CurFblk->siz += siz;
	return;
link:	/*连接前后描述符*/
	PreFblk->siz += (siz + CurFblk->siz);
	PreFblk->nxt = CurFblk->nxt;
	CurFblk->nxt = (FREE_BLK_DESC*)fbt->addr;	/*释放表项*/
	fbt->addr = (void*)CurFblk;
	return;
}

/*自由块连接分配*/
static BOOL LinkBlk(void *addr, DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk;

	for (CurFblk = (PreFblk = fbt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (addr <= CurFblk->addr)	/*搜索连接位置*/
		{
			if (addr == CurFblk->addr && CurFblk->siz >= siz)	/*匹配成功*/
			{
				fbt->siz -= siz;
				if ((CurFblk->siz -= siz) == 0)	/*表项已空*/
				{
					PreFblk->nxt = CurFblk->nxt;	/*去除表项*/
					CurFblk->nxt = (FREE_BLK_DESC*)fbt->addr;	/*释放表项*/
					fbt->addr = (void*)CurFblk;
				}
				CurFblk->addr += siz;
				return TRUE;	/*连接成功*/
			}
			return FALSE;
		}
	return FALSE;
}

/*内存分配*/
void *malloc(DWORD siz)
{
	void *addr;

	if ((long)siz <= 0)
		return NULL;
	siz = (siz + sizeof(DWORD) - 1) & 0xFFFFFFFC;	/*32位对齐*/
	if ((addr = AllocBlk(siz + sizeof(DWORD))) == NULL)
		return NULL;
	*(DWORD*)addr = siz;	/*设置长度字*/
	return addr + sizeof(DWORD);
}

/*内存回收*/
void free(void *addr)
{
	if (addr == NULL)
		return;
	addr -= sizeof(DWORD);	/*取得长度字地址*/
	FreeBlk(addr, *(DWORD*)addr + sizeof(DWORD));
}

/*内存重分配*/
void *realloc(void *addr, DWORD siz)
{
	DWORD oldsiz;

	if (addr == NULL)
		return malloc(siz);
	if (siz == 0)
	{
		free(addr);
		return NULL;
	}
	oldsiz = *(DWORD*)(addr - sizeof(DWORD));	/*取得长度字*/
	siz = (siz + sizeof(DWORD) - 1) & 0xFFFFFFFC;	/*32位对齐*/
	if (siz < oldsiz)	/*缩小*/
	{
		FreeBlk(addr + siz, oldsiz - siz);
		*(DWORD*)(addr - sizeof(DWORD)) = siz;
		return addr;
	}
	else if (siz > oldsiz)	/*扩大*/
	{
		if (siz - oldsiz > fbt->siz)	/*没有足够空间*/
			return NULL;
		if (LinkBlk(addr + oldsiz, siz - oldsiz))	/*连接到原块后面*/
		{
			*(DWORD*)(addr - sizeof(DWORD)) = siz;
			return addr;
		}
		else	/*重新分配*/
		{
			free(addr);
			return malloc(siz);
		}
	}
	return addr;	/*无改变,返回原地址*/
}
