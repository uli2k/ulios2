/*	guilib.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面功能库
	最后修改日期：2011-05-22
*/

#include "gui.h"

void *GDIvm;
DWORD GDIwidth, GDIheight, GDIPixBits, GDImode;

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

/*贴图*/
void GDIPutImage(long x, long y, DWORD *img, long w, long h)
{
	long memw;

	if (x >= (long)GDIwidth || x + w <= 0 || y >= (long)GDIheight || y + h <= 0)
		return;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return;	/*非法尺寸*/
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
}

/*截图*/
void GDIGetImage(long x, long y, DWORD *img, long w, long h)
{
	long memw;

	if (x >= (long)GDIwidth || x + w <= 0 || y >= (long)GDIheight || y + h <= 0)
		return;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return;	/*非法尺寸*/
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
}

/*去背景色贴图*/
void GDIPutBCImage(long x, long y, DWORD *img, long w, long h, DWORD bc)
{
	long memw;

	if (x >= (long)GDIwidth || x + w <= 0 || y >= (long)GDIheight || y + h <= 0)
		return;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return;	/*非法尺寸*/
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
}

/*GUI矩形块贴图*/
void GuiPutImage(long x, long y, DWORD *img, long memw, long w, long h)
{
	switch (GDIPixBits)
	{
	case 15:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + GDIwidth * y; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Dw2Rgb15(tmpvm, img, w);
		}
		break;
	case 16:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + GDIwidth * y; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Dw2Rgb16(tmpvm, img, w);
		}
		break;
	case 24:
		{
			RGB24 *tmpvm;
			for (tmpvm = (RGB24*)GDIvm + x + GDIwidth * y; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Dw2Rgb24(tmpvm, img, w);
		}
		break;
	case 32:
		{
			DWORD *tmpvm;
			for (tmpvm = (DWORD*)GDIvm + x + GDIwidth * y; h > 0; tmpvm += GDIwidth, img += memw, h--)
				memcpy32(tmpvm, img, w);
		}
		break;
	}
}
