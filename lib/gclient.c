/*	gclient.c for ulios graphical user interface
作者：孙亮
功能：GUI客户端功能库实现
最后修改日期：2011-08-15
*/

#include "gclient.h"
#include "gcres.h"
#include "malloc.h"
#include "../fs/fsapi.h"

/**********绘图功能实现**********/

const BYTE *GCfont;
DWORD GCwidth, GCheight;
DWORD GCCharWidth, GCCharHeight;

/*初始化GC库*/
long GCinit()
{
	long res;

	if ((res = GUIGetGinfo(&GCwidth, &GCheight)) != NO_ERROR)
		return res;
	if (GCfont == NULL && (res = FNTGetFont(&GCfont, &GCCharWidth, &GCCharHeight)) != NO_ERROR)
		return res;
	return NO_ERROR;
}

/*撤销GC库*/
void GCrelease()
{
	DWORD data[MSG_DATA_LEN];

	if (GCfont)
	{
		KUnmapProcAddr((void*)GCfont, data);
		GCfont = NULL;
	}
}

/*设置绘图区域的参数和缓存,第一次设置无根绘图区时先清空uda->vbuf*/
long GCSetArea(UDI_AREA *uda, DWORD width, DWORD height, const UDI_AREA *par, long x, long y)
{
	if (par && par != uda)	/*具有根缓存*/
	{
		if (x < 0 || x + (long)width > (long)par->width || y < 0 || y + (long)height > (long)par->height)
			return GC_ERR_LOCATION;
		uda->vbuf = par->vbuf + x + y * par->root->width;
		uda->root = par->root;
	}
	else	/*重新申请缓存*/
	{
		DWORD *p;
		if ((uda->vbuf = (DWORD*)realloc(uda->vbuf, width * height * sizeof(DWORD))) == NULL)
			return GC_ERR_OUT_OF_MEM;
		for (p = uda->vbuf + width * height - 1; p >= uda->vbuf; p -= 512)	/*填充GUI共享区域,重要,因系统可能升级到2K页面,故地址递减值为2KB*/
			*p = 0;
		*uda->vbuf = 0;
		uda->root = uda;
	}
	uda->width = width;
	uda->height = height;
	return NO_ERROR;
}

/*回收绘图区域缓存*/
void GCFreeArea(UDI_AREA *uda)
{
	if (uda->root == uda && uda->vbuf)
	{
		free(uda->vbuf);
		uda->vbuf = NULL;
	}
}

/*画点*/
long GCPutPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD c)
{
	if (x >= uda->width || y >= uda->height)
		return GC_ERR_LOCATION;	/*位置越界*/
	uda->vbuf[x + y * uda->root->width] = c;
	return NO_ERROR;
}

/*取点*/
long GCGetPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD *c)
{
	if (x >= uda->width || y >= uda->height)
		return GC_ERR_LOCATION;	/*位置越界*/
	*c = uda->vbuf[x + y * uda->root->width];
	return NO_ERROR;
}

/*贴图*/
long GCPutImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h)
{
	long memw;
	DWORD *tmpvm, vbufw;

	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*非法尺寸*/
	memw = w;
	if (x < 0)
	{
		img -= x;
		w += x;
		x = 0;
	}
	if (w > (long)uda->width - x)
		w = (long)uda->width - x;
	if (y < 0)
	{
		img -= y * memw;
		h += y;
		y = 0;
	}
	if (h > (long)uda->height - y)
		h = (long)uda->height - y;

	vbufw = uda->root->width;
	for (tmpvm = uda->vbuf + x + y * vbufw; h > 0; tmpvm += vbufw, img += memw, h--)
		memcpy32(tmpvm, img, w);
	return NO_ERROR;
}

/*去背景色复制图像数据*/
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
long GCPutBCImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h, DWORD bc)
{
	long memw;
	DWORD *tmpvm, vbufw;

	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*非法尺寸*/
	memw = w;
	if (x < 0)
	{
		img -= x;
		w += x;
		x = 0;
	}
	if (w > (long)uda->width - x)
		w = (long)uda->width - x;
	if (y < 0)
	{
		img -= y * memw;
		h += y;
		y = 0;
	}
	if (h > (long)uda->height - y)
		h = (long)uda->height - y;

	vbufw = uda->root->width;
	for (tmpvm = uda->vbuf + x + y * vbufw; h > 0; tmpvm += vbufw, img += memw, h--)
		BCDw2Rgb32(tmpvm, img, w, bc);
	return NO_ERROR;
}

/*截图*/
long GCGetImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h)
{
	long memw;
	DWORD *tmpvm, vbufw;

	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*非法尺寸*/
	memw = w;
	if (x < 0)
	{
		img -= x;
		w += x;
		x = 0;
	}
	if (w > (long)uda->width - x)
		w = (long)uda->width - x;
	if (y < 0)
	{
		img -= y * memw;
		h += y;
		y = 0;
	}
	if (h > (long)uda->height - y)
		h = (long)uda->height - y;

	vbufw = uda->root->width;
	for (tmpvm = uda->vbuf + x + y * vbufw; h > 0; tmpvm += vbufw, img += memw, h--)
		memcpy32(img, tmpvm, w);
	return NO_ERROR;
}

/*填充矩形*/
long GCFillRect(UDI_AREA *uda, long x, long y, long w, long h, DWORD c)
{
	DWORD *tmpvm, vbufw;

	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*非法尺寸*/
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (w > (long)uda->width - x)
		w = (long)uda->width - x;
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (h > (long)uda->height - y)
		h = (long)uda->height - y;

	vbufw = uda->root->width;
	for (tmpvm = uda->vbuf + x + y * vbufw; h > 0; tmpvm += vbufw, h--)
		memset32(tmpvm, c, w);
	return NO_ERROR;
}

#define abs(v) ((v) >= 0 ? (v) : -(v))

/*无越界判断画点*/
static inline void NCPutPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD c)
{
	uda->vbuf[x + y * uda->root->width] = c;
}

#define CS_LEFT		1
#define CS_RIGHT	2
#define CS_TOP		4
#define CS_BOTTOM	8

/*Cohen-Sutherland裁剪算法编码*/
static DWORD CsEncode(UDI_AREA *uda, long x, long y)
{
	DWORD mask;

	mask = 0;
	if (x < 0)
		mask |= CS_LEFT;
	else if (x >= (long)uda->width)
		mask |= CS_RIGHT;
	if (y < 0)
		mask |= CS_TOP;
	else if (y >= (long)uda->height)
		mask |= CS_BOTTOM;
	return mask;
}

#define F2L_SCALE	0x10000

