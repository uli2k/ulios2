/*	cintr.c for ulios
	作者：孙亮
	功能：C中断/异常/系统调用处理
	最后修改日期：2009-07-01
*/

#include "knldef.h"

/*C异常表*/
void (*IsrCallTable[])(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags) = {
	IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, FpuFaultProc,
	IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, PageFaultProc, IsrProc,
	IsrProc, IsrProc, IsrProc, IsrProc
};

/*中断表*/
void (*AsmIrqCallTable[])() = {
	AsmIrq0, AsmIrq1, AsmIrq2, AsmIrq3, AsmIrq4, AsmIrq5, AsmIrq6, AsmIrq7,
	AsmIrq8, AsmIrq9, AsmIrqA, AsmIrqB, AsmIrqC, AsmIrqD, AsmIrqE, AsmIrqF
};

/*C系统调用表*/
void (*ApiCallTable[])(DWORD *argv) = {
	ApiGetPtid, ApiGiveUp, ApiSleep, ApiCreateThread, ApiExitThread, ApiKillThread, ApiCreateProcess, ApiExitProcess,
	ApiKillProcess, ApiRegKnlPort, ApiUnregKnlPort, ApiGetKpToThed, ApiRegIrq, ApiUnregIrq, ApiSendMsg, ApiRecvMsg,
	ApiRecvProcMsg, ApiMapPhyAddr, ApiMapUserAddr, ApiFreeAddr, ApiWriteProcAddr, ApiReadProcAddr, ApiUnmapProcAddr, ApiCnlmapProcAddr,
	ApiGetClock, ApiLock
};

/*注册IRQ信号的响应线程*/
long RegIrq(DWORD IrqN)
{
	PROCESS_DESC *CurProc;
	BYTE mask;

	CurProc = CurPmd;
	if (CurProc->attr & PROC_ATTR_APPS)
		return KERR_NO_DRIVER_PRIVILEGE;	/*驱动进程特权API*/
	if (IrqN >= IRQ_LEN)
		return KERR_INVALID_IRQNUM;
	cli();
	if (idt[0x20 + IrqN].d1)
	{
		sti();
		return KERR_IRQ_ALREADY_REGISTERED;
	}
	SetGate(&idt[0x20 + IrqN], KCODE_SEL, (DWORD)AsmIrqCallTable[IrqN], DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	IrqPort[IrqN] = CurProc->CurTmd->id;
	if (IrqN < 8)
	{
		mask = inb(MASTER8259_PORT);	/*主片*/
		mask &= (~(1ul << IrqN));
		outb(MASTER8259_PORT, mask);
	}
	else
	{
		mask = inb(SLAVER8259_PORT);	/*从片*/
		mask &= (~(1ul << (IrqN & 7)));
		outb(SLAVER8259_PORT, mask);
	}
	sti();
	return NO_ERROR;
}

/*注销IRQ信号的响应线程*/
long UnregIrq(DWORD IrqN)
{
	PROCESS_DESC *CurProc;
	BYTE mask;

	CurProc = CurPmd;
	if (CurProc->attr & PROC_ATTR_APPS)
		return KERR_NO_DRIVER_PRIVILEGE;	/*驱动进程特权API*/
	if (IrqN >= IRQ_LEN)
		return KERR_INVALID_IRQNUM;
	cli();
	if (idt[0x20 + IrqN].d1 == 0)
	{
		sti();
		return KERR_IRQ_NOT_REGISTERED;
	}
	if (IrqPort[IrqN].ProcID != CurProc->CurTmd->id.ProcID)
	{
		sti();
		return KERR_CURPROC_NOT_REGISTRANT;
	}
	if (IrqN < 8)
	{
		mask = inb(MASTER8259_PORT);	/*主片*/
		mask |= (1ul << IrqN);
		outb(MASTER8259_PORT, mask);
	}
	else
	{
		mask = inb(SLAVER8259_PORT);	/*从片*/
		mask |= (1ul << (IrqN & 7));
		outb(SLAVER8259_PORT, mask);
	}
	*(DWORD*)(&IrqPort[IrqN]) = INVALID;
	idt[0x20 + IrqN].d1 = 0;
	sti();
	return NO_ERROR;
}

/*注销线程的所有IRQ信号*/
void UnregAllIrq()
{
	PROCESS_DESC *CurProc;
	THREAD_ID ptid;
	DWORD i;
	BYTE mask;

	CurProc = CurPmd;
	if (CurProc->attr & PROC_ATTR_APPS)
		return;	/*驱动进程特权API*/
	ptid = CurProc->CurTmd->id;
	cli();
	for (i = 0; i < IRQ_LEN; i++)
		if (*(DWORD*)(&IrqPort[i]) == *(DWORD*)(&ptid))
		{
			if (i < 8)
			{
				mask = inb(MASTER8259_PORT);	/*主片*/
				mask |= (1ul << i);
				outb(MASTER8259_PORT, mask);
			}
			else
			{
				mask = inb(SLAVER8259_PORT);	/*从片*/
				mask |= (1ul << (i & 7));
				outb(SLAVER8259_PORT, mask);
			}
			*(DWORD*)(&IrqPort[i]) = INVALID;
			idt[0x20 + i].d1 = 0;
		}
	sti();
}

/*不可恢复异常处理程序*/
void IsrProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags)
{
	MESSAGE_DESC *msg;

	if ((msg = AllocMsg()) != NULL)	/*通知报告服务器陷阱消息*/
	{
		msg->ptid = kpt[REP_KPORT];
		msg->data[MSG_ATTR_ID] = MSG_ATTR_ISR;
		msg->data[1] = IsrN;
		msg->data[2] = ErrCode;
		msg->data[3] = eip;
		if (SendMsg(msg) != NO_ERROR)
			FreeMsg(msg);
	}
	ThedExit(KERR_THED_EXCEPTION);
}

