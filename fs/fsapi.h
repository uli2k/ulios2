/*	fsapi.h for ulios file system
	作者：孙亮
	功能：ulios文件系统服务API接口定义，调用文件系统服务需要包含此文件
	最后修改日期：2010-04-30
*/

#ifndef	_FSAPI_H_
#define	_FSAPI_H_

#include "../MkApi/ulimkapi.h"

#define LABEL_SIZE			64
#define PART_ATTR_RDONLY	0x00000001	/*只读*/
typedef struct _PART_INFO
{
	char label[LABEL_SIZE];	/*卷标名*/
	QWORD size;				/*分区字节数*/
	QWORD remain;			/*剩余字节数*/
	DWORD attr;				/*属性*/
}PART_INFO;	/*分区信息*/

#define FILE_NAME_SIZE		256
#define FILE_ATTR_RDONLY	0x00000001	/*只读*/
#define FILE_ATTR_HIDDEN	0x00000002	/*隐藏*/
#define FILE_ATTR_SYSTEM	0x00000004	/*系统*/
#define FILE_ATTR_LABEL		0x00000008	/*卷标(只读)*/
#define FILE_ATTR_DIREC		0x00000010	/*目录(只读)*/
#define FILE_ATTR_ARCH		0x00000020	/*归档*/
#define FILE_ATTR_EXEC		0x00000040	/*可执行*/
#define FILE_ATTR_DIRTY		0x40000000	/*数据不一致*/
#define FILE_ATTR_UNMDFY	0x80000000	/*属性不可修改*/
typedef struct _FILE_INFO
{
	char name[FILE_NAME_SIZE];	/*文件名*/
	DWORD CreateTime;			/*创建时间1970-01-01经过的秒数*/
	DWORD ModifyTime;			/*修改时间*/
	DWORD AccessTime;			/*访问时间*/
	DWORD attr;					/*属性*/
	QWORD size;					/*文件字节数*/
}FILE_INFO;	/*文件信息*/

/*open参数*/
#define FS_OPEN_READ	0	/*打开文件读*/
#define FS_OPEN_WRITE	1	/*打开文件写*/

/*seek参数*/
#define FS_SEEK_SET		0	/*从文件头定位*/
#define FS_SEEK_CUR		1	/*从当前位置定位*/
#define FS_SEEK_END		2	/*从文件尾定位*/

/*settime参数*/
#define FS_SETIM_CREATE	1	/*设置创建时间*/
#define FS_SETIM_MODIFY	2	/*设置修改时间*/
#define FS_SETIM_ACCESS	4	/*设置访问时间*/

#define MAX_PATH	1024	/*路径最多字节数*/

#define SRV_FS_PORT		1	/*文件系统服务端口*/

#define MSG_ATTR_FS		0x01010000	/*文件系统消息*/

#define FS_API_GETEXEC	0	/*内核专用*/
#define FS_API_READPAGE	1	/*内核专用*/
#define FS_API_PROCEXIT	2	/*内核专用*/
#define FS_API_ENUMPART	3	/*枚举分区*/
#define FS_API_GETPART	4	/*取得分区信息*/
#define FS_API_SETPART	5	/*设置分区信息*/
#define FS_API_CREAT	6	/*创建文件*/
#define FS_API_OPEN		7	/*打开文件*/
#define FS_API_CLOSE	8	/*关闭文件*/
#define FS_API_READ		9	/*读取文件*/
#define FS_API_WRITE	10	/*写入文件*/
#define FS_API_SEEK		11	/*设置读写指针*/
#define FS_API_SETSIZE	12	/*设置文件大小*/
#define FS_API_OPENDIR	13	/*打开目录*/
#define FS_API_READDIR	14	/*读取目录*/
#define FS_API_CHDIR	15	/*切换当前目录*/
#define FS_API_GETCWD	16	/*取得当前目录*/
#define FS_API_MKDIR	17	/*创建目录*/
#define FS_API_REMOVE	18	/*删除文件或空目录*/
#define FS_API_RENAME	19	/*重命名文件或目录*/
#define FS_API_GETATTR	20	/*取得文件或目录的属性信息*/
#define FS_API_SETATTR	21	/*设置文件或目录的属性*/
#define FS_API_SETTIME	22	/*设置文件或目录的时间*/
#define FS_API_PROCINFO	23	/*取得进程信息*/

