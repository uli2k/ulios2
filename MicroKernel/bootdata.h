/*	bootdata.h for ulios
	作者：孙亮
	功能：实模式下初始化数据结构定义
	最后修改日期：2009-06-17
*/

#ifndef	_BOOTDATA_H_
#define	_BOOTDATA_H_

#include "ulidef.h"

typedef struct _PHYBLK_DESC
{
	DWORD addr;	/*起始物理地址*/
	DWORD siz;	/*字节数*/
}PHYBLK_DESC;	/*物理地址块描述符*/

#define ARDS_TYPE_RAM		1	/*可用随机存储器*/
#define ARDS_TYPE_RESERVED	2	/*保留*/
#define ARDS_TYPE_ACPI		3	/*ACPI专用*/
#define ARDS_TYPE_NVS		4	/*不挥发存储器(ROM/FLASH)*/

typedef struct _MEM_ARDS
{
	DWORD addr;	/*起始物理地址*/
	DWORD addrhi;
	DWORD siz;	/*字节数*/
	DWORD sizhi;
	DWORD type;	/*类型*/
}MEM_ARDS;	/*内存结构表*/

/**********启动数据**********/

extern PHYBLK_DESC BaseSrv[];	/*8 * 16字节基础服务程序段表*/
extern DWORD MemEnd;		/*内存上限*/
extern MEM_ARDS ards[];		/*20 * 19字节内存结构体*/

#endif
