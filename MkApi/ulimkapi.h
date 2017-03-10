/*	ulimkapi.h for ulios program
	作者：孙亮
	功能：ulios微内核API接口定义，开发应用程序需要包含此文件
	最后修改日期：2009-07-28
*/

#ifndef _ULIMKAPI_H_
#define _ULIMKAPI_H_

/**********数据类型**********/
typedef unsigned char		BYTE;	/*8位*/
typedef unsigned short		WORD;	/*16位*/
typedef unsigned long		DWORD;	/*32位*/
typedef unsigned long		BOOL;
typedef unsigned long long	QWORD;	/*64位*/
typedef long long			SQWORD;	/*有符号64位*/

typedef struct _THREAD_ID
{
	WORD ProcID;
	WORD ThedID;
}THREAD_ID;	/*进程线程ID*/

/**********常量**********/
#define TRUE	1
#define FALSE	0
#define NULL	((void *)0)
#define INVALID	(~0)

/**********错误代码**********/
#define NO_ERROR					0	/*无错*/

/*内核资源错误*/
#define KERR_OUT_OF_KNLMEM			-1	/*内核内存不足*/
#define KERR_OUT_OF_PHYMEM			-2	/*物理内存不足*/
#define KERR_OUT_OF_LINEADDR		-3	/*线性地址空间不足*/

/*进程线程错误*/
#define KERR_INVALID_PROCID			-4	/*非法进程ID*/
#define KERR_PROC_NOT_EXIST			-5	/*进程不存在*/
#define KERR_PROC_NOT_ENOUGH		-6	/*进程管理结构不足*/
#define KERR_PROC_KILL_SELF			-7	/*杀死进程自身*/
#define KERR_INVALID_THEDID			-8	/*非法线程ID*/
#define KERR_THED_NOT_EXIST			-9	/*线程不存在*/
#define KERR_THED_NOT_ENOUGH		-10	/*线程管理结构不足*/
#define KERR_THED_KILL_SELF			-11	/*杀死线程自身*/

/*内核注册表错误*/
#define KERR_INVALID_KPTNUN			-12	/*非法内核端口号*/
#define KERR_KPT_ALREADY_REGISTERED	-13	/*内核端口已被注册*/
#define KERR_KPT_NOT_REGISTERED		-14	/*内核端口未被注册*/
#define KERR_INVALID_IRQNUM			-15	/*非法IRQ号*/
#define KERR_IRQ_ALREADY_REGISTERED	-16	/*IRQ已被注册*/
#define KERR_IRQ_NOT_REGISTERED		-17	/*IRQ未被注册*/
#define KERR_CURPROC_NOT_REGISTRANT	-18	/*当前进程不是注册者*/

/*消息错误*/
#define KERR_INVALID_USERMSG_ATTR	-19	/*非法用户消息属性*/
#define KERR_MSG_NOT_ENOUGH			-20	/*消息结构不足*/
#define KERR_MSG_QUEUE_FULL			-21	/*消息队列满*/
#define KERR_MSG_QUEUE_EMPTY		-22	/*消息队列空*/

/*地址映射错误*/
#define KERR_MAPSIZE_IS_ZERO		-23	/*映射长度为0*/
#define KERR_MAPSIZE_TOO_LONG		-24	/*映射长度太大*/
#define KERR_PROC_SELF_MAPED		-25	/*映射进程自身*/
#define KERR_PAGE_ALREADY_MAPED		-26	/*目标进程页面已经被映射*/
#define KERR_ILLEGAL_PHYADDR_MAPED	-27	/*映射不允许的物理地址*/
#define KERR_ADDRARGS_NOT_FOUND		-28	/*地址参数未找到*/

/*程序执行错误*/
#define KERR_OUT_OF_TIME			-29	/*超时错误*/
#define KERR_ACCESS_ILLEGAL_ADDR	-30	/*访问非法地址*/
#define KERR_WRITE_RDONLY_ADDR		-31	/*写只读地址*/
#define KERR_THED_EXCEPTION			-32	/*线程执行异常*/
#define KERR_THED_KILLED			-33	/*线程被杀死*/

/*调用错误*/
#define KERR_INVALID_APINUM			-34	/*非法系统调用号*/
#define KERR_ARGS_TOO_LONG			-35	/*参数字串超长*/
#define KERR_INVALID_MEMARGS_ADDR	-36	/*非法内存参数地址*/
#define KERR_NO_DRIVER_PRIVILEGE	-37	/*没有执行驱动功能的特权*/

/**********消息定义**********/
#define MSG_DATA_LEN		8			/*消息数据双字数*/

#define MSG_ATTR_MASK		0xFFFF0000	/*消息数据属性字掩码*/
#define MSG_API_MASK		0x0000FFFF	/*建议:消息数据服务功能号掩码*/
#define MSG_MAP_MASK		0xFFFE0000	/*页映射消息属性掩码*/

#define MSG_ATTR_ID			0			/*消息数据属性字索引*/
#define MSG_API_ID			0			/*建议:消息数据服务功能号索引*/
#define MSG_ADDR_ID			1			/*消息数据地址字索引*/
#define MSG_SIZE_ID			2			/*消息数据字节数索引*/
#define MSG_RES_ID			7			/*建议:消息数据服务返回结果索引*/

