/*	rep.c for ulios driver
	作者：孙亮
	功能：进程异常报告程序
	最后修改日期：2010-06-16
*/

#include "../lib/string.h"
#include "basesrv.h"

int main()
{
	static const char *IsrStr[20] =
	{
		"被零除故障",
		"调试异常",
		"不可屏蔽中断",
		"调试断点",
		"INTO陷阱",
		"边界检查越界",
		"非法操作码",
		"协处理器不可用",
		"双重故障",
		"协处理器段越界",
		"无效TSS异常",
		"段不存在",
		"堆栈段异常",
		"通用保护异常",
		"页异常",
		"未知异常",
		"协处理器浮点运算出错",
		"对齐检验错",
		"Machine Check",
		"SIMD浮点异常"
	};
	long res;

	KDebug("booting... rep driver\n");
	if ((res = KRegKnlPort(SRV_REP_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];
		char buf[256];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		SPKSound(1000);	/*发出报警声音*/
		switch (data[MSG_ATTR_ID] & MSG_ATTR_MASK)
		{
		case MSG_ATTR_ISR:
			sprintf(buf, "报告：线程(pid=%d tid=%d)出现不可恢复的异常。ISR=%u：%s，错误码=0x%X，程序地址EIP=0x%X\n", ptid.ProcID, ptid.ThedID, data[1], IsrStr[data[1]], data[2], data[3]);
			CUIPutS(buf);
			break;
		case MSG_ATTR_EXCEP:
			sprintf(buf, "报告：线程(pid=%d tid=%d)不正常退出。错误=%d，异常地址=0x%X，程序地址EIP=0x%X\n", ptid.ProcID, ptid.ThedID, data[MSG_RES_ID], data[MSG_ADDR_ID], data[MSG_SIZE_ID]);
			CUIPutS(buf);
			break;
		}
		KSleep(20);
		SPKNosound();
	}
	return NO_ERROR;
}
