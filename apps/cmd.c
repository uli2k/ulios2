/*	cmd.c for ulios application
	作者：孙亮
	功能：命令提示符程序
	最后修改日期：2010-06-14
*/

#include "../driver/basesrv.h"
#include "../fs/fsapi.h"
#include "../gui/guiapi.h"

#define CMD_LEN		256
#define PROMPT		"命令:"

char cmd[CMD_LEN], *cmdp;	/*输入命令缓冲*/

/*双字转化为数字*/
char *Itoa(char *buf, DWORD n, DWORD r)
{
	static const char num[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	char *p, *q;

	q = p = buf;
	do
	{
		*p++ = num[n % r];
		n /= r;
	}
	while (n);
	buf = p;	/*确定字符串尾部*/
	*p-- = '\0';
	while (p > q)	/*翻转字符串*/
	{
		char c = *q;
		*q++ = *p;
		*p-- = c;
	}
	return buf;
}

/*格式化输出*/
void Sprintf(char *buf, const char *fmtstr, ...)
{
	long num;
	const DWORD *args = (DWORD*)(&fmtstr);

	while (*fmtstr)
	{
		if (*fmtstr == '%')
		{
			fmtstr++;
			switch (*fmtstr)
			{
			case 'd':
				num = *((long*)++args);
				if (num < 0)
				{
					*buf++ = '-';
					buf = Itoa(buf, -num, 10);
				}
				else
					buf = Itoa(buf, num, 10);
				break;
			case 'u':
				buf = Itoa(buf, *((DWORD*)++args), 10);
				break;
			case 'x':
			case 'X':
				buf = Itoa(buf, *((DWORD*)++args), 16);
				break;
			case 'o':
				buf = Itoa(buf, *((DWORD*)++args), 8);
				break;
			case 's':
				buf = strcpy(buf, *((const char**)++args)) - 1;
				break;
			case 'c':
				*buf++ = *((char*)++args);
				break;
			default:
				*buf++ = *fmtstr;
			}
		}
		else
			*buf++ = *fmtstr;
		fmtstr++;
	}
	*buf = '\0';
}

/*10进制字符串转换为无符号整数*/
DWORD Atoi10(const char *str)
{
	DWORD res;

	res = 0;
	for (;;)
	{
		if (*str >= '0' && *str <= '9')
			res = res * 10 + (*str - '0');
		else
			break;
		str++;
	}
	return res;
}

/*16进制字符串转换为无符号整数*/
DWORD Atoi16(const char *str)
{
	DWORD res;

	res = 0;
	for (;;)
	{
		if (*str >= '0' && *str <= '9')
			res = res * 16 + (*str - '0');
		else if (*str >= 'A' && *str <= 'F')
			res = res * 16 + (*str - 'A' + 10);
		else if (*str >= 'a' && *str <= 'f')
			res = res * 16 + (*str - 'a' + 10);
		else
			break;
		str++;
	}
	return res;
}

/*清屏*/
void cls(char *args)
{
	CUIClrScr();
}

/*设置字符颜色和背景颜色*/
void SetColor(char *args)
{
	char *p;

	for (p = args;; p++)
	{
		if (*p == ' ')
		{
			*(p++) = '\0';
			break;
		}
		if (*p == '\0')
		{
			CUIPutS("参数: 前景色 背景色\n");
			return;
		}
	}
	CUISetCol(Atoi16(args), Atoi16(p));
}

/*退出*/
void exitcmd(char *args)
{
	KExitProcess(NO_ERROR);
}

/*删除文件*/
void delfile(char *args)
{
	if (FSremove(args) != NO_ERROR)
		CUIPutS("无法删除！\n");
}

/*复制文件*/
void copy(char *args)
{
	char buf[4096], *bufp;
	long in, out, siz;

	for (bufp = args;; bufp++)
	{
		if (*bufp == ' ')
		{
			*(bufp++) = '\0';
			break;
		}
		if (*bufp == '\0')
		{
			CUIPutS("参数: 源文件路径 目的路径\n");
			return;
		}
	}
	if ((in = FSopen(args, FS_OPEN_READ)) < 0)
	{
		CUIPutS("源文件不存在！\n");
		return;
	}
	if ((out = FScreat(bufp)) < 0)
	{
		FSclose(in);
		CUIPutS("无法创建目的文件！\n");
		return;
	}
	while ((siz = FSread(in, buf, sizeof(buf))) > 0)
		FSwrite(out, buf, siz);
	FSclose(out);
	FSclose(in);
}

/*重命名*/
void ren(char *args)
{
	char *p;

	for (p = args;; p++)
	{
		if (*p == ' ')
		{
			*(p++) = '\0';
			break;
		}
		if (*p == '\0')
		{
			CUIPutS("参数: 目标路径 新名称\n");
			return;
		}
	}
	if (FSrename(args, p) != NO_ERROR)
		CUIPutS("重命名出错！\n");
}

/*显示分区列表*/
void partlist(char *args)
{
	struct _PI_INFO
	{
		PART_INFO info;
		char fstype[8];
	}pi;
	DWORD pid;

	pid = 0;
	while (FSEnumPart(&pid) == NO_ERROR)
	{
		THREAD_ID ptid;
		char buf[4096];

		FSGetPart(pid, &pi.info);
		Sprintf(buf, "/%u\t容量:%uMB\t剩余:%uMB\t格式:%s\t卷标:%s\n", pid, (DWORD)(pi.info.size / 0x100000), (DWORD)(pi.info.remain / 0x100000), pi.fstype, pi.info.label);
		CUIPutS(buf);
		ptid.ProcID = SRV_CUI_PORT;
		ptid.ThedID = INVALID;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR)
		{
			if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_CUIKEY && buf[4] == 27)	/*按下ESC键*/
			{
				CUIPutS("用户取消！\n");
				break;
			}
			else if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)	/*退出请求*/
				KExitProcess(NO_ERROR);
		}
		pid++;
	}
}

