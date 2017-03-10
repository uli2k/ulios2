/*	page.h for ulios
	作者：孙亮
	功能：分页内存管理定义
	最后修改日期：2009-05-29
*/

#ifndef _PAGE_H_
#define _PAGE_H_

#include "ulidef.h"

#define MAPMT_LEN			0x1000	/*地址映射管理表长度*/
typedef struct _MAPBLK_DESC
{
	void *addr;					/*映射进程起始地址*/
	void *addr2;				/*被映射进程起始地址*/
	DWORD siz;					/*字节数*/
	THREAD_ID ptid;				/*映射线程*/
	THREAD_ID ptid2;			/*被映射线程*/
	struct _MAPBLK_DESC *nxt;	/*映射进程后一项*/
	struct _MAPBLK_DESC *nxt2;	/*被映射进程后一项*/
}MAPBLK_DESC;	/*地址映射管理块描述符*/

#define PAGE_SIZE	0x00001000

/*分配物理页,返回物理地址,无法分配返回0*/
DWORD AllocPage();

/*回收物理页,参数:物理地址*/
void FreePage(DWORD pgaddr);

/*分配地址映射结构*/
MAPBLK_DESC *AllocMap();

/*释放地址映射结构*/
void FreeMap(MAPBLK_DESC *map);

/*填充连续的页地址*/
long FillConAddr(PAGE_DESC *FstPg, PAGE_DESC *EndPg, DWORD PhyAddr, DWORD attr);

/*填充页内容*/
long FillPage(EXEC_DESC *exec, void *addr, DWORD ErrCode);

/*清除页内容*/
void ClearPage(PAGE_DESC *FstPg, PAGE_DESC *EndPg, BOOL isFree);

/*清除页内容(不清除副本)*/
void ClearPageNoPt0(PAGE_DESC *FstPg, PAGE_DESC *EndPg);

/*映射物理地址*/
long MapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz);

/*映射用户地址*/
long MapUserAddr(void **addr, DWORD siz);

/*解除映射地址*/
long UnmapAddr(void *addr);

/*映射地址块给别的进程,并发送开始消息*/
long MapProcAddr(void *addr, DWORD siz, THREAD_ID *ptid, BOOL isWrite, BOOL isChkExec, DWORD *argv, DWORD cs);

/*解除映射进程共享地址,并发送完成消息*/
long UnmapProcAddr(void *addr, const DWORD *argv);

/*取消映射进程共享地址,并发送取消消息*/
long CnlmapProcAddr(void *addr, const DWORD *argv);

/*清除进程的映射队列*/
void FreeAllMap();

/*页故障处理程序*/
void PageFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags);

#endif