/*浮点协处理器异常处理程序*/
void FpuFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags)
{
	THREAD_DESC *CurThed;
	I387 *CurI387;

	CurThed = CurPmd->CurTmd;
	CurI387 = CurThed->i387;
	CurThed->attr &= (~THED_ATTR_APPS);	/*进入系统调用态*/
	if (CurI387 == NULL)	/*线程首次执行协处理器指令*/
	{
		if ((CurI387 = (I387*)LockKmalloc(sizeof(I387))) == NULL)
			ThedExit(KERR_OUT_OF_KNLMEM);
	}
	cli();
	ClearTs();
	if (LastI387 != CurI387)	/*使用协处理器的线程不变时不必切换*/
	{
		if (LastI387)
			__asm__("fwait;fnsave %0": "=m"(*LastI387));	/*保存协处理器寄存器*/
		if (CurThed->i387)	/*协处理器已经可用*/
			__asm__("frstor %0":: "m"(*CurI387));	/*加载协处理器寄存器*/
		else
		{
			__asm__("fninit");	/*初始化协处理器*/
			CurThed->i387 = CurI387;
		}
		LastI387 = CurI387;
	}
	sti();
	CurThed->attr |= THED_ATTR_APPS;	/*离开系统调用态*/
}

/*所有中断信号的总调函数*/
void IrqProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IrqN)
{
	THREAD_DESC *CurThed;

	/*进入中断处理程序以前中断已经关闭*/
	if (IrqN == 0)
	{
		clock++;
		if (SleepList && clock >= SleepList->WakeupClock)	/*延时阻塞链表中有已经超时的线程*/
			wakeup(SleepList);
		else
		{
			schedul();
			CurThed = CurPmd ? CurPmd->CurTmd : NULL;
			if (CurThed && (CurThed->attr & (THED_ATTR_APPS | THED_ATTR_KILLED)) == (THED_ATTR_APPS | THED_ATTR_KILLED))	/*线程在应用态下被杀死*/
			{
				CurThed->attr &= (~THED_ATTR_KILLED);
				sti();
				ThedExit(KERR_THED_KILLED);
			}
		}
	}
	else	/*给中断处理进程发送消息*/
	{
		MESSAGE_DESC *msg;

		sti();
		CurThed = CurPmd ? CurPmd->CurTmd : NULL;
		if (CurThed)
			CurThed->attr &= (~THED_ATTR_APPS);	/*进入系统调用态*/
		if ((msg = AllocMsg()) == NULL)	/*内存不足*/
		{
			if (CurThed)
				CurThed->attr |= THED_ATTR_APPS;	/*离开系统调用态*/
			return;
		}
		msg->ptid = IrqPort[IrqN];
		msg->data[MSG_ATTR_ID] = MSG_ATTR_IRQ;
		msg->data[1] = IrqN;
		if (SendMsg(msg) != NO_ERROR)
			FreeMsg(msg);
		if (CurThed)
			CurThed->attr |= THED_ATTR_APPS;	/*离开系统调用态*/
	}
}

