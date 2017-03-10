/*	ulios.c for ulios
	作者：孙亮
	功能：ulios系统的c入口代码，调用微内核初始化函数，启动核心服务
	最后修改日期：2009-05-28
*/

#include "ulios.h"

int main()
{
	long res;

	InitKnlVal();		/*内核变量*/
	InitKFMT();			/*内核内存管理*/
	if ((res = InitPMM()) != NO_ERROR)	/*物理内存管理*/
		return res;
	if ((res = InitMsg()) != NO_ERROR)	/*消息管理*/
		return res;
	if ((res = InitMap()) != NO_ERROR)	/*地址映射管理*/
		return res;
	InitPMT();			/*进程管理*/
	InitKnlProc();		/*内核进程*/
	InitINTR();			/*中断处理*/
	return InitBaseSrv();/*基础服务*/
}