/*Cohen-Sutherland算法裁剪Bresenham改进算法画线*/
long GCDrawLine(UDI_AREA *uda, long x1, long y1, long x2, long y2, DWORD c)
{
	DWORD mask, mask1, mask2;
	long dx, dy, dx2, dy2;
	long e, xinc, yinc, half;

	/*裁剪*/
	mask1 = CsEncode(uda, x1, y1);
	mask2 = CsEncode(uda, x2, y2);
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
				dx = (long)uda->width - 1;
				dy = y1 - ((x1 + 1 - (long)uda->width) * ydivx / F2L_SCALE);
			}
			if (mask & CS_TOP)
			{
				dy = 0;
				dx = x1 - (y1 * xdivy / F2L_SCALE);
			}
			else if (mask & CS_BOTTOM)
			{
				dy = (long)uda->height - 1;
				dx = x1 - ((y1 + 1 - (long)uda->height) * xdivy / F2L_SCALE);
			}
			if (mask == mask1)
			{
				x1 = dx;
				y1 = dy;
				mask1 = CsEncode(uda, dx, dy);
			}
			else
			{
				x2 = dx;
				y2 = dy;
				mask2 = CsEncode(uda, dx, dy);
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
	if (dx >= dy)
	{
		e = dy2 - dx;
		half = (dx + 1) >> 1;
		while (half--)
		{
			NCPutPixel(uda, x1, y1, c);
			NCPutPixel(uda, x2, y2, c);
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
			NCPutPixel(uda, x1, y1, c);
	}
	else
	{
		e = dx2 - dy;
		half = (dy + 1) >> 1;
		while (half--)
		{
			NCPutPixel(uda, x1, y1, c);
			NCPutPixel(uda, x2, y2, c);
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
			NCPutPixel(uda, x1, y1, c);
	}
	return NO_ERROR;
}

/*Bresenham算法画圆*/
long GCcircle(UDI_AREA *uda, long cx, long cy, long r, DWORD c)
{
	long x, y, d;

	if (!r)
		return GC_ERR_AREASIZE;
	y = r;
	d = (3 - y) << 1;
	for (x = 0; x <= y; x++)
	{
		GCPutPixel(uda, cx + x, cy + y, c);
		GCPutPixel(uda, cx + x, cy - y, c);
		GCPutPixel(uda, cx - x, cy + y, c);
		GCPutPixel(uda, cx - x, cy - y, c);
		GCPutPixel(uda, cx + y, cy + x, c);
		GCPutPixel(uda, cx + y, cy - x, c);
		GCPutPixel(uda, cx - y, cy + x, c);
		GCPutPixel(uda, cx - y, cy - x, c);
		if (d < 0)
			d += (x << 2) + 6;
		else
			d += ((x - y--) << 2) + 10;
	}
	return NO_ERROR;
}

#define HZ_COUNT	8178

/*绘制汉字*/
long GCDrawHz(UDI_AREA *uda, long x, long y, DWORD hz, DWORD c)
{
	DWORD *tmpvm, vbufw;
	long i, j, HzWidth;
	const WORD *p;

	HzWidth = GCCharWidth * 2;
	if (x <= -HzWidth || x >= (long)uda->width || y <= -(long)GCCharHeight || y >= (long)uda->height)
		return GC_ERR_LOCATION;
	hz = ((hz & 0xFF) - 161) * 94 + ((hz >> 8) & 0xFF) - 161;
	if (hz >= HZ_COUNT)
		return NO_ERROR;
	p = (WORD*)(GCfont + hz * GCCharHeight * 2);
	vbufw = uda->root->width;
	for (j = GCCharHeight - 1; j >= 0; j--, p++, y++)
	{
		if ((DWORD)y >= uda->height)
			continue;
		tmpvm = uda->vbuf + x + y * vbufw;
		for (i = HzWidth - 1; i >= 0; i--, x++, tmpvm++)
			if ((DWORD)x < uda->width && ((*p >> i) & 1u))
				*tmpvm = c;
		x -= HzWidth;
	}
	return NO_ERROR;
}

/*绘制ASCII字符*/
long GCDrawAscii(UDI_AREA *uda, long x, long y, DWORD ch, DWORD c)
{
	DWORD *tmpvm, vbufw;
	long i, j;
	const BYTE *p;

	if (x <= -(long)GCCharWidth || x >= (long)uda->width || y <= -(long)GCCharHeight || y >= (long)uda->height)
		return GC_ERR_LOCATION;
	p = GCfont + HZ_COUNT * GCCharHeight * 2 + (ch & 0xFF) * GCCharHeight;
	vbufw = uda->root->width;
	for (j = GCCharHeight - 1; j >= 0; j--, p++, y++)
	{
		if ((DWORD)y >= uda->height)
			continue;
		tmpvm = uda->vbuf + x + y * vbufw;
		for (i = GCCharWidth - 1; i >= 0; i--, x++, tmpvm++)
			if ((DWORD)x < uda->width && ((*p >> i) & 1u))
				*tmpvm = c;
		x -= GCCharWidth;
	}
	return NO_ERROR;
}

/*绘制字符串*/
long GCDrawStr(UDI_AREA *uda, long x, long y, const char *str, DWORD c)
{
	DWORD hzf;	/*汉字内码首字符标志*/

	for (hzf = 0; *str && x < (long)uda->width; str++)
	{
		if ((BYTE)(*str) > 160)
		{
			if (hzf)	/*显示汉字*/
			{
				GCDrawHz(uda, x, y, ((BYTE)(*str) << 8) | hzf, c);
				x += GCCharWidth * 2;
				hzf = 0;
			}
			else
				hzf = (BYTE)(*str);
		}
		else
		{
			if (hzf)	/*有未显示的ASCII*/
			{
				GCDrawAscii(uda, x, y, hzf, c);
				x += GCCharWidth;
				hzf = 0;
			}
			GCDrawAscii(uda, x, y, (BYTE)(*str), c);	/*显示当前ASCII*/
			x += GCCharWidth;
		}
	}
	return NO_ERROR;
}

/*加载BMP图像文件*/
long GCLoadBmp(char *path, DWORD *buf, DWORD len, DWORD *width, DWORD *height)
{
	BYTE BmpHead[32];
	DWORD bmpw, bmph;
	long file;

	if ((file = FSopen(path, FS_OPEN_READ)) < 0)	/*读取BMP文件*/
		return file;
	if (FSread(file, &BmpHead[2], 30) < 30 || *((WORD*)&BmpHead[2]) != 0x4D42 || *((WORD*)&BmpHead[30]) != 24)	/*保证32位对齐访问*/
	{
		FSclose(file);
		return -1;
	}
	bmpw = *((DWORD*)&BmpHead[20]);
	bmph = *((DWORD*)&BmpHead[24]);
	if (width)
		*width = bmpw;
	if (height)
		*height = bmph;
	if (bmpw * bmph > len)
	{
		FSclose(file);
		return NO_ERROR;
	}
	FSseek(file, 54, FS_SEEK_SET);
	len = (bmpw * 3 + 3) & 0xFFFFFFFC;
	for (buf += bmpw * (bmph - 1); bmph > 0; bmph--, buf -= bmpw)
	{
		BYTE *src, *dst;

		if (FSread(file, buf, len) < len)
		{
			FSclose(file);
			return -1;
		}
		src = (BYTE*)buf + bmpw * 3;
		dst = (BYTE*)buf + bmpw * 4;
		while (src > (BYTE*)buf)
		{
			*(--dst) = 0xFF;
			*(--dst) = *(--src);
			*(--dst) = *(--src);
			*(--dst) = *(--src);
		}
	}
	FSclose(file);
	return NO_ERROR;
}

/**********控件绘图库色彩定义**********/

#define COL_WND_FLAT		0xCCCCCC	// 窗口平面颜色
#define COL_WND_BORDER		0x7B858E	// 窗口边框色
#define COL_CAP_GRADDARK	0x589FCE	// 标题渐变色暗区
#define COL_CAP_GRADLIGHT	0xE1EEF6	// 标题渐变色亮区
#define COL_CAP_NOFCDARK	0xBFBFBF	// 无焦标题渐变色暗区
#define COL_CAP_NOFCLIGHT	0xFBFBFB	// 无焦标题渐变色暗区

#define COL_BTN_BORDER		0x848A8A	// 按钮边框色
#define COL_BTN_GRADDARK	0xA8ABAF	// 按钮渐变色暗区
#define COL_BTN_GRADLIGHT	0xEFEFF6	// 按钮渐变色亮区
#define COL_BTN_TEXT		0x000000	// 按钮文字
#define COL_BTN_CLICK_GRADDARK	0xA8ABAF	// 按钮按下渐变色暗区
#define COL_BTN_CLICK_GRADLIGHT	0xEFEFF6	// 按钮按下渐变色亮区
#define COL_BTN_CLICK_TEXT		0x000000	// 按钮按下文字
#define COL_BTN_HOVER_GRADDARK	0x818488	// 按钮鼠标经过渐变色暗区
#define COL_BTN_HOVER_GRADLIGHT	0xF9F9FC	// 按钮鼠标经过渐变色亮区
#define COL_BTN_HOVER_TEXT		0x000000	// 按钮鼠标经过文字
#define COL_BTN_DISABLED_GRADDARK	0xA8ABAF	// 按钮失效渐变色暗区
#define COL_BTN_DISABLED_GRADLIGHT	0xEFEFF6	// 按钮失效渐变色亮区
#define COL_BTN_DISABLED_TEXT		0x6E6A6A	// 按钮失效渐变色亮区

#define COL_TEXT_DARK		0x001C30	// 文本暗色
#define COL_TEXT_LIGHT		0x008080	// 文本亮色

/**********控件绘图库**********/

/*绘制渐变色块*/
static void FillGradRect(UDI_AREA *uda, long x, long y, long w, long h, DWORD c1, DWORD c2)
{
	long i;
	long BaseR, BaseG, BaseB, StepR, StepG, StepB;

	BaseR = (c1 & 0xFF0000) >> 16;
	BaseG = (c1 & 0xFF00) >> 8;
	BaseB = (c1 & 0xFF);
	StepR = ((c2 & 0xFF0000) >> 16) - BaseR;
	StepG = ((c2 & 0xFF00) >> 8) - BaseG;
	StepB = ((c2 & 0xFF)) - BaseB;
	for (i = 0; i < h; i++)
		GCFillRect(uda, x, y + i, w, 1, (((BaseR + StepR * i / h) & 0xFF) << 16) | (((BaseG + StepG * i / h) & 0xFF) << 8) | ((BaseB + StepB * i / h) & 0xFF));
}

/*按钮绘制*/
static void DrawButton(UDI_AREA *uda, long x, long y, long w, long h, DWORD c1, DWORD c2, DWORD bc)
{
	FillGradRect(uda, x + 1, y + 1, w - 2, h - 2, c1, c2);
	/*绘制圆角矩形*/
	GCFillRect(uda, x + 2, y, w - 4, 1, bc);	/*上边框*/
	GCFillRect(uda, x + 2, y + h - 1, w - 4, 1, bc);	/*下边框*/
	GCFillRect(uda, x, y + 2, 1, h - 4, bc);	/*左边框*/
	GCFillRect(uda, x + w - 1, y + 2, 1, h - 4, bc);	/*右边框*/
	GCPutPixel(uda, x + 1, y + 1, bc);	/*左上角*/
	GCPutPixel(uda, x + w - 2, y + 1, bc);	/*右上角*/
	GCPutPixel(uda, x + 1, y + h - 2, bc);	/*左下角*/
	GCPutPixel(uda, x + w - 2, y + h - 2, bc);	/*右下角*/
}

/**********控件基类**********/

/*初始化CTRL_GOBJ结构*/
long GCGobjInit(CTRL_GOBJ *gobj, const CTRL_ARGS *args, MSGPROC MsgProc, DRAWPROC DrawProc, DWORD pid, CTRL_GOBJ *ParGobj)
{
	long res;

	gobj->uda.vbuf = NULL;
	if ((res = GCSetArea(&gobj->uda, args->width, args->height, &ParGobj->uda, args->x, args->y)) != NO_ERROR)	/*分配绘图区域*/
		return res;
	memcpy32(&gobj->x, &args->x, 4);	// 复制需要的参数
	if (gobj->MsgProc == NULL)
		gobj->MsgProc = MsgProc;
	gobj->DrawProc = DrawProc;
	if ((res = GUIcreate(pid, (DWORD)gobj, args->x, args->y, args->width, args->height, ParGobj == NULL ? gobj->uda.vbuf : NULL)) != NO_ERROR)
	{
		GCFreeArea(&gobj->uda);
		return res;
	}
	if (ParGobj)
	{
		CTRL_GOBJ *CurGobj;
		CurGobj = ParGobj->chl;
		if (CurGobj)
		{
			while (CurGobj->nxt)	/*为了简化绘制窗体操作,采用后插法,连接兄弟节点*/
				CurGobj = CurGobj->nxt;
			CurGobj->nxt = gobj;
		}
		else
			ParGobj->chl = gobj;
		gobj->pre = CurGobj;
	}
	else
		gobj->pre = NULL;
	gobj->nxt = NULL;
	gobj->par = ParGobj;
	gobj->chl = NULL;
	return NO_ERROR;
}

/*递归释放窗体树*/
static void FreeGobjList(CTRL_GOBJ *gobj)
{
	CTRL_GOBJ *CurGobj;

	CurGobj = gobj->chl;
	while (CurGobj)
	{
		CTRL_GOBJ *TmpGobj;
		TmpGobj = CurGobj->nxt;
		FreeGobjList(CurGobj);
		CurGobj = TmpGobj;
	}
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];
		ptid.ProcID = INVALID;
		data[MSG_API_ID] = MSG_ATTR_GUI | GM_DESTROY;
		data[GUIMSG_GOBJ_ID] = (DWORD)gobj;
		data[MSG_RES_ID] = NO_ERROR;
		gobj->MsgProc(ptid, data);	/*调用窗体的销毁消息处理函数*/
	}
	GCFreeArea(&gobj->uda);
	free(gobj);
}

/*删除窗体树*/
void GCGobjDelete(CTRL_GOBJ *gobj)
{
	CTRL_GOBJ *ParGobj;

	ParGobj = gobj->par;
	if (ParGobj)	// 解链
	{
		if (ParGobj->chl == gobj)
			ParGobj->chl = gobj->nxt;
		if (gobj->pre)
			gobj->pre->nxt = gobj->nxt;
		if (gobj->nxt)
			gobj->nxt->pre = gobj->pre;
	}
	FreeGobjList(gobj);
}

/*递归绘制窗体树*/
static void DrawGobjList(CTRL_GOBJ *gobj)
{
	gobj->DrawProc(gobj);
	gobj = gobj->chl;
	while (gobj)
	{
		DrawGobjList(gobj);
		gobj = gobj->nxt;
	}
}

/*绘制窗体树*/
void GCGobjDraw(CTRL_GOBJ *gobj)
{
	DrawGobjList(gobj);
	GUIpaint(gobj->gid, 0, 0, gobj->uda.width, gobj->uda.height);	/*绘制完后提交*/
}

/*GUI客户端消息调度*/
long GCDispatchMsg(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_GOBJ *gobj;
	long res;

	if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) != MSG_ATTR_GUI)	/*抽取合法的GUI消息*/
		return GC_ERR_INVALID_GUIMSG;
	if (data[MSG_RES_ID] != NO_ERROR)	/*分离出错的GUI消息*/
		return data[MSG_RES_ID];
	gobj = (CTRL_GOBJ*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_ATTR_ID] & MSG_API_MASK)
	{
	case GM_CREATE:	/*获取新窗体的GUI服务端对象ID*/
		gobj->gid = data[5];
		break;
	}
	res = gobj->MsgProc(ptid, data);	/*调用窗体的消息处理函数*/
	switch (data[MSG_ATTR_ID] & MSG_API_MASK)
	{
	case GM_CREATE:	/*绘制新创建的Gobj根节点*/
		gobj->DrawProc(gobj);
		GUIpaint(gobj->gid, 0, 0, gobj->uda.width, gobj->uda.height);
		break;
	case GM_DESTROY:	/*销毁窗体*/
		GCGobjDelete(gobj);
		break;
	}
	return res;
}

