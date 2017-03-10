/*	gdi.h for ulios
	作者：孙亮
	功能：图形设备接口库定义
	最后修改日期：2010-06-10
*/

#ifndef	_GDI_H_
#define	_GDI_H_

#include "../driver/basesrv.h"

#define GDI_ERR_LOCATION	-1408	/*坐标错误*/
#define GDI_ERR_SIZE		-1409	/*尺寸错误*/
#define GDI_ERR_TEXTMODE	-1410	/*文本模式*/

extern void *GDIvm;
extern const BYTE *GDIfont;
extern DWORD GDIwidth, GDIheight, GDIPixBits, GDImode;
extern DWORD GDICharWidth, GDICharHeight;

/*初始化GDI库*/
long GDIinit();

/*撤销GDI库*/
void GDIrelease();

/*画点*/
long GDIPutPixel(DWORD x, DWORD y, DWORD c);

/*取点*/
long GDIGetPixel(DWORD x, DWORD y, DWORD *c);

/*贴图*/
long GDIPutImage(long x, long y, DWORD *img, long w, long h);

/*去背景色贴图*/
long GDIPutBCImage(long x, long y, DWORD *img, long w, long h, DWORD bc);

/*截图*/
long GDIGetImage(long x, long y, DWORD *img, long w, long h);

/*填充矩形*/
long GDIFillRect(long x, long y, long w, long h, DWORD c);

/*画线*/
long GDIDrawLine(long x1, long y1, long x2, long y2, DWORD c);

/*画圆*/
long GDIcircle(long cx, long cy, long r, DWORD c);

/*绘制汉字*/
long GDIDrawHz(long x, long y, DWORD hz, DWORD c);

/*绘制ASCII字符*/
long GDIDrawAscii(long x, long y, DWORD ch, DWORD c);

/*绘制字符串*/
long GDIDrawStr(long x, long y, const char *str, DWORD c);

#endif