/*系统调用接口*/
void ApiCall(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, volatile DWORD eax)
{
	THREAD_DESC *CurThed;

	CurThed = CurPmd->CurTmd;
	if (CurThed->attr & THED_ATTR_KILLED)	/*线程已被杀死*/
	{
		CurThed->attr &= (~THED_ATTR_KILLED);
		sti();
		ThedExit(KERR_THED_KILLED);
	}
	if (eax >= ((sizeof(ApiCallTable) / sizeof(void*)) << 16))
	{
		eax = KERR_INVALID_APINUM;
		return;
	}
	CurThed->attr &= (~THED_ATTR_APPS);	/*进入系统调用态*/
	ApiCallTable[eax >> 16](&edi);
	CurThed->attr |= THED_ATTR_APPS;	/*离开系统调用态*/
}

/*以下为API接口函数实现*/
#define EDI_ID	0
#define ESI_ID	1
#define EBP_ID	2
#define ESP_ID	3
#define EBX_ID	4
#define EDX_ID	5
#define ECX_ID	6
#define EAX_ID	7

/*取得线程ID*/
void ApiGetPtid(DWORD *argv)
{
	THREAD_DESC *CurThed;

	CurThed = CurPmd->CurTmd;
	argv[EBX_ID] = *(DWORD*)(&CurThed->id);
	argv[ECX_ID] = CurThed->par;
	argv[EDX_ID] = CurPmd->par;
	argv[EAX_ID] = NO_ERROR;
}

/*主动放弃处理机*/
void ApiGiveUp(DWORD *argv)
{
	cli();
	schedul();
	sti();
	argv[EAX_ID] = NO_ERROR;
}

/*睡眠*/
void ApiSleep(DWORD *argv)
{
	if (argv[EBX_ID])
		CliSleep(FALSE, argv[EBX_ID]);
	argv[EAX_ID] = NO_ERROR;
}

/*创建线程*/
void ApiCreateThread(DWORD *argv)
{
	argv[EAX_ID] = CreateThed(argv[EBX_ID], argv[ECX_ID], argv[EDX_ID], (THREAD_ID*)&argv[EBX_ID]);
}

/*退出线程*/
void ApiExitThread(DWORD *argv)
{
	ThedExit(argv[EBX_ID]);
}

/*杀死线程*/
void ApiKillThread(DWORD *argv)
{
	argv[EAX_ID] = KillThed(argv[EBX_ID]);
}

/*创建进程*/
void ApiCreateProcess(DWORD *argv)
{
	BYTE *addr;

	addr = (BYTE*)argv[ESI_ID];
	if (addr >= (BYTE*)UADDR_OFF && addr <= (BYTE*)(0 - PROC_ARGS_SIZE))
	{
		DWORD siz;

		for (siz = 0; *addr; addr++)
			if (++siz >= PROC_ARGS_SIZE)
			{
				argv[EAX_ID] = KERR_ARGS_TOO_LONG;
				return;
			}
		addr = (BYTE*)argv[ESI_ID];
		CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*防止访问用户内存时发生页异常,重新进入系统调用态*/
	}
	else
		addr = NULL;
	argv[EAX_ID] = CreateProc(argv[EBX_ID] & (~EXEC_ARGV_BASESRV), argv[EDI_ID], (DWORD)addr, (THREAD_ID*)&argv[EBX_ID]);
}

/*退出进程*/
void ApiExitProcess(DWORD *argv)
{
	DeleteProc(argv[EBX_ID]);
}

/*杀死进程*/
void ApiKillProcess(DWORD *argv)
{
	argv[EAX_ID] = KillProc(argv[EBX_ID]);
}

/*注册内核端口对应线程*/
void ApiRegKnlPort(DWORD *argv)
{
	argv[EAX_ID] = RegKnlPort(argv[EBX_ID]);
}

/*注销内核端口对应线程*/
void ApiUnregKnlPort(DWORD *argv)
{
	argv[EAX_ID] = UnregKnlPort(argv[EBX_ID]);
}

/*取得内核端口对应线程*/
void ApiGetKpToThed(DWORD *argv)
{
	argv[EAX_ID] = GetKptThed(argv[EBX_ID], (THREAD_ID*)&argv[EBX_ID]);
}

