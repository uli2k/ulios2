/*	jpegdec.c for ulios application
	作者：孙亮
	功能：jpg图像解码显示程序
	最后修改日期：2010-07-31
*/

#include "../driver/basesrv.h"
#include "../lib/gdi.h"
#include "../fs/fsapi.h"

#define FDAT_SIZ		0x00700000	/*动态内存大小*/

void *fm, *CurFm;

/*简略的动态内存分配*/
static inline void *malloc(DWORD siz)
{
	void *tmpfm;

	tmpfm = CurFm;
	CurFm += siz;
	return tmpfm;
}

#define FIXED_1				0x00010000
#define FIXED_half			0x00008000
#define LONG_TO_FIXED(l)	((l) * FIXED_1)
#define FLOAT_TO_FIXED(f)	((long)((f) * FIXED_1))
#define FIXED_TO_SHORT(f)	((short)((f) >> 16))
#define FIXED_TO_BYTE(f)	((BYTE)((f) >> 16))
#define FIXED_TO_LONG(f)	((f) >> 16)

typedef struct _HUFFMAN_NODE
{
	BYTE code;
	BOOL isLeave;
	struct _HUFFMAN_NODE *child[2];
}HUFFMAN_NODE;

typedef struct _DECODE_PARAM
{
	BYTE *stream;
	DWORD bufIdx;
	DWORD bitIdx;
	DWORD remain;
}DECODE_PARAM;

HUFFMAN_NODE *HufNewNode()
{
	HUFFMAN_NODE *root;

	if ((root = (HUFFMAN_NODE*)malloc(sizeof(HUFFMAN_NODE))) == NULL)
		return NULL;
	root->isLeave = FALSE;
	root->child[1] = root->child[0] = NULL;
	return root;
}

BOOL HufAddNode(HUFFMAN_NODE *node, DWORD len, BYTE c)
{
	DWORD i;
	HUFFMAN_NODE *chl;

	if (len == 1)
	{
		for (i = 0; i < 2; i++)
		if (node->child[i] == NULL)
		{
			chl = node->child[i] = (HUFFMAN_NODE*)malloc(sizeof(HUFFMAN_NODE));
			chl->code = c;
			chl->isLeave = TRUE;
			chl->child[1] = chl->child[0] = NULL;
			return TRUE;
		}
		return FALSE;
	}
	else
	{
		for (i = 0; i < 2; i++)
		{
			chl = node->child[i];
			if (chl == NULL)
			{
				chl = node->child[i] = (HUFFMAN_NODE*)malloc(sizeof(HUFFMAN_NODE));
				chl->isLeave = FALSE;
				chl->child[1] = chl->child[0] = NULL;
				return HufAddNode(chl, len - 1, c);
			}
			if (chl->isLeave == FALSE)
			{
				if (HufAddNode(chl, len - 1, c) == TRUE)
					return TRUE;
			}
		}
		return FALSE;
	}
}

void HufClearNode(HUFFMAN_NODE *node)
{
	if (node == NULL)
		return;
	HufClearNode(node->child[0]);
	HufClearNode(node->child[1]);
//	free(node);
}

BOOL HufDecode(HUFFMAN_NODE *root, DECODE_PARAM *pa, BYTE *p)
{
	BYTE uc;
	HUFFMAN_NODE *node = root;
	BYTE *stream = pa->stream + pa->bufIdx;
	DWORD bufIdx, bufIdxBK, bitIdx;
	bufIdx = bufIdxBK = pa->bufIdx;
	bitIdx = pa->bitIdx;
	uc = (*stream++);
	bufIdx++;
	if (uc == 0xFF)
	{
		while ((*stream) == 0xFF)
			stream++, bufIdx++;
		if ((*stream) != 0)
			return FALSE;
		else
			stream++, bufIdx++;
	}
	uc <<= bitIdx;
	for (;;)
	{
		node = node->child[uc >> 7];
		bitIdx++;
		if (node == NULL)
		{
			node = root;
			bitIdx = 8;
			goto ttt;
		}
		if (node->isLeave == TRUE)
		{
			pa->bitIdx = bitIdx;
			if (bitIdx == 8)
			{
				pa->bufIdx = bufIdx;
				pa->bitIdx = 0;
			}
			pa->remain -= pa->bufIdx - bufIdxBK;
			*p = node->code;
			return TRUE;
		}
ttt:	if (bitIdx == 8)
		{
			bitIdx = 0;
			pa->bufIdx = bufIdx;
			uc = *stream++;
			bufIdx++;
			if (uc == 0xFF)
			{
				while ((*stream) == 0xFF)
					stream++, bufIdx++;
				if ((*stream) != 0)
				{
					pa->remain -= pa->bufIdx - bufIdxBK;
					return FALSE;
				}
				else
					stream++, bufIdx++;
			}
			continue;
		}
		uc <<= 1;
	}
	pa->remain -= pa->bufIdx - bufIdxBK;
	return FALSE;
}


