/*	athd.c for ulios driver
	作者：孙亮
	功能：LBA模式硬盘驱动服务程序，中断控制PIO方式
	最后修改日期：2009-07-22
*/

#include "basesrv.h"

typedef struct _ATHD_REQ
{
	THREAD_ID ptid;
	void *addr;
	void *CurAddr;
	DWORD sec;
	BYTE cou;
	BYTE isWrite;
	BYTE drv;
	BYTE cmd;
	struct _ATHD_REQ *nxt;
}ATHD_REQ;	/*AT磁盘请求节点*/
#define REQ_LEN		0x100

#define ATHD_IRQ	0xE	/*AT磁盘中断请求号*/

#define STATE_WAIT	0	/*正常等待状态*/
#define STATE_BUSY	1	/*正在处理任务*/

/*端口读取数据*/
static inline void ReadPort(void *buf, WORD port, DWORD n)
{
	void *_buf;
	DWORD _n;
	__asm__ __volatile__("cld;rep insw": "=&D"(_buf), "=&c"(_n): "0"(buf), "d"(port), "1"(n): "flags", "memory");
}

/*端口写入数据*/
static inline void WritePort(void *buf, WORD port, DWORD n)
{
	void *_buf;
	DWORD _n;
	__asm__ __volatile__("cld;rep outsw": "=&S"(_buf), "=&c"(_n): "0"(buf), "d"(port), "1"(n): "flags");
}

typedef struct _HD_ARGS
{
	WORD cylinders;	/*柱面数*/
	BYTE heads;		/*磁头数*/
	WORD null0;		/*开始减小写电流的柱面*/
	WORD wpcom;		/*开始写前预补偿柱面号*/
	BYTE ecc;		/*最大ECC猝发长度*/
	BYTE ctrlbyte;	/*控制字节*/
	BYTE outime;	/*标准超时值*/
	BYTE fmtime;	/*格式化超时值*/
	BYTE checktime;	/*检测驱动器超时值*/
	WORD landzone;	/*磁头着陆柱面号*/
	BYTE spt;		/*每磁道扇区数*/
	BYTE res;		/*保留*/
}__attribute__((packed)) HD_ARGS;	/*硬盘参数表*/

#define HD_ARGS_ADDR	0x90600	/*硬盘参数物理地址*/

/*读写磁盘扇区*/
void RwSector(ATHD_REQ *req)
{
	while (inb(0x1F7) != 0x58);	/*等待控制器空闲和驱动器就绪*/
	if (req->isWrite)
		WritePort(req->CurAddr, 0x1F0, ATHD_BPS / sizeof(WORD));	/*写端口数据*/
	else
		ReadPort(req->CurAddr, 0x1F0, ATHD_BPS / sizeof(WORD));	/*读端口数据*/
	req->CurAddr += ATHD_BPS;
	req->cou--;
}

/*输出磁盘命令*/
void OutCmd(ATHD_REQ *req)
{
	DWORD sec;

	sec = req->sec;
	while (inb(0x1F7) != 0x50);	/*等待控制器空闲和驱动器就绪*/
	cli();
	outb(0x1F1, 0);			/*预补偿柱面号*/
	outb(0x1F2, req->cou);	/*设置扇区数*/
	outb(0x1F3, sec);		/*扇区号低8位*/
	outb(0x1F4, sec >> 8);	/*扇区号次8位*/
	outb(0x1F5, sec >> 16);	/*扇区号中8位*/
	outb(0x1F6, 0xE0 | (req->drv << 4) | ((sec >> 24) & 0x0F));	/*LBA方式,驱动器号,扇区号高4位*/
	outb(0x1F7, req->isWrite ? 0x30 : 0x20);	/*读写硬盘命令*/
	sti();
	if (req->isWrite)
		RwSector(req);
}

/*分配请求结构*/
static inline ATHD_REQ *AllocReq(ATHD_REQ **FstReq)
{
	ATHD_REQ *CurReq;

	if (*FstReq == NULL)
		return NULL;
	*FstReq = (CurReq = *FstReq)->nxt;
	return CurReq;
}

