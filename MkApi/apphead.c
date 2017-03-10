/*	apphead.c for ulios program
	作者：孙亮
	功能：应用程序的入口代码,格式化参数并调用程序的C语言main函数
	最后修改日期：2010-05-25
*/

#include "ulimkapi.h"

extern int main(int argc, char *argv[]);

void start(char *args)
{
	int argc;
	char *argv[0x100];

	for (argc = 1;;)
	{
		while (*args == ' ' || *args == '\t')	/*清除参数前无意义的空格*/
			args++;
		if (*args == '\0')
			break;
		if (*args == '\"')	/*双引号内的参数单独处理*/
		{
			argv[argc++] = ++args;
			while (*args != '\0' && *args != '\"')	/*搜索匹配的双引号*/
				args++;
		}
		else	/*普通参数用空格分隔*/
		{
			argv[argc++] = args;
			while (*args != '\0' && *args != ' ' && *args != '\t')	/*搜索空格*/
				args++;
		}
		if (*args == '\0')
			break;
		*args++ = '\0';
	}
	KExitProcess(main(argc, argv));
}