typedef struct _COMPONENT
{
	DWORD v, h;
	DWORD qtID;
}COMPONENT;

DWORD imgWidth, imgHeight;
DWORD bufWidth, bufHeight;
DWORD numOfCOM;
DWORD qt[4][64];
HUFFMAN_NODE *htDC[4];
HUFFMAN_NODE *htAC[4];
COMPONENT com[6];
DWORD maxH, maxV;
DWORD unitNumH, unitNumV;
BYTE *image[5];


#define NO_ZERO_ROW_TEST
#define BITS_IN_JSAMPLE 8

#define MAXJSAMPLE		255
#define CENTERJSAMPLE	128

#define CONST_BITS		13
#define PASS1_BITS		2

#define FIX_0_298631336	((long)2446)	/* FIX(0.298631336) */
#define FIX_0_390180644	((long)3196)	/* FIX(0.390180644) */
#define FIX_0_541196100	((long)4433)	/* FIX(0.541196100) */
#define FIX_0_765366865	((long)6270)	/* FIX(0.765366865) */
#define FIX_0_899976223	((long)7373)	/* FIX(0.899976223) */
#define FIX_1_175875602	((long)9633)	/* FIX(1.175875602) */
#define FIX_1_501321110	((long)12299)	/* FIX(1.501321110) */
#define FIX_1_847759065	((long)15137)	/* FIX(1.847759065) */
#define FIX_1_961570560	((long)16069)	/* FIX(1.961570560) */
#define FIX_2_053119869	((long)16819)	/* FIX(2.053119869) */
#define FIX_2_562915447	((long)20995)	/* FIX(2.562915447) */
#define FIX_3_072711026	((long)25172)	/* FIX(3.072711026) */

#define ONE	((long)1)
#define RIGHT_SHIFT(x, shft)	((shift_temp = (x)) < 0 ? (shift_temp >> (shft)) | ((~((long)0)) << (32 - (shft))) : (shift_temp >> (shft)))
#define DESCALE(x, n)	RIGHT_SHIFT((x) + (ONE << ((n) - 1)), n)
#define RANGE_MASK		(MAXJSAMPLE * 4 + 3) /* 2 bits wider than legal samples */
#define RANGE_LIMIT(i)	if ((i) > MAXJSAMPLE) (i) = MAXJSAMPLE; else if ((i) < 0) (i) = 0

