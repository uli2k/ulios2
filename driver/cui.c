/*	cui.c for ulios driver
	作者：孙亮
	功能：字符用户界面驱动
	最后修改日期：2010-06-10
*/

#include "basesrv.h"
#include "../lib/gdi.h"
#include "../lib/malloc.h"
#include "../lib/gclient.h"

#define MAX_LINE	200	/*每行最多字符数量*/
#define CURS_WIDTH	2	/*光标高度*/
#define DSP_MOD_TEXT	0	/*文本显示模式*/
#define DSP_MOD_GDI		1	/*图形设备模式*/
#define DSP_MOD_GUI		2	/*图形界面模式*/
#define GUI_MOD_WIDTH	80
#define GUI_MOD_HEIGHT	25
#define RECVPTID_LEN	0x300	/*接收消息线程ID栈长度*/

DWORD DspMode;	/*字符显示模式*/
DWORD width, height;	/*显示字符数量*/
DWORD CharWidth, CharHeight;	/*字符大小*/
DWORD CursX, CursY;		/*光标位置*/
DWORD CharColor, BgColor;	/*前景色,当前背景*/
CTRL_WND *wnd;
THREAD_ID RecvPtid[RECVPTID_LEN], *CurRecv;	/*接收消息线程ID栈*/

#define TEXT_MODE_COLOR	((CharColor & 0xF) | ((BgColor & 0xF) << 4))

/*字符模式设置光标*/
void SetTextCurs()
{
	DWORD PortData;

	PortData = CursX + CursY * width;
	outb(0x3D4, 0xF);
	outb(0x3D5, PortData & 0xFF);
	outb(0x3D4, 0xE);
	outb(0x3D5, (PortData >> 8));
}

/*向接受按键消息的线程发送消息*/
void SendKeyMsg(DWORD data[MSG_DATA_LEN])
{
	data[MSG_ATTR_ID] = MSG_ATTR_CUIKEY;
	while (CurRecv >= RecvPtid)
	{
		long res;
		res = KSendMsg(CurRecv, data, 0);
		if (res != KERR_PROC_NOT_EXIST && res != KERR_THED_NOT_EXIST)
			break;
		CurRecv--;	/*无法发送到栈中的线程则忽略该线程*/
	}
}

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_DESTROY:
		while (CurRecv >= RecvPtid)	/*关闭窗口时向所有服务对象发送退出请求*/
		{
			SendExitProcReq(*CurRecv);
			CurRecv--;
		}
		wnd = NULL;
		break;
	case GM_SETFOCUS:
		if (data[1])
			wnd->obj.style |= WND_STATE_FOCUS;
		else
			wnd->obj.style &= (~WND_STATE_FOCUS);
		break;
	case GM_LBUTTONDOWN:	/*鼠标按下*/
		if (!(wnd->obj.style & WND_STATE_FOCUS))
			GUISetFocus(wnd->obj.gid);
		break;
	case GM_KEY:	/*发送按键*/
		SendKeyMsg(data);
		data[MSG_API_ID] = MSG_ATTR_GUI | GM_KEY;
		break;
	}
	return GCWndDefMsgProc(ptid, data);
}

/*创建图像模式窗口*/
void CreateGuiWnd()
{
	CTRL_ARGS args;
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];

	args.width = GUI_MOD_WIDTH * CharWidth + 2;
	args.height = GUI_MOD_HEIGHT * CharHeight + 21;
	args.x = 128;
	args.y = 128;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "控制台");
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	if (KRecvProcMsg(&ptid, data, INVALID) != NO_ERROR)	/*等待创建完成消息*/
		return;
	GCDispatchMsg(ptid, data);	/*创建完成后续处理*/
}

/*清屏*/
void ClearScr()
{
	CursY = CursX = 0;
	switch (DspMode)
	{
	case DSP_MOD_TEXT:
		memset32(GDIvm, (TEXT_MODE_COLOR << 8) | (TEXT_MODE_COLOR << 24), width * height / 2);
		SetTextCurs();
		break;
	case DSP_MOD_GDI:
		GDIFillRect(0, 0, GDIwidth, GDIheight, BgColor);
		GDIFillRect(0, CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, CharColor);	/*画光标*/
		break;
	case DSP_MOD_GUI:
		{
			long CliX, CliY;
			if (!wnd)
				CreateGuiWnd();
			GCWndGetClientLoca(wnd, &CliX, &CliY);
			GCFillRect(&wnd->client, 0, 0, wnd->client.width, wnd->client.height, BgColor);
			GCFillRect(&wnd->client, 0, CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, CharColor);	/*画光标*/
			GUIpaint(wnd->obj.gid, CliX, CliY, wnd->client.width, wnd->client.height);
		}
		break;
	}
}

