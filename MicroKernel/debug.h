/*	cintr.c for ulios
	作者：孙亮
	功能：C中断/异常/系统调用处理
	最后修改日期：2019-09-01
*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "ulidef.h"

#ifdef DEBUG

/*格式化输出*/
void KPrint(const char *fmtstr, ...);

#else

#define KPrint(...)

#endif

#endif