/*	kalloc.c for ulios
	作者：孙亮
	功能：内核动态块管理
	最后修改日期：2009-05-28
*/

#include "knldef.h"

/*初始化自由块管理表*/
void InitFbt(FREE_BLK_DESC *fbt, DWORD FbtLen, void *addr, DWORD siz)
{
	FREE_BLK_DESC *fbd;

	fbt->addr = &fbt[2];	/*0项不用1项已用2以后的空闲*/
	fbt->siz = siz;
	fbt->nxt = &fbt[1];
	fbt[1].addr = addr;
	fbt[1].siz = siz;
	fbt[1].nxt = NULL;
	for (fbd = &fbt[2]; fbd < &fbt[FbtLen - 1]; fbd++)
		fbd->nxt = fbd + 1;
	fbd->nxt = NULL;
}

/*自由块分配*/
void *alloc(FREE_BLK_DESC *fbt, DWORD siz)
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
void free(FREE_BLK_DESC *fbt, void *addr, DWORD siz)
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
