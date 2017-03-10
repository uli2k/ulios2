/*	kalloc.h for ulios
	作者：孙亮
	功能：内核动态块管理定义
	最后修改日期：2009-05-29
*/

#ifndef	_KALLOC_H_
#define	_KALLOC_H_

#include "ulidef.h"

typedef struct _FREE_BLK_DESC
{
	void *addr;					/*起始地址*/
	DWORD siz;					/*字节数*/
	struct _FREE_BLK_DESC *nxt;	/*后一项*/
}FREE_BLK_DESC;	/*自由块描述符*/

/*初始化自由块管理表*/
void InitFbt(FREE_BLK_DESC *fbt, DWORD FbtLen, void *addr, DWORD siz);

/*自由块分配*/
void *alloc(FREE_BLK_DESC *fbt, DWORD siz);

/*自由块回收*/
void free(FREE_BLK_DESC *fbt, void *addr, DWORD siz);

#endif