/*移动简单窗体*/
void GCGobjMove(CTRL_GOBJ *gobj, long x, long y)
{
	if (GCSetArea(&gobj->uda, gobj->uda.width, gobj->uda.height, &gobj->par->uda, x, y) != NO_ERROR)	/*设置窗体区域*/
		return;
	gobj->x = x;
	gobj->y = y;
	gobj->DrawProc(gobj);
	GUImove(gobj->gid, gobj->x, gobj->y);
}

/*设置简单窗体位置大小*/
void GCGobjSetSize(CTRL_GOBJ *gobj, long x, long y, DWORD width, DWORD height)
{
	if (GCSetArea(&gobj->uda, width, height, &gobj->par->uda, x, y) != NO_ERROR)	/*设置窗体区域*/
		return;
	gobj->x = x;
	gobj->y = y;
	gobj->DrawProc(gobj);	/*重绘窗体*/
	GUIsize(gobj->gid, gobj->uda.root == &gobj->uda ? gobj->uda.vbuf : NULL, gobj->x, gobj->y, gobj->uda.width, gobj->uda.height);	/*重设窗体大小*/
}

/**********桌面**********/

/*创建桌面*/
long GCDskCreate(CTRL_DSK **dsk, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj)
{
	CTRL_DSK *NewDsk;
	long res;

	if ((NewDsk = (CTRL_DSK*)malloc(sizeof(CTRL_DSK))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewDsk->obj, args, GCDskDefMsgProc, (DRAWPROC)GCDskDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewDsk);
		return res;
	}
	NewDsk->obj.type = GC_CTRL_TYPE_DESKTOP;
	if (dsk)
		*dsk = NewDsk;
	return NO_ERROR;
}

/*桌面消息处理*/
long GCDskDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	return NO_ERROR;
}

/*桌面绘制处理*/
void GCDskDefDrawProc(CTRL_DSK *dsk)
{
}

/**********窗口**********/

#define WND_CAP_HEIGHT		20	/*窗口标题栏高度*/
#define WND_BORDER_WIDTH	1	/*窗口边框宽度*/
#define WND_CAPBTN_SIZE		16	/*标题栏按钮大小*/
#define WND_SIZEBTN_SIZE	14	/*缩放按钮大小*/
#define WND_MIN_WIDTH		128	/*窗口最小尺寸*/
#define WND_MIN_HEIGHT		WND_CAP_HEIGHT

/*创建窗口*/
long GCWndCreate(CTRL_WND **wnd, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *caption)
{
	CTRL_WND *NewWnd;
	long res;

	if ((NewWnd = (CTRL_WND*)malloc(sizeof(CTRL_WND))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewWnd->obj, args, GCWndDefMsgProc, (DRAWPROC)GCWndDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewWnd);
		return res;
	}
	NewWnd->obj.type = GC_CTRL_TYPE_WINDOW;
	NewWnd->obj.style |= WND_STATE_TOP;
	NewWnd->size = NewWnd->min = NewWnd->max = NewWnd->close = NULL;
	if (NewWnd->obj.style & WND_STYLE_CAPTION)	/*有标题栏*/
	{
		if (NewWnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width - WND_BORDER_WIDTH * 2, NewWnd->obj.uda.height - WND_CAP_HEIGHT - WND_BORDER_WIDTH, &NewWnd->obj.uda, WND_BORDER_WIDTH, WND_CAP_HEIGHT);	/*创建客户区*/
		else	/*无边框*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width, NewWnd->obj.uda.height - WND_CAP_HEIGHT, &NewWnd->obj.uda, 0, WND_CAP_HEIGHT);	/*创建客户区*/
	}
	else	/*无标题栏*/
	{
		if (NewWnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width - WND_BORDER_WIDTH * 2, NewWnd->obj.uda.height - WND_BORDER_WIDTH * 2, &NewWnd->obj.uda, WND_BORDER_WIDTH, WND_BORDER_WIDTH);	/*创建客户区*/
		else	/*无边框*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width, NewWnd->obj.uda.height, &NewWnd->obj.uda, 0, 0);	/*创建客户区*/
	}
	if (caption)
	{
		strncpy(NewWnd->caption, caption, WND_CAP_LEN - 1);
		NewWnd->caption[WND_CAP_LEN - 1] = 0;
	}
	else
		NewWnd->caption[0] = 0;
	NewWnd->MinWidth = WND_MIN_WIDTH;
	NewWnd->MinHeight = WND_MIN_HEIGHT;
	NewWnd->MaxWidth = GCwidth;
	NewWnd->MaxHeight = GCheight;
	NewWnd->x0 = NewWnd->obj.x;
	NewWnd->y0 = NewWnd->obj.y;
	NewWnd->width0 = NewWnd->obj.uda.width;
	NewWnd->height0 = NewWnd->obj.uda.height;
	if (wnd)
		*wnd = NewWnd;
	return NO_ERROR;
}

/*窗口关闭按钮处理函数*/
static void WndCloseBtnProc(CTRL_BTN *btn)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];

	ptid.ProcID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_CLOSE;
	data[GUIMSG_GOBJ_ID] = (DWORD)btn->obj.par;
	data[MSG_RES_ID] = NO_ERROR;
	btn->obj.par->MsgProc(ptid, data);
}

/*窗口最大化正常化切换*/
static void WndMaxOrNormal(CTRL_WND *wnd)
{
	if (wnd->obj.uda.width == wnd->MaxWidth && wnd->obj.uda.height == wnd->MaxHeight)
		GCWndSetSize(wnd, wnd->x0, wnd->y0, wnd->width0, wnd->height0);	/*正常化*/
	else
		GCWndSetSize(wnd, 0, 0, wnd->MaxWidth, wnd->MaxHeight);	/*最大化*/
}

/*窗口最小化正常化切换*/
static void WndMinOrNormal(CTRL_WND *wnd)
{
	if (wnd->obj.uda.width == wnd->MinWidth && wnd->obj.uda.height == wnd->MinHeight)
		GCWndSetSize(wnd, wnd->x0, wnd->y0, wnd->width0, wnd->height0);	/*正常化*/
	else
		GCWndSetSize(wnd, wnd->obj.x, wnd->obj.y, wnd->MinWidth, wnd->MinHeight);	/*最小化*/
}

/*窗口最大化按钮处理函数*/
static void WndMaxBtnProc(CTRL_BTN *btn)
{
	WndMaxOrNormal((CTRL_WND*)btn->obj.par);
}

/*窗口最小化按钮处理函数*/
static void WndMinBtnProc(CTRL_BTN *btn)
{
	WndMinOrNormal((CTRL_WND*)btn->obj.par);
}

/*窗口缩放按钮消息处理*/
static long WndSizeBtnProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	if ((data[MSG_API_ID] & MSG_API_MASK) == GM_LBUTTONDOWN)
		GUIdrag(((CTRL_GOBJ*)data[GUIMSG_GOBJ_ID])->par->gid, GM_DRAGMOD_SIZE);
	return GCBtnDefMsgProc(ptid, data);
}

/*窗口消息处理*/
long GCWndDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			CTRL_ARGS args;
			args.height = args.width = WND_CAPBTN_SIZE;
			args.y = (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
			args.style = 0;
			args.MsgProc = NULL;
			if (wnd->obj.style & WND_STYLE_CLOSEBTN)	/*有关闭按钮*/
			{
				args.x = (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->close, &args, wnd->obj.gid, &wnd->obj, "X", &WndCloseImg, WndCloseBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_MAXBTN)	/*有最大化按钮*/
			{
				args.x = WND_CAP_HEIGHT + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->max, &args, wnd->obj.gid, &wnd->obj, "[]", &WndMaxImg, WndMaxBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_MINBTN)	/*有最小化按钮*/
			{
				args.x = WND_CAP_HEIGHT * 2 + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->min, &args, wnd->obj.gid, &wnd->obj, "_ ", &WndMinImg, WndMinBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_SIZEBTN)	/*有缩放按钮*/
			{
				args.height = args.width = WND_SIZEBTN_SIZE;
				args.x = wnd->obj.uda.width - WND_SIZEBTN_SIZE;
				args.y = wnd->obj.uda.height - WND_SIZEBTN_SIZE;
				args.MsgProc = WndSizeBtnProc;
				GCBtnCreate(&wnd->size, &args, wnd->obj.gid, &wnd->obj, ".:", &WndSizeImg, NULL);
			}
		}
		break;
	case GM_MOVE:
		wnd->obj.x = data[1];
		wnd->obj.y = data[2];
		break;
	case GM_SIZE:
		if ((wnd->obj.uda.width != wnd->MinWidth || wnd->obj.uda.height != wnd->MinHeight) && (wnd->obj.uda.width != wnd->MaxWidth || wnd->obj.uda.height != wnd->MaxHeight))	/*记录窗口正常化时的大小*/
		{
			wnd->x0 = wnd->obj.x;
			wnd->y0 = wnd->obj.y;
			wnd->width0 = wnd->obj.uda.width;
			wnd->height0 = wnd->obj.uda.height;
		}
		break;
	case GM_SETTOP:
		if (data[1])
			wnd->obj.style |= WND_STATE_TOP;
		else
			wnd->obj.style &= (~WND_STATE_TOP);
		if (wnd->obj.style & WND_STYLE_CAPTION)	/*有标题栏*/
		{
			if (wnd->obj.style & WND_STATE_TOP)
				FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*绘制标题栏*/
			else
				FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*绘制无焦标题栏*/
			GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
			if (wnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
				GCFillRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_BORDER_WIDTH, COL_WND_BORDER);	/*上边框*/
			if (wnd->close)	/*有关闭按钮*/
				GCBtnDefDrawProc(wnd->close);
			if (wnd->max)	/*有最大化按钮*/
				GCBtnDefDrawProc(wnd->max);
			if (wnd->min)	/*有最小化按钮*/
				GCBtnDefDrawProc(wnd->min);
			GUIpaint(wnd->obj.gid, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT);
		}
		break;
	case GM_DRAG:
		switch (data[1])
		{
		case GM_DRAGMOD_MOVE:
			GUImove(wnd->obj.gid, data[2], data[3]);
			break;
		case GM_DRAGMOD_SIZE:
			GCWndSetSize(wnd, wnd->obj.x, wnd->obj.y, data[2], data[3]);
			break;
		}
		break;
	case GM_LBUTTONDOWN:
		if (wnd->obj.style & WND_STYLE_CAPTION && (data[5] >> 16) < WND_CAP_HEIGHT)	/*单击标题拖动窗口*/
			GUIdrag(wnd->obj.gid, GM_DRAGMOD_MOVE);
		if (!(wnd->obj.style & WND_STATE_TOP))
			GUISetTop(wnd->obj.gid);
		break;
	case GM_LBUTTONDBCLK:
		if ((wnd->obj.style & (WND_STYLE_CAPTION | WND_STYLE_MAXBTN)) == (WND_STYLE_CAPTION | WND_STYLE_MAXBTN) && (data[5] >> 16) < WND_CAP_HEIGHT)	/*双击标题缩放窗口*/
			WndMaxOrNormal(wnd);
		break;
	case GM_CLOSE:
		GUIdestroy(wnd->obj.gid);
		break;
	}
	return NO_ERROR;
}