void idctInt(short *coef_block, BYTE *output_buf)
{
	long tmp0, tmp1, tmp2, tmp3;
	long tmp10, tmp11, tmp12, tmp13;
	long z1, z2, z3, z4, z5;
	int itp;
	short *inptr;
	int *wsptr;
	char *outptr;
	int ctr;
	int workspace[64];
	long shift_temp;

	inptr = coef_block;
	wsptr = workspace;
	for (ctr = 8; ctr > 0; ctr--)
	{
		if ((inptr[8] | inptr[16] | inptr[24] | inptr[32] | inptr[40] | inptr[48] | inptr[56]) == 0)
		{
			int dcval = inptr[0] << PASS1_BITS;
			wsptr[0] = dcval;
			wsptr[8] = dcval;
			wsptr[16] = dcval;
			wsptr[24] = dcval;
			wsptr[32] = dcval;
			wsptr[40] = dcval;
			wsptr[48] = dcval;
			wsptr[56] = dcval;
			inptr++;
			wsptr++;
			continue;
		}
		z2 = inptr[16];
		z3 = inptr[48];
		z1 = (z2 + z3) * FIX_0_541196100;
		tmp2 = z1 + z3 * (-FIX_1_847759065);
		tmp3 = z1 + z2 * FIX_0_765366865;
		z2 = inptr[0];
		z3 = inptr[32];
		tmp0 = (z2 + z3) << CONST_BITS;
		tmp1 = (z2 - z3) << CONST_BITS;
		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		tmp0 = inptr[56];
		tmp1 = inptr[40];
		tmp2 = inptr[24];
		tmp3 = inptr[8];
		z1 = tmp0 + tmp3;
		z2 = tmp1 + tmp2;
		z3 = tmp0 + tmp2;
		z4 = tmp1 + tmp3;
		z5 = (z3 + z4) * FIX_1_175875602;
		tmp0 = tmp0 * FIX_0_298631336;
		tmp1 = tmp1 * FIX_2_053119869;
		tmp2 = tmp2 * FIX_3_072711026;
		tmp3 = tmp3 * FIX_1_501321110;
		z1 = z1 * (-FIX_0_899976223);
		z2 = z2 * (-FIX_2_562915447);
		z3 = z3 * (-FIX_1_961570560);
		z4 = z4 * (-FIX_0_390180644);
		z3 += z5;
		z4 += z5;
		tmp0 += z1 + z3;
		tmp1 += z2 + z4;
		tmp2 += z2 + z3;
		tmp3 += z1 + z4;
		wsptr[0] = (int)DESCALE(tmp10 + tmp3, CONST_BITS - PASS1_BITS);
		wsptr[56] = (int)DESCALE(tmp10 - tmp3, CONST_BITS - PASS1_BITS);
		wsptr[8] = (int)DESCALE(tmp11 + tmp2, CONST_BITS - PASS1_BITS);
		wsptr[48] = (int)DESCALE(tmp11 - tmp2, CONST_BITS - PASS1_BITS);
		wsptr[16] = (int)DESCALE(tmp12 + tmp1, CONST_BITS - PASS1_BITS);
		wsptr[40] = (int)DESCALE(tmp12 - tmp1, CONST_BITS - PASS1_BITS);
		wsptr[24] = (int)DESCALE(tmp13 + tmp0, CONST_BITS - PASS1_BITS);
		wsptr[32] = (int)DESCALE(tmp13 - tmp0, CONST_BITS - PASS1_BITS);
		inptr++;
		wsptr++;
	}
	wsptr = workspace;
	for (ctr = 0; ctr < 8; ctr++)
	{
		outptr = (char*)(output_buf + ctr * 8);
#ifndef NO_ZERO_ROW_TEST
		if ((wsptr[1] | wsptr[2] | wsptr[3] | wsptr[4] | wsptr[5] | wsptr[6] | wsptr[7]) == 0)
		{
			char dcval;
			itp = (int)DESCALE((long)wsptr[0], PASS1_BITS + 3) + 128;
			if (itp > MAXJSAMPLE)
				itp = MAXJSAMPLE;
			else if (itp < 0)
				itp = 0;
			dcval = (char)itp;
			outptr[0] = dcval;
			outptr[1] = dcval;
			outptr[2] = dcval;
			outptr[3] = dcval;
			outptr[4] = dcval;
			outptr[5] = dcval;
			outptr[6] = dcval;
			outptr[7] = dcval;
			wsptr += 8;
			continue;
		}
#endif
		z2 = (long)wsptr[2];
		z3 = (long)wsptr[6];
		z1 = (z2 + z3) * FIX_0_541196100;
		tmp2 = z1 + z3 * (-FIX_1_847759065);
		tmp3 = z1 + z2 * FIX_0_765366865;
		tmp0 = ((long)wsptr[0] + (long)wsptr[4]) << CONST_BITS;
		tmp1 = ((long)wsptr[0] - (long)wsptr[4]) << CONST_BITS;
		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		tmp0 = (long)wsptr[7];
		tmp1 = (long)wsptr[5];
		tmp2 = (long)wsptr[3];
		tmp3 = (long)wsptr[1];
		z1 = tmp0 + tmp3;
		z2 = tmp1 + tmp2;
		z3 = tmp0 + tmp2;
		z4 = tmp1 + tmp3;
		z5 = (z3 + z4) * FIX_1_175875602;
		tmp0 = tmp0 * FIX_0_298631336;
		tmp1 = tmp1 * FIX_2_053119869;
		tmp2 = tmp2 * FIX_3_072711026;
		tmp3 = tmp3 * FIX_1_501321110;
		z1 = z1 * (-FIX_0_899976223);
		z2 = z2 * (-FIX_2_562915447);
		z3 = z3 * (-FIX_1_961570560);
		z4 = z4 * (-FIX_0_390180644);
		z3 += z5;
		z4 += z5;
		tmp0 += z1 + z3;
		tmp1 += z2 + z4;
		tmp2 += z2 + z3;
		tmp3 += z1 + z4;
		itp = (int)DESCALE(tmp10 + tmp3, CONST_BITS + PASS1_BITS + 3) + 128;
		RANGE_LIMIT(itp);
		outptr[0] = (char)itp;
		itp = (int)DESCALE(tmp10 - tmp3, CONST_BITS + PASS1_BITS + 3) + 128;
		RANGE_LIMIT(itp);
		outptr[7] = (char)itp;
		itp = (int)DESCALE(tmp11 + tmp2, CONST_BITS + PASS1_BITS + 3) + 128;
		RANGE_LIMIT(itp);
		outptr[1] = (char)itp;
		itp = (int)DESCALE(tmp11 - tmp2, CONST_BITS + PASS1_BITS + 3) + 128;
		RANGE_LIMIT(itp);
		outptr[6] = (char)itp;
		itp = (int)DESCALE(tmp12 + tmp1, CONST_BITS + PASS1_BITS + 3) + 128;
		RANGE_LIMIT(itp);
		outptr[2] = (char)itp;
		itp = (int)DESCALE(tmp12 - tmp1, CONST_BITS + PASS1_BITS + 3) + 128;
		RANGE_LIMIT(itp);
		outptr[5] = (char)itp;
		itp = (int)DESCALE(tmp13 + tmp0, CONST_BITS + PASS1_BITS + 3) + 128;
		RANGE_LIMIT(itp);
		outptr[3] = (char)itp;
		itp = (int)DESCALE(tmp13 - tmp0, CONST_BITS + PASS1_BITS + 3) + 128;
		RANGE_LIMIT(itp);
		outptr[4] = (char)itp;
		wsptr += 8;
	}
}