/*输出字符串*/
void PutStr(const char *str)
{
	DWORD BufX, BufY;	/*字符串写入位置*/
	char LineBuf[MAX_LINE], *bufp;	/*行缓冲*/

	bufp = LineBuf;
	BufX = CharWidth * CursX;
	BufY = CharHeight * CursY;
	while (*str)
	{
		switch (*str)
		{
		case '\n':
			switch (DspMode)
			{
			case DSP_MOD_GDI:
				GDIFillRect(CharWidth * CursX, CharHeight * CursY, CharWidth, CharHeight, BgColor);	/*清除背景*/
				break;
			case DSP_MOD_GUI:
				{
					long CliX, CliY;
					if (!wnd)
						CreateGuiWnd();
					GCWndGetClientLoca(wnd, &CliX, &CliY);
					GCFillRect(&wnd->client, CharWidth * CursX, CharHeight * CursY, CharWidth, CharHeight, BgColor);	/*清除背景*/
					GUIpaint(wnd->obj.gid, CliX + CharWidth * CursX, CliY + CharHeight * CursY, CharWidth, CharHeight);
				}
				break;
			}
			CursX = width;
			break;
		case '\t':
			do
				*bufp++ = ' ';
			while (++CursX & 7);
			break;
		default:
			*bufp++ = *str;
			CursX++;
			break;
		}
		if (CursX >= width)	/*光标在最后一列，回到前面*/
		{
			CursX = 0;
			CursY++;
			if (bufp != LineBuf)
			{
				*bufp = '\0';
				switch (DspMode)
				{
				case DSP_MOD_TEXT:
					{
						WORD *CurVm;

						CurVm = ((WORD*)GDIvm) + BufX + BufY * width;
						for (bufp = LineBuf; *bufp; bufp++, CurVm++)
							*CurVm = *(BYTE*)bufp | (TEXT_MODE_COLOR << 8);
					}
					break;
				case DSP_MOD_GDI:
					GDIFillRect(BufX, BufY, CharWidth * (bufp - LineBuf), CharHeight, BgColor);
					GDIDrawStr(BufX, BufY, LineBuf, CharColor);
					break;
				case DSP_MOD_GUI:
					{
						long CliX, CliY;
						if (!wnd)
							CreateGuiWnd();
						GCWndGetClientLoca(wnd, &CliX, &CliY);
						GCFillRect(&wnd->client, BufX, BufY, CharWidth * (bufp - LineBuf), CharHeight, BgColor);
						GCDrawStr(&wnd->client, BufX, BufY, LineBuf, CharColor);
						GUIpaint(wnd->obj.gid, CliX + BufX, CliY + BufY, CharWidth * (bufp - LineBuf), CharHeight);
					}
					break;
				}
				bufp = LineBuf;
			}
			BufX = CharWidth * CursX;
			BufY = CharHeight * CursY;
		}
		if (CursY >= height)	/*光标在最后一行，向上滚屏*/
		{
			CursY--;
			switch (DspMode)
			{
			case DSP_MOD_TEXT:
				memcpy32(GDIvm, ((WORD*)GDIvm) + width, width * (height - 1) / 2);
				memset32(((WORD*)GDIvm) + width * (height - 1), (TEXT_MODE_COLOR << 8) | (TEXT_MODE_COLOR << 24), width / 2);
				break;
			case DSP_MOD_GDI:
				memcpy32(GDIvm, GDIvm + ((GDIPixBits + 7) >> 3) * GDIwidth * CharHeight, ((GDIPixBits + 7) >> 3) * GDIwidth * (GDIheight - CharHeight) / sizeof(DWORD));	/*向上滚屏*/
				GDIFillRect(0, CharHeight * (height - 1), GDIwidth, CharHeight, BgColor);
				break;
			case DSP_MOD_GUI:
				{
					long CliX, CliY;
					if (!wnd)
						CreateGuiWnd();
					GCWndGetClientLoca(wnd, &CliX, &CliY);
					memcpy32(wnd->obj.uda.vbuf + wnd->obj.uda.width * 20, wnd->obj.uda.vbuf + wnd->obj.uda.width * (20 + CharHeight), wnd->obj.uda.width * CharHeight * (height - 1));	/*向上滚屏*/
					GCFillRect(&wnd->client, 0, CharHeight * (height - 1), wnd->client.width, CharHeight, BgColor);
					GUIpaint(wnd->obj.gid, CliX, CliY, wnd->client.width, wnd->client.height);
				}
				break;
			}
			BufY = CharHeight * CursY;
		}
		str++;
	}
	if (bufp != LineBuf)	/*输出最后一行*/
	{
		*bufp = '\0';
		switch (DspMode)
		{
		case DSP_MOD_TEXT:
			{
				WORD *CurVm;

				CurVm = ((WORD*)GDIvm) + BufX + BufY * width;
				for (bufp = LineBuf; *bufp; bufp++, CurVm++)
					*CurVm = *(BYTE*)bufp | (TEXT_MODE_COLOR << 8);
			}
			break;
		case DSP_MOD_GDI:
			GDIFillRect(BufX, BufY, CharWidth * (bufp - LineBuf), CharHeight, BgColor);
			GDIDrawStr(BufX, BufY, LineBuf, CharColor);
			break;
		case DSP_MOD_GUI:
			{
				long CliX, CliY;
				if (!wnd)
					CreateGuiWnd();
				GCWndGetClientLoca(wnd, &CliX, &CliY);
				GCFillRect(&wnd->client, BufX, BufY, CharWidth * (bufp - LineBuf), CharHeight, BgColor);
				GCDrawStr(&wnd->client, BufX, BufY, LineBuf, CharColor);
				GUIpaint(wnd->obj.gid, CliX + BufX, CliY + BufY, CharWidth * (bufp - LineBuf), CharHeight);
			}
			break;
		}
	}
	switch (DspMode)
	{
	case DSP_MOD_TEXT:
		SetTextCurs();
		break;
	case DSP_MOD_GDI:
		GDIFillRect(CharWidth * CursX, CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, CharColor);	/*画光标*/
		break;
	case DSP_MOD_GUI:
		{
			long CliX, CliY;
			if (!wnd)
				CreateGuiWnd();
			GCWndGetClientLoca(wnd, &CliX, &CliY);
			GCFillRect(&wnd->client, CharWidth * CursX, CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, CharColor);	/*画光标*/
			GUIpaint(wnd->obj.gid, CliX + CharWidth * CursX, CliY + CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH);
		}
		break;
	}
}