#define MSG_ATTR_ISR		0x00010000	/*硬件陷阱消息*/
#define MSG_ATTR_IRQ		0x00020000	/*硬件中断消息*/
#define MSG_ATTR_THEDEXIT	0x00030000	/*线程退出消息*/
#define MSG_ATTR_PROCEXIT	0x00040000	/*进程退出消息*/
#define MSG_ATTR_EXCEP		0x00050000	/*异常退出消息*/
#define MSG_ATTR_ROMAP		0x00060000	/*页只读映射消息*/
#define MSG_ATTR_RWMAP		0x00070000	/*页读写映射消息*/
#define MSG_ATTR_UNMAP		0x00080000	/*解除页映射消息*/
#define MSG_ATTR_CNLMAP		0x00090000	/*取消页映射消息*/

#define MSG_ATTR_USER		0x01000000	/*用户自定义消息最小值*/
#define MSG_ATTR_EXTTHEDREQ	0x01000001	/*建议:退出线程请求*/
#define MSG_ATTR_EXTPROCREQ	0x01000002	/*建议:退出进程请求*/

#define EXEC_ATTR_DRIVER	0x00000002	/*KCreateProcess的attr参数,启动一个具有驱动权限的进程*/

/**********系统调用接口**********/

/*取得线程ID*/
static inline long KGetPtid(THREAD_ID *ptid, DWORD *ParThreadID, DWORD *ParProcessID)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid), "=c"(*ParThreadID), "=d"(*ParProcessID): "0"(0x000000));
	return res;
}

/*主动放弃处理机*/
static inline long KGiveUp()
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x010000));
	return res;
}

/*睡眠*/
static inline long KSleep(DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x020000), "b"(CentiSeconds));
	return res;
}

/*创建线程*/
static inline long KCreateThread(void (*ThreadProc)(void *data), DWORD StackSize, void *data, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x030000), "1"(ThreadProc), "c"(StackSize), "d"(data));
	return res;
}

/*退出线程*/
static inline void KExitThread(long ExitCode)
{
	__asm__ __volatile__("int $0xF0":: "a"(0x040000), "b"(ExitCode));
}

/*杀死线程*/
static inline long KKillThread(DWORD ThreadID)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x050000), "b"(ThreadID));
	return res;
}

/*创建进程*/
static inline long KCreateProcess(DWORD attr, const char *exec, const char *args, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x060000), "1"(attr), "D"(exec), "S"(args));
	return res;
}

/*退出进程*/
static inline void KExitProcess(long ExitCode)
{
	__asm__ __volatile__("int $0xF0":: "a"(0x070000), "b"(ExitCode));
}

/*杀死进程*/
static inline long KKillProcess(DWORD ProcessID)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x080000), "b"(ProcessID));
	return res;
}

/*注册内核端口对应线程*/
static inline long KRegKnlPort(DWORD KnlPort)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x090000), "b"(KnlPort));
	return res;
}

/*注销内核端口对应线程*/
static inline long KUnregKnlPort(DWORD KnlPort)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0A0000), "b"(KnlPort));
	return res;
}

/*取得内核端口对应线程*/
static inline long KGetKptThed(DWORD KnlPort, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x0B0000), "1"(KnlPort));
	return res;
}

/*注册IRQ信号的响应线程*/
static inline long KRegIrq(DWORD irq)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0C0000), "b"(irq));
	return res;
}

/*注销IRQ信号的响应线程*/
static inline long KUnregIrq(DWORD irq)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0D0000), "b"(irq));
	return res;
}

/*发送消息*/
static inline long KSendMsg(THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x0E0000), "1"(*ptid), "c"(CentiSeconds), "S"(data): "memory");
	return res;
}

/*接收消息*/
static inline long KRecvMsg(THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x0F0000), "c"(CentiSeconds), "S"(data): "memory");
	return res;
}

/*接收指定进程的消息*/
static inline long KRecvProcMsg(THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x100000), "1"(*ptid), "c"(CentiSeconds), "S"(data): "memory");
	return res;
}

/*映射物理地址*/
static inline long KMapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=S"(*addr): "0"(0x110000), "b"(PhyAddr), "c"(siz): "memory");
	return res;
}

/*映射用户地址*/
static inline long KMapUserAddr(void **addr, DWORD siz)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=S"(*addr): "0"(0x120000), "c"(siz));
	return res;
}

/*回收用户地址块*/
static inline long KFreeAddr(void *addr)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x130000), "S"(addr): "memory");
	return res;
}

/*映射进程地址写入*/
static inline long KWriteProcAddr(const void *addr, DWORD siz, THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x140000), "1"(*ptid), "c"(siz), "d"(CentiSeconds), "S"(data), "D"(addr): "memory");
	return res;
}

/*映射进程地址读取*/
static inline long KReadProcAddr(void *addr, DWORD siz, THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x150000), "1"(*ptid), "c"(siz), "d"(CentiSeconds), "S"(data), "D"(addr): "memory");
	return res;
}

