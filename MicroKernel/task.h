/*	task.h for ulios
	作者：孙亮
	功能：任务管理定义
	最后修改日期：2009-06-25
*/

#ifndef _TASK_H_
#define _TASK_H_

#include "ulidef.h"

typedef struct _BLK_DESC
{
	void *addr;					/*起始地址*/
	DWORD siz;					/*字节数*/
}BLK_DESC;	/*线性地址块描述符*/

#define THED_ATTR_SLEEP		0x0001	/*0:就绪状态1:阻塞状态*/
#define THED_ATTR_WAITMSG	0x0002	/*0:不等待消息1:消息可唤醒*/
#define THED_ATTR_WAITTIME	0x0004	/*0:不等待时钟1:时钟可唤醒*/
#define THED_ATTR_APPS		0x0008	/*0:系统调用态1:应用程序态*/
#define THED_ATTR_DEL		0x0010	/*0:正常状态1:正在被删除*/
#define THED_ATTR_KILLED	0x0020	/*0:正常状态1:被杀死标志*/
#define KSTK_LEN			0x400		/*内核堆栈双字数*/
#define THEDSTK_SIZ			0x00100000	/*线程默认堆栈大小*/
typedef struct _THREAD_DESC
{
	struct _THREAD_DESC	*pre, *nxt;	/*前后指针*/
	WORD par, attr;				/*父线程ID,属性*/
	THREAD_ID id;				/*线程ID*/

	MESSAGE_DESC *msg, *lst;	/*消息链表,最后一项消息指针*/
	DWORD MsgCou;				/*已用消息数*/
	THREAD_ID WaitId;			/*等待线程ID*/
	DWORD WakeupClock;			/*延时唤醒时钟*/

	void *ustk;					/*用户堆栈地址*/
	DWORD UstkSiz;				/*用户堆栈大小*/
	I387 *i387;					/*数学协处理器寄存器结构*/
	TSS tss;					/*任务状态结构*/
	DWORD kstk[KSTK_LEN];		/*内核栈*/
}THREAD_DESC;	/*线程结构*/

#define PROC_ATTR_APPS		0x0001	/*0:驱动进程1:应用进程*/
#define PROC_ATTR_DEL		0x0002	/*0:正常状态1:正在被删除*/
#define TMT_LEN				0x400	/*进程中线程表长度*/
#define UBLKT_LEN			0x400	/*地址块管理表长度*/
#define UFDMT_LEN			64		/*用户自由数据区管理表长度*/
typedef struct _PROCESS_DESC
{
	struct _PROCESS_DESC *pre, *nxt;	/*前后指针*/
	WORD par, attr;				/*父进程ID,属性*/

	THREAD_DESC **FstTmd;		/*首个空线程项指针*/
	THREAD_DESC **EndTmd;		/*末个非空线程项指针*/
	THREAD_DESC *CurTmd;		/*当前线程指针*/
	DWORD TmdCou;				/*已有线程数量*/
	BLK_DESC *FstUBlk;			/*首个空地址块管理表项指针*/
	BLK_DESC *EndUBlk;			/*末个非空地址块管理表项指针*/

	EXEC_DESC *exec;			/*可执行体指针*/
	MAPBLK_DESC *map;			/*映射结构链表*/
	MAPBLK_DESC *map2;			/*被映射结构链表*/
	volatile DWORD MapCou;		/*映射过程锁,记录当前有多少个映射正在进行*/
	volatile DWORD Ufdmt_l;		/*用户自由数据区锁*/
	volatile DWORD Page_l;		/*分页管理锁*/
	void *PageReadAddr;			/*缺页读取地址*/

	THREAD_DESC *tmt[TMT_LEN];	/*线程管理表*/
	BLK_DESC ublkt[UBLKT_LEN];	/*地址块管理表*/
	FREE_BLK_DESC ufdmt[UFDMT_LEN];	/*用户自由数据区管理表*/
}PROCESS_DESC;	/*进程结构*/

/*任务切换*/
void SwitchTS();

/*唤醒线程*/
void wakeup(THREAD_DESC *thed);

/*阻塞线程*/
void sleep(BOOL isWaitMsg, DWORD cs);

/*创建线程*/
long CreateThed(DWORD attr, DWORD proc, DWORD args, THREAD_ID *ptid);

/*删除线程*/
void DeleteThed();

/*杀死线程*/
long KillThed(WORD ThedID);

/*创建进程*/
long CreateProc(DWORD attr, DWORD exec, DWORD args, THREAD_ID *ptid);

/*删除进程*/
void DeleteProc(DWORD ExitCode);

/*杀死进程*/
long KillProc(WORD ProcID);

/*分配用户地址块*/
BLK_DESC *AllocUBlk(PROCESS_DESC *proc, DWORD siz);

/*搜索用户地址块*/
BLK_DESC *FindUBlk(PROCESS_DESC *proc, void *addr);

/*回收用户地址块*/
void FreeUBlk(PROCESS_DESC *proc, BLK_DESC *blk);

/*唤醒线程(关中断方式)*/
static inline void CliWakeup(THREAD_DESC *thed)
{
	cli();
	wakeup(thed);
	sti();
}

/*阻塞线程(关中断方式)*/
static inline void CliSleep(BOOL isWaitMsg, DWORD cs)
{
	cli();
	sleep(isWaitMsg, cs);
	sti();
}

/*分配用户地址块(锁定方式)*/
static inline BLK_DESC *LockAllocUBlk(PROCESS_DESC *proc, DWORD siz)
{
	BLK_DESC *res;
	lock(&proc->Ufdmt_l);
	res = AllocUBlk(proc, siz);
	ulock(&proc->Ufdmt_l);
	return res;
}

/*搜索用户地址块(锁定方式)*/
static inline BLK_DESC *LockFindUBlk(PROCESS_DESC *proc, void *addr)
{
	BLK_DESC *res;
	lock(&proc->Ufdmt_l);
	res = FindUBlk(proc, addr);
	ulock(&proc->Ufdmt_l);
	return res;
}

/*回收用户地址块(锁定方式)*/
static inline void LockFreeUBlk(PROCESS_DESC *proc, BLK_DESC *blk)
{
	lock(&proc->Ufdmt_l);
	FreeUBlk(proc, blk);
	ulock(&proc->Ufdmt_l);
}

/*分配用户自由数据区(锁定方式)*/
static inline void *LockAllocUFData(PROCESS_DESC *proc, DWORD siz)
{
	void *res;
	lock(&proc->Ufdmt_l);
	res = alloc(proc->ufdmt, siz);
	ulock(&proc->Ufdmt_l);
	return res;
}

/*回收用户自由数据区(锁定方式)*/
static inline void LockFreeUFData(PROCESS_DESC *proc, void *addr, DWORD siz)
{
	lock(&proc->Ufdmt_l);
	free(proc->ufdmt, addr, siz);
	ulock(&proc->Ufdmt_l);
}

#endif