/*窗口绘制处理*/
void GCWndDefDrawProc(CTRL_WND *wnd)
{
	if (wnd->obj.style & WND_STYLE_CAPTION)	/*有标题栏*/
	{
		if (wnd->obj.style & WND_STATE_TOP)
			FillGradRect(&wnd->obj.uda, 0, 0, wnd->obj.uda.width, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*绘制标题栏*/
		else
			FillGradRect(&wnd->obj.uda, 0, 0, wnd->obj.uda.width, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*绘制无焦标题栏*/
		GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
	}
	if (wnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
	{
		GCFillRect(&wnd->obj.uda, 0, 0, wnd->obj.uda.width, WND_BORDER_WIDTH, COL_WND_BORDER);	/*上边框*/
		GCFillRect(&wnd->obj.uda, 0, wnd->obj.uda.height - WND_BORDER_WIDTH, wnd->obj.uda.width, WND_BORDER_WIDTH, COL_WND_BORDER);	/*下边框*/
		GCFillRect(&wnd->obj.uda, 0, WND_BORDER_WIDTH, WND_BORDER_WIDTH, wnd->obj.uda.height - WND_BORDER_WIDTH * 2, COL_WND_BORDER);	/*左边框*/
		GCFillRect(&wnd->obj.uda, wnd->obj.uda.width - WND_BORDER_WIDTH, WND_BORDER_WIDTH, WND_BORDER_WIDTH, wnd->obj.uda.height - WND_BORDER_WIDTH * 2, COL_WND_BORDER);	/*右边框*/
	}
	GCFillRect(&wnd->client, 0, 0, wnd->client.width, wnd->client.height, COL_WND_FLAT);
}

/*设置窗口标题文本*/
void GCWndSetCaption(CTRL_WND *wnd, const char *caption)
{
	if (caption)
	{
		strncpy(wnd->caption, caption, WND_CAP_LEN - 1);
		wnd->caption[WND_CAP_LEN - 1] = 0;
	}
	else
		wnd->caption[0] = 0;
	if (wnd->obj.style & WND_STYLE_CAPTION)	/*有标题栏*/
	{
		if (wnd->obj.style & WND_STATE_TOP)
			FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*绘制标题栏*/
		else
			FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*绘制无焦标题栏*/
		GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
		if (wnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
			GCFillRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_BORDER_WIDTH, COL_WND_BORDER);	/*上边框*/
		if (wnd->close)	/*有关闭按钮*/
			GCBtnDefDrawProc(wnd->close);
		if (wnd->max)	/*有最大化按钮*/
			GCBtnDefDrawProc(wnd->max);
		if (wnd->min)	/*有最小化按钮*/
			GCBtnDefDrawProc(wnd->min);
		GUIpaint(wnd->obj.gid, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT);
	}
}

/*窗口检查最小最大尺寸*/
static void WndChkMinMax(CTRL_WND *wnd)
{
	if (wnd->MinWidth < WND_MIN_WIDTH)
		wnd->MinWidth = WND_MIN_WIDTH;
	if (wnd->MaxWidth > GCwidth)
		wnd->MaxWidth = GCwidth;
	if (wnd->MaxWidth < wnd->MinWidth)
		wnd->MaxWidth = wnd->MinWidth;
	if (wnd->MinHeight < WND_MIN_HEIGHT)
		wnd->MinHeight = WND_MIN_HEIGHT;
	if (wnd->MaxHeight > GCheight)
		wnd->MaxHeight = GCheight;
	if (wnd->MaxHeight < wnd->MinHeight)
		wnd->MaxHeight = wnd->MinHeight;
}

/*设置窗口位置大小*/
void GCWndSetSize(CTRL_WND *wnd, long x, long y, DWORD width, DWORD height)
{
	WndChkMinMax(wnd);
	if ((long)width < 0)
		width = 0;
	if ((long)height < 0)
		height = 0;
	if (width < wnd->MinWidth)	/*修正宽度*/
		width = wnd->MinWidth;
	else if (width > wnd->MaxWidth)
		width = wnd->MaxWidth;
	if (height < wnd->MinHeight)	/*修正高度*/
		height = wnd->MinHeight;
	else if (height > wnd->MaxHeight)
		height = wnd->MaxHeight;
	if (GCSetArea(&wnd->obj.uda, width, height, &wnd->obj.par->uda, x, y) != NO_ERROR)
		return;
	wnd->obj.x = x;
	wnd->obj.y = y;
	if (wnd->obj.style & WND_STYLE_CAPTION)	/*有标题栏*/
	{
		if (wnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
			GCSetArea(&wnd->client, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, wnd->obj.uda.height - WND_CAP_HEIGHT - WND_BORDER_WIDTH, &wnd->obj.uda, WND_BORDER_WIDTH, WND_CAP_HEIGHT);	/*创建客户区*/
		else	/*无边框*/
			GCSetArea(&wnd->client, wnd->obj.uda.width, wnd->obj.uda.height - WND_CAP_HEIGHT, &wnd->obj.uda, 0, WND_CAP_HEIGHT);
	}
	else	/*无标题栏*/
	{
		if (wnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
			GCSetArea(&wnd->client, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, wnd->obj.uda.height - WND_BORDER_WIDTH * 2, &wnd->obj.uda, WND_BORDER_WIDTH, WND_BORDER_WIDTH);	/*创建客户区*/
		else	/*无边框*/
			GCSetArea(&wnd->client, wnd->obj.uda.width, wnd->obj.uda.height, &wnd->obj.uda, 0, 0);
	}
	GCWndDefDrawProc(wnd);
	if (wnd->close)	/*有关闭按钮*/
	{
		GCSetArea(&wnd->close->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2, (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2);
		GCBtnDefDrawProc(wnd->close);
	}
	if (wnd->max)	/*有最大化按钮*/
	{
		GCSetArea(&wnd->max->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, WND_CAP_HEIGHT + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2, (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2);
		GCBtnDefDrawProc(wnd->max);
	}
	if (wnd->min)	/*有最小化按钮*/
	{
		GCSetArea(&wnd->min->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, WND_CAP_HEIGHT * 2 + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2, (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2);
		GCBtnDefDrawProc(wnd->min);
	}
	if (wnd->size)	/*有缩放按钮*/
	{
		wnd->size->obj.x = wnd->obj.uda.width - WND_SIZEBTN_SIZE;
		wnd->size->obj.y = wnd->obj.uda.height - WND_SIZEBTN_SIZE;
		GCSetArea(&wnd->size->obj.uda, WND_SIZEBTN_SIZE, WND_SIZEBTN_SIZE, &wnd->obj.uda, wnd->size->obj.x, wnd->size->obj.y);
		GCBtnDefDrawProc(wnd->size);
	}
	GUIsize(wnd->obj.gid, wnd->obj.uda.root == &wnd->obj.uda ? wnd->obj.uda.vbuf : NULL, wnd->obj.x, wnd->obj.y, wnd->obj.uda.width, wnd->obj.uda.height);	/*重设窗口大小*/
	if (wnd->size)	/*有缩放按钮*/
		GUImove(wnd->size->obj.gid, wnd->size->obj.x, wnd->size->obj.y);
}

/*取得窗口客户区位置*/
void GCWndGetClientLoca(CTRL_WND *wnd, long *x, long *y)
{
	DWORD PixCou;

	PixCou = wnd->client.vbuf - wnd->client.root->vbuf;
	*x = PixCou % wnd->client.root->width;
	*y = PixCou / wnd->client.root->width;
}

/**********按钮**********/

/*创建按钮*/
long GCBtnCreate(CTRL_BTN **btn, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text, UDI_IMAGE *img, void (*PressProc)(CTRL_BTN *btn))
{
	CTRL_BTN *NewBtn;
	long res;

	if ((NewBtn = (CTRL_BTN*)malloc(sizeof(CTRL_BTN))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewBtn->obj, args, GCBtnDefMsgProc, (DRAWPROC)GCBtnDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewBtn);
		return res;
	}
	NewBtn->obj.type = GC_CTRL_TYPE_BUTTON;
	if (text)
	{
		strncpy(NewBtn->text, text, BTN_TXT_LEN - 1);
		NewBtn->text[BTN_TXT_LEN - 1] = 0;
	}
	else
		NewBtn->text[0] = 0;
	NewBtn->img = img;
	NewBtn->isPressDown = FALSE;
	NewBtn->PressProc = PressProc;
	if (btn)
		*btn = NewBtn;
	return NO_ERROR;
}

/*按钮消息处理*/
long GCBtnDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_BTN *btn = (CTRL_BTN*)data[GUIMSG_GOBJ_ID];
	UDI_IMAGE *img;

	img = btn->img;
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_LBUTTONUP:	/*鼠标抬起状态*/
		if (!(btn->obj.style & BTN_STYLE_DISABLED) && btn->isPressDown)
		{
			btn->isPressDown = FALSE;
			if (btn->PressProc)
				btn->PressProc(btn);	/*执行按钮功能*/
		}	/*继续绘制按钮*/
	case GM_MOUSEENTER:	/*鼠标进入状态*/
		if (!(btn->obj.style & BTN_STYLE_DISABLED))
		{
			DrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_BTN_HOVER_GRADLIGHT, COL_BTN_HOVER_GRADDARK, COL_BTN_BORDER);	/*绘制按钮*/
			if (img)
				GCPutBCImage(&btn->obj.uda, (btn->obj.uda.width - (long)img->width) / 2, (btn->obj.uda.height - (long)img->height) / 2, img->buf, img->width, img->height, 0xFFFFFFFF);
			else
				GCDrawStr(&btn->obj.uda, (btn->obj.uda.width - (long)strlen(btn->text) * (long)GCCharWidth) / 2, (btn->obj.uda.height - (long)GCCharHeight) / 2, btn->text, COL_BTN_HOVER_TEXT);
			GUIpaint(btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		}
		break;
	case GM_MOUSELEAVE:	/*鼠标离开状态*/
		if (!(btn->obj.style & BTN_STYLE_DISABLED))
		{
			btn->isPressDown = FALSE;
			GCBtnDefDrawProc(btn);
			GUIpaint(btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		}
		break;
	case GM_LBUTTONDOWN:	/*鼠标按下状态*/
		if (!(btn->obj.style & BTN_STYLE_DISABLED))
		{
			btn->isPressDown = TRUE;
			DrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_BTN_CLICK_GRADDARK, COL_BTN_CLICK_GRADLIGHT, COL_BTN_BORDER);	/*绘制按钮*/
			if (img)
				GCPutBCImage(&btn->obj.uda, (btn->obj.uda.width - (long)img->width) / 2, (btn->obj.uda.height - (long)img->height) / 2, img->buf, img->width, img->height, 0xFFFFFFFF);
			else
				GCDrawStr(&btn->obj.uda, (btn->obj.uda.width - (long)strlen(btn->text) * (long)GCCharWidth) / 2, (btn->obj.uda.height - (long)GCCharHeight) / 2, btn->text, COL_BTN_CLICK_TEXT);
			GUIpaint(btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		}
		break;
	}
	return NO_ERROR;
}

/*按钮绘制处理*/
void GCBtnDefDrawProc(CTRL_BTN *btn)
{
	UDI_IMAGE *img;

	img = btn->img;
	if (btn->obj.style & BTN_STYLE_DISABLED)
	{
		DrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_BTN_DISABLED_GRADLIGHT, COL_BTN_DISABLED_GRADDARK, COL_BTN_BORDER);	/*绘制按钮*/
		if (img)
			GCPutBCImage(&btn->obj.uda, (btn->obj.uda.width - (long)img->width) / 2, (btn->obj.uda.height - (long)img->height) / 2, img->buf, img->width, img->height, 0xFFFFFFFF);
		else
			GCDrawStr(&btn->obj.uda, (btn->obj.uda.width - (long)strlen(btn->text) * (long)GCCharWidth) / 2, (btn->obj.uda.height - (long)GCCharHeight) / 2, btn->text, COL_BTN_DISABLED_TEXT);
	}
	else
	{
		DrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_BTN_GRADLIGHT, COL_BTN_GRADDARK, COL_BTN_BORDER);	/*绘制按钮*/
		if (img)
			GCPutBCImage(&btn->obj.uda, (btn->obj.uda.width - (long)img->width) / 2, (btn->obj.uda.height - (long)img->height) / 2, img->buf, img->width, img->height, 0xFFFFFFFF);
		else
			GCDrawStr(&btn->obj.uda, (btn->obj.uda.width - (long)strlen(btn->text) * (long)GCCharWidth) / 2, (btn->obj.uda.height - (long)GCCharHeight) / 2, btn->text, COL_BTN_TEXT);
	}
}

/*设置按钮文本*/
void GCBtnSetText(CTRL_BTN *btn, const char *text)
{
	if (text)
	{
		strncpy(btn->text, text, BTN_TXT_LEN - 1);
		btn->text[BTN_TXT_LEN - 1] = 0;
	}
	else
		btn->text[0] = 0;
	GCGobjDraw(&btn->obj);
}

/*设置按钮为不可用样式*/
void GCBtnSetDisable(CTRL_BTN *btn, BOOL isDisable)
{
	if (isDisable)
	{
		if (btn->obj.style & BTN_STYLE_DISABLED)
			return;
		btn->obj.style |= BTN_STYLE_DISABLED;
	}
	else
	{
		if (!(btn->obj.style & BTN_STYLE_DISABLED))
			return;
		btn->obj.style &= (~BTN_STYLE_DISABLED);
	}
	GCGobjDraw(&btn->obj);
}

/**********静态文本框**********/

/*创建静态文本框*/
long GCTxtCreate(CTRL_TXT **txt, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text)
{
	CTRL_TXT *NewTxt;
	long res;

	if ((NewTxt = (CTRL_TXT*)malloc(sizeof(CTRL_TXT))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewTxt->obj, args, GCTxtDefMsgProc, (DRAWPROC)GCTxtDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewTxt);
		return res;
	}
	NewTxt->obj.type = GC_CTRL_TYPE_TEXT;
	if (text)
	{
		strncpy(NewTxt->text, text, TXT_TXT_LEN - 1);
		NewTxt->text[TXT_TXT_LEN - 1] = 0;
	}
	else
		NewTxt->text[0] = 0;
	if (txt)
		*txt = NewTxt;
	return NO_ERROR;
}

/*静态文本框消息处理*/
long GCTxtDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	return NO_ERROR;
}

/*静态文本框绘制处理*/
void GCTxtDefDrawProc(CTRL_TXT *txt)
{
	GCFillRect(&txt->obj.uda, 0, ((long)txt->obj.uda.height - (long)GCCharHeight) / 2, txt->obj.uda.width, GCCharHeight, COL_WND_FLAT);	/*底色*/
	GCDrawStr(&txt->obj.uda, 0, ((long)txt->obj.uda.height - (long)GCCharHeight) / 2, txt->text, COL_TEXT_DARK);
}

/*设置文本框文本*/
void GCTxtSetText(CTRL_TXT *txt, const char *text)
{
	if (text)
	{
		strncpy(txt->text, text, TXT_TXT_LEN - 1);
		txt->text[TXT_TXT_LEN - 1] = 0;
	}
	else
		txt->text[0] = 0;
	GCGobjDraw(&txt->obj);
}

/**********单行编辑框**********/

/*创建单行编辑框*/
long GCSedtCreate(CTRL_SEDT **edt, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text, void (*EnterProc)(CTRL_SEDT *edt))
{
	CTRL_SEDT *NewEdt;
	long res;

	if ((NewEdt = (CTRL_SEDT*)malloc(sizeof(CTRL_SEDT))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewEdt->obj, args, GCSedtDefMsgProc, (DRAWPROC)GCSedtDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewEdt);
		return res;
	}
	NewEdt->obj.type = GC_CTRL_TYPE_SLEDIT;
	if (text)
	{
		strncpy(NewEdt->text, text, SEDT_TXT_LEN - 1);
		NewEdt->text[SEDT_TXT_LEN - 1] = 0;
	}
	else
		NewEdt->text[0] = 0;
	NewEdt->CurC = NewEdt->FstC = NewEdt->text;
	NewEdt->EnterProc = EnterProc;
	if (edt)
		*edt = NewEdt;
	return NO_ERROR;
}

/*单行编辑框文本区绘制*/
static void SedtDrawText(CTRL_SEDT *edt)
{
	GCFillRect(&edt->obj.uda, 1, ((long)edt->obj.uda.height - (long)GCCharHeight) / 2, edt->obj.uda.width - 2, GCCharHeight, COL_BTN_GRADLIGHT);	/*底色*/
	GCDrawStr(&edt->obj.uda, 1, ((long)edt->obj.uda.height - (long)GCCharHeight) / 2, edt->FstC, COL_TEXT_DARK);
	if (edt->obj.style & SEDT_STATE_FOCUS)	/*获得焦点时才显示光标*/
		GCFillRect(&edt->obj.uda, 1 + (edt->CurC - edt->FstC) * GCCharWidth, ((long)edt->obj.uda.height - (long)GCCharHeight) / 2, 2, GCCharHeight, COL_TEXT_DARK);	/*光标*/
	GUIpaint(edt->obj.gid, 1, ((long)edt->obj.uda.height - (long)GCCharHeight) / 2, edt->obj.uda.width - 2, GCCharHeight);
}

/*单行编辑框消息处理*/
long GCSedtDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_SEDT *edt = (CTRL_SEDT*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_SETFOCUS:
		if (data[1])
			edt->obj.style |= SEDT_STATE_FOCUS;
		else
			edt->obj.style &= (~SEDT_STATE_FOCUS);
		SedtDrawText(edt);
		break;
	case GM_LBUTTONDOWN:	/*鼠标按下*/
		if (edt->obj.style & SEDT_STYLE_RDONLY)
			break;
		if (!(edt->obj.style & SEDT_STATE_FOCUS))
			GUISetFocus(edt->obj.gid);
		{
			DWORD len;
			len = ((data[5] & 0xFFFF) - 1 + GCCharWidth / 2) / GCCharWidth;
			for (edt->CurC = edt->FstC; *edt->CurC; edt->CurC++, len--)
				if (len == 0)
					break;
		}
		SedtDrawText(edt);
		break;
	case GM_KEY:	/*按键*/
		if (edt->obj.style & SEDT_STYLE_RDONLY)
			break;
		if ((data[1] & (KBD_STATE_LCTRL | KBD_STATE_RCTRL)) && (data[1] & 0xFF) == ' ')	/*按下Ctrl+空格,开关输入法*/
		{
			if (edt->obj.style & SEDT_STATE_IME)
			{
				edt->obj.style &= (~SEDT_STATE_IME);
				IMECloseBar();
			}
			else
			{
				edt->obj.style |= SEDT_STATE_IME;
				IMEOpenBar(edt->obj.x + edt->obj.par->x, edt->obj.y + edt->obj.par->y - 20);
			}
			break;
		}
		else if (edt->obj.style & SEDT_STATE_IME)	/*输入法已开启*/
		{
			THREAD_ID ptid;
			ptid.ProcID = SRV_IME_PORT;
			ptid.ThedID = INVALID;
			data[MSG_API_ID] = MSG_ATTR_IME | IME_API_PUTKEY;
			if (KSendMsg(&ptid, data, 0) != NO_ERROR)	/*出错后不使用输入法*/
				edt->obj.style &= (~SEDT_STATE_IME);
			data[MSG_API_ID] = MSG_ATTR_GUI | GM_KEY;
			break;
		}
		switch (data[1] & 0xFF)
		{
		case '\b':	/*退格键*/
			if (edt->CurC > edt->text)	/*光标前还有字符*/
			{
				strcpy(edt->CurC - 1, edt->CurC);	/*移动字符串*/
				edt->CurC--;	/*移动光标*/
				if (edt->FstC > edt->text)
					edt->FstC--;	/*移动首字符*/
			}
			break;
		case '\r':	/*回车键*/
			if (edt->EnterProc)
				edt->EnterProc(edt);	/*执行回车动作*/
			break;
		case '\0':	/*无可显符号按键*/
			switch ((data[1] >> 8) & 0xFF)
			{
			case 0x4B:	/*左箭头*/
				if (edt->CurC > edt->text)
				{
					edt->CurC--;	/*移动光标*/
					if (edt->FstC > edt->CurC)
						edt->FstC = edt->CurC;	/*移动首字符*/
				}
				break;
			case 0x4D:	/*右箭头*/
				if (*edt->CurC != '\0')
				{
					DWORD len;
					len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
					edt->CurC++;	/*移动光标*/
					if (edt->FstC < edt->CurC - len)
						edt->FstC = edt->CurC - len;	/*移动首字符*/
				}
				break;
			case 0x47:	/*HOME键*/
				edt->CurC = edt->FstC = edt->text;
				break;
			case 0x4F:	/*END键*/
				{
					DWORD len;
					len = strlen(edt->text);
					edt->CurC = edt->text + len;	/*移动光标*/
					len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
					if (edt->FstC < edt->CurC - len)
						edt->FstC = edt->CurC - len;	/*移动首字符*/
				}
				break;
			case 0x53:	/*DELETE键*/
				if (*edt->CurC != '\0')
					strcpy(edt->CurC, edt->CurC + 1);	/*移动字符串*/
				if (strlen(edt->FstC) < (edt->obj.uda.width - 4) / GCCharWidth && edt->FstC > edt->text)
					edt->FstC--;	/*移动首字符*/
				break;
			}
			break;
		default:	/*其它字符插入缓冲*/
			{
				DWORD len;
				len = strlen(edt->text);
				if (len < SEDT_TXT_LEN - 1)	/*缓冲没满*/
				{
					char *strp;
					strp = edt->text + len + 1;
					do
					*strp = *(strp - 1);
					while (--strp > edt->CurC);	/*反向搬移字符*/
					*edt->CurC = (char)(BYTE)data[1];
					edt->CurC++;	/*移动光标*/
					len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
					if (edt->FstC < edt->CurC - len)
						edt->FstC = edt->CurC - len;	/*移动首字符*/
				}
			}
		}
		SedtDrawText(edt);
		break;
	case GM_IMEPUTKEY:
		{
			DWORD len;
			len = strlen(edt->text);
			if (len < SEDT_TXT_LEN - 2)	/*缓冲没满*/
			{
				char *strp;
				strp = edt->text + len + 2;
				do
				*strp = *(strp - 2);
				while (--strp > edt->CurC);	/*反向搬移字符*/
				*edt->CurC = (char)(BYTE)data[1];
				*(edt->CurC + 1) = (char)(BYTE)(data[1] >> 8);
				edt->CurC += 2;	/*移动光标*/
				len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
				if (edt->FstC < edt->CurC - len)
					edt->FstC = edt->CurC - len;	/*移动首字符*/
				SedtDrawText(edt);
			}
		}
		break;
	}
	return NO_ERROR;
}

/*单行编辑框绘制处理*/
void GCSedtDefDrawProc(CTRL_SEDT *edt)
{
	GCFillRect(&edt->obj.uda, 0, 0, edt->obj.uda.width, 1, COL_BTN_BORDER);	/*上边框*/
	GCFillRect(&edt->obj.uda, 0, edt->obj.uda.height - 1, edt->obj.uda.width, 1, COL_BTN_BORDER);	/*下边框*/
	GCFillRect(&edt->obj.uda, 0, 1, 1, edt->obj.uda.height - 2, COL_BTN_BORDER);	/*左边框*/
	GCFillRect(&edt->obj.uda, edt->obj.uda.width - 1, 1, 1, edt->obj.uda.height - 2, COL_BTN_BORDER);	/*右边框*/
	GCFillRect(&edt->obj.uda, 1, 1, edt->obj.uda.width - 2, edt->obj.uda.height - 2, (edt->obj.style & SEDT_STYLE_RDONLY) ? COL_BTN_GRADDARK : COL_BTN_GRADLIGHT);	/*底色*/
	GCDrawStr(&edt->obj.uda, 1, (edt->obj.uda.height - (long)GCCharHeight) / 2, edt->FstC, COL_TEXT_DARK);	/*文本*/
	if (edt->obj.style & SEDT_STATE_FOCUS)	/*获得焦点时才显示光标*/
		GCFillRect(&edt->obj.uda, 1 + (edt->CurC - edt->FstC) * GCCharWidth, (edt->obj.uda.height - (long)GCCharHeight) / 2, 2, GCCharHeight, COL_TEXT_DARK);	/*光标*/
}

/*设置单行编辑框文本*/
void GCSedtSetText(CTRL_SEDT *edt, const char *text)
{
	DWORD len;

	if (text)
	{
		strncpy(edt->text, text, SEDT_TXT_LEN - 1);
		edt->text[SEDT_TXT_LEN - 1] = 0;
	}
	else
		edt->text[0] = 0;
	len = strlen(edt->text);
	edt->CurC = edt->text + len;	/*移动光标*/
	len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
	if (edt->FstC < edt->CurC - len)
		edt->FstC = edt->CurC - len;	/*移动首字符*/
	GCGobjDraw(&edt->obj);
}

/*追加单行编辑框文本*/
void GCSedtAddText(CTRL_SEDT *edt, const char *text)
{
	DWORD len;

	if (text == NULL)
		return;
	len = strlen(edt->text);
	strncpy(edt->text + len, text, SEDT_TXT_LEN - 1 - len);	/*追加字符串*/
	edt->text[SEDT_TXT_LEN - 1] = 0;
	len += strlen(edt->text + len);
	edt->CurC = edt->text + len;	/*移动光标*/
	len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
	if (edt->FstC < edt->CurC - len)
		edt->FstC = edt->CurC - len;	/*移动首字符*/
	GCGobjDraw(&edt->obj);
}

/**********多行编辑框**********/

/**********滚动条**********/

#define SCRL_BTN_SIZE		16	/*滚动条按钮大小*/

/*创建滚动条*/
long GCScrlCreate(CTRL_SCRL **scl, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, long min, long max, long pos, long page, void (*ChangeProc)(CTRL_SCRL *scl))
{
	CTRL_SCRL *NewScl;
	long res;

	if (min >= max || page <= 0 || page > max - min || pos < min || pos > max - page)
		return GC_ERR_WRONG_ARGS;
	if ((NewScl = (CTRL_SCRL*)malloc(sizeof(CTRL_SCRL))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewScl->obj, args, GCScrlDefMsgProc, (DRAWPROC)GCScrlDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewScl);
		return res;
	}
	NewScl->obj.type = GC_CTRL_TYPE_SCROLL;
	NewScl->min = min;
	NewScl->max = max;
	NewScl->pos = pos;
	NewScl->page = page;
	NewScl->ChangeProc = ChangeProc;
	if (scl)
		*scl = NewScl;
	return NO_ERROR;
}

/*滚动条减小按钮处理函数*/
static void ScrlSubBtnProc(CTRL_BTN *sub)
{
	CTRL_SCRL *scl;
	CTRL_BTN *drag;

	if (sub == NULL)
		return;
	scl = (CTRL_SCRL*)sub->obj.par;
	drag = scl->drag;
	if (drag == NULL)
		return;
	if (scl->obj.style & SCRL_STYLE_VER)	/*竖直型*/
	{
		if (drag->obj.y <= (long)sub->obj.uda.height)
			return;
		drag->obj.y -= drag->obj.uda.height;
		if (drag->obj.y < (long)sub->obj.uda.height)
			drag->obj.y = (long)sub->obj.uda.height;
		scl->pos = (drag->obj.y - (long)sub->obj.uda.height) * (scl->max - scl->min) / (scl->add->obj.y - (long)sub->obj.uda.height) + scl->min;
	}
	else
	{
		if (drag->obj.x <= (long)sub->obj.uda.width)
			return;
		drag->obj.x -= drag->obj.uda.width;
		if (drag->obj.x < (long)sub->obj.uda.width)
			drag->obj.x = (long)sub->obj.uda.width;
		scl->pos = (drag->obj.x - (long)sub->obj.uda.width) * (scl->max - scl->min) / (scl->add->obj.x - (long)sub->obj.uda.width) + scl->min;
	}
	GCFillRect(&drag->obj.uda, 0, 0, drag->obj.uda.width, drag->obj.uda.height, COL_CAP_GRADLIGHT);	/*恢复底色*/
	GCSetArea(&drag->obj.uda, drag->obj.uda.width, drag->obj.uda.height, &scl->obj.uda, drag->obj.x, drag->obj.y);
	GCBtnDefDrawProc(drag);
	GUImove(drag->obj.gid, drag->obj.x, drag->obj.y);
	if (scl->ChangeProc)
		scl->ChangeProc(scl);
}

/*滚动条增大按钮处理函数*/
static void ScrlAddBtnProc(CTRL_BTN *add)
{
	CTRL_SCRL *scl;
	CTRL_BTN *drag;

	if (add == NULL)
		return;
	scl = (CTRL_SCRL*)add->obj.par;
	drag = scl->drag;
	if (drag == NULL)
		return;
	if (scl->obj.style & SCRL_STYLE_VER)	/*竖直型*/
	{
		if (drag->obj.y >= add->obj.y - (long)drag->obj.uda.height)
			return;
		drag->obj.y += drag->obj.uda.height;
		if (drag->obj.y > add->obj.y - (long)drag->obj.uda.height)
			drag->obj.y = add->obj.y - (long)drag->obj.uda.height;
		scl->pos = (drag->obj.y - (long)scl->sub->obj.uda.height) * (scl->max - scl->min) / (add->obj.y - (long)scl->sub->obj.uda.height) + scl->min;
	}
	else
	{
		if (drag->obj.x >= add->obj.x - (long)drag->obj.uda.width)
			return;
		drag->obj.x += drag->obj.uda.width;
		if (drag->obj.x > add->obj.x - (long)drag->obj.uda.width)
			drag->obj.x = add->obj.x - (long)drag->obj.uda.width;
		scl->pos = (drag->obj.x - (long)scl->sub->obj.uda.width) * (scl->max - scl->min) / (add->obj.x - (long)scl->sub->obj.uda.width) + scl->min;
	}
	GCFillRect(&drag->obj.uda, 0, 0, drag->obj.uda.width, drag->obj.uda.height, COL_CAP_GRADLIGHT);	/*恢复底色*/
	GCSetArea(&drag->obj.uda, drag->obj.uda.width, drag->obj.uda.height, &scl->obj.uda, drag->obj.x, drag->obj.y);
	GCBtnDefDrawProc(drag);
	GUImove(drag->obj.gid, drag->obj.x, drag->obj.y);
	if (scl->ChangeProc)
		scl->ChangeProc(scl);
}

/*滚动条拖动按钮消息处理*/
static long ScrlDragBtnProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_BTN *btn = (CTRL_BTN*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_DRAG:
		switch (data[1])
		{
		case GM_DRAGMOD_MOVE:
			{
				CTRL_SCRL *scl = (CTRL_SCRL*)btn->obj.par;
				CTRL_BTN *sub = scl->sub;
				CTRL_BTN *add = scl->add;
				if (scl->obj.style & SCRL_STYLE_VER)	/*竖直型*/
				{
					if ((long)data[3] <= (long)sub->obj.uda.height)
					{
						if (btn->obj.y <= (long)sub->obj.uda.height)
							break;
						data[3] = sub->obj.uda.height;
					}
					else if ((long)data[3] >= add->obj.y - (long)btn->obj.uda.height)
					{
						if (btn->obj.y >= add->obj.y - (long)btn->obj.uda.height)
							break;
						data[3] = (DWORD)add->obj.y - btn->obj.uda.height;
					}
					btn->obj.y = data[3];
					scl->pos = (btn->obj.y - (long)sub->obj.uda.height) * (scl->max - scl->min) / (add->obj.y - (long)sub->obj.uda.height) + scl->min;
				}
				else	/*水平型*/
				{
					if ((long)data[2] <= (long)sub->obj.uda.width)
					{
						if (btn->obj.x <= (long)sub->obj.uda.width)
							break;
						data[2] = sub->obj.uda.width;
					}
					else if ((long)data[2] >= add->obj.x - (long)btn->obj.uda.width)
					{
						if (btn->obj.x >= add->obj.x - (long)btn->obj.uda.width)
							break;
						data[2] = (DWORD)add->obj.x - btn->obj.uda.width;
					}
					btn->obj.x = data[2];
					scl->pos = (btn->obj.x - (long)sub->obj.uda.width) * (scl->max - scl->min) / (add->obj.x - (long)sub->obj.uda.width) + scl->min;
				}
				GCFillRect(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_CAP_GRADLIGHT);	/*恢复底色*/
				GCSetArea(&btn->obj.uda, btn->obj.uda.width, btn->obj.uda.height, &scl->obj.uda, btn->obj.x, btn->obj.y);
				GCBtnDefDrawProc(btn);
				GUImove(btn->obj.gid, btn->obj.x, btn->obj.y);
				if (scl->ChangeProc)
					scl->ChangeProc(scl);
			}
			break;
		}
		break;
	case GM_LBUTTONDOWN:
		GUIdrag(btn->obj.gid, GM_DRAGMOD_MOVE);
		break;
	}
	return GCBtnDefMsgProc(ptid, data);
}

/*滚动条消息处理*/
long GCScrlDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_SCRL *scl = (CTRL_SCRL*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			CTRL_ARGS args;
			args.style = 0;
			args.MsgProc = NULL;
			if (scl->obj.style & SCRL_STYLE_VER)	/*竖直型*/
			{
				args.x = 0;
				args.width = scl->obj.uda.width;
				args.height = (scl->obj.uda.height < SCRL_BTN_SIZE * 3) ? scl->obj.uda.height / 3 : SCRL_BTN_SIZE;
				args.y = 0;
				GCBtnCreate(&scl->sub, &args, scl->obj.gid, &scl->obj, "↑", &ScrlVSubImg, ScrlSubBtnProc);
				args.y = scl->obj.uda.height - args.height;
				GCBtnCreate(&scl->add, &args, scl->obj.gid, &scl->obj, "↓", &ScrlVAddImg, ScrlAddBtnProc);
				args.y = (scl->pos - scl->min) * (scl->obj.uda.height - args.height * 2) / (scl->max - scl->min) + args.height;
				args.height = scl->page * (scl->obj.uda.height - args.height * 2) / (scl->max - scl->min);
				args.MsgProc = ScrlDragBtnProc;
				GCBtnCreate(&scl->drag, &args, scl->obj.gid, &scl->obj, "-", &ScrlVDragImg, NULL);
			}
			else	/*水平型*/
			{
				args.y = 0;
				args.height = scl->obj.uda.height;
				args.width = (scl->obj.uda.width < SCRL_BTN_SIZE * 3) ? scl->obj.uda.width / 3 : SCRL_BTN_SIZE;
				args.x = 0;
				GCBtnCreate(&scl->sub, &args, scl->obj.gid, &scl->obj, "<", &ScrlHSubImg, ScrlSubBtnProc);
				args.x = scl->obj.uda.width - args.width;
				GCBtnCreate(&scl->add, &args, scl->obj.gid, &scl->obj, ">", &ScrlHAddImg, ScrlAddBtnProc);
				args.x = (scl->pos - scl->min) * (scl->obj.uda.width - args.width * 2) / (scl->max - scl->min) + args.width;
				args.width = scl->page * (scl->obj.uda.width - args.width * 2) / (scl->max - scl->min);
				args.MsgProc = ScrlDragBtnProc;
				GCBtnCreate(&scl->drag, &args, scl->obj.gid, &scl->obj, "|", &ScrlHDragImg, NULL);
			}
		}
		break;
	case GM_LBUTTONDOWN:
		if (scl->obj.style & SCRL_STYLE_VER)	/*竖直型*/
		{
			if ((data[5] >> 16) < scl->drag->obj.y)	/*减小*/
				ScrlSubBtnProc(scl->sub);
			else	/*增大*/
				ScrlAddBtnProc(scl->add);
		}
		else	/*水平型*/
		{
			if ((data[5] & 0xFFFF) < scl->drag->obj.x)	/*减小*/
				ScrlSubBtnProc(scl->sub);
			else	/*增大*/
				ScrlAddBtnProc(scl->add);
		}
		break;
	}
	return NO_ERROR;
}

