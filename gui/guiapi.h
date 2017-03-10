/*	guiapi.h for ulios graphical user interface
	作者：孙亮
	功能：ulios GUI服务API接口定义，调用GUI服务需要包含此文件
	最后修改日期：2010-10-05
*/

#ifndef	_GUIAPI_H_
#define	_GUIAPI_H_

#include "../driver/basesrv.h"

/*错误定义*/
#define GUI_ERR_HAVENO_GOBJ		-2560	/*窗体描述符不足*/
#define GUI_ERR_HAVENO_MEMORY	-2561	/*内存不足*/
#define GUI_ERR_HAVENO_CLIPRECT	-2562	/*剪切矩形不足,窗口系统过于复杂*/
#define GUI_ERR_WRONG_GOBJTYPE	-2563	/*错误的窗体类型*/
#define GUI_ERR_WRONG_GOBJLOC	-2564	/*错误的窗体位置*/
#define GUI_ERR_WRONG_GOBJSIZE	-2565	/*错误的窗体大小*/
#define GUI_ERR_INVALID_GOBJID	-2566	/*错误的窗体ID*/
#define GUI_ERR_NOT_OVERLAP		-2567	/*矩形不重叠*/
#define GUI_ERR_NOCHG_CLIPRECT	-2568	/*剪切矩形没有变化*/
#define GUI_ERR_WRONG_HANDLE	-2569	/*错误的窗体句柄*/
#define GUI_ERR_HAVENO_VBUF		-2570	/*显示缓冲不存在*/
#define GUI_ERR_WRONG_VBUF		-2571	/*显示缓冲错误*/
#define GUI_ERR_WRONG_ARGS		-2572	/*参数错误*/
#define GUI_ERR_NOCHG_TOP		-2573	/*顶端状态无变化*/
#define GUI_ERR_NOCHG_FOCUS		-2574	/*焦点无变化*/
#define GUI_ERR_BEING_DRAGGED	-2575	/*有窗体正在被拖拽*/
#define GUI_ERR_WRONG_VESAMODE	-2576	/*VESA显卡显示模式错误*/

#define SRV_GUI_PORT		10	/*GUI服务端口*/

#define MSG_ATTR_GUI	0x010A0000	/*GUI消息*/

#define GUIMSG_GOBJ_ID	6	/*GUI消息窗体对象索引*/

#define GM_GETGINFO		0x00	/*取得GUI信息*/
#define GM_CREATE		0x01	/*创建窗体*/
#define GM_DESTROY		0x02	/*销毁窗体*/
#define GM_MOVE			0x03	/*移动窗体*/
#define GM_SIZE			0x04	/*缩放窗体*/
#define GM_PAINT		0x05	/*绘制窗体*/
#define GM_SETTOP		0x06	/*顶端消息*/
#define GM_SETFOCUS		0x07	/*焦点消息*/
#define GM_DRAG			0x08	/*拖拽消息*/
#define GM_SETMOUSE		0x09	/*设置鼠标指针*/

#define GM_DRAGMOD_NONE	0		/*取消拖拽模式*/
#define GM_DRAGMOD_MOVE	1		/*拖拽移动模式*/
#define GM_DRAGMOD_SIZE	2		/*拖拽缩放模式*/

#define GM_MOUSEENTER	0x80	/*鼠标移入*/
#define GM_MOUSELEAVE	0x81	/*鼠标移出*/
#define GM_MOUSEMOVE	0x82	/*鼠标移动*/
#define GM_LBUTTONDOWN	0x83	/*左键按下*/
#define GM_RBUTTONDOWN	0x84	/*右键按下*/
#define GM_MBUTTONDOWN	0x85	/*中键按下*/
#define GM_LBUTTONUP	0x86	/*左键抬起*/
#define GM_RBUTTONUP	0x87	/*右键抬起*/
#define GM_MBUTTONUP	0x88	/*中键抬起*/
#define GM_LBUTTONCLICK	0x89	/*左键单击*/
#define GM_RBUTTONCLICK	0x8A	/*右键单击*/
#define GM_MBUTTONCLICK	0x8B	/*中键单击*/
#define GM_LBUTTONDBCLK	0x8C	/*左键双击*/
#define GM_RBUTTONDBCLK	0x8D	/*右键双击*/
#define GM_MBUTTONDBCLK	0x8E	/*中键双击*/
#define GM_MOUSEWHEEL	0x8F	/*鼠标滚轮*/

#define GM_KEY			0xA0	/*按键消息*/
#define GM_IMEPUTKEY	0xA1	/*输入法出字消息*/

#define GM_UNMAPVBUF	0x0100	/*撤销显示缓冲映射*/

/*取得GUI信息*/
static inline long GUIGetGinfo(DWORD *GUIwidth, DWORD *GUIheight)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_GETGINFO;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*GUIwidth = data[1];
	*GUIheight = data[2];
	return data[MSG_RES_ID];
}

/*创建窗体*/
static inline long GUIcreate(DWORD pid, DWORD ClintSign, short x, short y, WORD width, WORD height, DWORD *vbuf)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_CREATE;
	data[3] = (WORD)x | ((DWORD)(WORD)y << 16);
	data[4] = width | ((DWORD)height << 16);
	data[5] = pid;
	data[GUIMSG_GOBJ_ID] = ClintSign;
	if (vbuf)
		return KWriteProcAddr(vbuf, (DWORD)width * height * sizeof(DWORD), &ptid, data, 0);
	else
		return KSendMsg(&ptid, data, 0);
}

/*销毁窗体*/
static inline long GUIdestroy(DWORD gid)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_DESTROY;
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

/*移动窗体*/
static inline long GUImove(DWORD gid, long x, long y)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_MOVE;
	data[1] = x;
	data[2] = y;
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

/*设置窗体大小*/
static inline long GUIsize(DWORD gid, DWORD *vbuf, short x, short y, WORD width, WORD height)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_SIZE;
	data[3] = (WORD)x | ((DWORD)(WORD)y << 16);
	data[4] = width | ((DWORD)height << 16);
	data[GUIMSG_GOBJ_ID] = gid;
	if (vbuf)
		return KWriteProcAddr(vbuf, (DWORD)width * height * sizeof(DWORD), &ptid, data, 0);
	else
		return KSendMsg(&ptid, data, 0);
}

/*重绘窗体区域*/
static inline long GUIpaint(DWORD gid, short x, short y, WORD width, WORD height)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_PAINT;
	data[1] = (WORD)x | ((DWORD)(WORD)y << 16);
	data[2] = width | ((DWORD)height << 16);
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

/*设置窗体在顶端*/
static inline long GUISetTop(DWORD gid)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_SETTOP;
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

/*设置窗体焦点*/
static inline long GUISetFocus(DWORD gid)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_SETFOCUS;
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

/*发送拖拽消息*/
static inline long GUIdrag(DWORD gid, DWORD mode)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_DRAG;
	data[1] = mode;
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

/*设置鼠标指针*/
static inline long GUISetMouse(short x, short y, WORD width, WORD height, DWORD *buf)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_SETMOUSE;
	data[3] = (WORD)x | ((DWORD)(WORD)y << 16);
	data[4] = width | ((DWORD)height << 16);
	if ((data[0] = KReadProcAddr(buf, (DWORD)width * height * sizeof(DWORD), &ptid, data, 0)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

#endif