void storeBlock(DWORD comID, DWORD bNum, BYTE *block, BYTE *img)
{
	DWORD x, y, ux, uy, bx, by, uNum, mh, mv, j, k, m, n;
	BYTE *b, *p, *pImg = img;
	uNum = bNum / (com[comID].h * com[comID].v);
	bNum = bNum % (com[comID].h * com[comID].v);
	ux = uNum % unitNumH, uy = uNum / unitNumH;
	bx = bNum % com[comID].h, by = bNum / com[comID].h;
	if (ux >= unitNumH || uy >= unitNumV)
		return;
	ux *= maxH * 8, uy *= maxV * 8;
	mh = maxH / com[comID].h, mv = maxV / com[comID].v;
	bx *= mh * 8, by *= mv * 8;
	x = ux + bx, y = uy + by;
	pImg += bufWidth * y + x;
	for (m = 0; m < mv; m++)
	{
		for (n = 0; n < mh; n++)
		{
			b = block;
			for (j = 0; j < 8; j++)
			{
				p = pImg + (m + j * mv) * bufWidth + n;
				for (k = 0; k < 8; k++)
				{
					*p = *b++;
					p += mh;
				}
			}
		}
	}
}

#define PI	3.1415926535897932
#define ICHG(c1, c2)	((((DWORD)(c1)) << 8) | (c2))

BYTE zigzag[64] =
{
	0,	1,	8,	16,	9,	2,	3,	10,
	17,	24,	32,	25,	18,	11,	4,	5,
	12,	19,	26,	33,	40,	48,	41,	34,
	27,	20,	13,	6,	7,	14,	21,	28,
	35,	42,	49,	56,	57,	50,	43,	36,
	29,	22,	15,	23,	30,	37,	44,	51,
	58,	59,	52,	45,	38,	31,	39,	46,
	53,	60,	61,	54,	47,	55,	62,	63
};

void JpegInit()
{
	DWORD i;
	for (i = 0; i < 4; i++)
	{
		htDC[i] = HufNewNode();
		htAC[i] = HufNewNode();
	}
	for (i = 0; i < 5; i++)
		image[i] = NULL;
	maxH = 0, maxV = 0;
	unitNumH = 0, unitNumV = 0;
}