/*滚动条绘制处理*/
void GCScrlDefDrawProc(CTRL_SCRL *scl)
{
	GCFillRect(&scl->obj.uda, 0, 0, scl->obj.uda.width, scl->obj.uda.height, COL_CAP_GRADLIGHT);
}

/*设置滚动条位置大小*/
void GCScrlSetSize(CTRL_SCRL *scl, long x, long y, DWORD width, DWORD height)
{
	if ((long)width < SCRL_BTN_SIZE)	/*修正宽度*/
		width = SCRL_BTN_SIZE;
	else if (width > GCwidth)
		width = GCwidth;
	if ((long)height < SCRL_BTN_SIZE)	/*修正高度*/
		height = SCRL_BTN_SIZE;
	else if (height > GCheight)
		height = GCheight;
	if (GCSetArea(&scl->obj.uda, width, height, &scl->obj.par->uda, x, y) != NO_ERROR)
		return;
	scl->obj.x = x;
	scl->obj.y = y;
	GCScrlDefDrawProc(scl);
	if (scl->sub && scl->add && scl->drag)
	{
		if (scl->obj.style & SCRL_STYLE_VER)	/*竖直型*/
		{
			height = (scl->obj.uda.height < SCRL_BTN_SIZE * 3) ? scl->obj.uda.height / 3 : SCRL_BTN_SIZE;
			GCSetArea(&scl->sub->obj.uda, width, height, &scl->obj.uda, 0, 0);
			GCBtnDefDrawProc(scl->sub);
			scl->add->obj.y = scl->obj.uda.height - height;
			GCSetArea(&scl->add->obj.uda, width, height, &scl->obj.uda, 0, scl->add->obj.y);
			GCBtnDefDrawProc(scl->add);
			scl->drag->obj.y = (scl->pos - scl->min) * (scl->add->obj.y - height) / (scl->max - scl->min) + height;
			GCSetArea(&scl->drag->obj.uda, width, scl->page * (scl->add->obj.y - height) / (scl->max - scl->min), &scl->obj.uda, 0, scl->drag->obj.y);
			GCBtnDefDrawProc(scl->drag);
		}
		else	/*水平型*/
		{
			width = (scl->obj.uda.width < SCRL_BTN_SIZE * 3) ? scl->obj.uda.width / 3 : SCRL_BTN_SIZE;
			GCSetArea(&scl->sub->obj.uda, width, height, &scl->obj.uda, 0, 0);
			GCBtnDefDrawProc(scl->sub);
			scl->add->obj.x = scl->obj.uda.width - width;
			GCSetArea(&scl->add->obj.uda, width, height, &scl->obj.uda, scl->add->obj.x, 0);
			GCBtnDefDrawProc(scl->add);
			scl->drag->obj.x = (scl->pos - scl->min) * (scl->add->obj.x - width) / (scl->max - scl->min) + width;
			GCSetArea(&scl->drag->obj.uda, scl->page * (scl->add->obj.x - width) / (scl->max - scl->min), height, &scl->obj.uda, scl->drag->obj.x, 0);
			GCBtnDefDrawProc(scl->drag);
		}
	}
	GUIsize(scl->obj.gid, scl->obj.uda.root == &scl->obj.uda ? scl->obj.uda.vbuf : NULL, scl->obj.x, scl->obj.y, scl->obj.uda.width, scl->obj.uda.height);	/*重设滚动条大小*/
	if (scl->sub && scl->add && scl->drag)
	{
		GUIsize(scl->sub->obj.gid, NULL, scl->sub->obj.x, scl->sub->obj.y, scl->sub->obj.uda.width, scl->sub->obj.uda.height);
		GUIsize(scl->add->obj.gid, NULL, scl->add->obj.x, scl->add->obj.y, scl->add->obj.uda.width, scl->add->obj.uda.height);
		GUIsize(scl->drag->obj.gid, NULL, scl->drag->obj.x, scl->drag->obj.y, scl->drag->obj.uda.width, scl->drag->obj.uda.height);
	}
}

