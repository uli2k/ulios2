/*	gdi.c for ulios
	作者：孙亮
	功能：图形设备接口库实现
	最后修改日期：2010-06-10
*/

#include "gdi.h"

void *GDIvm;
const BYTE *GDIfont;
DWORD GDIwidth, GDIheight, GDIPixBits, GDImode;
DWORD GDICharWidth, GDICharHeight;

/*初始化GDI库*/
long GDIinit()
{
	long res;

	if (GDIvm == NULL && (res = VSGetVmem(&GDIvm, &GDIwidth, &GDIheight, &GDIPixBits, &GDImode)) != NO_ERROR)
		return res;
	if (!GDImode)
		return GDI_ERR_TEXTMODE;
	if (GDIfont == NULL && (res = FNTGetFont(&GDIfont, &GDICharWidth, &GDICharHeight)) != NO_ERROR)
		return res;
	return NO_ERROR;
}

/*撤销GDI库*/
void GDIrelease()
{
	DWORD data[MSG_DATA_LEN];

	if (GDIvm)
	{
		KUnmapProcAddr(GDIvm, data);
		GDIvm = NULL;
	}
	if (GDIfont)
	{
		KUnmapProcAddr((void*)GDIfont, data);
		GDIfont = NULL;
	}
}

typedef struct _RGB24
{
	BYTE blue, green, red;
}__attribute__((packed)) RGB24;	/*每像素24位模式专用结构*/

static inline DWORD DW2RGB15(DWORD c)
{
	return ((c >> 3) & 0x001F) | ((c >> 6) & 0x03E0) | ((c >> 9) & 0x7C00);
}

static inline DWORD RGB152DW(DWORD c)
{
	return ((c << 3) & 0x0000F8) | ((c << 6) & 0x00F800) | ((c << 9) & 0xF80000);
}

static inline DWORD DW2RGB16(DWORD c)
{
	return ((c >> 3) & 0x001F) | ((c >> 5) & 0x07E0) | ((c >> 8) & 0xF800);
}

static inline DWORD RGB162DW(DWORD c)
{
	return ((c << 3) & 0x0000F8) | ((c << 5) & 0x00FC00) | ((c << 8) & 0xF80000);
}

/*画点*/
long GDIPutPixel(DWORD x, DWORD y, DWORD c)
{
	if (x >= GDIwidth || y >= GDIheight)
		return GDI_ERR_LOCATION;	/*位置越界*/
	switch (GDIPixBits)
	{
	case 15:
		((WORD*)GDIvm)[x + y * GDIwidth] = DW2RGB15(c);
		break;
	case 16:
		((WORD*)GDIvm)[x + y * GDIwidth] = DW2RGB16(c);
		break;
	case 24:
		((RGB24*)GDIvm)[x + y * GDIwidth] = *(RGB24*)&c;
		break;
	case 32:
		((DWORD*)GDIvm)[x + y * GDIwidth] = c;
		break;
	}
	return NO_ERROR;
}

/*取点*/
long GDIGetPixel(DWORD x, DWORD y, DWORD *c)
{
	if (x >= GDIwidth || y >= GDIheight)
		return GDI_ERR_LOCATION;	/*位置越界*/
	switch (GDIPixBits)
	{
	case 15:
		*c = RGB152DW(((WORD*)GDIvm)[x + y * GDIwidth]);
		break;
	case 16:
		*c = RGB162DW(((WORD*)GDIvm)[x + y * GDIwidth]);
		break;
	case 24:
		*(RGB24*)c = ((RGB24*)GDIvm)[x + y * GDIwidth];
		break;
	case 32:
		*c = ((DWORD*)GDIvm)[x + y * GDIwidth];
		break;
	}
	return NO_ERROR;
}

static inline void Dw2Rgb15(WORD *dest, const DWORD *src, DWORD n)
{
	while (n--)
		*dest++ = DW2RGB15(*src++);
}

static inline void Dw2Rgb16(WORD *dest, const DWORD *src, DWORD n)
{
	while (n--)
		*dest++ = DW2RGB16(*src++);
}

static inline void Dw2Rgb24(RGB24 *dest, const DWORD *src, DWORD n)
{
	while (n--)
		*dest++ = *(RGB24*)(src++);
}

