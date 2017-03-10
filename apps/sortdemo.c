/*	sortdemo.c for ulios application
	作者：孙亮
	功能：多线程排序演示程序
	最后修改日期：2014-09-23
*/

#include "../lib/malloc.h"
#include "../lib/gclient.h"

#define SORTARRAY_LEN	400	/*排序数组长度*/
#define MAX_SORTDATA	128	/*数据最大值*/
#define SIDE_WIDTH		8	/*图形边框宽度*/

typedef struct _SORTDATA
{
	long length;
	DWORD array[SORTARRAY_LEN];
}SORTDATA;

#define DEMO_STATE_IDLE		0	/*空闲状态*/
#define DEMO_STATE_SORTING	1	/*正在排序*/
#define DEMO_STATE_PAUSE	2	/*暂停状态*/

DWORD RandSeed;
SORTDATA Sdata[4];
DWORD DemoState;
THREAD_ID SthPtid[4];
DWORD PtidCount;
DWORD StartClock;

CTRL_WND *wnd = NULL;
CTRL_BTN *RandBtn = NULL;
CTRL_BTN *KillBtn = NULL;
CTRL_BTN *DemoBtn = NULL;

DWORD rand(DWORD x)
{
	RandSeed = RandSeed * 1103515245 + 12345;
	return RandSeed % x;
}

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

/*格式化输出*/
void Sprintf(char *buf, const char *fmtstr, ...)
{
	long num;
	const DWORD *args = (DWORD*)(&fmtstr);
	
	while (*fmtstr)
	{
		if (*fmtstr == '%')
		{
			fmtstr++;
			switch (*fmtstr)
			{
			case 'd':
				num = *((long*)++args);
				if (num < 0)
				{
					*buf++ = '-';
					buf = Itoa(buf, -num, 10);
				}
				else
					buf = Itoa(buf, num, 10);
				break;
			case 'u':
				buf = Itoa(buf, *((DWORD*)++args), 10);
				break;
			case 'x':
			case 'X':
				buf = Itoa(buf, *((DWORD*)++args), 16);
				break;
			case 'o':
				buf = Itoa(buf, *((DWORD*)++args), 8);
				break;
			case 's':
				buf = strcpy(buf, *((const char**)++args)) - 1;
				break;
			case 'c':
				*buf++ = *((char*)++args);
				break;
			default:
				*buf++ = *fmtstr;
			}
		}
		else
			*buf++ = *fmtstr;
		fmtstr++;
	}
	*buf = '\0';
}

void DrawData(long i, long j, BOOL isWrite)
{
	THREAD_ID ptid;
	DWORD PTID, PPID, data[MSG_DATA_LEN];
	long x, y;

	GCWndGetClientLoca(wnd, &x, &y);
	GCFillRect(&wnd->client, SIDE_WIDTH + i * (MAX_SORTDATA + SIDE_WIDTH) + Sdata[i].array[j], SORTARRAY_LEN + SIDE_WIDTH - 1 - j, MAX_SORTDATA - Sdata[i].array[j], 1, 0xFFA0A0A0);
	GCFillRect(&wnd->client, SIDE_WIDTH + i * (MAX_SORTDATA + SIDE_WIDTH), SORTARRAY_LEN + SIDE_WIDTH - 1 - j, Sdata[i].array[j], 1, isWrite ? 0xFFFF0000 : 0xFF00FF00);
	GUIpaint(wnd->obj.gid, x + SIDE_WIDTH + i * (MAX_SORTDATA + SIDE_WIDTH), y + SORTARRAY_LEN + SIDE_WIDTH - 1 - j, MAX_SORTDATA, 1);
	KGetPtid(&ptid, &PTID, &PPID);
	ptid.ThedID = PTID;
	if (KRecvProcMsg(&ptid, data, 0) == NO_ERROR)	/*接收到暂停消息*/
		KRecvProcMsg(&ptid, data, INVALID);	/*等待接收继续消息*/
	GCFillRect(&wnd->client, SIDE_WIDTH + i * (MAX_SORTDATA + SIDE_WIDTH), SORTARRAY_LEN + SIDE_WIDTH - 1 - j, Sdata[i].array[j], 1, 0xFF000000);
	GUIpaint(wnd->obj.gid, x + SIDE_WIDTH + i * (MAX_SORTDATA + SIDE_WIDTH), y + SORTARRAY_LEN + SIDE_WIDTH - 1 - j, MAX_SORTDATA, 1);
}