/*设置滚动条参数*/
long GCScrlSetData(CTRL_SCRL *scl, long min, long max, long pos, long page)
{
	CTRL_BTN *drag;
	if (min >= max || page <= 0 || page > max - min || pos < min || pos > max - page)
		return GC_ERR_WRONG_ARGS;
	scl->min = min;
	scl->max = max;
	scl->pos = pos;
	scl->page = page;
	drag = scl->drag;
	if (drag)
	{
		GCFillRect(&drag->obj.uda, 0, 0, drag->obj.uda.width, drag->obj.uda.height, COL_CAP_GRADLIGHT);	/*恢复底色*/
		if (scl->obj.style & SCRL_STYLE_VER)	/*竖直型*/
		{
			DWORD height;
			height = scl->sub->obj.uda.height;
			drag->obj.y = (pos - min) * (scl->obj.uda.height - height * 2) / (max - min) + height;
			GCSetArea(&drag->obj.uda, drag->obj.uda.width, page * (scl->obj.uda.height - height * 2) / (max - min), &scl->obj.uda, 0, drag->obj.y);
			GCBtnDefDrawProc(drag);
		}
		else	/*水平型*/
		{
			DWORD width;
			width = scl->sub->obj.uda.width;
			drag->obj.x = (pos - min) * (scl->obj.uda.width - width * 2) / (max - min) + width;
			GCSetArea(&drag->obj.uda, page * (scl->obj.uda.width - width * 2) / (max - min), drag->obj.uda.height, &scl->obj.uda, drag->obj.x, 0);
			GCBtnDefDrawProc(drag);
		}
		GUIsize(drag->obj.gid, NULL, drag->obj.x, drag->obj.y, drag->obj.uda.width, drag->obj.uda.height);
	}
	return NO_ERROR;
}