/*量化表*/
BOOL readQTable(long fh)
{
	BYTE buf[1024], *bufp;
	int lenth;
	DWORD i, qtID;
	FSread(fh, buf, 2);
	lenth = (int)(ICHG(buf[0], buf[1]) - 2);
	FSread(fh, buf, lenth);
	bufp = buf;
	while (lenth > 0)
	{
		qtID = (*bufp) & 0x0F;
		if (((*bufp) >> 4) != 0 || qtID >= 4)
			return FALSE;
		bufp++;
		lenth--;
		for (i = 0; i < 64; i++)
			qt[qtID][i] = *bufp++;
		lenth -= 64;
	}
	if (lenth != 0)
		return FALSE;
	return TRUE;
}

/*帧开始*/
BOOL readSOF0(long fh)
{
	BYTE buf[1024], *bufp;
	int lenth;
	DWORD i;
	FSread(fh, buf, 2);
	lenth = (int)(ICHG(buf[0], buf[1]) - 2);
	FSread(fh, buf, lenth);
	bufp = buf;
	if (buf[0] != 8)
		return FALSE;
	imgHeight = ICHG(buf[1], buf[2]);
	imgWidth = ICHG(buf[3], buf[4]);
	if (imgWidth == 0 || imgHeight == 0)
		return FALSE;
	bufp += 5;
	numOfCOM = *bufp++;
	for (i = 0; i < numOfCOM; i++)
	{
		BYTE c;
		c = *bufp;
		if (c > 5)
			return FALSE;
		bufp++;
		com[c].h = (*bufp) >> 4;
		com[c].v = (*bufp) & 0x0F;
		if (com[c].h == 0 || com[c].v == 0)
			return FALSE;
		if (maxH < com[c].h)
			maxH = com[c].h;
		if (maxV < com[c].v)
			maxV = com[c].v;
		bufp++;
		com[c].qtID = *bufp++;
	}
	unitNumH = (imgWidth + (maxH * 8) - 1) / (maxH * 8);
	unitNumV = (imgHeight + (maxV * 8) - 1) / (maxV * 8);
	bufWidth = unitNumH * maxH * 8;
	bufHeight = unitNumV * maxV * 8;
	for (i = 0; i < numOfCOM; i++)
		if ((image[i] = (BYTE*)malloc(bufWidth * bufHeight * sizeof(BYTE))) == NULL)
			return FALSE;
	return TRUE;
}

/*霍夫曼表*/
BOOL readHTable(long fh)
{
	BYTE buf[1024], *bufp;
	int lenth;
	DWORD i, j;
	DWORD htID, type;
	HUFFMAN_NODE *h;
	FSread(fh, buf, 2);
	lenth = (int)(ICHG(buf[0], buf[1]) - 2);
	FSread(fh, buf, lenth);
	bufp = buf;
	while (lenth > 0)
	{
		BYTE *cn;
		htID = (*bufp) & 0x0F;
		type = ((*bufp) >> 4) & 0x01;
		bufp++;
		lenth--;
		if (htID >= 2)
			return FALSE;
		if (type == 0)
			h = htDC[htID];
		else
			h = htAC[htID];
		cn = bufp;
		bufp += 16;
		lenth -= 16;
		for (i = 0; i < 16; i++)
		{
			for (j = 0; j < cn[i]; j++)
				HufAddNode(h, i + 1, *bufp++);
			lenth -= cn[i];
		}
	}
	if (lenth != 0)
		return FALSE;
	return TRUE;
}

#define NEXT_BYTE												\
{																\
	if (bitIdx == 0)											\
	{															\
		if (cc[bufIndex] == 0xFF)								\
		{														\
			while (cc[bufIndex + 1] == 0xFF)					\
			{													\
				bufIndex++;										\
				remainInBuf--;									\
			}													\
			if (cc[bufIndex + 1] != 0)							\
				goto sync;										\
		}														\
		else if (cc[bufIndex] == 0 && cc[bufIndex - 1] == 0xFF)	\
		{														\
			bufIndex++;											\
			remainInBuf--;										\
			if (cc[bufIndex] == 0xFF)							\
			{													\
				while (cc[bufIndex + 1] == 0xFF)				\
				{												\
					bufIndex++;									\
					remainInBuf--;								\
				}												\
				if (cc[bufIndex + 1] != 0)						\
					goto sync;									\
			}													\
		}														\
	}															\
}

