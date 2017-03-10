/*	1024.c for ulios application
	作者：孙亮
	功能：方块类小游戏
	最后修改日期：2014-08-08
*/

#include "../lib/malloc.h"
#include "../lib/gclient.h"

#define BLOCK_WIDTH	32	/*块大小*/
#define LINE_WIDTH	2	/*隔线宽度*/

#define KEY_UP		0x4800
#define KEY_LEFT	0x4B00
#define KEY_RIGHT	0x4D00
#define KEY_DOWN	0x5000

DWORD GamDat[4][4];
DWORD MaxDat = 0;			/*记录最大数据*/
BOOL isDatChange = FALSE;	/*记录数据是否变化*/
CTRL_WND *wnd = NULL;

/*双字转化为数字*/
char *Itoa(char *buf, DWORD n)
{
	char *p, *q;
	
	q = p = buf;
	do
	{
		*p++ = '0' + (char)(n % 10);
		n /= 10;
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

void DrawScene()	/*场景绘图*/
{
	long x, y;

	for (y = 0; y < 4; y++)
		for (x = 0; x < 4; x++)
		{
			char buf[8];
			GCFillRect(&wnd->client, x * (BLOCK_WIDTH + LINE_WIDTH), y * (BLOCK_WIDTH + LINE_WIDTH), BLOCK_WIDTH, BLOCK_WIDTH, 0xFFFFFFFF);
			if (GamDat[x][y])
			{
				Itoa(buf, GamDat[x][y]);
				GCDrawStr(&wnd->client, x * (BLOCK_WIDTH + LINE_WIDTH) + (BLOCK_WIDTH - (long)strlen(buf) * (long)GCCharWidth) / 2, y * (BLOCK_WIDTH + LINE_WIDTH) + (BLOCK_WIDTH - (long)GCCharHeight) / 2, buf, 0);
			}
		}
	for (x = 0; x < 3; x++)
	{
		GCFillRect(&wnd->client, 0, x * (BLOCK_WIDTH + LINE_WIDTH) + BLOCK_WIDTH, BLOCK_WIDTH * 4 + LINE_WIDTH * 3, LINE_WIDTH, 0);
		GCFillRect(&wnd->client, x * (BLOCK_WIDTH + LINE_WIDTH) + BLOCK_WIDTH, 0, LINE_WIDTH, BLOCK_WIDTH * 4 + LINE_WIDTH * 3, 0);
	}
	GCWndGetClientLoca(wnd, &x, &y);
	GUIpaint(wnd->obj.gid, x, y, BLOCK_WIDTH * 4 + LINE_WIDTH * 3, BLOCK_WIDTH * 4 + LINE_WIDTH * 3);
}

void LogicLeft()	/*向左逻辑处理*/
{
	long x, y, i;

	for (y = 0; y < 4; y++)
		for (x = 0; x < 4; x++)
			if (GamDat[x][y])	/*查找数字*/
			{
				for (i = x + 1; i < 4; i++)	/*向右查找相等数*/
				{
					if (GamDat[i][y] == GamDat[x][y])	/*找到相等数*/
					{
						GamDat[x][y] += GamDat[i][y];	/*累加结果*/
						GamDat[i][y] = 0;
						if (MaxDat < GamDat[x][y])
							MaxDat = GamDat[x][y];
						isDatChange = TRUE;
						break;
					}
					else if (GamDat[i][y])
						break;
				}
				for (i = x - 1; i >= 0; i--)	/*向左查找空位*/
					if (GamDat[i][y])
						break;
				if (++i != x)
				{
					GamDat[i][y] = GamDat[x][y];	/*重新放置数字*/
					GamDat[x][y] = 0;
					isDatChange = TRUE;
				}
			}
}

void LogicRight()	/*向右逻辑处理*/
{
	long x, y, i;

	for (y = 0; y < 4; y++)
		for (x = 3; x >= 0; x--)
			if (GamDat[x][y])	/*查找数字*/
			{
				for (i = x - 1; i >= 0; i--)	/*向左查找相等数*/
				{
					if (GamDat[i][y] == GamDat[x][y])	/*找到相等数*/
					{
						GamDat[x][y] += GamDat[i][y];	/*累加结果*/
						GamDat[i][y] = 0;
						if (MaxDat < GamDat[x][y])
							MaxDat = GamDat[x][y];
						isDatChange = TRUE;
						break;
					}
					else if (GamDat[i][y])
						break;
				}
				for (i = x + 1; i < 4; i++)	/*向右查找空位*/
					if (GamDat[i][y])
						break;
				if (--i != x)
				{
					GamDat[i][y] = GamDat[x][y];	/*重新放置数字*/
					GamDat[x][y] = 0;
					isDatChange = TRUE;
				}
			}
}

void LogicUp()	/*向上逻辑处理*/
{
	long x, y, i;

	for (x = 0; x < 4; x++)
		for (y = 0; y < 4; y++)
			if (GamDat[x][y])	/*查找数字*/
			{
				for (i = y + 1; i < 4; i++)	/*向下查找相等数*/
				{
					if (GamDat[x][i] == GamDat[x][y])	/*找到相等数*/
					{
						GamDat[x][y] += GamDat[x][i];	/*累加结果*/
						GamDat[x][i] = 0;
						if (MaxDat < GamDat[x][y])
							MaxDat = GamDat[x][y];
						isDatChange = TRUE;
						break;
					}
					else if (GamDat[x][i])
						break;
				}
				for (i = y - 1; i >= 0; i--)	/*向上查找空位*/
					if (GamDat[x][i])
						break;
				if (++i != y)
				{
					GamDat[x][i] = GamDat[x][y];	/*重新放置数字*/
					GamDat[x][y] = 0;
					isDatChange = TRUE;
				}
			}
}

void LogicDown()	/*向下逻辑处理*/
{
	long x, y, i;

	for (x = 0; x < 4; x++)
		for (y = 3; y >= 0; y--)
			if (GamDat[x][y])	/*查找数字*/
			{
				for (i = y - 1; i >= 0; i--)	/*向上查找相等数*/
				{
					if (GamDat[x][i] == GamDat[x][y])	/*找到相等数*/
					{
						GamDat[x][y] += GamDat[x][i];	/*累加结果*/
						GamDat[x][i] = 0;
						if (MaxDat < GamDat[x][y])
							MaxDat = GamDat[x][y];
						isDatChange = TRUE;
						break;
					}
					else if (GamDat[x][i])
						break;
				}
				for (i = y + 1; i < 4; i++)	/*向下查找空位*/
					if (GamDat[x][i])
						break;
				if (--i != y)
				{
					GamDat[x][i] = GamDat[x][y];	/*重新放置数字*/
					GamDat[x][y] = 0;
					isDatChange = TRUE;
				}
			}
}

void LogicResult()	/*结果逻辑*/
{
	if (isDatChange)
	{
		if (MaxDat >= 1024)
			GCWndSetCaption(wnd, "恭喜你，获胜了！");
		else
		for (;;)	/*增加一个数字*/
		{
			DWORD i = TMGetRand();
			if (GamDat[i & 3][(i >> 2) & 3] == 0)
			{
				GamDat[i & 3][(i >> 2) & 3] = 1;
				break;
			}
		}
		isDatChange = FALSE;
	}
}

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
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
	case GM_KEY:	/*按键消息*/
		switch (data[1] & 0xFFFF)
		{
		case KEY_LEFT:
			LogicLeft();
			break;
		case KEY_RIGHT:
			LogicRight();
			break;
		case KEY_UP:
			LogicUp();
			break;
		case KEY_DOWN:
			LogicDown();
			break;
		}
		LogicResult();
		DrawScene();
		break;
	}
	return GCWndDefMsgProc(ptid, data);
}

int main()
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	CTRL_ARGS args;
	long res;

	GamDat[0][0] = 1;
	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*设置16MB堆内存*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = BLOCK_WIDTH * 4 + LINE_WIDTH * 3 + 2;
	args.height = BLOCK_WIDTH * 4 + LINE_WIDTH * 3 + 21;
	args.x = 300;
	args.y = 200;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "合并方块达到1024！");
	ptid.ProcID = SRV_GUI_PORT;
	ptid.ThedID = INVALID;
	if ((res = KRecvProcMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待创建完成消息*/
		return res;
	GCDispatchMsg(ptid, data);	/*创建完成后续处理*/
	DrawScene();

	for (;;)
	{
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