//冒泡排序
void BsortThread(SORTDATA *data)
{
	long i, j;
	DWORD tmp;
	for (i = data->length - 1; i > 0; i--)
	{
		for (j = 0; j < i; j++)
		{
			DrawData(0, j, FALSE);
			DrawData(0, j + 1, FALSE);
			if (data->array[j] > data->array[j + 1])
			{
				tmp = data->array[j];
				data->array[j] = data->array[j + 1];
				data->array[j + 1] = tmp;
				DrawData(0, j, TRUE);
				DrawData(0, j + 1, TRUE);
			}
		}
	}
	KExitThread(NO_ERROR);
}

//选择排序
void SsortThread(SORTDATA *data)
{
	long i, j, p;
	DWORD tmp;
	for (i = data->length - 1; i > 0; i--)
	{
		p = i;
		for(j = 0; j < i; j++)
		{
			DrawData(1, j, FALSE);
			DrawData(1, p, FALSE);
			if (data->array[j] > data->array[p])
				p = j;
		}
		if (p != i)
		{
			DrawData(1, p, FALSE);
			DrawData(1, i, FALSE);
			tmp = data->array[p];
			data->array[p] = data->array[i];
			data->array[i] = tmp;
			DrawData(1, p, TRUE);
			DrawData(1, i, TRUE);
		}
	}
	KExitThread(NO_ERROR);
}

//双向选择
void DsortThread(SORTDATA *data)
{
	long i, j, l, h;
	DWORD tmp;
	for (i = 0; i < data->length / 2; i++)
	{
		l = i;
		h = data->length - i - 1;
		for (j = i; j < data->length - i - 1; j++)
		{
			DrawData(2, j, FALSE);
			DrawData(2, h, FALSE);
			DrawData(2, l, FALSE);
			if (data->array[j] > data->array[h])
				h = j;
			if (data->array[j] < data->array[l])
				l = j;
		}
		if (h != data->length - i - 1)
		{
			DrawData(2, h, FALSE);
			DrawData(2, data->length - i - 1, FALSE);
			tmp = data->array[h];
			data->array[h] = data->array[data->length - i - 1];
			data->array[data->length - i - 1] = tmp;
			DrawData(2, h, TRUE);
			DrawData(2, data->length - i - 1, TRUE);
		}
		if (l != i)
		{
			DrawData(2, l, FALSE);
			DrawData(2, i, FALSE);
			tmp = data->array[l];
			data->array[l] = data->array[i];
			data->array[i] = tmp;
			DrawData(2, l, TRUE);
			DrawData(2, i, TRUE);
		}
	}
	KExitThread(NO_ERROR);
}

//快速排序
void qsort(DWORD array[], long length)
{
	long i, j;
	DWORD val;

	if (length > 1)	/*确保数组长度大于1，否则无需排序*/
	{
		i = 0;
		j = length - 1;
		DrawData(3, array - Sdata[3].array, FALSE);
		val = array[0];	/*指定参考值val大小*/
		do 
		{
			/*从后向前搜索比val小的元素，找到后填到a[i]中并跳出循环*/
			while (i < j && array[j] >= val)
			{
				DrawData(3, array + j - Sdata[3].array, FALSE);
				j--;
			}
			array[i] = array[j];
			DrawData(3, array + i - Sdata[3].array, TRUE);
			/*从前往后搜索比val大的元素，找到后填到a[j]中并跳出循环*/
			while (i < j && array[i] <= val)
			{
				DrawData(3, array + i - Sdata[3].array, FALSE);
				i++;
			}
			array[j] = array[i];
			DrawData(3, array + j - Sdata[3].array, TRUE);
		}
		while (i < j);
		array[i] = val;	/*将保存在val中的数放到a[i]中*/
		DrawData(3, array + i - Sdata[3].array, TRUE);
		qsort(array, i);	/*递归，对前i个数排序*/
		qsort(array + i + 1, length - i - 1);	/*对i+1到numsize-1这numsize-1-i个数排序*/
	}
}

