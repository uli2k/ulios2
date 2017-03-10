/*	uart.c for ulios driver
	作者：孙亮
	功能：COM串口驱动
	最后修改日期：2010-08-24
*/

#include "basesrv.h"

typedef struct _UART_CLIENT
{
	BYTE *addr;		/*起始地址*/
	BYTE *CurAddr;	/*当前地址*/
	DWORD cou;		/*剩余字节数*/
	DWORD clock;	/*超时时钟数*/
}UART_CLIENT;	/*串口客户端结构*/

#define QUE_LEN		0x400	/*数据队列长度*/
typedef struct _UART_REQ
{
	DWORD com;			/*端口号*/
	BYTE que[QUE_LEN];	/*数据队列*/
	BYTE *head;			/*数据头*/
	BYTE *tail;			/*数据尾*/
	volatile DWORD quel;/*队列锁*/
	UART_CLIENT reader;	/*读者*/
	UART_CLIENT writer;	/*写者*/
}UART_REQ;	/*COM串口请求结构*/

#define COM1_IRQ	0x4	/*COM1中断请求号*/
#define COM2_IRQ	0x3	/*COM2中断请求号*/

#define PORT_LEN	2	/*串口数量*/
const WORD ComPort[PORT_LEN] = {0x3F8, 0x2F8};	/*COM端口基地址*/

#define SER_DLL		0	/*波特率分频器低八位*/
#define SER_DLH		1	/*波特率分频器高八位*/
#define SER_DR		0	/*数据寄存器*/
#define SER_IER		1	/*中断启动寄存器*/
#define SER_IIR		2	/*中断辨识寄存器*/
#define SER_LCR		3	/*传输线控制寄存器*/
#define SER_MCR		4	/*调制解调器控制寄存器*/
#define SER_LSR		5	/*传输线状态寄存器*/
#define SER_MSR		6	/*调制解调器状态寄存器*/

#define MCR_DTR		0x01	/*数据终端就绪DTR有效*/
#define MCR_RTS		0x02	/*请求发送RTS有效*/
#define MCR_GP02	0x08	/*输出2,允许中断*/
#define LCR_DLAB	0x80	/*分频器锁存器存取位*/
#define LSR_THRE	0x20	/*传送器保存寄存器空闲*/  

/*写串口*/
void WriteCom(UART_REQ *req)
{
	if (req->writer.addr)
	{
		outb(ComPort[req->com] + SER_DR, *(req->writer.CurAddr++));
		if (--(req->writer.cou) == 0)
		{
			DWORD data[MSG_DATA_LEN];

			data[MSG_RES_ID] = NO_ERROR;
			data[MSG_SIZE_ID] = req->writer.cou;
			KUnmapProcAddr(req->writer.addr, data);
			req->writer.addr = NULL;
		}
	}
}

/*读串口*/
void ReadCom(UART_REQ *req)
{
	WORD BasePort;
	BYTE b;

	BasePort = ComPort[req->com];
	do
	{
		b = inb(ComPort[req->com] + SER_DR);
		if (req->reader.addr)	/*放入读线程的空间*/
		{
			*(req->reader.CurAddr++) = b;
			if (--(req->reader.cou) == 0)	/*已完成复制*/
			{
				DWORD data[MSG_DATA_LEN];

				data[MSG_RES_ID] = NO_ERROR;
				data[MSG_SIZE_ID] = req->reader.cou;
				KUnmapProcAddr(req->reader.addr, data);
				req->reader.addr = NULL;
			}
		}
		else	/*放入队列*/
		{
			lock(&req->quel);
			*req->head = b;
			if (++(req->head) >= &req->que[QUE_LEN])
				req->head = req->que;
			if (req->head == req->tail)	/*队列已满*/
				if (++(req->tail) >= &req->que[QUE_LEN])	/*丢弃队尾数据*/
					req->tail = req->que;
			ulock(&req->quel);
		}
	}
	while (inb(BasePort + SER_LSR) & 0x01);
}

/*串口中断响应线程*/
void ComProc(UART_REQ *req)
{
	long res;		/*返回结果*/

	if ((res = KRegIrq(COM1_IRQ)) != NO_ERROR)	/*注册中断请求号*/
		KExitThread(res);
	if ((res = KRegIrq(COM2_IRQ)) != NO_ERROR)	/*注册中断请求号*/
		KExitThread(res);
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_IRQ)	/*COM口中断请求消息*/
		{
			UART_REQ *CurReq;

			CurReq = &req[data[1] == COM2_IRQ];
			switch (inb(ComPort[CurReq->com] + SER_IIR) & 0x07)
			{
			case 0:	/*MODEM中断*/
				inb(ComPort[CurReq->com] + SER_MSR);
				break;
			case 2:	/*发送中断*/
				WriteCom(CurReq);
				break;
			case 4:	/*接收中断*/
				ReadCom(CurReq);
				break;
			case 6:	/*接收错中断*/
				inb(ComPort[CurReq->com] + SER_LSR);
				break;
			}
		}
	}
	KUnregIrq(COM1_IRQ);
	KUnregIrq(COM2_IRQ);
	KExitThread(res);
}

