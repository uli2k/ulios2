/*	x86cpu.h for ulios
	作者：孙亮
	功能：Intel Pentium CPU 相关结构定义
	最后修改日期：2009-05-26
*/

#ifndef	_X86CPU_H_
#define	_X86CPU_H_

#include "ulidef.h"

/*EFLAGS标志位定义*/
#define EFLAGS_CF		0x00000001
#define EFLAGS_PF		0x00000004
#define EFLAGS_AF		0x00000010
#define EFLAGS_ZF		0x00000040
#define EFLAGS_SF		0x00000080
#define EFLAGS_TF		0x00000100	/*陷阱标志*/
#define EFLAGS_IF		0x00000200	/*中断启用标志*/
#define EFLAGS_DF		0x00000400	/**/
#define EFLAGS_OF		0x00000800	/**/
#define EFLAGS_IOPL		0x00003000	/*I/O特权级*/
#define EFLAGS_NT		0x00004000	/*嵌套任务标志*/
#define EFLAGS_RF		0x00010000	/*恢复标志*/
#define EFLAGS_VM		0x00020000	/*虚拟8086模式*/
#define EFLAGS_AC		0x00040000	/*对齐检查*/
#define EFLAGS_VIF		0x00080000	/*虚拟中断标志*/
#define EFLAGS_VIP		0x00100000	/*虚拟中断等待*/
#define EFLAGS_ID		0x00200000	/*识别位*/

/*所有描述符共有属性*/
#define DESC_ATTR_DT	0x00001000	/*0:系统段描述符和门描述符1:存储段描述符*/
#define DESC_ATTR_DPL0	0x00000000	/*系统特权级*/
#define DESC_ATTR_DPL1	0x00002000
#define DESC_ATTR_DPL2	0x00004000
#define DESC_ATTR_DPL3	0x00006000	/*用户特权级*/
#define DESC_ATTR_P		0x00008000	/*0:描述符无效1:描述符有效*/
/*段描述符共有属性*/
#define SEG_ATTR_AVL	0x00100000	/*软件自定*/
#define SEG_ATTR_G		0x00800000	/*0:段界限粒度为字节1:段界限粒度为4KB*/
/*存储段描述符属性*/
#define STOSEG_ATTR_T_A		0x00000100	/*0:未被访问1:访问过(CPU自动修改)*/
#define STOSEG_ATTR_T_D_W	0x00000200	/*0:数据段不可写1:数据段可写*/
#define STOSEG_ATTR_T_D_ED	0x00000400	/*0:数据段向高端扩展1:数据段向低端扩展*/
#define STOSEG_ATTR_T_C_R	0x00000200	/*0:代码段不可读1:代码段可读*/
#define STOSEG_ATTR_T_C_C	0x00000400	/*0:普通代码段1:一致代码段*/
#define STOSEG_ATTR_T_E		0x00000800	/*0:不可执行(数据段)1:可执行(代码段)*/
#define STOSEG_ATTR_D		0x00400000	/*0:16位段1:32位段*/
/*系统段描述符和门共占属性*/
/*#define SYS_ATTR_T_UDEF0	0x00000000	未定义*/
/*#define SYS_ATTR_T_2TSS	0x00000100	可用286TSS*/
#define SYSSEG_ATTR_T_LDT	0x00000200	/*LDT*/
/*#define SYS_ATTR_T_B2TSS	0x00000300	忙的286TSS*/
/*#define SYS_ATTR_T_2CALG	0x00000400	286调用门*/
#define GATE_ATTR_T_TASK	0x00000500	/*任务门*/
/*#define SYS_ATTR_T_2INTG	0x00000600	286中断门*/
/*#define SYS_ATTR_T_2TRPG	0x00000700	286陷阱门*/
/*#define SYS_ATTR_T_UDEF8	0x00000800	未定义*/
#define SYSSEG_ATTR_T_TSS	0x00000900	/*可用386TSS*/
/*#define SYS_ATTR_T_UDEFA	0x00000A00	未定义*/
#define SYSSEG_ATTR_T_BTSS	0x00000B00	/*忙的386TSS*/
#define GATE_ATTR_T_CALL	0x00000C00	/*386调用门*/
/*#define SYS_ATTR_T_UDEFD	0x00000D00	未定义*/
#define GATE_ATTR_T_INTR	0x00000E00	/*386中断门*/
#define GATE_ATTR_T_TRAP	0x00000F00	/*386陷阱门*/

typedef struct _SEG_GATE_DESC
{
	DWORD d0;
	DWORD d1;
}SEG_GATE_DESC;	/*段门描述符*/

typedef struct _TSS
{
	DWORD link;		/*前一任务的TSS描述符选择子*/
	struct _TSS_STK
	{
		DWORD esp;	/*内层堆栈指针*/
		DWORD ss;	/*内层堆栈段*/
	}stk[3];
	DWORD cr3;		/*页目录指针*/

	void *eip;		/*寄存器状态*/
	DWORD eflags;
	DWORD eax;
	DWORD ecx;
	DWORD edx;
	DWORD ebx;
	DWORD esp;
	DWORD ebp;
	DWORD esi;
	DWORD edi;
	DWORD es;
	DWORD cs;
	DWORD ss;
	DWORD ds;
	DWORD fs;
	DWORD gs;

	DWORD ldtr;		/*LDT寄存器*/
	WORD t, io;		/*调试,I/O许可位图偏移*/
}TSS;	/*任务状态结构*/