/*释放请求结构*/
static inline void FreeReq(ATHD_REQ **FstReq, ATHD_REQ *CurReq)
{
	CurReq->nxt = *FstReq;
	*FstReq = CurReq;
}

/*插入请求结构*/
static inline void AddReq(ATHD_REQ **ReqList, ATHD_REQ **LstReq, ATHD_REQ *req)
{
	req->nxt = NULL;
	if (*ReqList == NULL)
		*ReqList = req;
	else
		(*LstReq)->nxt = req;
	*LstReq = req;
}

/*删除请求结构*/
static inline void DelReq(ATHD_REQ **ReqList)
{
	*ReqList = (*ReqList)->nxt;
}

int main()
{
	ATHD_REQ req[REQ_LEN], *FstReq, *ReqList, *LstReq;	/*服务请求列表*/
	DWORD secou[2];	/*两个硬盘的扇区数*/
	DWORD state;	/*当前状态*/
	long res;		/*返回结果*/
	HD_ARGS *haddr;

	if ((res = KRegKnlPort(SRV_ATHD_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	if ((res = KRegIrq(ATHD_IRQ)) != NO_ERROR)	/*注册中断请求号*/
		return res;
	if ((res = KMapPhyAddr((void**)&haddr, HD_ARGS_ADDR, sizeof(HD_ARGS) * 2)) != NO_ERROR)	/*映射硬盘参数*/
		return res;
	secou[0] = haddr[0].cylinders * haddr[0].heads * haddr[0].spt;
	secou[1] = haddr[1].cylinders * haddr[1].heads * haddr[1].spt;
	KFreeAddr(haddr);
	for (FstReq = req; FstReq < &req[REQ_LEN - 1]; FstReq++)	/*初始化变量*/
		FstReq->nxt = FstReq + 1;
	FstReq->nxt = NULL;
	FstReq = req;
	LstReq = ReqList = NULL;
	state = STATE_WAIT;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];
		ATHD_REQ *CurReq;

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		switch (data[MSG_ATTR_ID] & MSG_ATTR_MASK)
		{
		case MSG_ATTR_IRQ:	/*磁盘中断请求消息,只可能是ATHD_IRQ*/
			CurReq = ReqList;
			if (CurReq->isWrite)	/*写中断*/
			{
				if (CurReq->cou)
				{
					RwSector(CurReq);
					continue;
				}
			}
			else	/*读中断*/
			{
				RwSector(CurReq);
				if (CurReq->cou)
					continue;
			}
			data[MSG_RES_ID] = NO_ERROR;
			KUnmapProcAddr(CurReq->addr, data);
			DelReq(&ReqList);
			FreeReq(&FstReq, CurReq);
			if (ReqList)	/*检查有没有剩余任务*/
				OutCmd(ReqList);
			else
				state = STATE_WAIT;
			break;
		case MSG_ATTR_ROMAP:	/*磁盘驱动服务消息*/
		case MSG_ATTR_RWMAP:	/*RO:写磁盘,RW读磁盘*/
			if (secou[data[3] & 1] == 0)	/*驱动器不存在*/
			{
				data[MSG_RES_ID] = ATHD_ERR_WRONG_DRV;
				KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
				continue;
			}
			if ((CurReq = AllocReq(&FstReq)) == NULL)	/*服务请求列表已满*/
			{
				data[MSG_RES_ID] = ATHD_ERR_HAVENO_REQ;
				KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
				continue;
			}
			CurReq->ptid = ptid;
			CurReq->CurAddr = CurReq->addr = (void*)data[MSG_ADDR_ID];
			CurReq->sec = data[4];
			CurReq->cou = (data[MSG_SIZE_ID] + ATHD_BPS - 1) / ATHD_BPS;
			CurReq->cmd = CurReq->isWrite = ((~data[MSG_ATTR_ID]) >> 16) & 1;
			CurReq->drv = data[3] & 1;
			AddReq(&ReqList, &LstReq, CurReq);
			if (state == STATE_WAIT)
			{
				state = STATE_BUSY;
				OutCmd(ReqList);
			}
			break;
		}
	}
	KUnregIrq(ATHD_IRQ);
	KUnregKnlPort(SRV_ATHD_PORT);
	return res;
}
