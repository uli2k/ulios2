/*	3demo.c for ulios application
	作者：孙亮
	功能：3D点图形演示程序
	最后修改日期：2010-05-14
*/

#include "../driver/basesrv.h"
#include "../lib/gdi.h"
#include "../lib/math.h"

#define PN 2400
#define TN 11
#define ABS(x)	((x) < 0 ? -(x) : (x))

typedef struct _POINT3D
{
	float x, y, z;		/*当前坐标*/
	float tx, ty, tz;	/*目的坐标*/
	long sx, sy;		/*屏幕坐标*/
}POINT3D;

DWORD RandSeed;
POINT3D point[PN];

long rand(DWORD x)
{
	RandSeed = RandSeed * 1103515245 + 12345;
	return RandSeed % x;
}

void t0()
{
	int i;
	for (i = 0; i < PN; i++)
	{
		point[i].tx = (float)(rand(400) - 200);
		point[i].ty = (float)(rand(400) - 200);
		point[i].tz = 0.0f;
	}
}

void t1()
{
	int i;
	for (i = 0; i < PN; i++)
		point[i].tz = (float)cos((point[i].x * point[i].x + point[i].y * point[i].y) / 3000.0f) * 50.0f;
}

void t2()
{
	int i;
	for (i = 0; i < PN; i++)
	{
		point[i].tx = 0.0f;
		point[i].ty = (float)(rand(400) - 200);
		point[i].tz = (float)(rand(400) - 200);
	}
}

void t3()
{
	int i;
	for (i = 0; i < PN / 6; i++)
	{
		point[i].tx = -200.0f;
		point[i].ty = (float)(rand(400) - 200);
		point[i].tz = (float)(rand(400) - 200);
		point[i + PN / 6].tx = 199.0f;
		point[i + PN / 6].ty = (float)(rand(400) - 200);
		point[i + PN / 6].tz = (float)(rand(400) - 200);
		point[i + PN / 3].tx = (float)(rand(400) - 200);
		point[i + PN / 3].ty = -200.0f;
		point[i + PN / 3].tz = (float)(rand(400) - 200);
		point[i + PN / 2].tx = (float)(rand(400) - 200);
		point[i + PN / 2].ty = 199.0f;
		point[i + PN / 2].tz = (float)(rand(400) - 200);
		point[i + PN / 3 * 2].tx = (float)(rand(400) - 200);
		point[i + PN / 3 * 2].ty = (float)(rand(400) - 200);
		point[i + PN / 3 * 2].tz = -200.0f;
		point[i + PN / 6 * 5].tx = (float)(rand(400) - 200);
		point[i + PN / 6 * 5].ty = (float)(rand(400) - 200);
		point[i + PN / 6 * 5].tz = 199.0f;
	}
}

void t4()
{
	int i;
	for (i = 0; i < PN / 3; i++)
	{
		point[i].tx = -point[i].tx;
		point[i + PN / 3].ty = -point[i + PN / 3].ty;
		point[i + PN / 3 * 2].tz = -point[i + PN / 3 * 2].tz;
	}
}

void t5()
{
	int i, j;
	float a;
	for (i = 0; i < PN / 30; i++)
	for (j = 0; j < 10; j++)
	{
		a = (float)rand(628) / 100.0f;
		point[i * 10 + j].tx = (float)cos(i * 0.07848) * (200.0f + (float)cos(a) * 20.0f);
		point[i * 10 + j].ty = (float)sin(i * 0.07848) * (200.0f + (float)cos(a) * 20.0f);
		point[i * 10 + j].tz = (float)sin(a) * 20.0f;
		point[PN / 3 + i * 10 + j].tx = (float)sin(a) * 20.0f;
		point[PN / 3 + i * 10 + j].ty = (float)cos(i * 0.07848) * (200.0f + (float)cos(a) * 20.0f);
		point[PN / 3 + i * 10 + j].tz = (float)sin(i * 0.07848) * (200.0f + (float)cos(a) * 20.0f);
		point[PN / 3 * 2 + i * 10 + j].tx = (float)sin(i * 0.07848) * (200.0f + (float)cos(a) * 20.0f);
		point[PN / 3 * 2 + i * 10 + j].ty = (float)sin(a) * 20.0f;
		point[PN / 3 * 2 + i * 10 + j].tz = (float)cos(i * 0.07848) * (200.0f + (float)cos(a) * 20.0f);
	}
}

void t6()
{
	int n, i, j;
	float a;
	for (n = 0; n < 6; n++)
	for (i = 0; i < PN / 60; i++)
	for (j = 0; j < 10; j++)
	{
		a = (float)rand(314) / 100.0f - 1.57f;
		point[PN / 6 * n + i * 10 + j].tx = (float)cos(n * 1.0467) * 200.0f + (float)cos(i * 0.15698) * (float)cos(a) * 30.0f;
		point[PN / 6 * n + i * 10 + j].ty = (float)sin(n * 1.0467) * 200.0f + (float)sin(i * 0.15698) * (float)cos(a) * 30.0f;
		point[PN / 6 * n + i * 10 + j].tz = (float)sin(a) * 30.0f;
	}
}

