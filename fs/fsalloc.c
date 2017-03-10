/*	fsalloc.c for ulios file system
	作者：孙亮
	功能：文件系统动态内存管理
	最后修改日期：2009-05-28
*/

#include "fs.h"

FREE_BLK_DESC fmt[FMT_LEN];
DWORD fmtl;	/*动态内存管理锁*/

/*自由块分配*/
void *malloc(DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk;

	lock(&fmtl);
	if (siz == 0 || siz > fmt->siz)	/*没有足够空间*/
	{
		ulock(&fmtl);
		return NULL;
	}
	for (CurFblk = (PreFblk = fmt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (CurFblk->siz >= siz)	/*首次匹配法*/
		{
			void *addr;
			fmt->siz -= siz;
			if ((CurFblk->siz -= siz) == 0)	/*表项已空*/
			{
				PreFblk->nxt = CurFblk->nxt;	/*去除表项*/
				CurFblk->nxt = (FREE_BLK_DESC*)fmt->addr;	/*释放表项*/
				fmt->addr = (void*)CurFblk;
			}
			addr = CurFblk->addr + CurFblk->siz;
			ulock(&fmtl);
			return addr;
		}
	ulock(&fmtl);
	return NULL;
}

/*自由块回收*/
void free(void *addr, DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk, *TmpFblk;

	lock(&fmtl);
	fmt->siz += siz;
	for (CurFblk = (PreFblk = fmt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (addr < CurFblk->addr)
			break;	/*搜索释放位置*/
	if (PreFblk != fmt)
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
	TmpFblk = (FREE_BLK_DESC*)fmt->addr;
	if (TmpFblk == NULL)	/*无空表项,无法释放*/
		return;
	fmt->addr = (void*)TmpFblk->nxt;	/*申请表项*/
	TmpFblk->addr = addr;
	TmpFblk->siz = siz;
	TmpFblk->nxt = PreFblk->nxt;
	PreFblk->nxt = TmpFblk;
	ulock(&fmtl);
	return;
addpre:	/*加入到前面*/
	PreFblk->siz += siz;
	ulock(&fmtl);
	return;
addnxt:	/*加入到后面*/
	CurFblk->addr = addr;
	CurFblk->siz += siz;
	ulock(&fmtl);
	return;
link:	/*连接前后描述符*/
	PreFblk->siz += (siz + CurFblk->siz);
	PreFblk->nxt = CurFblk->nxt;
	CurFblk->nxt = (FREE_BLK_DESC*)fmt->addr;	/*释放表项*/
	fmt->addr = (void*)CurFblk;
	ulock(&fmtl);
	return;
}
