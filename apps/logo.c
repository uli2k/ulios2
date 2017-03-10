/*	logo.c for ulios application
	作者：刘高辉
	功能：开机LOGO
	最后修改日期：2013-02-03
*/

#include "../fs/fsapi.h"
#include "../lib/malloc.h"
#include "../lib/gdi.h"
#include "../lib/math.h"

/*加载BMP图像文件*/
long LoadBmp(char *path, DWORD *buf, DWORD len, DWORD *width, DWORD *height)
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
		return -1;
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

/*24位图像绕中心旋转,参数:源缓冲,源尺寸,目的缓冲,目的尺寸,旋转角度*/
void RotateImage(DWORD *src, DWORD sw, DWORD sh, DWORD *dst, DWORD dw, DWORD dh, float radian)
{
	DWORD x, y, sx, sy;
	long dx, dy;
	float sinr, cosr;

	sinr = sin(radian);
	cosr = cos(radian);
	for (y = 0, dy = -(long)(dh >> 1); y < dh; y++, dy++)
		for (x = 0, dx = -(long)(dw >> 1); x < dw; x++, dx++)
		{
			sx = (sw >> 1) + (DWORD)(dx * cosr + dy * sinr);
			sy = (sh >> 1) + (DWORD)(dy * cosr - dx * sinr);
			*dst++ = (sx < sw && sy < sh) ? src[sx + sy * sw] : 0;
		}
}

int main()
{
	DWORD *img, *bmp, BmpWidth, BmpHeight, ImgWidth;
	float radian;
	long i, res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*设置16MB堆内存*/
		return res;
	LoadBmp("logo.bmp", NULL, 0, &BmpWidth, &BmpHeight);
	bmp = (DWORD*)malloc(BmpWidth * BmpHeight * sizeof(DWORD));
	LoadBmp("logo.bmp", bmp, BmpWidth * BmpHeight, NULL, NULL);
	ImgWidth = (DWORD)sqrt(BmpWidth * BmpWidth + BmpHeight * BmpHeight);
	img = (DWORD*)malloc(ImgWidth * ImgWidth * sizeof(DWORD));
	if ((res = GDIinit()) != NO_ERROR)
		return res;
	radian = 0.0f;
	for (i = 0; i < 1000; i++)
	{
		radian += 0.02f;
		RotateImage(bmp, BmpWidth, BmpHeight, img, ImgWidth, ImgWidth, radian);
		GDIPutImage(100, 100, img, ImgWidth, ImgWidth);
		KSleep(10);
	}
	GDIrelease();
	return NO_ERROR;
}
