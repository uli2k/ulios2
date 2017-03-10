/*	malloc.h for ulios
	作者：孙亮
	功能：用户内存堆管理定义
	最后修改日期：2009-05-29
*/

#ifndef	_MALLOC_H_
#define	_MALLOC_H_

#include "../MkApi/ulimkapi.h"

/*初始化堆管理表*/
long InitMallocTab(DWORD siz);

/*内存分配*/
void *malloc(DWORD siz);

/*内存回收*/
void free(void *addr);

/*内存重分配*/
void *realloc(void *addr, DWORD siz);

#endif