void QsortThread(SORTDATA *data)
{
	qsort(data->array, data->length);
	KExitThread(NO_ERROR);
}

void DrawScene()	/*场景绘图*/
{
	long i, j;

	for (i = 0; i < 4; i++)
	{
		GCFillRect(&wnd->client, SIDE_WIDTH + i * (MAX_SORTDATA + SIDE_WIDTH), SIDE_WIDTH, MAX_SORTDATA, SORTARRAY_LEN, 0xFFA0A0A0);
		for (j = 0; j < SORTARRAY_LEN; j++)
			GCFillRect(&wnd->client, SIDE_WIDTH + i * (MAX_SORTDATA + SIDE_WIDTH), SORTARRAY_LEN + SIDE_WIDTH - 1 - j, Sdata[i].array[j], 1, 0xFF000000);
	}
	GCDrawStr(&wnd->client, MAX_SORTDATA * 0 + SIDE_WIDTH * 1, SORTARRAY_LEN + SIDE_WIDTH * 2, "冒泡排序", 0xFFFF8000);
	GCDrawStr(&wnd->client, MAX_SORTDATA * 1 + SIDE_WIDTH * 2, SORTARRAY_LEN + SIDE_WIDTH * 2, "选择排序", 0xFFFF8000);
	GCDrawStr(&wnd->client, MAX_SORTDATA * 2 + SIDE_WIDTH * 3, SORTARRAY_LEN + SIDE_WIDTH * 2, "双向选择", 0xFFFF8000);
	GCDrawStr(&wnd->client, MAX_SORTDATA * 3 + SIDE_WIDTH * 4, SORTARRAY_LEN + SIDE_WIDTH * 2, "快速排序", 0xFFFF8000);
	GCWndGetClientLoca(wnd, &i, &j);
	GUIpaint(wnd->obj.gid, i + SIDE_WIDTH, j + SIDE_WIDTH, MAX_SORTDATA * 4 + SIDE_WIDTH * 3, SORTARRAY_LEN + SIDE_WIDTH * 2 + GCCharHeight);
}

void RandBtnPressProc(CTRL_BTN *btn)
{
	long i, j;

	RandSeed = ~(DWORD)TMGetRand();
	for (i = 0; i < 4; i++)
	{
		Sdata[i].length = SORTARRAY_LEN;
		for (j = 0; j < SORTARRAY_LEN; j++)
			Sdata[i].array[j] = rand(MAX_SORTDATA) + 1u;
	}
	DrawScene();
	GCBtnSetDisable(DemoBtn, FALSE);
}

void KillBtnPressProc(CTRL_BTN *btn)
{
	KKillThread(SthPtid[0].ThedID);
	KKillThread(SthPtid[1].ThedID);
	KKillThread(SthPtid[2].ThedID);
	KKillThread(SthPtid[3].ThedID);
}

