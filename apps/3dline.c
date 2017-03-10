/*	3dline.c for ulios application
	作者：孙亮
	功能：3D线条浮点运算测试程序
	最后修改日期：2010-05-14
*/

#include "../driver/basesrv.h"
#include "../lib/gdi.h"

#define PN		50

int main()
{
	float x[PN], y[PN], z[PN];
	long sx[PN], sy[PN];
	long i, res;

	for (i = 0; i < PN; i++)
	{
		x[i] = (float)((TMGetRand() & 0x1FF) - 256);
		y[i] = (float)((TMGetRand() & 0x1FF) - 256);
		z[i] = (float)((TMGetRand() & 0x1FF) - 256);
	}
	if ((res = GDIinit()) != NO_ERROR)
		return res;
	GDIFillRect(0, 0, GDIwidth, GDIheight, 0x4080FF);
	GDIDrawStr(0, 0, "welcome to ulios 3Dline Demo!", 0xABCDEF);
	for (res = 0; res < 1000; res++)
	{
		for (i = 0; i < PN - 1; i++)
			GDIDrawLine(sx[i], sy[i], sx[i + 1], sy[i + 1], 0x4080FF);
		for (i = 0; i < PN; i++)
		{
			float temp = x[i];
			x[i] = x[i] * 0.99955f + y[i] * 0.0299955f;
			y[i] = y[i] * 0.99955f - temp * 0.0299955f;
			sx[i] = (GDIwidth >> 1) + (long)(1000.0f * y[i] / (1400.0f - x[i]));
			sy[i] = (GDIheight >> 1) - (long)(1000.0f * z[i] / (1400.0f - x[i]));
		}
		for (i = 0; i < PN - 1; i++)
			GDIDrawLine(sx[i], sy[i], sx[i + 1], sy[i + 1], 0xFFFFFF);
		KSleep(5);
	}
	GDIrelease();
	return NO_ERROR;
}
