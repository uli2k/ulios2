/*	fs.h for ulios file system
	作者：孙亮
	功能：文件系统相关结构、常量定义
	最后修改日期：2009-05-26
*/

#ifndef	_FS_H_
#define	_FS_H_

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"
#include "fsapi.h"

/**********动态内存管理相关**********/

typedef struct _FREE_BLK_DESC
{
	void *addr;						/*起始地址*/
	DWORD siz;						/*字节数*/
	struct _FREE_BLK_DESC *nxt;		/*后一项*/
}FREE_BLK_DESC;	/*自由块描述符*/

#define FDAT_SIZ		0x00700000	/*动态内存大小*/
#define FMT_LEN			0x100		/*动态内存管理表长度*/
extern FREE_BLK_DESC fmt[];			/*动态内存管理表*/
extern DWORD fmtl;					/*动态内存管理锁*/

/**********高速缓冲相关**********/

#define CACHE_ATTR_TIMES	0x00000003	/*0:缓冲块没有被访问过,1:缓冲块被访问了,2:缓冲块被再次访问,3:缓冲块被频繁访问*/
#define CACHE_ATTR_DIRTY	0x00000004	/*0:缓冲块只读过,1:缓冲块脏*/
typedef struct _CACHE_DESC
{
	DWORD DrvID;	/*设备号*/
	DWORD BlkID;	/*块号(哈希值)*/
}CACHE_DESC;	/*缓冲块描述符*/

#define BLK_SHIFT		9							/*缓冲块位移(硬盘每扇区2的9次方字节)*/
#define BLK_SIZ			(1ul << BLK_SHIFT)			/*设备每块字节数*/
#define BLKID_SHIFT		14							/*块号位移*/
#define BLKID_MASK		(INVALID << BLKID_SHIFT)	/*块号蒙板*/
#define BLKATTR_MASK	(~BLKID_MASK)				/*块属性蒙板*/
#define BMT_LEN			(1ul << BLKID_SHIFT)		/*缓冲块数量*/
#define MAXPROC_COU		0x80						/*最多处理块数量*/
#define PROC_INTERVAL	0x400						/*缓冲数据保存时间间隔*/
extern CACHE_DESC *bmt;		/*高速缓冲管理表*/
extern void *cache;			/*高速缓冲*/
extern DWORD cahl;			/*缓冲管理锁*/

/**********文件系统上层功能相关**********/

typedef struct _PART_DESC
{
	DWORD FsID;				/*文件系统接口ID*/
	DWORD DrvID;			/*所在磁盘ID*/
	DWORD SecID;			/*起始扇区号*/
	DWORD SeCou;			/*扇区数*/
	PART_INFO part;			/*分区可见信息*/
	void *data;				/*文件系统私有数据*/
}PART_DESC;	/*分区描述符*/

#define FILE_FLAG_WRITE		0x0001	/*0:只读方式1:可写方式*/
typedef struct _FILE_DESC
{
	PART_DESC *part;		/*所在分区*/
	WORD id, flag;			/*标志*/
	DWORD cou;				/*访问计数*/
	struct _FILE_DESC *par;	/*父目录*/
	FILE_INFO file;			/*文件可见信息*/
	DWORD idx;				/*目录中的编号*/
	void *data;				/*文件系统私有数据*/
}FILE_DESC;	/*文件描述符*/

typedef struct _FILE_HANDLE
{
	FILE_DESC *fd;			/*文件描述符*/
	DWORD avl;				/*文件系统自定*/
	QWORD seek;				/*读写指针*/
}FILE_HANDLE;	/*打开文件句柄*/

#define EXEC_DFTENTRY		0x08000000	/*可执行文件默认入口*/
#define FHT_LEN				0x100
typedef struct _PROCRES_DESC
{
	FILE_DESC *exec;		/*可执行文件*/
	DWORD CodeOff;			/*代码段开始地址*/
	DWORD CodeEnd;			/*代码段结束地址*/
	DWORD CodeSeek;			/*代码段文件偏移*/
	DWORD DataOff;			/*数据段开始地址*/
	DWORD DataEnd;			/*数据段结束地址*/
	DWORD DataSeek;			/*数据段文件偏移*/
	DWORD entry;			/*入口点*/
	FILE_DESC *CurDir;		/*进程当前目录*/
	FILE_HANDLE fht[FHT_LEN];	/*打开文件列表*/
}PROCRES_DESC;	/*进程资源描述符*/

#define PART_LEN	0x40	/*最多支持64个分区*/
extern PART_DESC* part;		/*分区信息表*/
#define FILT_LEN	0x1000	/*最多打开4k个目录项*/
extern FILE_DESC** filt;	/*打开文件指针表*/
extern FILE_DESC** FstFd;	/*第一个空文件描述符指针*/
extern FILE_DESC** EndFd;	/*最后非空文件描述符下一项指针*/
#define PRET_LEN	0x0400	/*进程资源1k个,与内核最多进程数相等*/
extern PROCRES_DESC** pret;	/*进程资源表*/
extern DWORD prel;			/*进程资源表管理锁*/
extern DWORD SubthCou;		/*子线程计数*/

typedef struct _FSUI
{
	char name[8];			/*文件系统标示*/
	long (*MntPart)(PART_DESC *pd);					/*挂载分区*/
	void (*UmntPart)(PART_DESC *pd);				/*卸载分区*/
	long (*SetPart)(PART_DESC *pd, PART_INFO *pi);	/*设置分区信息*/
	BOOL (*CmpFile)(FILE_DESC *fd, const char *path);	/*比较路径字符串与文件名是否匹配*/
	long (*SchFile)(FILE_DESC *fd, const char *path);	/*搜索并设置文件项*/
	long (*NewFile)(FILE_DESC *fd, const char *path);	/*创建并设置文件项*/
	long (*DelFile)(FILE_DESC *fd);					/*删除文件项*/
	long (*SetFile)(FILE_DESC *fd, FILE_INFO *fi);	/*设置文件项信息*/
	long (*SetSize)(FILE_DESC *fd, QWORD siz);		/*设置文件项长度*/
	long (*RwFile)(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, void *buf, DWORD *avl);	/*读写文件项*/
	long (*ReadDir)(FILE_DESC *fd, QWORD *seek, FILE_INFO *fi, DWORD *avl);	/*获取目录列表*/
	void (*FreeData)(FILE_DESC *fd);				/*释放文件描述符中的私有数据*/
}FSUI;	/*文件系统上层接口*/

/*自由块分配*/
void *malloc(DWORD siz);

/*自由块回收*/
void free(void *addr, DWORD siz);

/*缓冲读写*/
long RwCache(DWORD drv, BOOL isWrite, DWORD sec, DWORD cou, void *buf);

/*读写磁盘*/
static inline long RwHd(DWORD drv, BOOL isWrite, DWORD sec, DWORD cou, void *buf)
{
	return RwCache(drv, isWrite, sec, cou, buf);
}

/*读写分区*/
static inline long RwPart(PART_DESC *part, BOOL isWrite, DWORD sec, DWORD cou, void *buf)
{
	return RwCache(part->DrvID, isWrite, part->SecID + sec, cou, buf);
}

/*保存脏数据*/
long SaveCache();

#endif
