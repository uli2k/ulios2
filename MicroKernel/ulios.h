/*	ulios.h for ulios
	作者：孙亮
	功能：ulios内核一次初始化代码
	最后修改日期：2010-09-13
*/

#include "knldef.h"

/*内核变量初始化*/
static inline void InitKnlVal()
{
	memset32(KnlValue, 0, KVAL_LEN * sizeof(BYTE) / sizeof(DWORD));	/*清空零散变量*/
	memset32(kpt, INVALID, KPT_LEN * sizeof(THREAD_ID) / sizeof(DWORD));	/*初始化内核端口注册表*/
}

/*内核内存初始化*/
static inline void InitKFMT()
{
	InitFbt(kmmt, FMT_LEN, kdat, KDAT_SIZ);
}

/*物理内存管理初始化*/
static inline long InitPMM()
{
	DWORD i;
	MEM_ARDS *CurArd;

	i = (((MemEnd - UPHYMEM_ADDR) + 0x0001FFFF) >> 17);	/*取得进程物理页管理表长度*/
	if ((pmmap = (DWORD*)kmalloc(i * sizeof(DWORD))) == NULL)	/*建立用户内存位图,以4字节为单位*/
		return KERR_OUT_OF_KNLMEM;
	memset32(pmmap, 0xFFFFFFFF, i);	/*先标记为已用*/
	PmmLen = (i << 5);	/*用户内存总页数*/
	PmpID = INVALID;
	RemmSiz = 0;
	for (CurArd = ards; CurArd->addr != INVALID; CurArd++)
		if (CurArd->type == ARDS_TYPE_RAM && CurArd->addr + CurArd->siz > UPHYMEM_ADDR)	/*内存区有效且包含了进程内存页面*/
		{
			DWORD fst, cou, end, tcu;	/*页面起始块,数量,循环结束值,临时数量*/

			if (CurArd->addr < UPHYMEM_ADDR)	/*地址转换为进程内存相对地址*/
			{
				fst = 0;
				cou = (CurArd->addr + CurArd->siz - UPHYMEM_ADDR) >> 12;
			}
			else
			{
				fst = (CurArd->addr + 0x00000FFF - UPHYMEM_ADDR) >> 12;	/*从字节地址高端的页面起始*/
				cou = CurArd->siz >> 12;
			}
			if (PmpID > fst)
				PmpID = fst;
			RemmSiz += (cou << 12);
			end = (fst + 0x0000001F) & 0xFFFFFFE0;
			tcu = end - fst;
			if (fst + cou < end)	/*32页边界内的小整块*/
			{
				cou = (fst + cou) & 0x0000001F;
				pmmap[fst >> 5] &= ((0xFFFFFFFF >> tcu) | (0xFFFFFFFF << cou));
				continue;
			}
			pmmap[fst >> 5] &= (0xFFFFFFFF >> tcu);	/*32页边界开始的零碎块*/
			fst = end;
			cou -= tcu;
			memset32(&pmmap[fst >> 5], 0, cou >> 5);	/*大整块*/
			fst += (cou & 0xFFFFFFE0);
			cou &= 0x0000001F;
			if (cou)	/*32页边界结束的零碎块*/
				pmmap[fst >> 5] &= (0xFFFFFFFF << cou);
		}
	return NO_ERROR;
}

/*消息管理初始化*/
static inline long InitMsg()
{
	MESSAGE_DESC *msg;

	if ((FstMsg = (MESSAGE_DESC*)kmalloc(MSGMT_LEN * sizeof(MESSAGE_DESC))) == NULL)
		return KERR_OUT_OF_KNLMEM;
	for (msg = FstMsg; msg < &FstMsg[MSGMT_LEN - 1]; msg++)
		msg->nxt = msg + 1;
	msg->nxt = NULL;
	return NO_ERROR;
}

/*地址映射管理初始化*/
static inline long InitMap()
{
	MAPBLK_DESC *map;

	if ((FstMap = (MAPBLK_DESC*)kmalloc(MAPMT_LEN * sizeof(MAPBLK_DESC))) == NULL)
		return KERR_OUT_OF_KNLMEM;
	for (map = FstMap; map < &FstMap[MAPMT_LEN - 1]; map++)
		map->nxt = map + 1;
	map->nxt = NULL;
	return NO_ERROR;
}