/*贴图*/
long GDIPutImage(long x, long y, DWORD *img, long w, long h)
{
	long memw;

	if (x >= (long)GDIwidth || x + w <= 0 || y >= (long)GDIheight || y + h <= 0)
		return GDI_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GDI_ERR_SIZE;	/*非法尺寸*/
	memw = w;
	if (x < 0)
	{
		img -= x;
		w += x;
		x = 0;
	}
	if (w > (long)GDIwidth - x)
		w = (long)GDIwidth - x;
	if (y < 0)
	{
		img -= y * memw;
		h += y;
		y = 0;
	}
	if (h > (long)GDIheight - y)
		h = (long)GDIheight - y;

	switch (GDIPixBits)
	{
	case 15:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Dw2Rgb15(tmpvm, img, w);
		}
		break;
	case 16:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Dw2Rgb16(tmpvm, img, w);
		}
		break;
	case 24:
		{
			RGB24 *tmpvm;
			for (tmpvm = (RGB24*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Dw2Rgb24(tmpvm, img, w);
		}
		break;
	case 32:
		{
			DWORD *tmpvm;
			for (tmpvm = (DWORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				memcpy32(tmpvm, img, w);
		}
		break;
	}
	return NO_ERROR;
}

static inline void BCDw2Rgb15(WORD *dest, const DWORD *src, DWORD n, DWORD bc)
{
	while (n--)
	{
		if (*src != bc)
			*dest = DW2RGB15(*src);
		src++;
		dest++;
	}
}

static inline void BCDw2Rgb16(WORD *dest, const DWORD *src, DWORD n, DWORD bc)
{
	while (n--)
	{
		if (*src != bc)
			*dest = DW2RGB16(*src);
		src++;
		dest++;
	}
}

static inline void BCDw2Rgb24(RGB24 *dest, const DWORD *src, DWORD n, DWORD bc)
{
	while (n--)
	{
		if (*src != bc)
			*dest = *(RGB24*)src;
		src++;
		dest++;
	}
}

static inline void BCDw2Rgb32(DWORD *dest, const DWORD *src, DWORD n, DWORD bc)
{
	while (n--)
	{
		if (*src != bc)
			*dest = *src;
		src++;
		dest++;
	}
}

/*去背景色贴图*/
long GDIPutBCImage(long x, long y, DWORD *img, long w, long h, DWORD bc)
{
	long memw;
	
	if (x >= (long)GDIwidth || x + w <= 0 || y >= (long)GDIheight || y + h <= 0)
		return GDI_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GDI_ERR_SIZE;	/*非法尺寸*/
	memw = w;
	if (x < 0)
	{
		img -= x;
		w += x;
		x = 0;
	}
	if (w > (long)GDIwidth - x)
		w = (long)GDIwidth - x;
	if (y < 0)
	{
		img -= y * memw;
		h += y;
		y = 0;
	}
	if (h > (long)GDIheight - y)
		h = (long)GDIheight - y;

	switch (GDIPixBits)
	{
	case 15:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				BCDw2Rgb15(tmpvm, img, w, bc);
		}
		break;
	case 16:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				BCDw2Rgb16(tmpvm, img, w, bc);
		}
		break;
	case 24:
		{
			RGB24 *tmpvm;
			for (tmpvm = (RGB24*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				BCDw2Rgb24(tmpvm, img, w, bc);
		}
		break;
	case 32:
		{
			DWORD *tmpvm;
			for (tmpvm = (DWORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				BCDw2Rgb32(tmpvm, img, w, bc);
		}
		break;
	}
	return NO_ERROR;
}

static inline void Rgb152Dw(DWORD *dest, const WORD *src, DWORD n)
{
	while (n--)
		*dest++ = RGB152DW(*src++);
}

static inline void Rgb162Dw(DWORD *dest, const WORD *src, DWORD n)
{
	while (n--)
		*dest++ = RGB162DW(*src++);
}

static inline void Rgb242Dw(DWORD *dest, const RGB24 *src, DWORD n)
{
	while (n--)
		*dest++ = *(DWORD*)(src++);
}

/*截图*/
long GDIGetImage(long x, long y, DWORD *img, long w, long h)
{
	long memw;
	
	if (x >= (long)GDIwidth || x + w <= 0 || y >= (long)GDIheight || y + h <= 0)
		return GDI_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GDI_ERR_SIZE;	/*非法尺寸*/
	memw = w;
	if (x < 0)
	{
		img -= x;
		w += x;
		x = 0;
	}
	if (w > (long)GDIwidth - x)
		w = (long)GDIwidth - x;
	if (y < 0)
	{
		img -= y * memw;
		h += y;
		y = 0;
	}
	if (h > (long)GDIheight - y)
		h = (long)GDIheight - y;

	switch (GDIPixBits)
	{
	case 15:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Rgb152Dw(img, tmpvm, w);
		}
		break;
	case 16:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Rgb162Dw(img, tmpvm, w);
		}
		break;
	case 24:
		{
			RGB24 *tmpvm;
			for (tmpvm = (RGB24*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Rgb242Dw(img, tmpvm, w);
		}
		break;
	case 32:
		{
			DWORD *tmpvm;
			for (tmpvm = (DWORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, img += memw, h--)
				memcpy32(img, tmpvm, w);
		}
		break;
	}
	return NO_ERROR;
}

static inline void SetRgb1516(WORD *dest, DWORD d, DWORD n)
{
	void *_dest;
	DWORD _n;
	__asm__ __volatile__("cld;rep stosw": "=&D"(_dest), "=&c"(_n): "0"(dest), "a"(d), "1"(n): "flags", "memory");
}

static inline void SetRgb24(RGB24 *dest, DWORD d, DWORD n)
{
	while (n--)
		*dest++ = *(RGB24*)&d;
}

/*填充矩形*/
long GDIFillRect(long x, long y, long w, long h, DWORD c)
{
	if (x >= (long)GDIwidth || x + w <= 0 || y >= (long)GDIheight || y + h <= 0)
		return GDI_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GDI_ERR_SIZE;	/*非法尺寸*/
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (w > (long)GDIwidth - x)
		w = (long)GDIwidth - x;
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (h > (long)GDIheight - y)
		h = (long)GDIheight - y;

	switch (GDIPixBits)
	{
	case 15:
		c = DW2RGB15(c);
		break;
	case 16:
		c = DW2RGB16(c);
		break;
	}
	switch (GDIPixBits)
	{
	case 15:
	case 16:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, h--)
				SetRgb1516(tmpvm, c, w);
		}
		break;
	case 24:
		{
			RGB24 *tmpvm;
			for (tmpvm = (RGB24*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, h--)
				SetRgb24(tmpvm, c, w);
		}
		break;
	case 32:
		{
			DWORD *tmpvm;
			for (tmpvm = (DWORD*)GDIvm + x + y * GDIwidth; h > 0; tmpvm += GDIwidth, h--)
				memset32(tmpvm, c, w);
		}
		break;
	}
	return NO_ERROR;
}

#define abs(v) ((v) >= 0 ? (v) : -(v))

/*无越界判断画点*/
static inline void NCPutPixel(DWORD x, DWORD y, DWORD c)
{
	switch (GDIPixBits)
	{
	case 15:
	case 16:
		((WORD*)GDIvm)[x + y * GDIwidth] = c;
		break;
	case 24:
		((RGB24*)GDIvm)[x + y * GDIwidth] = *(RGB24*)&c;
		break;
	case 32:
		((DWORD*)GDIvm)[x + y * GDIwidth] = c;
		break;
	}
}

#define CS_LEFT		1
#define CS_RIGHT	2
#define CS_TOP		4
#define CS_BOTTOM	8

/*Cohen-Sutherland裁剪算法编码*/
static DWORD CsEncode(long x, long y)
{
	DWORD mask;

	mask = 0;
	if (x < 0)
		mask |= CS_LEFT;
	else if (x >= (long)GDIwidth)
		mask |= CS_RIGHT;
	if (y < 0)
		mask |= CS_TOP;
	else if (y >= (long)GDIheight)
		mask |= CS_BOTTOM;
	return mask;
}

#define F2L_SCALE	0x10000

/*Cohen-Sutherland算法裁剪Bresenham改进算法画线*/
long GDIDrawLine(long x1, long y1, long x2, long y2, DWORD c)
{
	DWORD mask, mask1, mask2;
	long dx, dy, dx2, dy2;
	long e, xinc, yinc, half;

	/*裁剪*/
	mask1 = CsEncode(x1, y1);
	mask2 = CsEncode(x2, y2);
	if (mask1 || mask2)
	{
		long ydivx, xdivy;

		xdivy = ydivx = 0;
		if (x1 != x2)
			ydivx = ((y2 - y1) * F2L_SCALE) / (x2 - x1);
		if (y1 != y2)
			xdivy = ((x2 - x1) * F2L_SCALE) / (y2 - y1);
		do
		{
			if (mask1 & mask2)	/*在裁剪区一侧,完全裁剪掉*/
				return NO_ERROR;

			dy = dx = 0;
			mask = mask1 ? mask1 : mask2;
			if (mask & CS_LEFT)
			{
				dx = 0;
				dy = y1 - (x1 * ydivx / F2L_SCALE);
			}
			else if (mask & CS_RIGHT)
			{
				dx = (long)GDIwidth - 1;
				dy = y1 - ((x1 + 1 - (long)GDIwidth) * ydivx / F2L_SCALE);
			}
			if (mask & CS_TOP)
			{
				dy = 0;
				dx = x1 - (y1 * xdivy / F2L_SCALE);
			}
			else if (mask & CS_BOTTOM)
			{
				dy = (long)GDIheight - 1;
				dx = x1 - ((y1 + 1 - (long)GDIheight) * xdivy / F2L_SCALE);
			}
			if (mask == mask1)
			{
				x1 = dx;
				y1 = dy;
				mask1 = CsEncode(dx, dy);
			}
			else
			{
				x2 = dx;
				y2 = dy;
				mask2 = CsEncode(dx, dy);
			}
		}
		while (mask1 || mask2);
	}

	/*画线*/
	dx = abs(x2 - x1);
	dx2 = dx << 1;
	xinc = (x2 > x1) ? 1 : (x2 < x1 ? -1 : 0);
	dy = abs(y2 - y1);
	dy2 = dy << 1;
	yinc = (y2 > y1) ? 1 : (y2 < y1 ? -1 : 0);
	switch (GDIPixBits)
	{
	case 15:
		c = DW2RGB15(c);
		break;
	case 16:
		c = DW2RGB16(c);
		break;
	}
	if (dx >= dy)
	{
		e = dy2 - dx;
		half = (dx + 1) >> 1;
		while (half--)
		{
			NCPutPixel(x1, y1, c);
			NCPutPixel(x2, y2, c);
			if (e > 0)
			{
				e -= dx2;
				y1 += yinc;
				y2 -= yinc;
			}
			e += dy2;
			x1 += xinc;
			x2 -= xinc;
		}
		if (x1 == x2)	/*需多写一个中间点*/
			NCPutPixel(x1, y1, c);
	}
	else
	{
		e = dx2 - dy;
		half = (dy + 1) >> 1;
		while (half--)
		{
			NCPutPixel(x1, y1, c);
			NCPutPixel(x2, y2, c);
			if (e > 0)
			{
				e -= dy2;
				x1 += xinc;
				x2 -= xinc;
			}
			e += dx2;
			y1 += yinc;
			y2 -= yinc;
		}
		if (y1 == y2)	/*需多写一个中间点*/
			NCPutPixel(x1, y1, c);
	}
	return NO_ERROR;
}

/*Bresenham算法画圆*/
long GDIcircle(long cx, long cy, long r, DWORD c)
{
	long x, y, d;

	if (!r)
		return GDI_ERR_SIZE;
	y = r;
	d = (3 - y) << 1;
	for (x = 0; x <= y; x++)
	{
		GDIPutPixel(cx + x, cy + y, c);
		GDIPutPixel(cx + x, cy - y, c);
		GDIPutPixel(cx - x, cy + y, c);
		GDIPutPixel(cx - x, cy - y, c);
		GDIPutPixel(cx + y, cy + x, c);
		GDIPutPixel(cx + y, cy - x, c);
		GDIPutPixel(cx - y, cy + x, c);
		GDIPutPixel(cx - y, cy - x, c);
		if (d < 0)
			d += (x << 2) + 6;
		else
			d += ((x - y--) << 2) + 10;
	}
	return NO_ERROR;
}

#define HZ_COUNT	8178

/*绘制汉字*/
long GDIDrawHz(long x, long y, DWORD hz, DWORD c)
{
	void *tmpvm;
	long i, j, HzWidth;
	const WORD *p;

	HzWidth = GDICharWidth * 2;
	if (x <= -HzWidth || x >= (long)GDIwidth || y <= -(long)GDICharHeight || y >= (long)GDIheight)
		return GDI_ERR_LOCATION;
	hz = ((hz & 0xFF) - 161) * 94 + ((hz >> 8) & 0xFF) - 161;
	if (hz >= HZ_COUNT)
		return NO_ERROR;
	p = (WORD*)(GDIfont + hz * GDICharHeight * 2);
	switch (GDIPixBits)
	{
	case 15:
		c = DW2RGB15(c);
		break;
	case 16:
		c = DW2RGB16(c);
		break;
	}
	switch (GDIPixBits)
	{
	case 15:
	case 16:
		for (j = GDICharHeight - 1; j >= 0; j--, p++, y++)
		{
			if ((DWORD)y >= GDIheight)
				continue;
			tmpvm = GDIvm + (x + y * GDIwidth) * sizeof(WORD);
			for (i = HzWidth - 1; i >= 0; i--, x++, tmpvm += sizeof(WORD))
				if ((DWORD)x < GDIwidth && ((*p >> i) & 1u))
					*((WORD*)tmpvm) = c;
			x -= HzWidth;
		}
		break;
	case 24:
		for (j = GDICharHeight - 1; j >= 0; j--, p++, y++)
		{
			if ((DWORD)y >= GDIheight)
				continue;
			tmpvm = GDIvm + (x + y * GDIwidth) * sizeof(RGB24);
			for (i = HzWidth - 1; i >= 0; i--, x++, tmpvm += sizeof(RGB24))
				if ((DWORD)x < GDIwidth && ((*p >> i) & 1u))
					*((RGB24*)tmpvm) = *(RGB24*)&c;
			x -= HzWidth;
		}
		break;
	case 32:
		for (j = GDICharHeight - 1; j >= 0; j--, p++, y++)
		{
			if ((DWORD)y >= GDIheight)
				continue;
			tmpvm = GDIvm + (x + y * GDIwidth) * sizeof(DWORD);
			for (i = HzWidth - 1; i >= 0; i--, x++, tmpvm += sizeof(DWORD))
				if ((DWORD)x < GDIwidth && ((*p >> i) & 1u))
					*((DWORD*)tmpvm) = c;
			x -= HzWidth;
		}
		break;
	}
	return NO_ERROR;
}

/*绘制ASCII字符*/
long GDIDrawAscii(long x, long y, DWORD ch, DWORD c)
{
	void *tmpvm;
	long i, j;
	const BYTE *p;

	if (x <= -(long)GDICharWidth || x >= (long)GDIwidth || y <= -(long)GDICharHeight || y >= (long)GDIheight)
		return GDI_ERR_LOCATION;
	p = GDIfont + HZ_COUNT * GDICharHeight * 2 + (ch & 0xFF) * GDICharHeight;
	switch (GDIPixBits)
	{
	case 15:
		c = DW2RGB15(c);
		break;
	case 16:
		c = DW2RGB16(c);
		break;
	}
	switch (GDIPixBits)
	{
	case 15:
	case 16:
		for (j = GDICharHeight - 1; j >= 0; j--, p++, y++)
		{
			if ((DWORD)y >= GDIheight)
				continue;
			tmpvm = GDIvm + (x + y * GDIwidth) * sizeof(WORD);
			for (i = GDICharWidth - 1; i >= 0; i--, x++, tmpvm += sizeof(WORD))
				if ((DWORD)x < GDIwidth && ((*p >> i) & 1u))
					*((WORD*)tmpvm) = c;
			x -= GDICharWidth;
		}
		break;
	case 24:
		for (j = GDICharHeight - 1; j >= 0; j--, p++, y++)
		{
			if ((DWORD)y >= GDIheight)
				continue;
			tmpvm = GDIvm + (x + y * GDIwidth) * sizeof(RGB24);
			for (i = GDICharWidth - 1; i >= 0; i--, x++, tmpvm += sizeof(RGB24))
				if ((DWORD)x < GDIwidth && ((*p >> i) & 1u))
					*((RGB24*)tmpvm) = *(RGB24*)&c;
			x -= GDICharWidth;
		}
		break;
	case 32:
		for (j = GDICharHeight - 1; j >= 0; j--, p++, y++)
		{
			if ((DWORD)y >= GDIheight)
				continue;
			tmpvm = GDIvm + (x + y * GDIwidth) * sizeof(DWORD);
			for (i = GDICharWidth - 1; i >= 0; i--, x++, tmpvm += sizeof(DWORD))
				if ((DWORD)x < GDIwidth && ((*p >> i) & 1u))
					*((DWORD*)tmpvm) = c;
			x -= GDICharWidth;
		}
		break;
	}
	return NO_ERROR;
}

/*绘制字符串*/
long GDIDrawStr(long x, long y, const char *str, DWORD c)
{
	DWORD hzf;	/*汉字内码首字符标志*/

	for (hzf = 0; *str && x < (long)GDIwidth; str++)
	{
		if ((BYTE)(*str) > 160)
		{
			if (hzf)	/*显示汉字*/
			{
				GDIDrawHz(x, y, ((BYTE)(*str) << 8) | hzf, c);
				x += GDICharWidth * 2;
				hzf = 0;
			}
			else
				hzf = (BYTE)(*str);
		}
		else
		{
			if (hzf)	/*有未显示的ASCII*/
			{
				GDIDrawAscii(x, y, hzf, c);
				x += GDICharWidth;
				hzf = 0;
			}
			GDIDrawAscii(x, y, (BYTE)(*str), c);	/*显示当前ASCII*/
			x += GDICharWidth;
		}
	}
	return NO_ERROR;
}