/*注册IRQ信号的响应线程*/
void ApiRegIrq(DWORD *argv)
{
	argv[EAX_ID] = RegIrq(argv[EBX_ID]);
}

/*注销IRQ信号的响应线程*/
void ApiUnregIrq(DWORD *argv)
{
	argv[EAX_ID] = UnregIrq(argv[EBX_ID]);
}

/*发送消息*/
void ApiSendMsg(DWORD *argv)
{
	MESSAGE_DESC *msg;
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
	{
		argv[EAX_ID] = KERR_INVALID_MEMARGS_ADDR;
		return;
	}
	memcpy32(data, addr, MSG_DATA_LEN);	/*复制数据到内核空间*/
	CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*防止访问用户内存时发生页异常,重新进入系统调用态*/
	if (data[MSG_ATTR_ID] < MSG_ATTR_USER)
	{
		argv[EAX_ID] = KERR_INVALID_USERMSG_ATTR;
		return;
	}
	if ((msg = AllocMsg()) == NULL)
	{
		argv[EAX_ID] = KERR_MSG_NOT_ENOUGH;
		return;
	}
	if (((THREAD_ID*)&argv[EBX_ID])->ThedID == 0xFFFF && ((THREAD_ID*)&argv[EBX_ID])->ProcID < KPT_LEN)	/*处理内核端口参数*/
		argv[EBX_ID] = *((DWORD*)&kpt[((THREAD_ID*)&argv[EBX_ID])->ProcID]);
	msg->ptid = *((THREAD_ID*)&argv[EBX_ID]);
	memcpy32(msg->data, data, MSG_DATA_LEN);
	argv[EAX_ID] = SendMsg(msg);
	if (argv[EAX_ID] != NO_ERROR)
		FreeMsg(msg);
	else if (argv[ECX_ID] && (argv[EAX_ID] = RecvProcMsg(&msg, *((THREAD_ID*)&argv[EBX_ID]), argv[ECX_ID])) == NO_ERROR)	/*等待返回消息*/
	{
		argv[EBX_ID] = *((DWORD*)&msg->ptid);
		memcpy32(data, msg->data, MSG_DATA_LEN);
		FreeMsg(msg);
		memcpy32(addr, data, MSG_DATA_LEN);	/*复制数据到用户空间*/
	}
}

/*接收消息*/
void ApiRecvMsg(DWORD *argv)
{
	MESSAGE_DESC *msg;
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
	{
		argv[EAX_ID] = KERR_INVALID_MEMARGS_ADDR;
		return;
	}
	if ((argv[EAX_ID] = RecvMsg(&msg, argv[ECX_ID])) == NO_ERROR)
	{
		argv[EBX_ID] = *((DWORD*)&msg->ptid);
		memcpy32(data, msg->data, MSG_DATA_LEN);
		FreeMsg(msg);
		memcpy32(addr, data, MSG_DATA_LEN);	/*复制数据到用户空间*/
	}
}

/*接收指定进程的消息*/
void ApiRecvProcMsg(DWORD *argv)
{
	MESSAGE_DESC *msg;
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
	{
		argv[EAX_ID] = KERR_INVALID_MEMARGS_ADDR;
		return;
	}
	if (((THREAD_ID*)&argv[EBX_ID])->ThedID == 0xFFFF && ((THREAD_ID*)&argv[EBX_ID])->ProcID < KPT_LEN)	/*处理内核端口参数*/
		argv[EBX_ID] = *((DWORD*)&kpt[((THREAD_ID*)&argv[EBX_ID])->ProcID]);
	if ((argv[EAX_ID] = RecvProcMsg(&msg, *((THREAD_ID*)&argv[EBX_ID]), argv[ECX_ID])) == NO_ERROR)
	{
		argv[EBX_ID] = *((DWORD*)&msg->ptid);
		memcpy32(data, msg->data, MSG_DATA_LEN);
		FreeMsg(msg);
		memcpy32(addr, data, MSG_DATA_LEN);	/*复制数据到用户空间*/
	}
}

/*映射物理地址*/
void ApiMapPhyAddr(DWORD *argv)
{
	argv[EAX_ID] = MapPhyAddr((void**)&argv[ESI_ID], argv[EBX_ID], argv[ECX_ID]);
}

/*映射用户地址*/
void ApiMapUserAddr(DWORD *argv)
{
	argv[EAX_ID] = MapUserAddr((void**)&argv[ESI_ID], argv[ECX_ID]);
}

