/*	fsapi.c for ulios file system
	作者：孙亮
	功能：文件系统接口，响应应用程序的请求，执行服务
	最后修改日期：2010-04-24
*/

#include "fs.h"

extern long InitFS();
extern void InitPart();
extern void CloseFS();
extern long GetExec(PROCRES_DESC *pres, const char *path, DWORD pid, DWORD *exec);
extern long ReadPage(PROCRES_DESC *pres, void *buf, DWORD siz, DWORD seek);
extern long ProcExit(PROCRES_DESC *pres);
extern long EnumPart(DWORD *pid);
extern long GetPart(DWORD pid, PART_INFO *pi);
extern long SetPart(DWORD pid, PART_INFO *pi);
extern long creat(PROCRES_DESC *pres, const char *path, DWORD *fhi);
extern long open(PROCRES_DESC *pres, const char *path, BOOL isWrite, DWORD *fhi);
extern long close(PROCRES_DESC *pres, DWORD fhi);
extern long read(PROCRES_DESC *pres, DWORD fhi, void *buf, DWORD *siz);
extern long write(PROCRES_DESC *pres, DWORD fhi, void *buf, DWORD *siz);
extern long seek(PROCRES_DESC *pres, DWORD fhi, SQWORD seek, DWORD from);
extern long SetSize(PROCRES_DESC *pres, DWORD fhi, QWORD siz);
extern long OpenDir(PROCRES_DESC *pres, const char *path, DWORD *fhi);
extern long ReadDir(PROCRES_DESC *pres, DWORD fhi, FILE_INFO *fi);
extern long ChDir(PROCRES_DESC *pres, const char *path);
extern long GetCwd(PROCRES_DESC *pres, char *path, DWORD *siz);
extern long MkDir(PROCRES_DESC *pres, const char *path);
extern long remove(PROCRES_DESC *pres, const char *path);
extern long rename(PROCRES_DESC *pres, const char *path, const char *name);
extern long GetAttr(PROCRES_DESC *pres, const char *path, FILE_INFO *fi);
extern long SetAttr(PROCRES_DESC *pres, const char *path, DWORD attr);
extern long SetTime(PROCRES_DESC *pres, const char *path, DWORD time, DWORD cma);
extern long ProcInfo(DWORD *pid, FILE_INFO *fi);

#define PTID_ID	MSG_DATA_LEN

static const char *CheckPathSize(const char *path, DWORD siz)
{
	if (siz > MAX_PATH)
		return NULL;
	do
	{
		if (*path == 0)
			return path;
		path++;
	}
	while (--siz);
	return NULL;
}

