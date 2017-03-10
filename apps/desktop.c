/*	desktop.c for ulios application
	作者：孙亮
	功能：桌面应用进程
	最后修改日期：2011-05-21
*/

#include "../fs/fsapi.h"
#include "../lib/malloc.h"
#include "../lib/gclient.h"

/*双线性插值24位图像缩放,参数:源缓冲,源尺寸,目的缓冲,目的尺寸*/
void ZoomImage(DWORD *src, DWORD sw, DWORD sh, DWORD *dst, DWORD dw, DWORD dh)
{
	DWORD ScaleX, ScaleY, x, y, IdxX, IdxY, DecX, DecY;	// 比例,循环计数,源数组索引,点间偏移小数
	ScaleX = ((sw - 1) << 16) / dw;
	ScaleY = ((sh - 1) << 16) / dh;
	IdxY = 0;
	y = dh;
	do
	{
		DecY = (IdxY >> 8) & 0xFF;
		IdxX = 0;
		x = dw;
		do
		{
			DecX = (IdxX >> 8) & 0xFF;

			DWORD tr1, tg1, tb1, tr2, tg2, tb2;	// 三原色临时变量
			DWORD a, b, c, d;
			DWORD *psrc = &src[(IdxX >> 16) + (IdxY >> 16) * sw];
			a = *(psrc);	// 取得源中与目标点相关的四个点
			b = *(psrc + 1);
			c = *(psrc + sw);
			d = *(psrc + sw + 1);

			tr1 = DecX * (long)((b & 0x0000FF) - (a & 0x0000FF)) + ((a & 0x0000FF) << 8);	// a和b c和d横向计算
			tg1 = DecX * (long)((b & 0x00FF00) - (a & 0x00FF00)) + ((a & 0x00FF00) << 8);
			tb1 = DecX * (long)((b & 0xFF0000) - (a & 0xFF0000)) + ((a & 0xFF0000) << 8);
			tr2 = DecX * (long)((d & 0x0000FF) - (c & 0x0000FF)) + ((c & 0x0000FF) << 8);
			tg2 = DecX * (long)((d & 0x00FF00) - (c & 0x00FF00)) + ((c & 0x00FF00) << 8);
			tb2 = DecX * (long)((d & 0xFF0000) - (c & 0xFF0000)) + ((c & 0xFF0000) << 8);
			tr1 += DecY * (long)((tr2 >> 8) - (tr1 >> 8));	// 纵向计算
			tg1 += DecY * (long)((tg2 >> 8) - (tg1 >> 8));
			tb1 += DecY * (long)((tb2 >> 8) - (tb1 >> 8));
			*dst++ = ((tr1 & 0x0000FF00) | (tg1 & 0x00FF0000) | (tb1 & 0xFF000000)) >> 8;	// 三原色合成
			IdxX += ScaleX;
		}
		while (--x > 0);
		IdxY += ScaleY;
	}
	while (--y > 0);
}

void CmdProc(CTRL_BTN *btn)
{
	THREAD_ID ptid;
	KCreateProcess(0, "cmd.bin", NULL, &ptid);
}

void GmgrProc(CTRL_BTN *btn)
{
	THREAD_ID ptid;
	KCreateProcess(0, "gmgr.bin", NULL, &ptid);
}

/*桌面消息处理函数*/
long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_DSK *dsk = (CTRL_DSK*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			DWORD *bmp, width, height;

			if (dsk->obj.gid)	/*非主桌面,主动退出*/
			{
				GUIdestroy(dsk->obj.gid);
				break;
			}
			if (GCLoadBmp("desktop.bmp", NULL, 0, &width, &height) == NO_ERROR)
			{
				bmp = (DWORD*)malloc(width * height * sizeof(DWORD));
				GCLoadBmp("desktop.bmp", bmp, width * height, NULL, NULL);
				ZoomImage(bmp, width, height, dsk->obj.uda.vbuf, dsk->obj.uda.width, dsk->obj.uda.height);
				free(bmp);
			}
			else	/*没有桌面图片,用背景色填充*/
				GCFillRect(&dsk->obj.uda, 0, 0, dsk->obj.uda.width, dsk->obj.uda.height, 0xFFBDB68A);
		}
		{
			CTRL_ARGS args;
			args.width = 80;
			args.height = 20;
			args.x = 16;
			args.y = 16;
			args.style = 0;
			args.MsgProc = NULL;
			GCBtnCreate(NULL, &args, dsk->obj.gid, &dsk->obj, "命令提示符", NULL, CmdProc);
			args.y = 40;
			GCBtnCreate(NULL, &args, dsk->obj.gid, &dsk->obj, "资源管理器", NULL, GmgrProc);
		}
		break;
	}
	return GCDskDefMsgProc(ptid, data);
}

int main()
{
	CTRL_DSK *dsk;
	CTRL_ARGS args;
	long res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*设置16MB堆内存*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	{
		UDI_AREA uda;

		if (GCLoadBmp("mouse.bmp", NULL, 0, &uda.width, &uda.height) == NO_ERROR)
		{
			uda.vbuf = (DWORD*)malloc(uda.width * uda.height * sizeof(DWORD));
			GCLoadBmp("mouse.bmp", uda.vbuf, uda.width * uda.height, NULL, NULL);
		}
		else	/*没有鼠标图片,自行绘制*/
		{
			uda.width = 12;
			uda.height = 16;
			uda.vbuf = (DWORD*)malloc(uda.width * uda.height * sizeof(DWORD));
			uda.root = &uda;
			GCFillRect(&uda, 0, 0, uda.width, uda.height, 0xFFFFFFFF);
			GCDrawLine(&uda, 0, 0, 0, 15, 0xFFFF0000);
			GCDrawLine(&uda, 0, 0, 11, 11, 0xFFFF0000);
			GCDrawLine(&uda, 0, 15, 11, 11, 0xFFFF0000);
		}
		GUISetMouse(0, 0, uda.width, uda.height, uda.vbuf);
		free(uda.vbuf);
	}
	args.width = GCwidth;
	args.height = GCheight;
	args.x = 0;
	args.y = 0;
	args.style = 0;
	args.MsgProc = MainMsgProc;
	GCDskCreate(&dsk, &args, 0, NULL);

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*处理GUI消息*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)dsk)	/*销毁主窗体,退出程序*/
				break;
		}
	}
	return NO_ERROR;
}