/*复制队列中已读的数据*/
static inline void CopyQue(UART_REQ *req)
{
	DWORD cou;	/*要复制的数据量*/

	lock(&req->quel);
	if (req->tail > req->head)	/*尾在头前*/
	{
		cou = &req->que[QUE_LEN] - req->tail;
		if (cou > req->reader.cou)	/*已接收数据大于需求数据*/
			cou = req->reader.cou;
		memcpy8(req->reader.CurAddr, req->tail, cou);
		req->reader.CurAddr += cou;
		req->reader.cou -= cou;
		if ((req->tail += cou) >= &req->que[QUE_LEN])
			req->tail = req->que;
	}
	if (req->head > req->tail)	/*头在尾前,继续复制*/
	{
		cou = req->head - req->tail;
		if (cou > req->reader.cou)	/*已接收数据大于需求数据*/
			cou = req->reader.cou;
		memcpy8(req->reader.CurAddr, req->tail, cou);
		req->reader.CurAddr += cou;
		req->reader.cou -= cou;
		req->tail += cou;
	}
	ulock(&req->quel);
	if (req->reader.cou == 0)	/*已完成复制*/
	{
		DWORD data[MSG_DATA_LEN];

		data[MSG_RES_ID] = NO_ERROR;
		data[MSG_SIZE_ID] = req->reader.cou;
		KUnmapProcAddr(req->reader.addr, data);
		req->reader.addr = NULL;
	}
	else if (req->reader.clock == 0)	/*超时*/
	{
		DWORD data[MSG_DATA_LEN];

		data[MSG_RES_ID] = UART_ERR_NOTIME;
		data[MSG_SIZE_ID] = req->reader.cou;
		KUnmapProcAddr(req->reader.addr, data);
		req->reader.addr = NULL;
	}
}

/*打开串口*/
static inline long OpenCom(UART_REQ *req, DWORD com, DWORD baud, DWORD args)
{
	WORD BasePort;
	if (com >= PORT_LEN)
		return UART_ERR_NOPORT;
	BasePort = ComPort[com];
	baud = 115200 / baud;
	if (baud == 0)
		return UART_ERR_BAUD;
	cli();	/*设置串口参数,打开串口*/
	outb(BasePort + SER_LCR, LCR_DLAB);
	outb(BasePort + SER_DLL, baud);
	outb(BasePort + SER_DLH, baud >> 8);
	outb(BasePort + SER_LCR, args & (~LCR_DLAB));
	outb(BasePort + SER_MCR, MCR_GP02);
	outb(BasePort + SER_IER, 0x0F);
	sti();
	req += com;
	lock(&req->quel);
	req->tail = req->head = req->que;	/*丢弃已存储的数据*/
	ulock(&req->quel);
	req->writer.addr = req->reader.addr = NULL;	/*撤销已有任务*/
	return NO_ERROR;
}

/*关闭串口*/
static inline long CloseCom(DWORD com)
{
	WORD BasePort;
	if (com >= PORT_LEN)
		return UART_ERR_NOPORT;
	BasePort = ComPort[com];
	cli();
	outb(BasePort + SER_MCR, 0);
	outb(BasePort + SER_IER, 0);	/*关闭串口中断*/
	sti();
	return NO_ERROR;
}

int main()
{
	UART_REQ req[PORT_LEN];
	THREAD_ID ptid;
	long res;		/*返回结果*/

	if ((res = KRegKnlPort(SRV_UART_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	memset32(req, 0, sizeof(req) / sizeof(DWORD));	/*初始化变量*/
	for (res = 0; res < PORT_LEN; res++)
	{
		req[res].com = res;
		req[res].tail = req[res].head = req[res].que;
		outb(ComPort[res] + SER_MCR, 0);
		outb(ComPort[res] + SER_IER, 0);	/*关闭串口中断*/
	}
	KCreateThread((void(*)(void*))ComProc, 0x1000, req, &ptid);	/*启动串口中断响应线程*/
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];
		UART_REQ *CurReq;

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		switch (data[MSG_ATTR_ID] & MSG_ATTR_MASK)
		{
		case MSG_ATTR_UART:	/*COM控制消息*/
			switch (data[data[MSG_API_ID] & MSG_API_MASK])
			{
			case UART_API_OPENCOM:
				data[MSG_RES_ID] = OpenCom(req, data[1], data[2], data[3]);
				KSendMsg(&ptid, data, 0);
				break;
			case UART_API_CLOSECOM:
				data[MSG_RES_ID] = CloseCom(data[1]);
				KSendMsg(&ptid, data, 0);
				break;
			}
			break;
		case MSG_ATTR_ROMAP:	/*写串口消息*/
			if (data[3] >= PORT_LEN)
			{
				data[MSG_RES_ID] = UART_ERR_NOPORT;
				KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
				continue;
			}
			CurReq = &req[data[3]];
			if (CurReq->writer.addr)
			{
				data[MSG_RES_ID] = UART_ERR_BUSY;
				KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
				continue;
			}
			CurReq->writer.CurAddr = CurReq->writer.addr = (BYTE*)data[MSG_ADDR_ID];
			CurReq->writer.cou = data[MSG_SIZE_ID];
			CurReq->writer.clock = data[4];
			WriteCom(CurReq);
			break;
		case MSG_ATTR_RWMAP:	/*读串口消息*/
			if (data[3] >= PORT_LEN)
			{
				data[MSG_RES_ID] = UART_ERR_NOPORT;
				KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
				continue;
			}
			CurReq = &req[data[3]];
			if (CurReq->reader.addr)
			{
				data[MSG_RES_ID] = UART_ERR_BUSY;
				KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
				continue;
			}
			CurReq->reader.CurAddr = CurReq->reader.addr = (BYTE*)data[MSG_ADDR_ID];
			CurReq->reader.cou = data[MSG_SIZE_ID];
			CurReq->reader.clock = data[4];
			CopyQue(CurReq);
			break;
		}
	}
	KUnregKnlPort(SRV_UART_PORT);
	return res;
}
