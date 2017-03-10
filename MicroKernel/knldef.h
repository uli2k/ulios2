/*	knldef.h for ulios
	作者：孙亮
	功能：内核相关结构体、变量分布定义
	最后修改日期：2009-05-26
*/

#ifndef	_KNLDEF_H_
#define	_KNLDEF_H_

#include "ulidef.h"
#include "x86cpu.h"
#include "error.h"
#include "kalloc.h"
#include "bootdata.h"
#include "cintr.h"
#include "exec.h"
#include "ipc.h"
#include "page.h"
#include "task.h"

/**********内核属性常数**********/

#define KCODE_SEL	0x08	/*内核代码段选择子*/
#define KDATA_SEL	0x10	/*内核数据段选择子*/
#define UCODE_SEL	0x1B	/*用户代码段选择子*/
#define UDATA_SEL	0x23	/*用户数据段选择子*/
#define TSS_SEL		0x28	/*任务状态段选择子*/

#define UADDR_OFF	((void*)0x08000000)	/*用户地址偏移*/
#define BASESRV_OFF	((void*)0x08000000)	/*基础服务地址偏移*/
#define UFDATA_OFF	((void*)0x40000000)	/*用户自由数据空间偏移*/
#define UFDATA_SIZ			0x80000000	/*用户自由数据空间大小*/
#define SHRDLIB_OFF	((void*)0xC0000000)	/*共享库区偏移*/
#define SHRDLIB_SIZ			0x40000000	/*共享库区大小*/

#define BOOTDAT_ADDR		0x00090000	/*启动数据物理地址*/
#define UPHYMEM_ADDR		0x00800000	/*用户内存起始物理地址*/

/**********内核数据表**********/

/*保护模式相关表*/
#define GDT_LEN		0x0100	/*256项*/
extern SEG_GATE_DESC gdt[];	/*全局描述符表2KB*/
#define IDT_LEN		0x0100	/*256项*/
extern SEG_GATE_DESC idt[];	/*中断描述符表2KB*/
#define PDT_LEN		0x0400	/*1024项*/
extern PAGE_DESC kpdt[];	/*内核页目录表4KB*/
extern PAGE_DESC pddt[];	/*页目录表的目录表4KB*/
/*内核管理表*/
#define FMT_LEN		0x0400	/*1024项*/
extern FREE_BLK_DESC kmmt[];/*内核自由数据管理表*/
#define PMT_LEN		0x0400	/*1024项*/
extern PROCESS_DESC* pmt[];	/*进程管理表4KB*/
#define KPT_LEN		0x0400	/*1024项*/
extern THREAD_ID kpt[];		/*内核端口注册表4KB*/

/**********内核零散变量**********/

#define KVAL_LEN	0x1000	/*4096项*/
extern BYTE KnlValue[];		/*内核零散变量区4KB*/
/*进程管理*/
#define FstPmd		(*((PROCESS_DESC***)(KnlValue + 0x0000)))	/*首个空进程项指针*/
#define EndPmd		(*((PROCESS_DESC***)(KnlValue + 0x0004)))	/*末个非空进程项指针*/
#define CurPmd		(*((PROCESS_DESC**)	(KnlValue + 0x0008)))	/*当前进程指针*/
#define PmdCou		(*((DWORD*)			(KnlValue + 0x000C)))	/*已有进程数量*/
/*用户物理内存管理*/
#define pmmap		(*((DWORD**)		(KnlValue + 0x0010)))	/*物理页面使用位图指针*/
#define PmmLen		(*((DWORD*)			(KnlValue + 0x0014)))	/*物理页管理表长度*/
#define PmpID		(*((DWORD*)			(KnlValue + 0x0018)))	/*首个空物理页ID*/
#define RemmSiz		(*((DWORD*)			(KnlValue + 0x001C)))	/*剩余内存字节数*/
/*消息管理*/
#define FstMsg		(*((MESSAGE_DESC**)	(KnlValue + 0x0020)))	/*消息管理表指针*/
/*地址映射管理*/
#define FstMap		(*((MAPBLK_DESC**)	(KnlValue + 0x0024)))	/*地址映射管理表指针*/
/*零散变量*/
#define clock		(*((volatile DWORD*)(KnlValue + 0x0028)))	/*时钟计数器*/
#define SleepList	(*((THREAD_DESC**)	(KnlValue + 0x002C)))	/*延时阻塞链表指针*/
#define LastI387	(*((I387**)			(KnlValue + 0x0030)))	/*被推迟保存的协处理器寄存器指针*/
/*锁变量*/
#define Kmalloc_l	(*((volatile DWORD*)(KnlValue + 0x0034)))	/*kmalloc锁*/
#define AllocPage_l	(*((volatile DWORD*)(KnlValue + 0x0038)))	/*AllocPage锁*/
#define MapMgr_l	(*((volatile DWORD*)(KnlValue + 0x003C)))	/*映射管理锁*/
/*表结构*/
#define IRQ_LEN		0x10	/*IRQ信号数量16项*/
#define IrqPort		((THREAD_ID*)		(KnlValue + 0x0F58))	/*Irq端口注册表*/
#define KnlTss		(*((TSS*)			(KnlValue + 0x0F98)))	/*内核线程的TSS*/

/**********内核大块数据区**********/

#define KDAT_SIZ	0x00700000	/*大小7M*/
extern BYTE kdat[];		/*内核自由数据*/

/**********分页机制相关表**********/

#define PDT_ID	2
#define PT_ID	3
#define PT0_ID	4
#define PT2_ID	5
#define PG0_ID	6

#define PT_LEN	0x00100000	/*1048576项*/
#define PG_LEN	0x00400000	/*4194304项*/
extern PAGE_DESC pdt[];		/*所有进程页目录表4MB*/
extern PAGE_DESC pt[];		/*当前进程页表4MB*/
extern PAGE_DESC pt0[];		/*当前进程副本页表4MB*/
extern PAGE_DESC pt2[];		/*关系进程页表4MB*/
extern BYTE pg0[];			/*当前页副本4MB*/

/**********工具函数**********/

/*注册内核端口对应线程*/
long RegKnlPort(DWORD PortN);

/*注销内核端口对应线程*/
long UnregKnlPort(DWORD PortN);

/*取得内核端口对应线程*/
long GetKptThed(DWORD PortN, THREAD_ID *ptid);

/*注销线程的所有内核端口*/
long UnregAllKnlPort();

/*内核内存分配*/
static inline void *kmalloc(DWORD siz)
{
	return alloc(kmmt, siz);
}

/*内核内存回收*/
static inline void kfree(void *addr, DWORD siz)
{
	free(kmmt, addr, siz);
}

/*内核内存分配(锁定方式)*/
static inline void *LockKmalloc(DWORD siz)
{
	void *res;
	lock(&Kmalloc_l);
	res = kmalloc(siz);
	ulock(&Kmalloc_l);
	return res;
}

/*内核内存回收(锁定方式)*/
static inline void LockKfree(void *addr, DWORD siz)
{
	lock(&Kmalloc_l);
	kfree(addr, siz);
	ulock(&Kmalloc_l);
}

/*分配物理页(锁定方式)*/
static inline DWORD LockAllocPage()
{
	DWORD res;
	lock(&AllocPage_l);
	res = AllocPage();
	ulock(&AllocPage_l);
	return res;
}

/*回收物理页(锁定方式)*/
static inline void LockFreePage(DWORD pgaddr)
{
	lock(&AllocPage_l);
	FreePage(pgaddr);
	ulock(&AllocPage_l);
}

#endif