typedef struct _I387
{
	DWORD cwd;	/*控制字*/
	DWORD swd;	/*状态字*/
	DWORD twd;	/*标记字*/
	DWORD fip;	/*指令指针偏移*/
	DWORD fcs;	/*指令指针选择*/
	DWORD foo;	/*操作数偏移*/
	DWORD fos;	/*操作数选择*/
	DWORD st[20];	/*10个8字节FP寄存器*/
}I387;	/*I387浮点协处理器寄存器*/

typedef struct _I387SSE
{
	WORD cwd;	/*控制字*/
	WORD swd;	/*状态字*/
	WORD twd;	/*标记字*/
	WORD fop;	/*最后一条指令操作码*/
	union _I387SSE_REG
	{
		struct _I387SSE_INS_R
		{
			QWORD rip;	/*指令指针*/
			QWORD rdp;	/*数据指针*/
		}rreg;
		struct _I387SSE_INS_F
		{
			DWORD fip;	/*FPU指令指针偏移*/
			DWORD fcs;	/*FPU指令指针选择*/
			DWORD foo;	/*FPU操作数偏移*/
			DWORD fos;	/*FPU操作数选择*/
		}freg;
	}reg;
	DWORD mxcsr;		/*MXCSR寄存器状态*/
	DWORD mxcsr_mask;	/*MXCSR位蒙板*/
	DWORD st[32];		/*8个16字节FP寄存器*/
	DWORD xmm[64];		/*16个16字节XMM寄存器*/
	DWORD padding[12];
	union _I387SSE_PAD
	{
		DWORD padding1[12];
		DWORD sw_reserved[12];
	}pad;
}__attribute__((aligned(16))) I387SSE;	/*SSE处理器寄存器*/

/*页目录表页表项*/
#define PAGE_ATTR_P		0x00000001	/*0:页不存在1:页存在*/
#define PAGE_ATTR_W		0x00000002	/*0:只读1:可写*/
#define PAGE_ATTR_U		0x00000004	/*0:只系统(0,1,2)访问1:用户(3)可访问*/
#define PAGE_ATTR_PWT	0x00000008	/*0:表项所指向的页或页表在写缓存时使用回写法1:使用透写法，CR0的CD位为1时PWT和PCD标志都会被忽略*/
#define PAGE_ATTR_PCD	0x00000010	/*0:表项所指向的页或页表可以被缓存1:不可以*/
#define PAGE_ATTR_A		0x00000020	/*0:未被访问1:访问过(CPU自动修改)*/
#define PAGE_ATTR_D		0x00000040	/*0:未被写1:被写过(CPU自动修改)*/
#define PAGE_ATTR_PS	0x00000080	/*0:页面大小为4KB，页目录项指向一个页表1:页面大小为4MB或2MB(由CR4的PAE位决定)，页目录项直接指向一个页*/
#define PAGE_ATTR_G		0x00000100	/*0:普通页1:全局页(当PGE为1时，若发生CR3载入新值或任务切换，指向全局页的表项在TLB中仍然有效。这个标志只在直接指向一个页的表项中有效，否则会被忽略。)*/
#define PAGE_ATTR_AVL	0x00000E00	/*软件自定*/

typedef DWORD PAGE_DESC;	/*页目录表页表项*/

/*设置段描述符*/
static inline void SetSegDesc(SEG_GATE_DESC *desc, DWORD base, DWORD limit, DWORD attr)
{
	desc->d0 = (limit & 0x0000FFFF) | (base << 16);
	desc->d1 = ((base >> 16) & 0x000000FF) | attr | (limit & 0x000F0000) | (base & 0xFF000000);
}

/*取得段描述符基址*/
static inline DWORD GetSegDescBase(SEG_GATE_DESC *desc)
{
	return (desc->d0 >> 16) | ((desc->d1 << 16) & 0x00FF0000) | (desc->d1 & 0xFF000000);
}

/*取得段描述符段限*/
static inline DWORD GetSegDescLimit(SEG_GATE_DESC *desc)
{
	return (desc->d0 & 0x0000FFFF) | (desc->d1 & 0x000F0000);
}

/*设置段描述符基址*/
static inline void SetSegDescBase(SEG_GATE_DESC *desc, DWORD base)
{
	desc->d0 = (desc->d0 & 0x0000FFFF) | (base << 16);
	desc->d1 = (desc->d1 & 0x00FFFF00) | ((base >> 16) & 0x000000FF) | (base & 0xFF000000);
}

/*设置段描述符段限*/
static inline void SetSegDescLimit(SEG_GATE_DESC *desc, DWORD limit)
{
	desc->d0 = (desc->d0 & 0xFFFF0000) | (limit & 0x0000FFFF);
	desc->d1 = (desc->d1 & 0xFFF0FFFF) | (limit & 0x000F0000);
}

/*设置门描述符*/
static inline void SetGate(SEG_GATE_DESC *desc, DWORD seg, DWORD off, DWORD attr)
{
	desc->d0 = (off & 0x0000FFFF) | (seg << 16);
	desc->d1 = (off & 0xFFFF0000) | attr;
}

/*清除CR0的TS标志*/
static inline void ClearTs()
{
	__asm__("clts");
}

/*刷新页表缓冲*/
static inline void RefreshTlb()
{
	register DWORD reg;
	__asm__ __volatile__("movl %%cr3, %0;movl %0, %%cr3": "=r"(reg));
}

/*取得页故障线性地址*/
static inline void *GetPageFaultAddr()
{
	register void *_addr;
	__asm__("movl %%cr2, %0": "=r"(_addr));
	return _addr;
}

#endif
