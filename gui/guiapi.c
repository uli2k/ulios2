/*	guiapi.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面接口，响应应用程序的请求，执行服务
	最后修改日期：2010-10-05
*/

#include "gui.h"

extern GOBJ_DESC *gobjt;	/*窗体描述符管理表*/
extern GOBJ_DESC *FstGobj;	/*窗体描述符管理表指针*/
extern CLIPRECT *ClipRectt;	/*剪切矩形管理表*/
extern CLIPRECT *FstClipRect;	/*剪切矩形管理表指针*/
extern DWORD *MusBak;	/*鼠标背景缓存*/
extern DWORD *MusPic;	/*鼠标图像*/

#define PTID_ID	MSG_DATA_LEN

void ApiGetGinfo(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	argv[1] = GDIwidth;
	argv[2] = GDIheight;
	argv[MSG_RES_ID] = NO_ERROR;
	KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiCreate(DWORD *argv)
{
	GOBJ_DESC *gobj = NULL;

	if (argv[5] < GOBJT_LEN && *(DWORD*)(&(gobj = &gobjt[argv[5]])->ptid) != INVALID)	/*检查父窗体ID有效,窗体有效*/
	{
		if ((argv[MSG_RES_ID] = CreateGobj(gobj, *(THREAD_ID*)&argv[PTID_ID], argv[GUIMSG_GOBJ_ID], (short)argv[3], (short)(argv[3] >> 16), (short)argv[4], (short)(argv[4] >> 16), (argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_GUI ? NULL : (DWORD*)argv[MSG_ADDR_ID], argv[MSG_SIZE_ID] / sizeof(DWORD), &gobj)) == NO_ERROR)
		{
			argv[5] = gobj - gobjt;
			argv[GUIMSG_GOBJ_ID] = gobj->ClientSign;
		}
	}
	else if (*(DWORD*)(&gobjt[0].ptid) == INVALID)	/*gobjt的0项为空,创建主桌面*/
	{
		if ((argv[MSG_RES_ID] = CreateDesktop(*(THREAD_ID*)&argv[PTID_ID], argv[GUIMSG_GOBJ_ID], (argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_GUI ? NULL : (DWORD*)argv[MSG_ADDR_ID], argv[MSG_SIZE_ID] / sizeof(DWORD))) == NO_ERROR)
		{
			argv[5] = 0;
			argv[GUIMSG_GOBJ_ID] = gobjt[0].ClientSign;
		}
	}
	else
		argv[MSG_RES_ID] = GUI_ERR_INVALID_GOBJID;
	if (argv[MSG_RES_ID] != NO_ERROR && (argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
	else
	{
		argv[MSG_API_ID] = MSG_ATTR_GUI | GM_CREATE;
		KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
	}
}

void ApiDestroy(DWORD *argv)
{
	GOBJ_DESC *gobj, *ParGobj = NULL;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	if (argv[GUIMSG_GOBJ_ID] < GOBJT_LEN && ((THREAD_ID*)&argv[PTID_ID])->ProcID == (gobj = &gobjt[argv[GUIMSG_GOBJ_ID]])->ptid.ProcID)	/*检查窗体ID有效,窗体有效,进程访问自身的窗体*/
	{
		ParGobj = gobj->par;
		argv[GUIMSG_GOBJ_ID] = gobj->ClientSign;
		if (ParGobj)
			argv[MSG_RES_ID] = DeleteGobj(gobj);
		else	/*gobj的ID为0,删除主桌面*/
			argv[MSG_RES_ID] = DeleteDesktop(gobj);
	}
	else
		argv[MSG_RES_ID] = GUI_ERR_INVALID_GOBJID;
	KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiMove(DWORD *argv)
{
	GOBJ_DESC *gobj;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	if (argv[GUIMSG_GOBJ_ID] && argv[GUIMSG_GOBJ_ID] < GOBJT_LEN && ((THREAD_ID*)&argv[PTID_ID])->ProcID == (gobj = &gobjt[argv[GUIMSG_GOBJ_ID]])->ptid.ProcID)	/*检查窗体ID不为0且有效,窗体有效,进程访问自身的窗体*/
	{
		if ((argv[MSG_RES_ID] = MoveGobj(gobj, argv[1], argv[2])) == NO_ERROR)
			argv[GUIMSG_GOBJ_ID] = gobj->ClientSign;
	}
	else
		argv[MSG_RES_ID] = GUI_ERR_INVALID_GOBJID;
	KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiSize(DWORD *argv)
{
	GOBJ_DESC *gobj;

	if (argv[GUIMSG_GOBJ_ID] && argv[GUIMSG_GOBJ_ID] < GOBJT_LEN && ((THREAD_ID*)&argv[PTID_ID])->ProcID == (gobj = &gobjt[argv[GUIMSG_GOBJ_ID]])->ptid.ProcID)	/*检查窗体ID不为0且有效,窗体有效,进程访问自身的窗体*/
	{
		if ((argv[MSG_RES_ID] = SizeGobj(gobj, (short)argv[3], (short)(argv[3] >> 16), (short)argv[4], (short)(argv[4] >> 16), (argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_GUI ? NULL : (DWORD*)argv[MSG_ADDR_ID], argv[MSG_SIZE_ID] / sizeof(DWORD))) == NO_ERROR)
			argv[GUIMSG_GOBJ_ID] = gobj->ClientSign;
	}
	else
		argv[MSG_RES_ID] = GUI_ERR_INVALID_GOBJID;
	if (argv[MSG_RES_ID] != NO_ERROR && (argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
	else
	{
		argv[MSG_API_ID] = MSG_ATTR_GUI | GM_SIZE;
		KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
	}
}

void ApiPaint(DWORD *argv)
{
	GOBJ_DESC *gobj;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	if (argv[GUIMSG_GOBJ_ID] < GOBJT_LEN && ((THREAD_ID*)&argv[PTID_ID])->ProcID == (gobj = &gobjt[argv[GUIMSG_GOBJ_ID]])->ptid.ProcID)	/*检查窗体ID有效,窗体有效,进程访问自身的窗体*/
		PaintGobj(gobj, (short)argv[1], (short)(argv[1] >> 16), (short)argv[2], (short)(argv[2] >> 16));	/*不反馈消息*/
}

void ApiSetTop(DWORD *argv)
{
	GOBJ_DESC *gobj = NULL;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	if (argv[GUIMSG_GOBJ_ID] < GOBJT_LEN && ((THREAD_ID*)&argv[PTID_ID])->ProcID == (gobj = &gobjt[argv[GUIMSG_GOBJ_ID]])->ptid.ProcID)	/*检查窗体ID有效,窗体有效,进程访问自身的窗体*/
	{
		if ((argv[MSG_RES_ID] = SetTopGobj(gobj)) == NO_ERROR)
		{
			argv[1] = TRUE;
			argv[GUIMSG_GOBJ_ID] = gobj->ClientSign;
		}
	}
	else
		argv[MSG_RES_ID] = GUI_ERR_INVALID_GOBJID;
	KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiSetFocus(DWORD *argv)
{
	GOBJ_DESC *gobj = NULL;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	if (argv[GUIMSG_GOBJ_ID] < GOBJT_LEN && ((THREAD_ID*)&argv[PTID_ID])->ProcID == (gobj = &gobjt[argv[GUIMSG_GOBJ_ID]])->ptid.ProcID)	/*检查窗体ID有效,窗体有效,进程访问自身的窗体*/
	{
		if ((argv[MSG_RES_ID] = SetFocusGobj(gobj)) == NO_ERROR)
		{
			argv[1] = TRUE;
			argv[GUIMSG_GOBJ_ID] = gobj->ClientSign;
		}
	}
	else
		argv[MSG_RES_ID] = GUI_ERR_INVALID_GOBJID;
	KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiDrag(DWORD *argv)
{
	GOBJ_DESC *gobj;

	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	if (argv[GUIMSG_GOBJ_ID] && argv[GUIMSG_GOBJ_ID] < GOBJT_LEN && ((THREAD_ID*)&argv[PTID_ID])->ProcID == (gobj = &gobjt[argv[GUIMSG_GOBJ_ID]])->ptid.ProcID)	/*检查窗体ID不为0且有效,窗体有效,进程访问自身的窗体*/
		DragGobj(gobj, argv[1]);	/*不反馈消息*/
}

void ApiSetMouse(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) != MSG_ATTR_ROMAP)
		return;
	argv[MSG_RES_ID] = SetMousePic((short)argv[3], (short)(argv[3] >> 16), (short)argv[4], (short)(argv[4] >> 16), (DWORD*)argv[MSG_ADDR_ID], argv[MSG_SIZE_ID] / sizeof(DWORD));
	KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
}

/*系统调用表*/
void (*ApiTable[])(DWORD *argv) = {
	ApiGetGinfo, ApiCreate, ApiDestroy, ApiMove, ApiSize, ApiPaint, ApiSetTop, ApiSetFocus, ApiDrag, ApiSetMouse
};

/*初始化GUI,如果不成功必须退出*/
long InitGUI()
{
	long res;
	GOBJ_DESC *gobj;
	CLIPRECT *clip;

	if ((res = KRegKnlPort(SRV_GUI_PORT)) != NO_ERROR)	/*注册服务端口*/
		return res;
	if ((res = KMapUserAddr((void**)&gobjt, sizeof(GOBJ_DESC) * GOBJT_LEN + sizeof(CLIPRECT) * CLIPRECTT_LEN + sizeof(DWORD) * MUSPIC_MAXLEN * 2)) != NO_ERROR)	/*申请窗体描述符管理表,剪切矩形管理表,鼠标图像缓存*/
		return res;
	if ((res = VSGetVmem(&GDIvm, &GDIwidth, &GDIheight, &GDIPixBits, &GDImode)) != NO_ERROR)	/*初始化GDI*/
		return res;
	if (!GDImode)
		return GUI_ERR_WRONG_VESAMODE;

	for (gobj = FstGobj = gobjt; gobj < &gobjt[GOBJT_LEN - 1]; gobj++)
	{
		*(DWORD*)(&gobj->ptid) = INVALID;
		gobj->nxt = gobj + 1;
	}
	*(DWORD*)(&gobj->ptid) = INVALID;
	gobj->nxt = NULL;
	ClipRectt = (CLIPRECT*)(gobjt + GOBJT_LEN);
	for (clip = FstClipRect = ClipRectt; clip < &FstClipRect[CLIPRECTT_LEN - 1]; clip++)
		clip->nxt = clip + 1;
	clip->nxt = NULL;
	MusBak = (DWORD*)(ClipRectt + CLIPRECTT_LEN);
	MusPic = MusBak + MUSPIC_MAXLEN;

	return NO_ERROR;
}

int main()
{
	long res;

	KDebug("booting... gui server\n");
	if ((res = InitGUI()) != NO_ERROR)
		return res;
	InitKbdMus();
	for (;;)
	{
		DWORD data[MSG_DATA_LEN + 1];

		if ((res = KRecvMsg((THREAD_ID*)&data[PTID_ID], data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (data[MSG_ATTR_ID] == MSG_ATTR_KBD)	/*键盘驱动消息*/
			KeyboardProc(data[1]);
		else if (data[MSG_ATTR_ID] == MSG_ATTR_MUS)	/*鼠标驱动消息*/
			MouseProc(data[1], (long)data[2], (long)data[3], (long)data[4]);
		else if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_CNLMAP)	/*窗口进程异常退出的处理*/
		{
			GOBJ_DESC *gobj;

			for (gobj = gobjt; gobj < &gobjt[GOBJT_LEN]; gobj++)
				if (*(DWORD*)(&gobj->ptid) != INVALID && (DWORD)gobj->vbuf == data[MSG_ADDR_ID])	/*查找所属窗体*/
				{
					data[MSG_API_ID] = MSG_ATTR_GUI | GM_DESTROY;
					data[GUIMSG_GOBJ_ID] = gobj - gobjt;
					break;
				}
		}
		else if ((data[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP || (data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_GUI)	/*API调用消息*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) >= sizeof(ApiTable) / sizeof(void*))
			{
				data[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
				if (data[MSG_ATTR_ID] < MSG_ATTR_USER)
					KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
				else
					KSendMsg((THREAD_ID*)&data[PTID_ID], data, 0);
			}
			else
				ApiTable[data[MSG_API_ID] & MSG_API_MASK](data);
		}
		else if (data[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)	/*退出请求*/
			break;
	}
	KUnregKnlPort(SRV_GUI_PORT);
	return NO_ERROR;
}