/*退格处理*/
void BackSp()
{
	if (CursX == 0 && CursY == 0)
		return;
	switch (DspMode)
	{
	case DSP_MOD_GDI:
		GDIFillRect(CharWidth * CursX, CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, BgColor);	/*清除光标*/
		break;
	case DSP_MOD_GUI:
		{
			long CliX, CliY;
			if (!wnd)
				CreateGuiWnd();
			GCWndGetClientLoca(wnd, &CliX, &CliY);
			GCFillRect(&wnd->client, CharWidth * CursX, CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, BgColor);	/*清除光标*/
			GUIpaint(wnd->obj.gid, CliX + CharWidth * CursX, CliY + CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH);
		}
		break;
	}
	if (CursX)
		CursX--;
	else
	{
		CursX = width - 1;
		CursY--;
	}
	switch (DspMode)
	{
	case DSP_MOD_TEXT:
		((WORD*)GDIvm)[CursX + CursY * width] = (TEXT_MODE_COLOR << 8);
		SetTextCurs();
		break;
	case DSP_MOD_GDI:
		GDIFillRect(CharWidth * CursX, CharHeight * CursY, CharWidth, CharHeight, BgColor);	/*清除背景*/
		GDIFillRect(CharWidth * CursX, CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, CharColor);	/*画光标*/
		break;
	case DSP_MOD_GUI:
		{
			long CliX, CliY;
			if (!wnd)
				CreateGuiWnd();
			GCWndGetClientLoca(wnd, &CliX, &CliY);
			GCFillRect(&wnd->client, CharWidth * CursX, CharHeight * CursY, CharWidth, CharHeight, BgColor);	/*清除背景*/
			GCFillRect(&wnd->client, CharWidth * CursX, CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, CharColor);	/*画光标*/
			GUIpaint(wnd->obj.gid, CliX + CharWidth * CursX, CliY + CharHeight * CursY, CharWidth, CharHeight);
		}
		break;
	}
}

