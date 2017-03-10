// Bmp2Res.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>

/*加载BMP图像文件*/
long GCLoadBmp(_TCHAR *path, DWORD *buf, DWORD len, DWORD *width, DWORD *height)
{
	BYTE BmpHead[32];
	DWORD bmpw, bmph;
	FILE *file;

	if ((file = _tfopen(path, TEXT("rb"))) == NULL)	/*读取BMP文件*/
		return -1;
	if (fread(&BmpHead[2], 1, 30, file) < 30 || *((WORD*)&BmpHead[2]) != 0x4D42 || *((WORD*)&BmpHead[30]) != 24)	/*保证32位对齐访问*/
	{
		fclose(file);
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
		fclose(file);
		return NO_ERROR;
	}
	fseek(file, 54, SEEK_SET);
	len = (bmpw * 3 + 3) & 0xFFFFFFFC;
	for (buf += bmpw * (bmph - 1); bmph > 0; bmph--, buf -= bmpw)
	{
		BYTE *src, *dst;

		if (fread(buf, 1, len, file) < len)
		{
			fclose(file);
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
	fclose(file);
	return NO_ERROR;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int k;
	DWORD *bmp, *bmpp, BmpWidth, BmpHeight, i, j;

	if (argc < 2)
	{
		_tprintf(TEXT("Bmp2Res file1.bmp file2.bmp ...\n"));
		return 0;
	}
	for (k = 1; k < argc; k++)
	{
		BmpWidth = 0;
		GCLoadBmp(argv[k], NULL, 0, &BmpWidth, &BmpHeight);
		if (BmpWidth == 0)
		{
			_tprintf(TEXT("read bmp file error\n"));
			return 0;
		}
		_tprintf(TEXT("%s\n0x%08X, 0x%08X,\n"), argv[k], BmpWidth, BmpHeight);
		bmpp = bmp = (DWORD*)malloc(BmpWidth * BmpHeight * sizeof(DWORD));
		GCLoadBmp(argv[k], bmp, BmpWidth * BmpHeight, NULL, NULL);
		for (j = 0; j < BmpHeight; j++)
		{
			for (i = 0; i < BmpWidth; i++)
				_tprintf(TEXT("0x%08X, "), *bmpp++);
			_tprintf(TEXT("\n"));
		}
		free(bmp);
	}
	return 0;
}