void DemoBtnPressProc(CTRL_BTN *btn)
{
	switch (DemoState)
	{
	case DEMO_STATE_IDLE:	/*转为正在排序*/
		PtidCount = 4;
		KCreateThread((void(*)(void*))BsortThread, 0x4000, Sdata + 0, &SthPtid[0]);
		KCreateThread((void(*)(void*))SsortThread, 0x4000, Sdata + 1, &SthPtid[1]);
		KCreateThread((void(*)(void*))DsortThread, 0x4000, Sdata + 2, &SthPtid[2]);
		KCreateThread((void(*)(void*))QsortThread, 0x4000, Sdata + 3, &SthPtid[3]);
		DemoState = DEMO_STATE_SORTING;
		KGetClock(&StartClock);
		GCBtnSetDisable(RandBtn, TRUE);
		GCBtnSetDisable(KillBtn, FALSE);
		GCBtnSetText(btn, "暂停");
		break;
	case DEMO_STATE_SORTING:	/*转为暂停*/
		{
			DWORD data[MSG_DATA_LEN];
			data[MSG_ATTR_ID] = MSG_ATTR_USER;
			KSendMsg(&SthPtid[0], data, 0);
			KSendMsg(&SthPtid[1], data, 0);
			KSendMsg(&SthPtid[2], data, 0);
			KSendMsg(&SthPtid[3], data, 0);
		}
		DemoState = DEMO_STATE_PAUSE;
		GCBtnSetText(btn, "继续");
		break;
	case DEMO_STATE_PAUSE:	/*转为继续*/
		{
			DWORD data[MSG_DATA_LEN];
			data[MSG_ATTR_ID] = MSG_ATTR_USER;
			KSendMsg(&SthPtid[0], data, 0);
			KSendMsg(&SthPtid[1], data, 0);
			KSendMsg(&SthPtid[2], data, 0);
			KSendMsg(&SthPtid[3], data, 0);
		}
		DemoState = DEMO_STATE_SORTING;
		GCBtnSetText(btn, "暂停");
		break;
	}
}

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			long CliX, CliY;
			CTRL_ARGS args;
			GCWndGetClientLoca(wnd, &CliX, &CliY);
			args.width = MAX_SORTDATA;
			args.height = 24;
			args.x = CliX + SIDE_WIDTH;
			args.y = CliY + SORTARRAY_LEN + SIDE_WIDTH * 3 + GCCharHeight;
			args.style = 0;
			args.MsgProc = NULL;
			GCBtnCreate(&RandBtn, &args, wnd->obj.gid, &wnd->obj, "生成乱序数据", NULL, RandBtnPressProc);
			args.x = CliX + MAX_SORTDATA * 3 + SIDE_WIDTH * 4;
			args.style = BTN_STYLE_DISABLED;
			GCBtnCreate(&KillBtn, &args, wnd->obj.gid, &wnd->obj, "停止演示线程", NULL, KillBtnPressProc);
			args.width = MAX_SORTDATA * 2 + SIDE_WIDTH;
			args.x = CliX + MAX_SORTDATA + SIDE_WIDTH * 2;
			GCBtnCreate(&DemoBtn, &args, wnd->obj.gid, &wnd->obj, "开始排序演示", NULL, DemoBtnPressProc);
		}
	}
	return GCWndDefMsgProc(ptid, data);
}

int main()
{
	CTRL_ARGS args;
	long res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*设置16MB堆内存*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = MAX_SORTDATA * 4 + SIDE_WIDTH * 5 + 2;
	args.height = SORTARRAY_LEN + SIDE_WIDTH * 4 + GCCharHeight + 24 + 21;
	args.x = 80;
	args.y = 40;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "ulios2多线程排序演示程序");

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
		else if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_THEDEXIT)
		{
			long i;

			for (i = 0; i < 4; i++)
				if (ptid.ThedID == SthPtid[i].ThedID)
				{
					char buf[32];
					DWORD clk;
					long x, y;
					
					KGetClock(&clk);
					Sprintf(buf, ":%uMS", (clk - StartClock) * 10);
					GCFillRect(&wnd->client, SIDE_WIDTH + i * (MAX_SORTDATA + SIDE_WIDTH) + GCCharWidth * 8, SORTARRAY_LEN + SIDE_WIDTH * 2, 14 * GCCharWidth, GCCharHeight, 0xFFC0C0C0);
					GCDrawStr(&wnd->client, SIDE_WIDTH + i * (MAX_SORTDATA + SIDE_WIDTH) + GCCharWidth * 8, SORTARRAY_LEN + SIDE_WIDTH * 2, buf, 0xFF602060);
					GCWndGetClientLoca(wnd, &x, &y);
					GUIpaint(wnd->obj.gid, x + SIDE_WIDTH + i * (MAX_SORTDATA + SIDE_WIDTH) + GCCharWidth * 8, y + SORTARRAY_LEN + SIDE_WIDTH * 2, 14 * GCCharWidth, GCCharHeight);
					break;
				}
			if (--PtidCount == 0)
			{
				DemoState = DEMO_STATE_IDLE;
				GCBtnSetDisable(RandBtn, FALSE);
				GCBtnSetDisable(KillBtn, TRUE);
				GCBtnSetText(DemoBtn, "开始排序演示");
			}
		}
	}
	return NO_ERROR;
}
