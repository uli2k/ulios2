/*	guitest.c for ulios application
	作者：孙亮
	功能：GUI测试程序
	最后修改日期：2010-12-11
*/

#include "../lib/malloc.h"
#include "../lib/gclient.h"

CTRL_TXT *txt;
CTRL_SEDT *edt;
CTRL_SCRL *scl;
CTRL_LST *lst;
long CliX, CliY, x0, y0;

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

void ScrlChg(CTRL_SCRL *scrl)
{
	char buf[12];
	Itoa(buf, scrl->pos, 10);
	GCTxtSetText(txt, buf);
}

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		GCWndGetClientLoca(wnd, &CliX, &CliY);
		{
			CTRL_ARGS args;
			args.width = 100;
			args.height = 16;
			args.x = CliX + (wnd->client.width - 100) / 2;
			args.y = CliY + (wnd->client.height - 16) / 2 - 10;
			args.style = 0;
			args.MsgProc = NULL;
			GCTxtCreate(&txt, &args, wnd->obj.gid, &wnd->obj, "hello ulios2 gui");
			args.y += 20;
			GCSedtCreate(&edt, &args, wnd->obj.gid, &wnd->obj, "hello", NULL);
			args.x = CliX;
			args.y = CliY + wnd->client.height - 16;
			args.width = wnd->client.width - 16;
			GCScrlCreate(&scl, &args, wnd->obj.gid, &wnd->obj, 0, 100, 32, 16, ScrlChg);
			args.width = 100;
			args.height = 80;
			args.x = CliX + 100;
			args.y = CliY + 10;
			args.style = 0;
			args.MsgProc = NULL;
			GCLstCreate(&lst, &args, wnd->obj.gid, &wnd->obj, NULL);
		}
		break;
	case GM_SIZE:
		txt->obj.x = CliX + (wnd->client.width - 100) / 2;
		txt->obj.y = CliY + (wnd->client.height - 16) / 2 - 10;
		GCSetArea(&txt->obj.uda, 100, 16, &wnd->obj.uda, txt->obj.x, txt->obj.y);
		GCTxtDefDrawProc(txt);
		GUImove(txt->obj.gid, txt->obj.x, txt->obj.y);
		edt->obj.x = CliX + (wnd->client.width - 100) / 2;
		edt->obj.y = CliY + (wnd->client.height - 16) / 2 + 10;
		GCSetArea(&edt->obj.uda, 100, 16, &wnd->obj.uda, edt->obj.x, edt->obj.y);
		GCSedtDefDrawProc(edt);
		GUImove(edt->obj.gid, edt->obj.x, edt->obj.y);

		GCScrlSetSize(scl, CliX, CliY + wnd->client.height - 16, wnd->client.width - 16, 16);
		break;
	case GM_LBUTTONDOWN:
		x0 = (data[5] & 0xFFFF) - CliX;
		y0 = (data[5] >> 16) - CliY;
		{
			static DWORD num;
			static LIST_ITEM *item;
			DWORD i;
			char buf[128];
			num++;
			for (i = 0; i < num; i++)
				buf[i] = num + 'A';
			buf[i] = 0;
			GCLstInsertItem(lst, item, buf, &item);
		}
		break;
	case GM_MOUSEMOVE:
		if (data[1] & MUS_STATE_LBUTTON)
		{
			GCDrawLine(&wnd->client, x0, y0, (data[5] & 0xFFFF) - CliX, (data[5] >> 16) - CliY, 0);
			x0 = (data[5] & 0xFFFF) - CliX;
			y0 = (data[5] >> 16) - CliY;
			GUIpaint(wnd->obj.gid, 0, 0, wnd->obj.uda.width, wnd->obj.uda.height);
		}
		break;
	case GM_LBUTTONDBCLK:
		GCWndSetCaption(wnd, "双击了！");
		break;
	case GM_MOUSEWHEEL:
		GUImove(wnd->obj.gid, wnd->obj.x, wnd->obj.y + (long)data[4]);
		break;
	}
	return GCWndDefMsgProc(ptid, data);
}

int main()
{
	CTRL_WND *wnd;
	CTRL_ARGS args;
	long res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*设置16MB堆内存*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = 256;
	args.height = 200;
	args.x = 100;
	args.y = 100;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN | WND_STYLE_MAXBTN | WND_STYLE_MINBTN | WND_STYLE_SIZEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "hello窗口");

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*处理GUI消息*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)wnd)	/*销毁主窗体,退出程序*/
				break;
		}
	}
	return NO_ERROR;
}