void t7()
{
	int i, j;
	float a;
	for (i = 0; i < PN / 10; i++)
	for (j = 0; j < 10; j++)
	{
		a = (float)rand(628) / 100.0f;
		point[i * 10 + j].tx = (float)cos(i * 0.02616) * (200.0f + (float)cos(a) * 30.0f);
		point[i * 10 + j].ty = (float)sin(i * 0.02616) * (200.0f + (float)cos(a) * 30.0f);
		point[i * 10 + j].tz = (float)sin(a) * 30.0f;
	}
}

void t8()
{
	int i, j;
	float a;
	for (i = 0; i < PN / 10; i++)
	for (j = 0; j < 10; j++)
	{
		a = (float)rand(314) / 100.0f - 1.57f;
		point[i * 10 + j].tx = (float)cos(i * 0.02616) * (float)cos(a) * 200.0f;
		point[i * 10 + j].ty = (float)sin(i * 0.02616) * (float)cos(a) * 200.0f;
		point[i * 10 + j].tz = (float)sin(a) * 200.0f;
	}
}

void t9()
{
	int i;
	float a, b;
	for (i = 0; i < PN; i++)
	{
		a = (float)rand(628) / 100.0f;
		b = (float)rand(198) + 2.0f;
		point[i].tx = (float)cos(a) * b;
		point[i].ty = (float)sin(a) * b;
		point[i].tz = ((float)rand(400) - 200.0f) / b;
	}
}

void ulios()
{
	int i, j, k, c = 0;
	float a, b;
	for (j = 0; j < 120; j += 4)
	for (i = 0; i < 300; i += 4)
	{
		DWORD col;
		GDIGetPixel(66 + i / 10, j / 10, &col);
		if (col)
		{
			for (k = 0; k < 4; k++)
			{
				point[c + k].tx = (float)(150 - i);
				point[c + k].ty = (float)(60 - j);
				point[c + k].tz = (float)(k * 8 - 12);
			}
			c += 4;
		}
	}
	for (; c < PN; c++)
	{
		a = (float)rand(628) / 100.0f;
		b = (float)rand(198) + 2.0f;
		point[c].tx = (float)cos(a) * b;
		point[c].ty = (float)sin(a) * b;
		point[c].tz = 0.0f;
	}
}

void (*GraFunc[TN])() = {t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, ulios};

int main()
{
	float a = 0.0f, b = 0.0f;
	DWORD i, j;
	long res;

	RandSeed = (DWORD)TMGetRand();
	for (i = 0; i < PN; i++)
	{
		point[i].x = (float)(rand(400) - 200);
		point[i].y = (float)(rand(400) - 200);
		point[i].z = (float)(rand(400) - 200);
		point[i].sx = point[i].sy = 0;
	}
	if ((res = GDIinit()) != NO_ERROR)
		return res;
	GDIFillRect(0, 0, GDIwidth, GDIheight, 0);
	GDIDrawStr(0, 0, "Welcome to ulios 3Ddemo program!", 0xFFFFFF);

	for (j = 0; j < 0x5800; j++)
	{
		float sa, ca, sb, cb;

		sa = sin(a);	/*显示*/
		ca = cos(a);
		sb = sin(b);
		cb = cos(b);
		a += 0.005f;
		b += 0.005f;
		for (i = 0; i < PN; i++)
		{
			float x, y, z;

			x = point[i].x * ca + point[i].y * sa;
			y = point[i].y * ca - point[i].x * sa;
			z = point[i].z * cb + x * sb;
			x = x * cb - point[i].z * sb;
			GDIPutPixel(point[i].sx, point[i].sy, 0);
			point[i].sx = (GDIwidth >> 1) + (long)(1000.0f * y / (1400.0f - x));
			point[i].sy = (GDIheight >> 1) - (long)(1000.0f * z / (1400.0f - x));
			GDIPutPixel(point[i].sx, point[i].sy, ((ABS((long)point[i].x) + 20) << 16) | ((ABS((long)point[i].y) + 20) << 8) | (ABS((long)point[i].z) + 20));
		}

		if ((j & 0x7FF) == 0)	/*设置目的坐标*/
			GraFunc[(j >> 11) % TN]();

		for (i = 0; i < PN; i++)	/*移动*/
		{
			point[i].x -= ((point[i].x - point[i].tx) / 200.0f);
			point[i].y -= ((point[i].y - point[i].ty) / 200.0f);
			point[i].z -= ((point[i].z - point[i].tz) / 200.0f);
		}
		KSleep(2);
	}
	GDIrelease();
	return NO_ERROR;
}