/*显示目录列表*/
void dir(char *args)
{
	FILE_INFO fi;
	long dh;

	if ((dh = FSOpenDir(args)) < 0)
	{
		CUIPutS("目录不存在！\n");
		return;
	}
	while (FSReadDir(dh, &fi) == NO_ERROR)
	{
		THREAD_ID ptid;
		TM tm;
		char buf[4096];

		TMLocalTime(fi.ModifyTime, &tm);
		Sprintf(buf, "%d-%d-%d\t%d:%d:%d   \t%s\t%d\t%c%c%c%c%c%c\t%s\n", tm.yer, tm.mon, tm.day, tm.hor, tm.min, tm.sec, (fi.attr & FILE_ATTR_DIREC) ? "目录" : "文件", (DWORD)fi.size, (fi.attr & FILE_ATTR_RDONLY) ? 'R' : ' ', (fi.attr & FILE_ATTR_HIDDEN) ? 'H' : ' ', (fi.attr & FILE_ATTR_SYSTEM) ? 'S' : ' ', (fi.attr & FILE_ATTR_LABEL) ? 'L' : ' ', (fi.attr & FILE_ATTR_ARCH) ? 'A' : ' ', (fi.attr & FILE_ATTR_EXEC) ? 'X' : ' ', fi.name);
		CUIPutS(buf);
		ptid.ProcID = SRV_CUI_PORT;
		ptid.ThedID = INVALID;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR)
		{
			if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_CUIKEY && buf[4] == 27)	/*按下ESC键*/
			{
				CUIPutS("用户取消！\n");
				break;
			}
			else if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)	/*退出请求*/
				KExitProcess(NO_ERROR);
		}
	}
	FSclose(dh);
}

