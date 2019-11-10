/*	life.c for ulios application
	作者：孙亮
	功能：生命游戏演示程序
	最后修改日期：2019-03-24
*/

#include "../driver/basesrv.h"
#include "../lib/gdi.h"

int main()
{
	long i, x, y;
	const char OffX[] = {-1, -1, -1,  0,  0,  1,  1,  1};
	const char OffY[] = {-1,  0,  1, -1,  1, -1,  0,  1};
	BYTE *data[2], *count;
	BYTE dataIdx = 0;
	long res;

	if ((res = GDIinit()) != NO_ERROR)
		return res;
	if ((res = KMapUserAddr((void**)data, GDIwidth * GDIheight * 3)) != NO_ERROR)
		return res;
	GDIFillRect(0, 0, GDIwidth, GDIheight, 0);
	GDIDrawStr((GDIwidth - GDICharWidth * 27) / 2, (GDIheight - GDICharHeight) / 2, "Welcome to ulios life game!", 0xFFFFFF);

	data[1] = data[0] + GDIwidth * GDIheight;
	count = data[1] + GDIwidth * GDIheight;
	for (y = 0; y < GDIheight; y++)
		for (x = 0; x < GDIwidth; x++)
		{
			DWORD c;
			GDIGetPixel(x, y, &c);
			data[dataIdx][x + y * GDIwidth] = c ? 1 : 0;
			count[x + y * GDIwidth] = 0;
		}
	for (i = 0; i < 4096; i++)
	{
		BYTE j;
		for (y = 1; y < GDIheight - 1; y++)
			for (x = 1; x < GDIwidth - 1; x++)
				if (data[dataIdx][x + y * GDIwidth])
					for (j = 0; j < 8; j++)
						count[x + OffX[j] + (y + OffY[j]) * GDIwidth]++;
		dataIdx = 1 - dataIdx;
		for (y = 1; y < GDIheight - 1; y++)
			for (x = 1; x < GDIwidth - 1; x++)
			{
				if (count[x + y * GDIwidth] == 2)
					data[dataIdx][x + y * GDIwidth] = data[1 - dataIdx][x + y * GDIwidth];
				else if (count[x + y * GDIwidth] == 3)
				{
					data[dataIdx][x + y * GDIwidth] = 1;
					GDIPutPixel(x, y, 0xFFFFFF);
				}
				else if (count[x + y * GDIwidth] > 0)
				{
					data[dataIdx][x + y * GDIwidth] = 0;
					GDIPutPixel(x, y, 0);
				}
				count[x + y * GDIwidth] = 0;
			}
	}
	KFreeAddr(data[0]);
	GDIrelease();
	return NO_ERROR;
}