static inline DWORD DwordSwap(DWORD n)
{
	DWORD ret;
	__asm__("bswap %0": "=r"(ret): "0"(n));
	return ret;
}

/*扫描线开始*/
BOOL readSOS(long fh)
{
	DWORD comNum;
	DWORD i, j;
	int k, l;
	DWORD comID[4], htDcID[4], htAcID[4], blockIdx[4];
	DWORD DCbk[4];
	DWORD dcac[100];
	short dcacUZ[64];
	BYTE imgBlock[64];
	BYTE cc[140];
	int remainInBuf = 0, bitIdx = 0;
	DWORD bufIndex = 0;
	BOOL readOK = FALSE;
	BYTE c;
	BYTE cv;
	DECODE_PARAM param;
	BOOL retVal;
	FSseek(fh, 2, FS_SEEK_CUR);
	FSread(fh, &c, 1);
	if (c < 1 || c > 4)
		return FALSE;
	comNum = c;
	for (i = 0; i < comNum; i++)
	{
		FSread(fh, &c, 1);
		if (c > 5)
			return FALSE;
		comID[i] = c;
		FSread(fh, &c, 1);
		htDcID[i] = c >> 4;
		htAcID[i] = c & 0x0F;
		if (htDcID[i] > 4 || htAcID[i] > 4)
			return FALSE;
	}
	FSseek(fh, 3, FS_SEEK_CUR);
	blockIdx[0] = 0, blockIdx[1] = 0, blockIdx[2] = 0, blockIdx[3] = 0;
	DCbk[0] = 0, DCbk[1] = 0, DCbk[2] = 0, DCbk[3] = 0;
	param.stream = cc;
startScan:
	for (;;)
	{
		for (i = 0; i < comNum; i++)
		{
			HUFFMAN_NODE *htD, *htA;
			DWORD ID;
			htD = htDC[htDcID[i]];
			htA = htAC[htAcID[i]];
			ID = comID[i];
			for (j = 0; j < com[ID].h * com[ID].v; j++)
			{
				DWORD *q;
				for (l = 0; l < 64;)
				{
					if (remainInBuf <= 30 && readOK == FALSE)
					{
						for (k = 0; k < remainInBuf; k++)
							cc[k] = cc[bufIndex + k];
						bufIndex = 0;
						remainInBuf += FSread(fh, cc + remainInBuf, 100);
//						if (k != 100)
//							readOK = TRUE;
					}
					param.bufIdx = bufIndex;
					param.bitIdx = bitIdx;
					param.remain = remainInBuf;
					if (l == 0)
						retVal = HufDecode(htD, &param, &cv);
					else
						retVal = HufDecode(htA, &param, &cv);
					bitIdx = param.bitIdx;
					bufIndex = param.bufIdx;
					remainInBuf = param.remain;
					bitIdx = bitIdx % 8;
					if (remainInBuf <= 0)
						return FALSE;
					if (retVal == TRUE)
					{
						DWORD u, zn, gn, bno;
						BYTE ct[4];
						int bn, bn1, it;
						if (l == 0)
						{
							if (cv == 0)
								u = 0;
							else
							{
								bn = (int)cv;
								it = 0;
								bno = bitIdx;
								for (;;)
								{
									NEXT_BYTE
									if (bn <= 0 || it >= 4)
										break;
									bn1 = 8 - bitIdx;
									ct[it] = cc[bufIndex];
									if (bn >= bn1)
										bufIndex++, bitIdx = 0, remainInBuf--;
									else
										bitIdx += bn;
									bn -= bn1;
									it++;
								}
								u = DwordSwap(*((DWORD*)ct)) << bno;
								if (((long)u) > 0)
									u = 0 - ((~u) >> (32 - cv));
								else
									u = u >> (32 - cv);
							}
							dcac[0] = DCbk[i] = DCbk[i] + u;
							l++;
						}
						else
						{
							if (cv == 0)
							{
								while (l < 64)
								{
									dcac[l] = 0;
									l++;
								}
							}
							else
							{
								DWORD m;
								if (cv == 0xF0)
								{
									for (m = 0; m < 16; m++)
									{
										dcac[l] = 0;
										l++;
									}
								}
								else
								{
									zn = cv >> 4;
									gn = cv & 0x0F;
									for (m = 0; m < zn; m++)
									{
										dcac[l] = 0;
										l++;
									}
									if (gn != 0)
									{
										bn = (int)gn;
										it = 0;
										bno = bitIdx;
										for (;;)
										{
											NEXT_BYTE
											if (bn <= 0 || it >= 4)
												break;
											bn1 = 8 - bitIdx;
											ct[it] = cc[bufIndex];
											if (bn >= bn1)
												bufIndex++, bitIdx = 0, remainInBuf--;
											else
												bitIdx += bn;
											bn -= bn1;
											it++;
										}
										u = DwordSwap(*((DWORD*)ct)) << bno;
										if (((long)u) > 0)
											u = 0 - ((~u) >> (32 - gn));
										else
											u = u >> (32 - gn);
										dcac[l] = u;
										l++;
									}
								}
							}
						}
					}
					else
						goto sync;
				}
				if (l > 64)
					goto sync;
				q = qt[com[ID].qtID];
				for (k = 0; k < 64; k++)
					dcac[k] = ((long)dcac[k]) * ((long)q[k]);
				for (k = 0; k < 64; k++)
					dcacUZ[zigzag[k]] = (short)((long)dcac[k]);
				idctInt(dcacUZ, imgBlock);
				storeBlock(ID, blockIdx[i], imgBlock, image[i]);
				blockIdx[i]++;
			}
		}
	}
sync:
	for (;;)
	{
		BYTE c1, c2;
		c1 = cc[bufIndex], c2 = cc[bufIndex + 1];
		if (c1 == 0xFF && c2 != 0xFF && c2 != 0)
		{
			if (c2 < 0xD0 || c2 > 0xD7)
			{
				FSseek(fh, -remainInBuf, FS_SEEK_CUR);
				return TRUE;
			}
			else
			{
				DCbk[0] = 0, DCbk[1] = 0, DCbk[2] = 0, DCbk[3] = 0;
				bufIndex += 2, bitIdx = 0, remainInBuf -= 2;
				break;
			}
		}
		else
		{
			if (remainInBuf <= 30 && readOK == FALSE)
			{
				for (k = 0; k < remainInBuf; k++)
					cc[k] = cc[bufIndex + k];
				bufIndex = 0;
				remainInBuf += FSread(fh, cc + remainInBuf, 100);
//				if (k != 100)
//					readOK = TRUE;
			}
			bufIndex++, remainInBuf--;
			if (remainInBuf <= 0)
				return TRUE;
			c1 = cc[bufIndex], c2 = cc[bufIndex + 1];
		}
	}
	goto startScan;
}