/*切换目录*/
void cd(char *args)
{
	if (*args)
	{
		if (FSChDir(args) != NO_ERROR)
			CUIPutS("目录不存在！\n");
	}
	else
	{
		char path[MAX_PATH];
		long siz;
		if ((siz = FSGetCwd(path, MAX_PATH - 1)) < 0)
			CUIPutS("当前路径错误！\n");
		else
		{
			path[siz - 1] = '\n';
			path[siz] = '\0';
			CUIPutS(path);
		}
	}
}

/*创建目录*/
void md(char *args)
{
	if (FSMkDir(args) != NO_ERROR)
		CUIPutS("无法创建目录！\n");
}

void show(char *args)
{
	long fh, siz;
	char buf[4097];

	if ((fh = FSopen(args, FS_OPEN_READ)) < 0)
	{
		CUIPutS("文件不存在！\n");
		return;
	}
	while ((siz = FSread(fh, buf, 4096)) > 0)
	{
		THREAD_ID ptid;

		buf[siz] = '\0';
		CUIPutS(buf);
		ptid.ProcID = SRV_CUI_PORT;
		ptid.ThedID = INVALID;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR)
		{
			if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_CUIKEY && buf[4] == 27)	/*按下ESC键*/
			{
				CUIPutS("用户取消！\n");
				break;
			}
			else if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)	/*退出请求*/
				KExitProcess(NO_ERROR);
		}
	}
	FSclose(fh);
}

/*显示时间*/
void showtim(char *args)
{
	static const char *WeekName[7] = {"日", "一", "二", "三", "四", "五", "六"};
	TM tm;
	char buf[40];
	TMCurTime(&tm);
	Sprintf(buf, "现在时刻:%d年%d月%d日 星期%s %d时%d分%d秒\n", tm.yer, tm.mon, tm.day, WeekName[tm.wday], tm.hor, tm.min, tm.sec);
	CUIPutS(buf);
}

/*显示进程列表*/
void proclist(char *args)
{
	FILE_INFO fi;
	DWORD pid;

	pid = 0;
	while (FSProcInfo(&pid, &fi) == NO_ERROR)
	{
		THREAD_ID ptid;
		char buf[4096];

		Sprintf(buf, "PID:%d\t%s\n", pid, fi.name);
		CUIPutS(buf);
		ptid.ProcID = SRV_CUI_PORT;
		ptid.ThedID = INVALID;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR)
		{
			if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_CUIKEY && buf[4] == 27)	/*按下ESC键*/
			{
				CUIPutS("用户取消！\n");
				break;
			}
			else if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)	/*退出请求*/
				KExitProcess(NO_ERROR);
		}
		pid++;
	}
}

/*杀死进程*/
void killproc(char *args)
{
	if (KKillProcess(Atoi10(args)) != NO_ERROR)
		CUIPutS("强行结束进程失败！\n");
}

/*启动GUI和桌面应用程序*/
void startgui(char *args)
{
	THREAD_ID ptid;
	
	if (KGetKptThed(SRV_GUI_PORT, &ptid) == NO_ERROR)
	{
		CUIPutS("图形界面已启动！\n");
		return;
	}
	ptid.ProcID = SRV_CUI_PORT;
	ptid.ThedID = INVALID;
	SendExitProcReq(ptid);	/*关闭CUI*/
	KCreateProcess(0, "gui.bin", NULL, &ptid);
	KSleep(5);	/*延时,防止进程间依赖关系不满足*/
	KCreateProcess(0, "desktop.bin", NULL, &ptid);
	KSleep(5);
	KCreateProcess(0, "cui.bin", NULL, &ptid);	/*启动图像模式CUI*/
	KExitProcess(NO_ERROR);
}

/*发声*/
void sound(char *args)
{
	if (SPKSound(Atoi10(args)) != NO_ERROR)
	{
		CUIPutS("无法连接到扬声器驱动！\n");
		return;
	}
	KSleep(100);
	SPKNosound();
}