/*进程管理表初始化*/
static inline void InitPMT()
{
	memset32(pmt, 0, PMT_LEN * sizeof(PROCESS_DESC*) / sizeof(DWORD));
	EndPmd = FstPmd = pmt;
/*	CurPmd = NULL;
	PmdCou = 0;
	clock = 0;
	SleepList = NULL;
	LastI387 = NULL;
*/
}

/*初始化内核进程*/
static inline void InitKnlProc()
{
/*	memset32(&KnlTss, 0, sizeof(TSS) / sizeof(DWORD));
*/	KnlTss.cr3 = (DWORD)kpdt;
	KnlTss.io = sizeof(TSS);
	SetSegDesc(&gdt[UCODE_SEL >> 3], 0, 0xFFFFF, DESC_ATTR_P | DESC_ATTR_DPL3 | DESC_ATTR_DT | STOSEG_ATTR_T_E | STOSEG_ATTR_T_A | SEG_ATTR_G | STOSEG_ATTR_D);	/*初始化用户GDT项*/
	SetSegDesc(&gdt[UDATA_SEL >> 3], 0, 0xFFFFF, DESC_ATTR_P | DESC_ATTR_DPL3 | DESC_ATTR_DT | STOSEG_ATTR_T_D_W | STOSEG_ATTR_T_A | SEG_ATTR_G | STOSEG_ATTR_D);
	SetSegDesc(&gdt[TSS_SEL >> 3], (DWORD)&KnlTss, sizeof(TSS) - 1, DESC_ATTR_P | SYSSEG_ATTR_T_TSS);
	__asm__("ltr %%ax":: "a"(TSS_SEL));
}

/*异常IDT表副本(函数地址无法进行位运算,只好用加减了)*/
SEG_GATE_DESC IsrIdtTable[] = {
	{(DWORD)AsmIsr00 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr01 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr02 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr03 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL3 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr04 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL3 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr05 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL3 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr06 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr07 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr08 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr09 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr0A - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr0B - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr0C - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr0D - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr0E - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR},
	{(DWORD)AsmIsr0F - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr10 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr11 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr12 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr13 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP}
};

/*中断处理初始化*/
static inline void InitINTR()
{
	memcpy32(idt, IsrIdtTable, sizeof(IsrIdtTable) / sizeof(DWORD));	/*复制20个ISR的门描述符*/
	SetGate(&idt[INTN_APICALL], KCODE_SEL, (DWORD)AsmApiCall, DESC_ATTR_P | DESC_ATTR_DPL3 | GATE_ATTR_T_TRAP);	/*设置系统调用*/
	memset32(IrqPort, INVALID, IRQ_LEN * sizeof(THREAD_ID) / sizeof(DWORD));	/*初始化IRQ端口注册表*/
	/*开时钟和从片8259中断*/
	SetGate(&idt[0x20 + IRQN_TIMER], KCODE_SEL, (DWORD)AsmIrq0, DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	SetGate(&idt[0x20 + IRQN_SLAVE8259], KCODE_SEL, (DWORD)AsmIrq2, DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	/*对8259中断芯片进行编程,参考linux 0.11*/
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(MASTER8259_PORT, 0x20);
	outb(SLAVER8259_PORT, 0x28);
	outb(MASTER8259_PORT, 0x04);
	outb(SLAVER8259_PORT, 0x02);
	outb(MASTER8259_PORT, 0x01);
	outb(SLAVER8259_PORT, 0x01);
	outb(MASTER8259_PORT, 0xFA);	/*主8259A只开启时钟*/
	outb(SLAVER8259_PORT, 0xFF);	/*从8259A全部禁止*/
	/*对8253时钟芯片进行编程,参考linux 0.00*/
	outb(0x43, 0x36);
	outb(0x40, 0x9C);	/*时钟中断频率:100HZ,写入字:1193180/100*/
	outb(0x40, 0x2E);
}

/*初始化基础服务*/
static inline long InitBaseSrv()
{
	PHYBLK_DESC *CurSeg;
	THREAD_ID ptid;
	long res;

	for (CurSeg = &BaseSrv[1]; CurSeg->addr; CurSeg++)
		if ((res = CreateProc(EXEC_ARGV_BASESRV | EXEC_ARGV_DRIVER, CurSeg->addr, CurSeg->siz, &ptid)) != NO_ERROR)
			return res;
	return NO_ERROR;
}