static inline long fixedMul(long a, long b)
{
	long ret;
	__asm__("imul %2;shrd $16, %3, %0": "=a"(ret): "0"(a), "b"(b), "r"(0));
	return ret;
}

void YCbCrToRGB()
{
	DWORD j, k;
	BYTE *p1, *p2, *p3;
	long y, cb, cr, rr, gg, bb;
	if (numOfCOM != 3)
		return;
	p1 = image[0], p2 = image[1], p3 = image[2];
	for (j = 0; j < bufHeight; j++)
	{
		for (k = 0; k < bufWidth; k++)
		{
			y = LONG_TO_FIXED((long)(*p1));
			cb = LONG_TO_FIXED((long)(*p2)) - 0x800000;
			cr = LONG_TO_FIXED((long)(*p3)) - 0x800000;
			rr = FIXED_TO_LONG(y + fixedMul(91881, cr));
			gg = FIXED_TO_LONG(y - fixedMul(22554, cb) - fixedMul(46802, cr));
			bb = FIXED_TO_LONG(y + fixedMul(116130, cb));
			if (rr > 255)
				rr = 255;
			else if (rr < 0)
				rr = 0;
			if (gg > 255)
				gg = 255;
			else if (gg < 0)
				gg = 0;
			if (bb > 255)
				bb = 255;
			else if (bb < 0)
				bb = 0;
			*p1++ = (BYTE)rr;
			*p2++ = (BYTE)gg;
			*p3++ = (BYTE)bb;
		}
	}
}

