/*	cintr.c for ulios
	作者：孙亮
	功能：C中断/异常/系统调用处理
	最后修改日期：2009-07-01
*/

#ifndef _CINTR_H_
#define _CINTR_H_

#include "ulidef.h"

#define MASTER8259_PORT	0x21	/*主8259端口*/
#define SLAVER8259_PORT	0xA1	/*从8259端口*/

#define IRQN_TIMER		0		/*时钟中断信号*/
#define IRQN_SLAVE8259	2		/*从片8259信号*/

#define INTN_APICALL	0xF0	/*系统调用号*/

/*中断异常系统调用处理函数*/
extern void AsmIsr00();
extern void AsmIsr01();
extern void AsmIsr02();
extern void AsmIsr03();
extern void AsmIsr04();
extern void AsmIsr05();
extern void AsmIsr06();
extern void AsmIsr07();
extern void AsmIsr08();
extern void AsmIsr09();
extern void AsmIsr0A();
extern void AsmIsr0B();
extern void AsmIsr0C();
extern void AsmIsr0D();
extern void AsmIsr0E();
extern void AsmIsr0F();
extern void AsmIsr10();
extern void AsmIsr11();
extern void AsmIsr12();
extern void AsmIsr13();

extern void AsmIrq0();
extern void AsmIrq1();
extern void AsmIrq2();
extern void AsmIrq3();
extern void AsmIrq4();
extern void AsmIrq5();
extern void AsmIrq6();
extern void AsmIrq7();
extern void AsmIrq8();
extern void AsmIrq9();
extern void AsmIrqA();
extern void AsmIrqB();
extern void AsmIrqC();
extern void AsmIrqD();
extern void AsmIrqE();
extern void AsmIrqF();

extern void AsmApiCall();

/*注册IRQ信号的响应线程*/
long RegIrq(DWORD IrqN);

/*注销IRQ信号的响应线程*/
long UnregIrq(DWORD IrqN);

/*注销线程的所有IRQ信号*/
void UnregAllIrq();

/*不可恢复异常处理程序*/
void IsrProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags);

/*浮点协处理器异常处理程序*/
void FpuFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags);

/*所有中断信号的总调函数*/
void IrqProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IrqN);

/*系统调用接口*/
void ApiCall(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, volatile DWORD eax);

/*以下为API接口函数*/

/*取得线程ID*/
void ApiGetPtid(DWORD *argv);

/*主动放弃处理机*/
void ApiGiveUp(DWORD *argv);

/*睡眠*/
void ApiSleep(DWORD *argv);

/*创建线程*/
void ApiCreateThread(DWORD *argv);

/*退出线程*/
void ApiExitThread(DWORD *argv);

/*杀死线程*/
void ApiKillThread(DWORD *argv);

/*创建进程*/
void ApiCreateProcess(DWORD *argv);

/*退出进程*/
void ApiExitProcess(DWORD *argv);

/*杀死进程*/
void ApiKillProcess(DWORD *argv);

/*注册内核端口对应线程*/
void ApiRegKnlPort(DWORD *argv);

/*注销内核端口对应线程*/
void ApiUnregKnlPort(DWORD *argv);

/*取得内核端口对应线程*/
void ApiGetKpToThed(DWORD *argv);

/*注册IRQ信号的响应线程*/
void ApiRegIrq(DWORD *argv);

/*注销IRQ信号的响应线程*/
void ApiUnregIrq(DWORD *argv);

/*发送消息*/
void ApiSendMsg(DWORD *argv);

/*接收消息*/
void ApiRecvMsg(DWORD *argv);

/*接收指定进程的消息*/
void ApiRecvProcMsg(DWORD *argv);

/*映射物理地址*/
void ApiMapPhyAddr(DWORD *argv);

/*映射用户地址*/
void ApiMapUserAddr(DWORD *argv);

/*回收用户地址块*/
void ApiFreeAddr(DWORD *argv);

/*映射进程地址读取*/
void ApiReadProcAddr(DWORD *argv);

/*映射进程地址写入*/
void ApiWriteProcAddr(DWORD *argv);

/*撤销映射进程地址*/
void ApiUnmapProcAddr(DWORD *argv);

/*取消映射进程地址*/
void ApiCnlmapProcAddr(DWORD *argv);

/*取得开机经过的时钟*/
void ApiGetClock(DWORD *argv);

/*线程同步锁操作*/
void ApiLock(DWORD *argv);

#endif
