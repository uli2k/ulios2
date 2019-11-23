/*	string.h for ulios
	作者：孙亮
	功能：用户字符串处理
	最后修改日期：2019-11-23
*/

#ifndef	_STRING_H_
#define	_STRING_H_

#include "../MkApi/ulimkapi.h"

/*双字转化为数字*/
char *itoa(char *buf, DWORD n, DWORD r);

/*格式化输出*/
void sprintf(char *buf, const char *fmtstr, ...);

/*10进制字符串转换为无符号整数*/
DWORD atoi10(const char *str);

/*16进制字符串转换为无符号整数*/
DWORD atoi16(const char *str);

#endif
