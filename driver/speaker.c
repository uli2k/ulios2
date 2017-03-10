/*	speaker.c for ulios driver
	作者：孙亮
	功能：PC喇叭驱动
	最后修改日期：2010-08-23
*/

#include "basesrv.h"

static void sound(DWORD freq)
{
	if (freq == 0)
		return;
	freq = 1193180 / freq;
	cli();
	outb(0x43, 0xB6);
	outb(0x42, freq & 0xFF);
	outb(0x42, freq >> 8);
	outb(0x61, inb(0x61) | 3);
	sti();
}

static void nosound()
{
	cli();
	outb(0x61, inb(0x61) & (~3));
	sti();
}

int main()
{
	long res;		/*返回结果*/

	if ((res = KRegKnlPort(SRV_SPK_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_SPK)	/*应用请求消息*/
		{
			switch (data[MSG_API_ID] & MSG_API_MASK)
			{
			case SPK_API_SOUND:	/*发声*/
				sound(data[1]);
				break;
			case SPK_API_NOSOUND:	/*停止发声*/
				nosound();
				break;
			}
		}
	}
	KUnregKnlPort(SRV_SPK_PORT);
	return res;
}