int main()
{
	long res;

	if ((res = KRegKnlPort(SRV_CUI_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	if (GCinit() != NO_ERROR)	/*优先使用GUI*/
		goto initgdi;
	if (InitMallocTab(0x1000000) != NO_ERROR)	/*设置16MB堆内存*/
	{
		GCrelease();
		goto initgdi;
	}
	DspMode = DSP_MOD_GUI;	/*初始化GUI参数*/
	width = GUI_MOD_WIDTH;
	height = GUI_MOD_HEIGHT;
	CharWidth = GCCharWidth;
	CharHeight = GCCharHeight;
	CharColor = 0xFF000000;
	BgColor = 0xFFCCCCCC;
	goto initok;
initgdi:
	if ((res = KMSetRecv()) != NO_ERROR)
		return res;
	res = GDIinit();	/*次之使用GDI*/
	if (res != NO_ERROR)
		goto inittext;
	DspMode = DSP_MOD_GDI;
	width = GDIwidth / GDICharWidth;	/*计算显示字符数量*/
	height = GDIheight / GDICharHeight;
	CharWidth = GDICharWidth;
	CharHeight = GDICharHeight;
	CharColor = 0xFFFFFFFF;
	BgColor = 0;
	ClearScr();
	goto initok;
inittext:
	if (res != GDI_ERR_TEXTMODE)	/*最后使用文本模式*/
		goto initerr;
	DspMode = DSP_MOD_TEXT;
	width = GDIwidth;	/*计算显示字符数量*/
	height = GDIheight;
	CharHeight = CharWidth = 1;
	CharColor = 0x7;
	BgColor = 0;
	ClearScr();
	goto initok;
initerr:
	return res;
initok:
	CurRecv = RecvPtid - 1;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*处理GUI消息*/
			continue;
		switch (data[MSG_ATTR_ID] & MSG_ATTR_MASK)
		{
		case MSG_ATTR_KBDMUS:	/*键盘服务消息*/
			if (data[MSG_ATTR_ID] == MSG_ATTR_KBD)
				SendKeyMsg(data);
			break;
		case MSG_ATTR_CUI:	/*普通服务消息*/
			switch (data[MSG_API_ID] & MSG_API_MASK)
			{
			case CUI_API_SETRECV:	/*注册接收按键消息的线程*/
				if (CurRecv < &RecvPtid[RECVPTID_LEN - 1])	/*PTID栈不满时注册接收按键消息的线程*/
					*(++CurRecv) = ptid;
				break;
			case CUI_API_GETCOL:	/*取得字符界面颜色*/
				data[1] = CharColor;
				data[2] = BgColor;
				KSendMsg(&ptid, data, 0);
				break;
			case CUI_API_SETCOL:	/*设置字符界面颜色*/
				CharColor = data[1];
				BgColor = data[2];
				break;
			case CUI_API_GETCUR:	/*取得光标位置功能号*/
				data[1] = CursX;
				data[2] = CursY;
				KSendMsg(&ptid, data, 0);
				break;
			case CUI_API_SETCUR:	/*设置光标位置*/
				switch (DspMode)
				{
				case DSP_MOD_TEXT:
					if (data[1] < width)
						CursX = data[1];
					if (data[2] < height)
						CursY = data[2];
					SetTextCurs();
					break;
				case DSP_MOD_GDI:
					GDIFillRect(CharWidth * CursX, CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, BgColor);	/*清除光标*/
					if (data[1] < width)
						CursX = data[1];
					if (data[2] < height)
						CursY = data[2];
					GDIFillRect(CharWidth * CursX, CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, CharColor);	/*画光标*/
					break;
				case DSP_MOD_GUI:
					{
						long CliX, CliY;
						if (!wnd)
							CreateGuiWnd();
						GCWndGetClientLoca(wnd, &CliX, &CliY);
						GCFillRect(&wnd->client, CharWidth * CursX, CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, BgColor);	/*清除光标*/
						GUIpaint(wnd->obj.gid, CliX + CharWidth * CursX, CliY + CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH);
						if (data[1] < width)
							CursX = data[1];
						if (data[2] < height)
							CursY = data[2];
						GCFillRect(&wnd->client, CharWidth * CursX, CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH, CharColor);	/*画光标*/
						GUIpaint(wnd->obj.gid, CliX + CharWidth * CursX, CliY + CharHeight * CursY + CharHeight - CURS_WIDTH, CharWidth, CURS_WIDTH);
					}
					break;
				}
				break;
			case CUI_API_CLRSCR:	/*清屏*/
				ClearScr();
				break;
			case CUI_API_PUTC:		/*输出字符*/
				if (data[1] == '\b')
					BackSp();
				else
				{
					data[1] &= 0xFF;
					PutStr((const char*)&data[1]);
				}
				break;
			}
			break;
		case MSG_ATTR_ROMAP:	/*映射消息*/
		case MSG_ATTR_RWMAP:
			switch (data[MSG_API_ID] & MSG_API_MASK)
			{
			case CUI_API_PUTS:	/*输出字符串*/
				if (((const char*)data[MSG_ADDR_ID])[data[MSG_SIZE_ID] - 1])
					data[MSG_RES_ID] = CUI_ERR_ARGS;
				else
				{
					PutStr((const char*)data[MSG_ADDR_ID]);
					data[MSG_RES_ID] = NO_ERROR;
				}
				break;
			}
			KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
			break;
		case MSG_ATTR_USER:	/*退出请求*/
			if (data[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)
				goto quit;
			break;
		}
	}
quit:
	KUnregKnlPort(SRV_CUI_PORT);
	return NO_ERROR;
}
