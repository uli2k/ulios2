/*	bsrvapi.h for ulios driver
	作者：孙亮
	功能：ulios基础服务API接口定义，调用基础服务需要包含此文件
	最后修改日期：2010-03-09
*/

#ifndef	_BSRVAPI_H_
#define	_BSRVAPI_H_

#include "../MkApi/ulimkapi.h"

#define SRV_OUT_TIME	6000	/*服务调用超时厘秒数INVALID:无限等待*/

/**********进程异常报告**********/
#define SRV_REP_PORT	0	/*进程异常报告服务端口*/

/**********AT硬盘相关**********/
#define SRV_ATHD_PORT	2	/*AT硬盘驱动服务端口*/
#define ATHD_BPS		512	/*磁盘每扇区字节数*/

#define ATHD_API_WRITESECTOR	0	/*写硬盘扇区功能号*/
#define ATHD_API_READSECTOR		1	/*读硬盘扇区功能号*/

#define ATHD_ERR_WRONG_DRV		-512	/*错误的驱动器号*/
#define ATHD_ERR_HAVENO_REQ		-513	/*无法接受更多的服务请求*/

/*写硬盘扇区*/
static inline long HDWriteSector(DWORD drv, DWORD sec, BYTE cou, void *buf)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_ATHD_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = ATHD_API_WRITESECTOR;
	data[3] = drv;
	data[4] = sec;
	if ((data[0] = KWriteProcAddr(buf, ATHD_BPS * cou, &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*读硬盘扇区*/
static inline long HDReadSector(DWORD drv, DWORD sec, BYTE cou, void *buf)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_ATHD_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = ATHD_API_READSECTOR;
	data[3] = drv;
	data[4] = sec;
	if ((data[0] = KReadProcAddr(buf, ATHD_BPS * cou, &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/**********时间服务相关**********/
#define SRV_TIME_PORT	3	/*时间服务端口*/

#define MSG_ATTR_TIME	0x01030000	/*时间服务消息*/

#define TIME_API_CURSECOND		0	/*取得1970年经过的秒功能号*/
#define TIME_API_CURTIME		1	/*取得当前时间功能号*/
#define TIME_API_MKTIME			2	/*TM结构转换为秒功能号*/
#define TIME_API_LOCALTIME		3	/*秒转换为TM结构功能号*/
#define TIME_API_GETRAND		4	/*取得随机数功能号*/

#define TIME_ERR_WRONG_TM		-768	/*非法TM结构*/

typedef struct _TM
{
	BYTE sec;	/*秒[0,59]*/
	BYTE min;	/*分[0,59]*/
	BYTE hor;	/*时[0,23]*/
	BYTE day;	/*日[1,31]*/
	BYTE mon;	/*月[1,12]*/
	BYTE wday;	/*星期[0,6]*/
	WORD yday;	/*一年中的第几天[0,365]*/
	WORD yer;	/*年[1970,...]*/
	WORD mil;	/*毫秒[0,999]*/
}TM;	/*时间结构*/

/*取得1970年经过的秒*/
static inline long TMCurSecond(DWORD *sec)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_TIME_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_TIME | TIME_API_CURSECOND;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*sec = data[1];
	return NO_ERROR;
}

/*取得当前时间*/
static inline long TMCurTime(TM *tm)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_TIME_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_TIME | TIME_API_CURTIME;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	memcpy32(tm, &data[1], sizeof(TM) / sizeof(DWORD));
	return NO_ERROR;
}

/*TM结构转换为秒*/
static inline long TMMkTime(DWORD *sec, const TM *tm)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_TIME_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_TIME | TIME_API_MKTIME;
	memcpy32(&data[1], tm, sizeof(TM) / sizeof(DWORD));
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	*sec = data[1];
	return NO_ERROR;
}

/*秒转换为TM结构*/
static inline long TMLocalTime(DWORD sec, TM *tm)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_TIME_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_TIME | TIME_API_LOCALTIME;
	data[1] = sec;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	memcpy32(tm, &data[1], sizeof(TM) / sizeof(DWORD));
	return NO_ERROR;
}

/*取得随机数*/
static inline long TMGetRand()
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_TIME_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_TIME | TIME_API_GETRAND;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[1];
}

/**********键盘鼠标服务相关**********/
#define SRV_KBDMUS_PORT	4	/*键盘鼠标服务端口*/

#define MSG_ATTR_KBDMUS	0x01040000	/*键盘鼠标消息*/
#define MSG_ATTR_KBD	0x01040001	/*键盘按键消息*/
#define MSG_ATTR_MUS	0x01040002	/*鼠标状态消息*/

#define KBD_STATE_LSHIFT	0x00010000
#define KBD_STATE_RSHIFT	0x00020000
#define KBD_STATE_LCTRL		0x00040000
#define KBD_STATE_RCTRL		0x00080000
#define KBD_STATE_LALT		0x00100000
#define KBD_STATE_RALT		0x00200000
#define KBD_STATE_PAUSE		0x00400000
#define KBD_STATE_PRTSC		0x00800000
#define KBD_STATE_SCRLOCK	0x01000000
#define KBD_STATE_NUMLOCK	0x02000000
#define KBD_STATE_CAPSLOCK	0x04000000
#define KBD_STATE_INSERT	0x08000000

#define MUS_STATE_LBUTTON	0x01
#define MUS_STATE_RBUTTON	0x02
#define MUS_STATE_MBUTTON	0x04
#define MUS_STATE_XSIGN		0x10
#define MUS_STATE_YSIGN		0x20
#define MUS_STATE_XOVRFLW	0x40
#define MUS_STATE_YOVRFLW	0x80

#define KBDMUS_API_SETRECV		0	/*注册接收键盘鼠标消息的线程功能号*/

/*注册接收键盘鼠标消息的线程*/
static inline long KMSetRecv()
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_KBDMUS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_KBDMUS | KBDMUS_API_SETRECV;
	return KSendMsg(&ptid, data, 0);
}