/*撤销映射进程地址*/
static inline long KUnmapProcAddr(void *addr, const DWORD data[MSG_DATA_LEN])
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x160000), "S"(data), "D"(addr): "memory");
	return res;
}

/*取消映射进程地址*/
static inline long KCnlmapProcAddr(void *addr, const DWORD data[MSG_DATA_LEN])
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x170000), "S"(data), "D"(addr): "memory");
	return res;
}

/*取得开机经过的时钟*/
static inline long KGetClock(DWORD *clock)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*clock): "0"(0x180000));
	return res;
}

/*线程同步锁操作*/
static inline long KLock(volatile DWORD *addr, DWORD val, DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x190000), "b"(CentiSeconds), "c"(val), "S"(addr): "memory");
	return res;
}

/*线程同步解锁操作*/
static inline void KUlock(volatile DWORD *addr)
{
	*addr = FALSE;
}

/**********通用操作**********/

/*内存设置*/
static inline void memset8(void *dest, BYTE b, DWORD n)
{
	void *_dest;
	DWORD _n;
	__asm__ __volatile__("cld;rep stosb": "=&D"(_dest), "=&c"(_n): "0"(dest), "a"(b), "1"(n): "flags", "memory");
}

/*内存复制*/
static inline void memcpy8(void *dest, const void *src, DWORD n)
{
	void *_dest;
	const void *_src;
	DWORD _n;
	__asm__ __volatile__("cld;rep movsb": "=&D"(_dest), "=&S"(_src), "=&c"(_n): "0"(dest), "1"(src), "2"(n): "flags", "memory");
}

/*32位内存设置*/
static inline void memset32(void *dest, DWORD d, DWORD n)
{
	void *_dest;
	DWORD _n;
	__asm__ __volatile__("cld;rep stosl": "=&D"(_dest), "=&c"(_n): "0"(dest), "a"(d), "1"(n): "flags", "memory");
}

/*32位内存复制*/
static inline void memcpy32(void *dest, const void *src, DWORD n)
{
	void *_dest;
	const void *_src;
	DWORD _n;
	__asm__ __volatile__("cld;rep movsl": "=&D"(_dest), "=&S"(_src), "=&c"(_n): "0"(dest), "1"(src), "2"(n): "flags", "memory");
}

/*字符串长度*/
static inline DWORD strlen(const char *str)
{
	DWORD d0;
	register DWORD _res;
	__asm__ __volatile__
	(
		"cld\n"
		"repne\n"
		"scasb\n"
		"notl %0\n"
		"decl %0"
		: "=c"(_res), "=&D"(d0): "1"(str), "a"(0), "0"(0xFFFFFFFFU): "flags"
	);
	return _res;
}

/*字符串复制*/
static inline char *strcpy(char *dest, const char *src)
{
	char *_dest;
	const char *_src;
	__asm__ __volatile__
	(
		"cld\n"
		"1:\tlodsb\n"
		"stosb\n"
		"testb %%al, %%al\n"
		"jne 1b"
		: "=&D"(_dest), "=&S"(_src): "0"(dest), "1"(src): "flags", "al", "memory"
	);
	return _dest;
}

/*字符串限量复制*/
static inline void strncpy(char *dest, const char *src, DWORD n)
{
	char *_dest;
	const char *_src;
	DWORD _n;
	__asm__ __volatile__
	(
		"cld\n"
		"1:\tdecl %2\n"
		"js 2f\n"
		"lodsb\n"
		"stosb\n"
		"testb %%al, %%al\n"
		"jne 1b\n"
		"rep stosb\n"
		"2:"
		: "=&D"(_dest), "=&S"(_src), "=&c"(_n): "0"(dest), "1"(src), "2"(n): "flags", "al", "memory"
	);
}

/*端口输出字节(驱动专用)*/
static inline void outb(WORD port, BYTE b)
{
	__asm__ __volatile__("outb %1, %w0":: "d"(port), "a"(b));
}

/*端口输入字节(驱动专用)*/
static inline BYTE inb(WORD port)
{
	register BYTE b;
	__asm__ __volatile__("inb %w1, %0": "=a"(b): "d"(port));
	return b;
}

/*关中断(驱动专用)*/
static inline void cli()
{
	__asm__("cli");
}

/*开中断(驱动专用)*/
static inline void sti()
{
	__asm__("sti");
}

/*锁变量锁定(驱动专用)*/
static inline void lock(volatile DWORD *l)
{
	cli();
	while (*l)
		KGiveUp();
	*l = TRUE;
	sti();
}

/*锁变量解锁(驱动专用)*/
static inline void ulock(volatile DWORD *l)
{
	*l = FALSE;
}

/*发送退出线程请求*/
static inline long SendExitThedReq(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_ATTR_ID] = MSG_ATTR_EXTTHEDREQ;
	return KSendMsg(&ptid, data, 0);
}

/*发送退出进程请求*/
static inline long SendExitProcReq(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_ATTR_ID] = MSG_ATTR_EXTPROCREQ;
	return KSendMsg(&ptid, data, 0);
}

#endif