void ApiGetExec(DWORD *argv)
{
	const char *path;
	DWORD exec[8];

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	path = (const char*)argv[MSG_ADDR_ID];
	if (CheckPathSize(path, argv[MSG_SIZE_ID]) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = GetExec(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, argv[3], exec);
	KUnmapProcAddr((void*)path, argv);
	if (argv[MSG_RES_ID] == NO_ERROR)
	{
		exec[MSG_ATTR_ID] |= MSG_ATTR_FS;
		KSendMsg((THREAD_ID*)&argv[PTID_ID], exec, 0);
	}
}

void ApiReadPage(DWORD *argv)
{
	void *buf;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	if ((argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	buf = (void*)argv[MSG_ADDR_ID];
	argv[MSG_RES_ID] = ReadPage(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], buf, argv[MSG_SIZE_ID], argv[3]);
	KUnmapProcAddr(buf, argv);
}

void ApiProcExit(DWORD *argv)
{
	PROCRES_DESC *CurPres;

	CurPres = pret[((THREAD_ID*)&argv[PTID_ID])->ProcID];
	ProcExit(CurPres);
	free(CurPres, sizeof(PROCRES_DESC));
	pret[((THREAD_ID*)&argv[PTID_ID])->ProcID] = NULL;
}

void ApiEnumPart(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	argv[MSG_RES_ID] = EnumPart(&argv[1]);
	KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiGetPart(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	if ((argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_ROMAP || argv[MSG_SIZE_ID] < sizeof(PART_INFO) + 8)
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	argv[MSG_RES_ID] = GetPart(argv[3], (PART_INFO*)argv[MSG_ADDR_ID]);
	KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
}

void ApiSetPart(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	if (argv[MSG_SIZE_ID] < sizeof(PART_INFO))
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	argv[MSG_RES_ID] = SetPart(argv[3], (PART_INFO*)argv[MSG_ADDR_ID]);
	KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
}

void ApiCreat(DWORD *argv)
{
	const char *path;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	path = (const char*)argv[MSG_ADDR_ID];
	if (CheckPathSize(path, argv[MSG_SIZE_ID]) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = creat(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, &argv[2]);
	KUnmapProcAddr((void*)path, argv);
}

void ApiOpen(DWORD *argv)
{
	const char *path;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	path = (const char*)argv[MSG_ADDR_ID];
	if (CheckPathSize(path, argv[MSG_SIZE_ID]) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = open(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, argv[3], &argv[2]);
	KUnmapProcAddr((void*)path, argv);
}

void ApiClose(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	argv[MSG_RES_ID] = close(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[1]);
	KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiRead(DWORD *argv)
{
	void *buf;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	if ((argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	buf = (void*)argv[MSG_ADDR_ID];
	argv[MSG_RES_ID] = read(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[3], buf, &argv[MSG_SIZE_ID]);
	KUnmapProcAddr(buf, argv);
}

void ApiWrite(DWORD *argv)
{
	void *buf;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	buf = (void*)argv[MSG_ADDR_ID];
	argv[MSG_RES_ID] = write(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[3], buf, &argv[MSG_SIZE_ID]);
	KUnmapProcAddr(buf, argv);
}

void ApiSeek(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	argv[MSG_RES_ID] = seek(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[1], *((SQWORD*)&argv[2]), argv[4]);
	KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiSetSize(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	argv[MSG_RES_ID] = SetSize(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[1], *((QWORD*)&argv[2]));
	KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiOpenDir(DWORD *argv)
{
	const char *path;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	path = (const char*)argv[MSG_ADDR_ID];
	if (CheckPathSize(path, argv[MSG_SIZE_ID]) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = OpenDir(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, &argv[2]);
	KUnmapProcAddr((void*)path, argv);
}

void ApiReadDir(DWORD *argv)
{
	FILE_INFO *buf;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	if ((argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_ROMAP || argv[MSG_SIZE_ID] < sizeof(FILE_INFO))
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	buf = (FILE_INFO*)argv[MSG_ADDR_ID];
	argv[MSG_RES_ID] = ReadDir(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], argv[3], buf);
	KUnmapProcAddr((void*)buf, argv);
}

void ApiChDir(DWORD *argv)
{
	const char *path;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	path = (const char*)argv[MSG_ADDR_ID];
	if (CheckPathSize(path, argv[MSG_SIZE_ID]) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = ChDir(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path);
	KUnmapProcAddr((void*)path, argv);
}

void ApiGetCwd(DWORD *argv)
{
	char *path;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	if ((argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	path = (char*)argv[MSG_ADDR_ID];
	argv[MSG_RES_ID] = GetCwd(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, &argv[MSG_SIZE_ID]);
	KUnmapProcAddr((void*)path, argv);
}

void ApiMkDir(DWORD *argv)
{
	const char *path;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	path = (const char*)argv[MSG_ADDR_ID];
	if (CheckPathSize(path, argv[MSG_SIZE_ID]) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = MkDir(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path);
	KUnmapProcAddr((void*)path, argv);
}

void ApiRemove(DWORD *argv)
{
	const char *path;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	path = (const char*)argv[MSG_ADDR_ID];
	if (CheckPathSize(path, argv[MSG_SIZE_ID]) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = remove(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path);
	KUnmapProcAddr((void*)path, argv);
}

void ApiReName(DWORD *argv)
{
	const char *path, *name;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	path = (const char*)argv[MSG_ADDR_ID];
	if ((name = CheckPathSize(path, argv[MSG_SIZE_ID])) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else if (name++, CheckPathSize(name, argv[MSG_SIZE_ID] - (name - path)) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = rename(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, name);
	KUnmapProcAddr((void*)path, argv);
}

void ApiGetAttr(DWORD *argv)
{
	char *path;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	if ((argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_ROMAP || argv[MSG_SIZE_ID] < sizeof(FILE_INFO))
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	path = (char*)argv[MSG_ADDR_ID];
	if (CheckPathSize(path, argv[MSG_SIZE_ID]) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = GetAttr(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, (FILE_INFO*)path);
	KUnmapProcAddr(path, argv);
}

void ApiSetAttr(DWORD *argv)
{
	const char *path;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	path = (const char*)argv[MSG_ADDR_ID];
	if (CheckPathSize(path, argv[MSG_SIZE_ID]) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = SetAttr(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, argv[3]);
	KUnmapProcAddr((void*)path, argv);
}

void ApiSetTime(DWORD *argv)
{
	const char *path;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	path = (const char*)argv[MSG_ADDR_ID];
	if (CheckPathSize(path, argv[MSG_SIZE_ID]) == NULL)
		argv[MSG_RES_ID] = FS_ERR_ARGS_TOOLONG;
	else
		argv[MSG_RES_ID] = SetTime(pret[((THREAD_ID*)&argv[PTID_ID])->ProcID], path, argv[3], argv[4]);
	KUnmapProcAddr((void*)path, argv);
}

void ApiProcInfo(DWORD *argv)
{
	FILE_INFO *buf;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	if ((argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_ROMAP || argv[MSG_SIZE_ID] < sizeof(FILE_INFO))
	{
		argv[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	buf = (FILE_INFO*)argv[MSG_ADDR_ID];
	argv[MSG_RES_ID] = ProcInfo(&argv[3], buf);
	KUnmapProcAddr((void*)buf, argv);
}

/*系统调用表*/
void (*ApiTable[])(DWORD *argv) = {
	ApiGetExec, ApiReadPage, ApiProcExit, ApiEnumPart, ApiGetPart, ApiSetPart, ApiCreat, ApiOpen,
	ApiClose, ApiRead, ApiWrite, ApiSeek, ApiSetSize, ApiOpenDir, ApiReadDir, ApiChDir,
	ApiGetCwd, ApiMkDir, ApiRemove, ApiReName, ApiGetAttr, ApiSetAttr, ApiSetTime, ApiProcInfo
};

/*高速缓冲保存线程*/
void CacheProc(DWORD interval)
{
	for (;;)
	{
		KSleep(interval);
		SaveCache();
	}
}

void ApiProc(DWORD *argv)
{
	PROCRES_DESC *CurPres;

	if ((CurPres = pret[((THREAD_ID*)&argv[PTID_ID])->ProcID]) == NULL)
	{
		if ((CurPres = (PROCRES_DESC*)malloc(sizeof(PROCRES_DESC))) == NULL)
		{
			argv[MSG_RES_ID] = FS_ERR_HAVENO_MEMORY;
			if (argv[MSG_ATTR_ID] < MSG_ATTR_USER)
				KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
			else
				KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
			free(argv, sizeof(DWORD) * (MSG_DATA_LEN + 1));
			KExitThread(FS_ERR_HAVENO_MEMORY);
		}
		memset32(CurPres, 0, sizeof(PROCRES_DESC) / sizeof(DWORD));
		pret[((THREAD_ID*)&argv[PTID_ID])->ProcID] = CurPres;
	}
	ApiTable[argv[MSG_API_ID] & MSG_API_MASK](argv);
	free(argv, sizeof(DWORD) * (MSG_DATA_LEN + 1));
	KExitThread(NO_ERROR);
}

int main()
{
	THREAD_ID ptid;
	long res;

	if ((res = InitFS()) != NO_ERROR)
		return res;
	InitPart();
	KCreateThread((void(*)(void*))CacheProc, 0x40000, (void*)PROC_INTERVAL, &ptid);	/*启动后台定时缓冲保存线程*/
	for (;;)
	{
		DWORD data[MSG_DATA_LEN + 1];

		if ((res = KRecvMsg((THREAD_ID*)&data[PTID_ID], data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_THEDEXIT)	/*子线程退出*/
			SubthCou--;
		else if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_PROCEXIT)
			data[MSG_ATTR_ID] = MSG_ATTR_FS | FS_API_PROCEXIT;
		if ((data[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP || (data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_FS)
		{
			DWORD *buf;

			if ((data[MSG_API_ID] & MSG_API_MASK) >= sizeof(ApiTable) / sizeof(void*))
			{
				data[MSG_RES_ID] = FS_ERR_WRONG_ARGS;
				if (data[MSG_ATTR_ID] < MSG_ATTR_USER)
					KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
				else
					KSendMsg((THREAD_ID*)&data[PTID_ID], data, 0);
			}
			else if ((buf = (DWORD*)malloc(sizeof(DWORD) * (MSG_DATA_LEN + 1))) == NULL)
			{
				data[MSG_RES_ID] = FS_ERR_HAVENO_MEMORY;
				if (data[MSG_ATTR_ID] < MSG_ATTR_USER)
					KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
				else
					KSendMsg((THREAD_ID*)&data[PTID_ID], data, 0);
			}
			else
			{
				memcpy32(buf, data, MSG_DATA_LEN + 1);
				KCreateThread((void(*)(void*))ApiProc, 0x40000, buf, &ptid);
				SubthCou++;
			}
		}
		else if (data[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)
			break;
	}
	CloseFS();
	return NO_ERROR;
}