/**********VESA显卡驱动服务和GDI库相关**********/
#define SRV_VESA_PORT	5	/*VESA显卡服务端口*/
#define VESA_MAX_MODE	512	/*显示模式列表最大数量*/

#define MSG_ATTR_VESA	0x01050000	/*VESA显卡消息*/

#define VESA_API_GETVMEM	0	/*取得显存映射功能号*/
#define VESA_API_GETMODE	1	/*取得模式列表功能号*/

#define VESA_ERR_ARGS		-1280	/*参数错误*/

/*取得显存映射*/
static inline long VSGetVmem(void **vm, DWORD *width, DWORD *height, DWORD *PixBits, DWORD *CurMode)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_VESA_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_VESA | VESA_API_GETVMEM;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	*vm = (void*)data[MSG_ADDR_ID];
	*width = data[3];
	*height = data[4];
	*PixBits = data[5];
	*CurMode = data[6];
	return NO_ERROR;
}

/*取得模式列表*/
static inline long VSGetMode(WORD mode[VESA_MAX_MODE], DWORD *ModeCou, DWORD *CurMode)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_VESA_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = VESA_API_GETMODE;
	if ((data[0] = KReadProcAddr(mode, VESA_MAX_MODE * sizeof(WORD), &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	*ModeCou = data[MSG_SIZE_ID];
	*CurMode = data[3];
	return NO_ERROR;
}

/**********点阵字体服务相关**********/
#define SRV_FONT_PORT	6	/*点阵字体服务端口*/

#define MSG_ATTR_FONT	0x01060000	/*点阵字体服务消息*/

#define FONT_API_GETFONT	0	/*取得字体映射功能号*/

#define FONT_ERR_ARGS		-1536	/*参数错误*/

/*取得字体映射*/
static inline long FNTGetFont(const BYTE **font, DWORD *CharWidth, DWORD *CharHeight)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FONT_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_FONT | FONT_API_GETFONT;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	*font = (const BYTE*)data[MSG_ADDR_ID];
	*CharWidth = data[3];
	*CharHeight = data[4];
	return NO_ERROR;
}

/**********CUI字符界面服务相关**********/
#define SRV_CUI_PORT	7	/*CUI字符界面服务端口*/

#define MSG_ATTR_CUI	0x01070000	/*CUI字符界面消息*/
#define MSG_ATTR_CUIKEY	0x01070001	/*CUI按键消息*/

#define CUI_API_SETRECV	0	/*注册接收按键消息的线程功能号*/
#define CUI_API_GETCOL	1	/*取得字符界面颜色功能号*/
#define CUI_API_SETCOL	2	/*设置字符界面颜色功能号*/
#define CUI_API_GETCUR	3	/*取得光标位置功能号*/
#define CUI_API_SETCUR	4	/*设置光标位置功能号*/
#define CUI_API_CLRSCR	5	/*清屏功能号*/
#define CUI_API_PUTC	6	/*输出字符功能号*/
#define CUI_API_PUTS	7	/*输出字符串功能号*/

#define CUI_ERR_ARGS	-1792	/*参数错误*/

/*注册接收按键消息的线程*/
static inline long CUISetRecv()
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_CUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_SETRECV;
	return KSendMsg(&ptid, data, 0);
}

/*取得字符界面颜色*/
static inline long CUIGetCol(DWORD *CharColor, DWORD *BgColor)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_CUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_GETCOL;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*CharColor = data[1];
	*BgColor = data[2];
	return NO_ERROR;
}

/*设置字符界面颜色*/
static inline long CUISetCol(DWORD CharColor, DWORD BgColor)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_CUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_SETCOL;
	data[1] = CharColor;
	data[2] = BgColor;
	return KSendMsg(&ptid, data, 0);
}

/*取得光标位置*/
static inline long CUIGetCur(DWORD *CursX, DWORD *CursY)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_CUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_GETCUR;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*CursX = data[1];
	*CursY = data[2];
	return NO_ERROR;
}

