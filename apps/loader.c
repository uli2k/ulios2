/*	loader.c for ulios application
	作者：孙亮
	功能：用户应用程序加载器
	最后修改日期：2010-05-19
*/

#include "../fs/fsapi.h"

#define LOADLIST_SIZ	10240

void ProcStr(char *str, char **exec, char **args)
{
	(*args) = (*exec) = NULL;
	while (*str == ' ' || *str == '\t')	/*清除参数前无意义的空格*/
		str++;
	if (*str == '\0')
		return;
	if (*str == '\"')	/*双引号内的参数单独处理*/
	{
		*exec = ++str;
		while (*str != '\0' && *str != '\"')	/*搜索匹配的双引号*/
			str++;
	}
	else	/*普通参数用空格分隔*/
	{
		*exec = str;
		while (*str != '\0' && *str != ' ' && *str != '\t')	/*搜索空格*/
			str++;
	}
	if (*str == '\0')
		return;
	*str++ = '\0';
	while (*str == ' ' || *str == '\t')	/*清除参数前无意义的空格*/
		str++;
	*args = str;
}

int main()
{
	char LoadList[LOADLIST_SIZ], *path;
	DWORD siz;
	THREAD_ID ptid;
	void *addr;
	long res;

	if ((res = KMapPhyAddr(&addr, 0x90280, 0x7C)) != NO_ERROR)	/*取得系统目录*/
		return res;
	if ((res = FSChDir((const char*)addr)) != NO_ERROR)	/*切换到系统目录*/
		return res;
	KFreeAddr(addr);
	if ((res = FSopen("bootlist", FS_OPEN_READ)) < 0)	/*读取配置文件*/
		return res;
	siz = FSread(res, LoadList, LOADLIST_SIZ - 1);
	FSclose(res);
	path = LoadList;
	LoadList[siz] = '\0';
	for (;;)
	{
		char *pathp = path, *exec, *args;

		KSleep(5);	/*延时,防止进程间依赖关系不满足*/
		while (*pathp != '\n' && *pathp != '\0')
			pathp++;
		if (*pathp)
			*pathp++ = '\0';
		switch (*path++)
		{
		case 'D':	/*driver*/
		case 'd':
			ProcStr(path, &exec, &args);
			KCreateProcess(EXEC_ATTR_DRIVER, exec, args, &ptid);
			break;
		case 'A':	/*apps*/
		case 'a':
			ProcStr(path, &exec, &args);
			KCreateProcess(0, exec, args, &ptid);
			break;
		}
		if (*(path = pathp) == '\0')	/*没有文件了*/
			break;
	}
	return NO_ERROR;
}