/*回收用户地址块*/
void ApiFreeAddr(DWORD *argv)
{
	argv[EAX_ID] = UnmapAddr((void*)argv[ESI_ID]);
}

/*映射进程地址写入*/
void ApiWriteProcAddr(DWORD *argv)
{
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
	{
		argv[EAX_ID] = KERR_INVALID_MEMARGS_ADDR;
		return;
	}
	memcpy32(data, addr, MSG_DATA_LEN);	/*复制数据到内核空间*/
	CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*防止访问用户内存时发生页异常,重新进入系统调用态*/
	if (((THREAD_ID*)&argv[EBX_ID])->ThedID == 0xFFFF && ((THREAD_ID*)&argv[EBX_ID])->ProcID < KPT_LEN)	/*处理内核端口参数*/
		argv[EBX_ID] = *((DWORD*)&kpt[((THREAD_ID*)&argv[EBX_ID])->ProcID]);
	if ((argv[EAX_ID] = MapProcAddr((void*)argv[EDI_ID], argv[ECX_ID], (THREAD_ID*)&argv[EBX_ID], FALSE, TRUE, data, argv[EDX_ID])) == NO_ERROR)
		memcpy32(addr, data, MSG_DATA_LEN);	/*复制数据到用户空间*/
}

/*映射进程地址读取*/
void ApiReadProcAddr(DWORD *argv)
{
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
	{
		argv[EAX_ID] = KERR_INVALID_MEMARGS_ADDR;
		return;
	}
	memcpy32(data, addr, MSG_DATA_LEN);	/*复制数据到内核空间*/
	CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*防止访问用户内存时发生页异常,重新进入系统调用态*/
	if (((THREAD_ID*)&argv[EBX_ID])->ThedID == 0xFFFF && ((THREAD_ID*)&argv[EBX_ID])->ProcID < KPT_LEN)	/*处理内核端口参数*/
		argv[EBX_ID] = *((DWORD*)&kpt[((THREAD_ID*)&argv[EBX_ID])->ProcID]);
	if ((argv[EAX_ID] = MapProcAddr((void*)argv[EDI_ID], argv[ECX_ID], (THREAD_ID*)&argv[EBX_ID], TRUE, TRUE, data, argv[EDX_ID])) == NO_ERROR)
		memcpy32(addr, data, MSG_DATA_LEN);	/*复制数据到用户空间*/
}

/*撤销映射进程地址*/
void ApiUnmapProcAddr(DWORD *argv)
{
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
	{
		argv[EAX_ID] = KERR_INVALID_MEMARGS_ADDR;
		return;
	}
	memcpy32(data, addr, MSG_DATA_LEN);	/*复制数据到内核空间*/
	CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*防止访问用户内存时发生页异常,重新进入系统调用态*/
	argv[EAX_ID] = UnmapProcAddr((void*)argv[EDI_ID], data);
}

/*取消映射进程地址*/
void ApiCnlmapProcAddr(DWORD *argv)
{
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
	{
		argv[EAX_ID] = KERR_INVALID_MEMARGS_ADDR;
		return;
	}
	memcpy32(data, addr, MSG_DATA_LEN);	/*复制数据到内核空间*/
	CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*防止访问用户内存时发生页异常,重新进入系统调用态*/
	argv[EAX_ID] = CnlmapProcAddr((void*)argv[EDI_ID], data);
}

/*取得开机经过的时钟*/
void ApiGetClock(DWORD *argv)
{
	argv[EBX_ID] = clock;
	argv[EAX_ID] = NO_ERROR;
}

/*线程同步锁操作*/
void ApiLock(DWORD *argv)
{
	void *addr;
	DWORD OutClock;

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(DWORD)))
	{
		argv[EAX_ID] = KERR_INVALID_MEMARGS_ADDR;
		return;
	}
	argv[EDX_ID] = *((volatile DWORD*)addr);	/*提前读锁变量*/
	CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*防止访问用户内存时发生页异常,重新进入系统调用态*/
	OutClock = (argv[EBX_ID] == INVALID ? INVALID : clock + argv[EBX_ID]);
	cli();
	while (*((volatile DWORD*)addr))
	{
		if (clock >= OutClock)
		{
			sti();
			argv[EAX_ID] = KERR_OUT_OF_TIME;
			return;
		}
		schedul();
	}
	*((volatile DWORD*)addr) = argv[ECX_ID];
	sti();
	argv[EAX_ID] = NO_ERROR;
}