/*设置光标位置*/
static inline long CUISetCur(DWORD CursX, DWORD CursY)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_CUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_SETCUR;
	data[1] = CursX;
	data[2] = CursY;
	return KSendMsg(&ptid, data, 0);
}

/*清屏*/
static inline long CUIClrScr()
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_CUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_CLRSCR;
	return KSendMsg(&ptid, data, 0);
}

/*输出字符*/
static inline long CUIPutC(char c)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_CUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_PUTC;
	data[1] = c;
	return KSendMsg(&ptid, data, 0);
}

/*输出字符串*/
static inline long CUIPutS(const char *str)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_CUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = CUI_API_PUTS;
	if ((data[0] = KWriteProcAddr((void*)str, strlen(str) + 1, &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/**********系统喇叭服务相关**********/
#define SRV_SPK_PORT	8	/*系统喇叭服务端口*/

#define MSG_ATTR_SPK	0x01080000	/*系统喇叭消息*/

#define SPK_API_SOUND	0	/*发声功能号*/
#define SPK_API_NOSOUND	1	/*停止发声功能号*/

/*发声*/
static inline long SPKSound(DWORD freq)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_SPK_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_SPK | SPK_API_SOUND;
	data[1] = freq;
	return KSendMsg(&ptid, data, 0);
}

/*停止发声*/
static inline long SPKNosound()
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_SPK_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_SPK | SPK_API_NOSOUND;
	return KSendMsg(&ptid, data, 0);
}

/**********COM串口服务相关**********/
#define SRV_UART_PORT	9	/*COM串口服务端口*/

#define MSG_ATTR_UART	0x01090000	/*COM串口消息*/

#define UART_API_OPENCOM	0	/*打开串口功能号*/
#define UART_API_CLOSECOM	1	/*关闭串口功能号*/
#define UART_API_WRITECOM	2	/*写串口功能号*/
#define UART_API_READCOM	3	/*读串口功能号*/

#define UART_ARGS_BITS_5		0x00	/*5个数据位*/
#define UART_ARGS_BITS_6		0x01	/*6个数据位*/
#define UART_ARGS_BITS_7		0x02	/*7个数据位*/
#define UART_ARGS_BITS_8		0x03	/*8个数据位*/
#define UART_ARGS_STOP_1		0x00	/*1个停止位*/
#define UART_ARGS_STOP_1_5		0x04	/*5个数据位时1.5个停止位*/
#define UART_ARGS_STOP_2		0x04	/*2个停止位*/
#define UART_ARGS_PARITY_NONE	0x00	/*无奇偶校验*/
#define UART_ARGS_PARITY_ODD	0x08	/*奇位校验*/
#define UART_ARGS_PARITY_EVEN	0x18	/*偶位校验*/

#define UART_ERR_NOPORT	-2304	/*COM端口不存在*/
#define UART_ERR_BAUD	-2305	/*波特率错误*/
#define UART_ERR_NOTIME	-2306	/*超时错误*/
#define UART_ERR_BUSY	-2307	/*端口驱动正忙*/

/*打开串口*/
static inline long UARTOpenCom(DWORD com, DWORD baud, DWORD args)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_UART_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_UART | UART_API_OPENCOM;
	data[1] = com;
	data[2] = baud;
	data[3] = args;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*关闭串口*/
static inline long UARTCloseCom(DWORD com)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_UART_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_UART | UART_API_CLOSECOM;
	data[1] = com;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*写串口*/
static inline long UARTWriteCom(DWORD com, void *buf, DWORD siz, DWORD time)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_UART_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = UART_API_WRITECOM;
	data[3] = com;
	data[4] = time;
	if ((data[0] = KWriteProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*读串口*/
static inline long UARTReadCom(DWORD com, void *buf, DWORD siz, DWORD time)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_UART_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = UART_API_READCOM;
	data[3] = com;
	data[4] = time;
	if ((data[0] = KReadProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/**********输入法服务相关**********/
#define SRV_IME_PORT	11	/*输入法服务端口*/

#define MSG_ATTR_IME	0x010B0000	/*输入法消息*/

#define IME_API_OPENBAR		0	/*打开输入法工具条功能号*/
#define IME_API_CLOSEBAR	1	/*关闭输入法工具条功能号*/
#define IME_API_PUTKEY		2	/*向输入法发送按键功能号*/

/*打开输入法工具条*/
static inline long IMEOpenBar(long x, long y)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_IME_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_IME | IME_API_OPENBAR;
	data[1] = x;
	data[2] = y;
	return KSendMsg(&ptid, data, 0);
}

/*关闭输入法工具条*/
static inline long IMECloseBar()
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_IME_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_IME | IME_API_CLOSEBAR;
	return KSendMsg(&ptid, data, 0);
}

/*向输入法发送按键*/
static inline long IMEPutKey(DWORD key)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_IME_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_IME | IME_API_PUTKEY;
	data[1] = key;
	return KSendMsg(&ptid, data, 0);
}

#endif