/**********列表框**********/

#define LST_MIN_WIDTH		32	/*列表框最小尺寸*/
#define LST_MIN_HEIGHT		32

long GCLstCreate(CTRL_LST **lst, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, void (*SelProc)(CTRL_LST *lst))
{
	CTRL_LST *NewLst;
	long res;

	if ((NewLst = (CTRL_LST*)malloc(sizeof(CTRL_LST))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewLst->obj, args, GCLstDefMsgProc, (DRAWPROC)GCLstDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewLst);
		return res;
	}
	NewLst->obj.type = GC_CTRL_TYPE_LIST;
	NewLst->TextX = NewLst->MaxWidth = NewLst->ItemCou = 0;
	NewLst->SelItem = NewLst->CurItem = NewLst->item = NULL;
	NewLst->vscl = NewLst->hscl = NULL;
	NewLst->SelProc = SelProc;
	GCSetArea(&NewLst->cont, NewLst->obj.uda.width - 2, NewLst->obj.uda.height - 2, &NewLst->obj.uda, 1, 1);	/*创建内容绘图区*/
	if (lst)
		*lst = NewLst;
	return NO_ERROR;
}

static void LstFreeItems(LIST_ITEM *item)
{
	while (item)
	{
		LIST_ITEM *TmpItem;
		TmpItem = item->nxt;
		free(item);
		item = TmpItem;
	}
}

static void LstVsclProc(CTRL_SCRL *scl)
{
	CTRL_LST *lst;

	lst = (CTRL_LST*)scl->obj.par;
	if (lst->CurPos == scl->pos)
		return;
	if (lst->CurPos < scl->pos)	/*列表向后移动*/
	{
		do
			lst->CurItem = lst->CurItem->nxt;
		while (++(lst->CurPos) < scl->pos);
	}
	else	/*列表向前移动*/
	{
		do
			lst->CurItem = lst->CurItem->pre;
		while (--(lst->CurPos) > scl->pos);
	}
	GCGobjDraw(&lst->obj);
}

static void LstHsclProc(CTRL_SCRL *scl)
{
	CTRL_LST *lst;

	lst = (CTRL_LST*)scl->obj.par;
	if (lst->TextX == scl->pos)
		return;
	lst->TextX = scl->pos;
	GCGobjDraw(&lst->obj);
}

static void LstMoveItem(CTRL_LST *lst, long move)
{
	CTRL_SCRL *scl;
	scl = lst->vscl;
	if (scl)	/*没有滚动条,无需移动*/
	{
		if (move > 0)
		{
			if (scl->pos >= scl->max - scl->page)	/*已在最大端*/
				return;
			if ((move += scl->pos) > scl->max - scl->page)
				move = scl->max - scl->page;
		}
		else
		{
			if (scl->pos <= 0)	/*已在最小端*/
				return;
			if ((move += scl->pos) < 0)
				move = 0;
		}
		GCScrlSetData(scl, 0, scl->max, move, scl->page);
		LstVsclProc(scl);
	}
}

long GCLstDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_LST *lst = (CTRL_LST*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_LBUTTONDOWN:
		if ((data[5] && 0xFFFF) >= 1 && (data[5] && 0xFFFF) <= lst->cont.width && (data[5] >> 16) >= 1 && (data[5] >> 16) <= lst->cont.height)
		{
			DWORD y;
			LIST_ITEM *item;
			if (lst->SelItem)
			{
				lst->SelItem->attr &= (~LSTITM_ATTR_SELECTED);
				lst->SelItem = NULL;
			}
			for (y = 0, item = lst->CurItem; item; y++, item = item->nxt)
				if (y == ((data[5] >> 16) - 1) / GCCharHeight)
				{
					item->attr |= LSTITM_ATTR_SELECTED;
					break;
				}
			lst->SelItem = item;
			GCGobjDraw(&lst->obj);
			if (lst->SelProc)
				lst->SelProc(lst);
		}
		break;
	case GM_MOUSEWHEEL:
		LstMoveItem(lst, (long)data[4]);
		break;
	case GM_DESTROY:
		LstFreeItems(lst->item);
		lst->TextX = lst->MaxWidth = lst->ItemCou = 0;
		lst->SelItem = lst->CurItem = lst->item = NULL;
		break;
	}
	return NO_ERROR;
}