BOOL JpegOpen(const char *fileName)
{
	long fh;
	BYTE seg[2];

	if ((fh = FSopen(fileName, FS_OPEN_READ)) < 0)
	{
		CUIPutS("打开文件出错！\n");
		return FALSE;
	}
	FSread(fh, seg, 2);
	if (seg[0] != 0xFF || seg[1] != 0xD8)
	{
		FSclose(fh);
		CUIPutS("JPEG文件格式错！\n");
		return FALSE;
	}
	for (;;)
	{
		if (FSread(fh, seg, 2) < 2)
			break;
		if (seg[0] != 0xFF)
			break;
		while (seg[1] == 0xFF)
			FSread(fh, &seg[1], 1);
		switch (seg[1])
		{
		case 0x01:
		case 0xD0:
		case 0xD1:
		case 0xD2:
		case 0xD3:
		case 0xD4:
		case 0xD5:
		case 0xD6:
		case 0xD7:
			break;
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
		case 0xCF:
			FSclose(fh);
			CUIPutS("不支持的JPEG文件！\n");
			return FALSE;
		case 0xDB:	/*量化表*/
			if (readQTable(fh) == FALSE)
				goto error;
			break;
		case 0xC0:	/*帧开始*/
			if (readSOF0(fh) == FALSE)
				goto error;
			break;
		case 0xC4:	/*霍夫曼表*/
			if (readHTable(fh) == FALSE)
				goto error;
			break;
		case 0xDA:	/*扫描线开始*/
			if (readSOS(fh) == FALSE)
				goto error;
			break;
		case 0xD9:	/*图像结束*/
			FSclose(fh);
			YCbCrToRGB();
			return TRUE;
		default:
			FSread(fh, seg, 2);
			FSseek(fh, ICHG(seg[0], seg[1]) - 2, FS_SEEK_CUR);
		}
	}
error:
	FSclose(fh);
	CUIPutS("JPEG文件格式错！\n");
	return FALSE;
}

void JpegClose()
{
	int i;
	for (i = 0; i < 4; i++)
	{
		HufClearNode(htDC[i]);
		htDC[i] = NULL;
		HufClearNode(htAC[i]);
		htAC[i] = NULL;
	}
	for (i = 0; i < 5; i++)
	if (image[i] != NULL)
	{
//		free(image[i]);
		image[i] = NULL;
	}
}

int main(int argc, char *argv[])
{
	BYTE *p1, *p2, *p3;
	DWORD i, j, *buf, *bufp;

	if (argc <= 1)
	{
		CUIPutS(
			"参数:JPG文件路径\n"
			"本程序集成JPEG解码器，可以显示JPEG格式的图像。\n"
			);
		return NO_ERROR;
	}
	/*初始化动态内存分配*/
	if (KMapUserAddr(&fm, FDAT_SIZ) != NO_ERROR)
		return NO_ERROR;
	if (GDIinit() != NO_ERROR)
	{
		CUIPutS("GDI初始化出错！\n");
		return NO_ERROR;
	}
	CurFm = fm;
	JpegInit();
	if (JpegOpen(argv[1]) != FALSE)
	{
		buf = (DWORD*)malloc(imgWidth * sizeof(DWORD));
		if (numOfCOM == 3)
		{
			p1 = image[0];
			p2 = image[1];
			p3 = image[2];
			for (j = 0; j < imgHeight; j++)
			{
				bufp = buf;
				for (i = 0; i < imgWidth; i++)
					*bufp++ = ((((DWORD)p1[i]) << 16) | (((DWORD)p2[i]) << 8) | ((DWORD)p3[i]));
				p1 += bufWidth;
				p2 += bufWidth;
				p3 += bufWidth;
				GDIPutImage(0, j, buf, imgWidth, 1);	/*一行一行输出*/
			}
		}
		else
		{
			p1 = image[0];
			for (j = 0; j < imgHeight; j++)
			{
				bufp = buf;
				for (i = 0; i < imgWidth; i++)
					*bufp++ = ((((DWORD)p1[i]) << 16) | (((DWORD)p1[i]) << 8) | ((DWORD)p1[i]));
				p1 += bufWidth;
				GDIPutImage(0, j, buf, imgWidth, 1);	/*一行一行输出*/
			}
		}
	}
	JpegClose();
	GDIrelease();
	/*回收动态内存分配*/
	KFreeAddr(fm);
	return NO_ERROR;
}