/*帮助*/
void help(char *args)
{
	CUIPutS(
		"cls:清屏\n"
		"color rrggbb rrggbb:设置前景和背景色\n"
		"exit:退出\n"
		"part:分区列表\n"
		"del path:删除文件或空目录\n"
		"copy SrcPath DestPath:复制文件\n"
		"ren path name:重命名\n"
		"dir DirPath:目录列表\n"
		"cd DirPath:显示或切换当前目录\n"
		"md DirPath:创建目录\n"
		"show FilePath:显示文件内容\n"
		"time:显示当前时间\n"
		"ps:进程列表\n"
		"kill ProcID:强行结束进程\n"
		"startgui:启动图形界面\n"
		"sound freq:以一定频率发声一秒\n"
		"help:帮助\n"
		"输入可执行文件路径将运行该程序\n");
}

/*命令匹配*/
char *cmdcmp(char *str1, char *str2)
{
	while (*str1)
		if (*str1++ != *str2++)
			return NULL;
	if (*str2 == '\0')
		return str2;
	if (*str2 == ' ')
		return ++str2;
	return NULL;
}

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

void CmdProc(char *str)
{
	struct
	{
		char *str;
		void (*cmd)(char *args);
	}CMD[] = {
		{"cls", cls},
		{"color", SetColor},
		{"exit", exitcmd},
		{"part", partlist},
		{"del", delfile},
		{"copy", copy},
		{"ren", ren},
		{"dir", dir},
		{"cd", cd},
		{"md", md},
		{"show", show},
		{"time", showtim},
		{"ps", proclist},
		{"kill", killproc},
		{"startgui", startgui},
		{"sound", sound},
		{"help", help}
	};
	long i;
	char *exec, *args;
	THREAD_ID ptid;
	char buf[40];
	for (i = 0; i < sizeof(CMD) / 8; i++)
		if ((args = cmdcmp(CMD[i].str, str)) != NULL)
		{
			CMD[i].cmd(args);	/*内部命令*/
			CUIPutS(PROMPT);
			return;
		}
	ProcStr(str, &exec, &args);
	if (KCreateProcess(0, exec, args, &ptid) != NO_ERROR)
		Sprintf(buf, "无效的命令或可执行文件!\n%s", PROMPT);
	else
		Sprintf(buf, "进程ID: %d\n%s", ptid.ProcID, PROMPT);
	CUIPutS(buf);
}

/*键盘输入响应*/
void KeyProc(char c)
{
	switch (c)
	{
	case '\0':
		return;
	case '\b':
		if (cmdp != cmd)
		{
			cmdp--;	/*删除字符*/
			CUIPutC('\b');
		}
		break;
	case '\r':
		CUIPutC('\n');
		*cmdp = '\0';
		CmdProc(cmd);	/*执行命令*/
		cmdp = cmd;
		*cmdp = '\0';
		break;
	default:
		if (cmdp - cmd < CMD_LEN - 1)
		{
			if (c == '\t')
				c = ' ';
			*cmdp++ = c;
			CUIPutC(c);
		}
		break;
	}
}

int main()
{
	THREAD_ID ptid;
	long res;

	if ((res = CUISetRecv()) != NO_ERROR)
		return res;
	CUIPutS(
		"欢迎来到\n"
		"┏┓┏┓┏┓　　┏━━┓┏━━┓┏━━┓\n"
		"┃┃┃┃┃┃　　┗┓┏┛┃┏┓┃┃┏━┛\n"
		"┃┃┃┃┃┃　　　┃┃　┃┃┃┃┃┗━┓\n"
		"┃┃┃┃┃┃　　　┃┃　┃┃┃┃┗━┓┃\n"
		"┃┗┛┃┃┗━┓┏┛┗┓┃┗┛┃┏━┛┃\n"
		"┗━━┛┗━━┛┗━━┛┗━━┛┗━━┛\n"
		"输入help获得命令帮助!\n");
	CUIPutS(PROMPT);
	cmdp = cmd;
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)
			break;
		if (data[MSG_ATTR_ID] == MSG_ATTR_CUIKEY)	/*按键消息*/
			KeyProc(data[1]);
		else if (data[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)
			break;
	}
	return NO_ERROR;
}