void GCLstDefDrawProc(CTRL_LST *lst)
{
	DWORD y, DrawFlag;
	LIST_ITEM *item;

	for (y = 0, DrawFlag = 0, item = lst->CurItem; y < lst->cont.height; y += GCCharHeight)
	{
		GCFillRect(&lst->cont, 0, y, lst->cont.width, GCCharHeight, (DrawFlag ^= 1) ? COL_BTN_GRADDARK : COL_BTN_GRADLIGHT);
		if (item)
		{
			GCDrawStr(&lst->cont, -lst->TextX, y, item->text, (item->attr & LSTITM_ATTR_SELECTED) ? COL_TEXT_LIGHT : COL_TEXT_DARK);
			item = item->nxt;
		}
	}
	GCFillRect(&lst->obj.uda, 0, 0, lst->obj.uda.width, 1, COL_BTN_BORDER);	/*上边框*/
	GCFillRect(&lst->obj.uda, 0, lst->obj.uda.height - 1, lst->obj.uda.width, 1, COL_BTN_BORDER);	/*下边框*/
	GCFillRect(&lst->obj.uda, 0, 1, 1, lst->obj.uda.height - 2, COL_BTN_BORDER);	/*左边框*/
	GCFillRect(&lst->obj.uda, lst->obj.uda.width - 1, 1, 1, lst->obj.uda.height - 2, COL_BTN_BORDER);	/*右边框*/
}

/*设置列表框位置大小*/
void GCLstSetSize(CTRL_LST *lst, long x, long y, DWORD width, DWORD height)
{
	DWORD VsclLen, HsclLen;
	CTRL_ARGS argsv, argsh;

	if ((long)width < LST_MIN_WIDTH)	/*修正宽度*/
		width = LST_MIN_WIDTH;
	if ((long)height < LST_MIN_HEIGHT)	/*修正高度*/
		height = LST_MIN_HEIGHT;
	if (GCSetArea(&lst->obj.uda, width, height, &lst->obj.par->uda, x, y) != NO_ERROR)
		return;
	lst->obj.x = x;
	lst->obj.y = y;
	GCSetArea(&lst->cont, lst->obj.uda.width - 2, lst->obj.uda.height - 2, &lst->obj.uda, 1, 1);	/*内容绘图区*/
	VsclLen = lst->cont.height / GCCharHeight;
	if (lst->ItemCou > VsclLen)
	{
		while (lst->CurPos > lst->ItemCou - VsclLen)
		{
			lst->CurItem = lst->CurItem->pre;
			lst->CurPos--;
		}
		argsv.width = (lst->obj.uda.width < SCRL_BTN_SIZE * 2) ? lst->obj.uda.width / 2 : SCRL_BTN_SIZE;
		argsv.height = lst->obj.uda.height - 2;
		argsv.x = lst->obj.uda.width - argsv.width - 1;
		argsv.y = 1;
		lst->cont.width -= argsv.width;
	}
	else
	{
		lst->CurItem = lst->item;
		lst->CurPos = 0;
		if (lst->vscl)
		{
			GUIdestroy(lst->vscl->obj.gid);	/*自动销毁纵向滚动条*/
			lst->vscl = NULL;
		}
		VsclLen = 0;
	}
	HsclLen = lst->cont.width;
	if (lst->MaxWidth > HsclLen)
	{
		if (lst->TextX > lst->MaxWidth - HsclLen)
			lst->TextX = lst->MaxWidth - HsclLen;
		argsh.width = HsclLen;
		argsh.height = (lst->obj.uda.height < SCRL_BTN_SIZE * 2) ? lst->obj.uda.height / 2 : SCRL_BTN_SIZE;
		argsh.x = 1;
		argsh.y = lst->obj.uda.height - argsh.height - 1;
		lst->cont.height -= argsh.height;
	}
	else
	{
		lst->TextX = 0;
		if (lst->hscl)
		{
			GUIdestroy(lst->hscl->obj.gid);	/*自动销毁横向滚动条*/
			lst->hscl = NULL;
		}
		HsclLen = 0;
	}
	GCLstDefDrawProc(lst);
	GUIsize(lst->obj.gid, lst->obj.uda.root == &lst->obj.uda ? lst->obj.uda.vbuf : NULL, lst->obj.x, lst->obj.y, lst->obj.uda.width, lst->obj.uda.height);	/*重设滚动条大小*/
	if (VsclLen)
	{
		if (lst->vscl == NULL)	/*增加纵向滚动条*/
		{
			argsv.style = SCRL_STYLE_VER;
			argsv.MsgProc = NULL;
			GCScrlCreate(&lst->vscl, &argsv, lst->obj.gid, &lst->obj, 0, lst->ItemCou, lst->CurPos, VsclLen, LstVsclProc);
		}
		else	/*调整纵向滚动条大小*/
		{
			GCScrlSetSize(lst->vscl, argsv.x, argsv.y, argsv.width, argsv.height);
			GCScrlSetData(lst->vscl, 0, lst->ItemCou, lst->CurPos, VsclLen);
		}
		if (lst->hscl)
			GCScrlSetSize(lst->hscl, 1, lst->hscl->obj.y, lst->hscl->obj.uda.width - argsv.width, lst->hscl->obj.uda.height);
	}
	if (HsclLen)
	{
		if (lst->hscl == NULL)	/*增加横向滚动条*/
		{
			argsh.style = SCRL_STYLE_HOR;
			argsh.MsgProc = NULL;
			GCScrlCreate(&lst->hscl, &argsh, lst->obj.gid, &lst->obj, 0, lst->MaxWidth, lst->TextX, HsclLen, LstHsclProc);
		}
		else	/*调整横向滚动条大小*/
		{
			GCScrlSetSize(lst->hscl, argsh.x, argsh.y, argsh.width, argsh.height);
			GCScrlSetData(lst->hscl, 0, lst->MaxWidth, lst->TextX, HsclLen);
		}
	}
}

/*插入项*/
long GCLstInsertItem(CTRL_LST *lst, LIST_ITEM *pre, const char *text, LIST_ITEM **item)
{
	LIST_ITEM *NewItem, *nxt;
	DWORD len;

	if ((NewItem = (LIST_ITEM*)malloc(sizeof(LIST_ITEM))) == NULL)	/*申请新项*/
		return GC_ERR_OUT_OF_MEM;
	if (text)	/*复制文本*/
	{
		DWORD MaxWidth;
		strncpy(NewItem->text, text, LSTITM_TXT_LEN - 1);
		NewItem->text[LSTITM_TXT_LEN - 1] = 0;
		MaxWidth = strlen(NewItem->text) * GCCharWidth;	/*修改最大像素宽度*/
		if (lst->MaxWidth < MaxWidth)
			lst->MaxWidth = MaxWidth;
	}
	else
		NewItem->text[0] = 0;
	NewItem->attr = 0;
	nxt = (pre ? pre->nxt : lst->item);	/*新节点插入链表*/
	NewItem->pre = pre;
	NewItem->nxt = nxt;
	if (pre)
		pre->nxt = NewItem;
	else
		lst->item = NewItem;
	if (nxt)
		nxt->pre = NewItem;
	if (lst->CurItem == NULL)	/*设置当前项及位置*/
	{
		nxt = lst->CurItem = NewItem;
		lst->CurPos = 0;
	}
	else if (NewItem->nxt && NewItem->pre != lst->CurItem)
	{
		for (nxt = lst->item;; nxt = nxt->nxt)
		{
			if (nxt == lst->CurItem)
				break;
			if (nxt == NewItem)
			{
				lst->CurPos++;
				break;
			}
		}
	}
	else
		nxt = lst->CurItem;
	lst->ItemCou++;	/*项数增加*/
	len = lst->cont.height / GCCharHeight;
	if (lst->ItemCou > len)
	{
		if (lst->vscl == NULL)
		{
			CTRL_ARGS args;	/*增加纵向滚动条*/
			args.width = (lst->obj.uda.width < SCRL_BTN_SIZE * 2) ? lst->obj.uda.width / 2 : SCRL_BTN_SIZE;
			args.height = lst->obj.uda.height - 2;
			args.x = lst->obj.uda.width - args.width - 1;
			args.y = 1;
			args.style = SCRL_STYLE_VER;
			args.MsgProc = NULL;
			GCScrlCreate(&lst->vscl, &args, lst->obj.gid, &lst->obj, 0, lst->ItemCou, lst->CurPos, len, LstVsclProc);
			lst->cont.width -= args.width;
			if (lst->hscl)
				GCScrlSetSize(lst->hscl, 1, lst->hscl->obj.y, lst->hscl->obj.uda.width - args.width, lst->hscl->obj.uda.height);
		}
		else
			GCScrlSetData(lst->vscl, 0, lst->ItemCou, lst->CurPos, len);
	}
	else
	{
		lst->CurItem = lst->item;
		lst->CurPos = 0;
	}
	len = lst->cont.width;
	if (lst->MaxWidth > len)
	{
		if (lst->hscl == NULL)
		{
			CTRL_ARGS args;	/*增加横向滚动条*/
			args.width = len;
			args.height = (lst->obj.uda.height < SCRL_BTN_SIZE * 2) ? lst->obj.uda.height / 2 : SCRL_BTN_SIZE;
			args.x = 1;
			args.y = lst->obj.uda.height - args.height - 1;
			args.style = SCRL_STYLE_HOR;
			args.MsgProc = NULL;
			GCScrlCreate(&lst->hscl, &args, lst->obj.gid, &lst->obj, 0, lst->MaxWidth, lst->TextX, len, LstHsclProc);
			lst->cont.height -= args.height;
		}
		else
			GCScrlSetData(lst->hscl, 0, lst->MaxWidth, lst->TextX, len);
	}
	if (nxt == lst->CurItem)
	{
		len = (lst->cont.height + GCCharHeight - 1) / GCCharHeight;
		while (len)
		{
			if (nxt == NewItem)
			{
				GCGobjDraw(&lst->obj);
				break;
			}
			nxt = nxt->nxt;
			len--;
		}
	}
	if (item)
		*item = NewItem;

	return NO_ERROR;
}

/*删除项*/
long GCLstDeleteItem(CTRL_LST *lst, LIST_ITEM *item)
{
	if (item->pre)
		item->pre->nxt = item->nxt;
	else
		lst->item = item->nxt;
	if (item->nxt)
		item->nxt->pre = item->pre;
	free(item);

	return NO_ERROR;
}

/*删除所有项*/
long GCLstDelAllItem(CTRL_LST *lst)
{
	if (lst->vscl)
	{
		GUIdestroy(lst->vscl->obj.gid);	/*自动销毁纵向滚动条*/
		lst->vscl = NULL;
		lst->cont.width = lst->obj.uda.width - 2;
	}
	if (lst->hscl)
	{
		GUIdestroy(lst->hscl->obj.gid);	/*自动销毁横向滚动条*/
		lst->hscl = NULL;
		lst->cont.height = lst->obj.uda.height - 2;
	}
	LstFreeItems(lst->item);
	lst->TextX = lst->MaxWidth = lst->ItemCou = 0;
	lst->SelItem = lst->CurItem = lst->item = NULL;
	GCGobjDraw(&lst->obj);
	return NO_ERROR;
}