/*错误定义*/
#define FS_ERR_HAVENO_FILD		-256	/*文件描述符不足*/
#define FS_ERR_HAVENO_HANDLE	-257	/*句柄不足*/
#define FS_ERR_HAVENO_MEMORY	-258	/*内存不足*/
#define FS_ERR_HAVENO_SPACE		-259	/*磁盘空间不足*/
#define FS_ERR_WRONG_ARGS		-260	/*参数错误*/
#define FS_ERR_WRONG_HANDLE		-261	/*句柄错误*/
#define FS_ERR_WRONG_CURDIR		-262	/*当前目录错误*/
#define FS_ERR_WRONG_PARTID		-263	/*错误的分区ID*/
#define FS_ERR_NAME_FORMAT		-264	/*名称格式错*/
#define FS_ERR_NAME_TOOLONG		-265	/*名称超长*/
#define FS_ERR_ARGS_TOOLONG		-266	/*参数超长*/
#define FS_ERR_PATH_FORMAT		-267	/*路径格式错*/
#define FS_ERR_PATH_EXISTS		-268	/*路径已存在*/
#define FS_ERR_PATH_NOT_FOUND	-269	/*路径未找到*/
#define FS_ERR_PATH_NOT_DIR		-270	/*路径不是目录*/
#define FS_ERR_PATH_NOT_FILE	-271	/*路径不是文件*/
#define FS_ERR_PATH_READED		-272	/*路径已被打开读*/
#define FS_ERR_PATH_WRITTEN		-273	/*路径已被打开写*/
#define FS_ERR_ATTR_RDONLY		-274	/*属性只读或系统*/
#define FS_ERR_ATTR_UNMDFY		-275	/*属性不可修改*/
#define FS_ERR_DIR_NOT_EMPTY	-276	/*目录非空*/
#define FS_ERR_END_OF_FILE		-277	/*到达文件,目录,进程表结尾*/
#define FS_ERR_SIZE_LIMIT		-278	/*数量空间限制*/
#define FS_ERR_FILE_EMPTY		-279	/*空文件*/

/*枚举分区*/
static inline long FSEnumPart(DWORD *pid)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_FS | FS_API_ENUMPART;
	data[1] = *pid;
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	*pid = data[1];
	return data[MSG_RES_ID];
}

/*取得分区信息*/
static inline long FSGetPart(DWORD pid, PART_INFO *pi)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_GETPART;
	data[3] = pid;
	if ((data[0] = KReadProcAddr(pi, sizeof(PART_INFO) + 8, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*设置分区信息*/
static inline long FSSetPart(DWORD pid, PART_INFO *pi)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_SETPART;
	data[3] = pid;
	if ((data[0] = KWriteProcAddr(pi, sizeof(PART_INFO), &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*创建文件*/
static inline long FScreat(const char *path)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_CREAT;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[2];
}

/*打开文件*/
static inline long FSopen(const char *path, DWORD op)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_OPEN;
	data[3] = op;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[2];
}

/*关闭文件*/
static inline long FSclose(DWORD handle)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_FS | FS_API_CLOSE;
	data[1] = handle;
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*读取文件*/
static inline long FSread(DWORD handle, void *buf, DWORD siz)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_READ;
	data[3] = handle;
	if ((data[0] = KReadProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[MSG_SIZE_ID];
}

/*写入文件*/
static inline long FSwrite(DWORD handle, void *buf, DWORD siz)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_WRITE;
	data[3] = handle;
	if ((data[0] = KWriteProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[MSG_SIZE_ID];
}

/*设置读写指针*/
static inline long FSseek(DWORD handle, SQWORD seek, DWORD from)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_FS | FS_API_SEEK;
	data[1] = handle;
	*((SQWORD*)&data[2]) = seek;
	data[4] = from;
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*设置文件大小*/
static inline long FSSetSize(DWORD handle, QWORD siz)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_FS | FS_API_SETSIZE;
	data[1] = handle;
	*((QWORD*)&data[2]) = siz;
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*打开目录*/
static inline long FSOpenDir(const char *path)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_OPENDIR;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[2];
}

/*读取目录*/
static inline long FSReadDir(DWORD handle, FILE_INFO *fi)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_READDIR;
	data[3] = handle;
	if ((data[0] = KReadProcAddr(fi, sizeof(FILE_INFO), &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*切换当前目录*/
static inline long FSChDir(const char *path)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_CHDIR;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*取得当前目录*/
static inline long FSGetCwd(char *path, DWORD siz)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_GETCWD;
	if ((data[0] = KReadProcAddr(path, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[MSG_SIZE_ID];
}

/*创建目录*/
static inline long FSMkDir(const char *path)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_MKDIR;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*删除文件或空目录*/
static inline long FSremove(const char *path)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_REMOVE;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*重命名文件或目录*/
static inline long FSrename(const char *path, const char *name)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_RENAME;
	if ((data[0] = KWriteProcAddr(buf, strcpy(strcpy(buf, path), name) - buf, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*取得文件或目录的属性信息*/
static inline long FSGetAttr(const char *path, FILE_INFO *fi)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_GETATTR;
	if ((data[0] = KReadProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	memcpy32(fi, buf, sizeof(FILE_INFO) / sizeof(DWORD));
	return NO_ERROR;
}

/*设置文件或目录的属性*/
static inline long FSSetAttr(const char *path, DWORD attr)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_SETATTR;
	data[3] = attr;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*设置文件或目录的时间*/
static inline long FSSetTime(const char *path, DWORD time, DWORD cma)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_SETTIME;
	data[3] = time;
	data[4] = cma;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*取得进程信息*/
static inline long FSProcInfo(DWORD *pid, FILE_INFO *fi)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_PROCINFO;
	data[3] = *pid;
	if ((data[0] = KReadProcAddr(fi, sizeof(FILE_INFO), &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	*pid = data[3];
	return data[MSG_RES_ID];
}

#endif
