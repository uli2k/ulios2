/*	clock.c for ulios application
	作者：刘高辉
	功能：图形化时钟程序
	最后修改日期：2010-12-11
*/

#include "../driver/basesrv.h"
#include "../lib/gdi.h"
#include "../lib/math.h"

#define PI			3.1415927f
#define CLOCK_R		31							/*表盘半径*/
#define CLOCK_WID	(CLOCK_R * 2 + 1)			/*表盘宽度*/
#define CLOCK_X		(GDIwidth - CLOCK_R - 1)	/*中心位置*/
#define CLOCK_Y		(CLOCK_R + 1)

int main(int argc, char *argv[])
{
	TM nowtime;

	if (GDIinit() != NO_ERROR)
		return NO_ERROR;
	for(;;)
	{
		float i, tmphor;
		TMCurTime(&nowtime);
		//背景
		GDIFillRect(GDIwidth - CLOCK_WID, 0, CLOCK_WID, CLOCK_WID, 0);
		//表盘
		GDIcircle(CLOCK_X, CLOCK_Y, CLOCK_R, 0xFFFFFF);
		//刻度
		for (i = 0.f; i < 12.f * PI / 6.f; i += PI / 6.f)
		{
			float cosa, sina;

			cosa = cos(i);
			sina = sin(i);
			GDIDrawLine(CLOCK_X + (long)(CLOCK_R * cosa), CLOCK_Y + (long)(CLOCK_R * sina), CLOCK_X + (long)((CLOCK_R - 3) * cosa), CLOCK_Y + (long)((CLOCK_R - 3) * sina), 0xFFFFFF);
		}
		GDIDrawStr(CLOCK_X + CLOCK_R - 8, CLOCK_Y - 6, "3", 0xFFFFFF);
		GDIDrawStr(CLOCK_X - 2, CLOCK_Y + CLOCK_R - 15, "6", 0xFFFFFF);
		GDIDrawStr(CLOCK_X - CLOCK_R + 5, CLOCK_Y - 6, "9", 0xFFFFFF);
		GDIDrawStr(CLOCK_X - 5, CLOCK_Y - CLOCK_R + 3, "12", 0xFFFFFF);
		//指针
		tmphor = (nowtime.hor + (float)nowtime.min / 60.f) * PI / 6.f;
		GDIDrawLine(CLOCK_X, CLOCK_Y, CLOCK_X + 15.f * sin(tmphor), CLOCK_Y - 15.f * cos(tmphor), 0xFFFFFF);
		GDIDrawLine(CLOCK_X, CLOCK_Y, CLOCK_X + 20.f * sin(nowtime.min * PI / 30.f), CLOCK_Y - 20.f * cos(nowtime.min * PI / 30.f), 0xFFFFFF);
		GDIDrawLine(CLOCK_X, CLOCK_Y, CLOCK_X + 25.f * sin(nowtime.sec * PI / 30.f), CLOCK_Y - 25.f * cos(nowtime.sec * PI / 30.f), 0xFFFFFF);
		KSleep(100);
	}

	GDIrelease();
	return NO_ERROR;
}
