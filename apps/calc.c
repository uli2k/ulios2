/*	calc.c for ulios application
	作者：孙亮
	功能：计算器程序
	最后修改日期：2011-10-30
*/

#include "../lib/malloc.h"
#include "../lib/gclient.h"

#include "../lib/math.h"

#define WOERR_NOERR		0	// 运算正确
#define WOERR_ARGUMENT	1	// 参数错误
#define WOERR_MEMORY	2	// 内存错误
#define WOERR_VARIABLE	3	// 变量名错误或含有非法字符
#define WOERR_FORMAT	4	// 表达式格式错误
#define WOERR_BRACKETS	5	// 括号不匹配
#define WOERR_DIVZERO	6	// 被除数为零
#define WOERR_MODZERO	7	// 整数运算被模数为零
#define WOERR_FMODZERO	8	// 浮点运算被模数为零
#define WOERR_COMPLEX	9	// 负数的小数次方产生复数
#define WOERR_ARCCOS	10	// 反余弦定义域错误
#define WOERR_ARCSIN	11	// 反正弦定义域错误
#define WOERR_LN		12	// 自然对数定义域错误
#define WOERR_LG		13	// 自然对数定义域错误
#define WOERR_SQR		14	// 开方函数定义域错误

#define WOTYP_END		0
#define WOTYP_START		1
#define WOTYP_BINOP		2
#define WOTYP_FUNC		3
#define WOTYP_NEGA		4
#define WOTYP_NUM		5
#define WOTYP_LBRACK	6
#define WOTYP_RBRACK	7

typedef struct _WONODE
{
	union
	{
		struct
		{
			DWORD lev;
			DWORD(*fun)(double*, double);
		}BinOp;	/*二元运算*/
		struct
		{
			DWORD lev;
			DWORD(*fun)(double*);
		}Func;	/*函数运算*/
		struct
		{
			DWORD lev;
		}Nega;	/*负数运算*/
		double Num;	/*运算数*/
	}node;	/*节点数据*/
	DWORD type;	/*节点类型标志*/
	struct _WONODE *nxt;	/*next指针*/
}WONODE;	/*计算类型节点*/

#define CTRLCOU		3
#define BINOPCOU	7
#define FUNCCOU		14
#define KEYWORDCOU	(CTRLCOU + BINOPCOU + FUNCCOU)
#define NEGAID		4

#define NEGALEV		3
#define FUNCLEV		4
#define LEVCOU		5

#define GRA_MODE_NONE	0
#define GRA_MODE_2DGRA	1
#define GRA_MODE_3DGRA	2
#define GRA_MODE_4DGRA	3

const char *Keyword[] =	/*关键字数组*/
{
	"(",	")",	" ",	/*3个控制字符*/
	"+",	"-",	"*",	"/",	"%",	"#",	"^",	/*7个二元运算符*/
	"abs",	"acos",	"asin",	"atan",	"cos",	"cosh",	"exp",
	"log",	"ln",	"sin",	"sinh",	"sqr",	"tan",	"tanh",	/*14个函数*/
};

DWORD WoAdd(double *x, double y)
{
	*x += y;
	return WOERR_NOERR;
}
DWORD WoDec(double *x, double y)
{
	*x -= y;
	return WOERR_NOERR;
}
DWORD WoMul(double *x, double y)
{
	*x *= y;
	return WOERR_NOERR;
}
DWORD WoDiv(double *x, double y)
{
	if (y == 0.0)
		return WOERR_DIVZERO;
	*x /= y;
	return WOERR_NOERR;
}
DWORD WoMod(double *x, double y)
{
	if ((long)y == 0)
		return WOERR_MODZERO;
	*x = (long)*x % (long)y;
	return WOERR_NOERR;
}
DWORD WoFmod(double *x, double y)
{
	if (y == 0.0)
		return WOERR_FMODZERO;
	*x = fmod(*x, y);
	return WOERR_NOERR;
}
DWORD WoPow(double *x, double y)
{
	if (*x < 0.0 && y != floor(y))
		return WOERR_COMPLEX;
	*x = pow(*x, y);
	return WOERR_NOERR;
}

DWORD WoAbs(double *x)
{
	*x = fabs(*x);
	return WOERR_NOERR;
}
DWORD WoAcos(double *x)
{
	if (*x < -1.0 || *x > 1.0)
		return WOERR_ARCCOS;
	*x = acos(*x);
	return WOERR_NOERR;
}
DWORD WoAsin(double *x)
{
	if (*x < -1.0 || *x > 1.0)
		return WOERR_ARCSIN;
	*x = asin(*x);
	return WOERR_NOERR;
}
DWORD WoAtan(double *x)
{
	*x = atan(*x);
	return WOERR_NOERR;
}
DWORD WoCos(double *x)
{
	*x = cos(*x);
	return WOERR_NOERR;
}
DWORD WoCosh(double *x)
{
	*x = cosh(*x);
	return WOERR_NOERR;
}
DWORD WoExp(double *x)
{
	*x = exp(*x);
	return WOERR_NOERR;
}
DWORD WoLog(double *x)
{
	if (*x <= 0.0)
		return WOERR_LG;
	*x = log10(*x);
	return WOERR_NOERR;
}
DWORD WoLn(double *x)
{
	if (*x <= 0.0)
		return WOERR_LN;
	*x = log(*x);
	return WOERR_NOERR;
}
DWORD WoSin(double *x)
{
	*x = sin(*x);
	return WOERR_NOERR;
}
DWORD WoSinh(double *x)
{
	*x = sinh(*x);
	return WOERR_NOERR;
}
DWORD WoSqr(double *x)
{
	if (*x < 0.0)
		return WOERR_SQR;
	*x = sqrt(*x);
	return WOERR_NOERR;
}
DWORD WoTan(double *x)
{
	*x = tan(*x);
	return WOERR_NOERR;
}
DWORD WoTanh(double *x)
{
	*x = tanh(*x);
	return WOERR_NOERR;
}

DWORD (*WoBinOp[])(double*, double) =	// 7个二元运算符
{	WoAdd,	WoDec,	WoMul,	WoDiv,	WoMod,	WoFmod,	WoPow};
const DWORD WoBinLev[] =	//7个二元运算的级别
{	0,		0,		1,		1,		1,		1,		2};
DWORD (*WoFunc[])(double*) =	// 14个函数
{	WoAbs,	WoAcos,	WoAsin,	WoAtan,	WoCos,	WoCosh,	WoExp,	WoLog,	WoLn,	WoSin,	WoSinh,	WoSqr,	WoTan,	WoTanh};

// 取得后一个运算的级别
DWORD NxtLev(const WONODE *wol)
{
	while (wol)
	{
		switch (wol->type)
		{
		case WOTYP_BINOP:
			return wol->node.BinOp.lev;
		case WOTYP_FUNC:
			return wol->node.Func.lev;
		case WOTYP_NEGA:
			return wol->node.Nega.lev;
		}
		wol = wol->nxt;
	}
	return 0;
}

// 字符串转化为双精度
double atof(const char *str)
{
	double res, step;

	res = 0.0;
	while (*str >= '0' && *str <= '9')
	{
		res = res * 10.0 + (*str - '0');
		str++;
	}
	if (*str != '.')
		return res;
	str++;
	step = 1.0;
	while (*str >= '0' && *str <= '9')
	{
		step *= 0.1;
		res += step * (*str - '0');
		str++;
	}
	return res;
}

// 测试格式是否正确，并设置类型标志
DWORD ChkFmtErr(DWORD id, DWORD *type)
{
	static const DWORD FrontErr0[] = {WOTYP_START,	WOTYP_BINOP,	WOTYP_FUNC,		WOTYP_NEGA,		WOTYP_LBRACK,	WOTYP_END};
	static const DWORD FrontErr1[] = {WOTYP_END};
	static const DWORD FrontErr2[] = {WOTYP_START,	WOTYP_BINOP,	WOTYP_FUNC,		WOTYP_NEGA,		WOTYP_LBRACK,	WOTYP_END};
	static const DWORD FrontErr3[] = {WOTYP_FUNC,	WOTYP_NUM,		WOTYP_RBRACK,	WOTYP_END};
	static const DWORD FrontErr4[] = {WOTYP_BINOP,	WOTYP_FUNC,		WOTYP_NEGA,		WOTYP_NUM,		WOTYP_RBRACK,	WOTYP_END};
	static const DWORD FrontErr5[] = {WOTYP_FUNC,	WOTYP_NUM,		WOTYP_RBRACK,	WOTYP_END};
	static const DWORD FrontErr6[] = {WOTYP_NUM,	WOTYP_RBRACK,	WOTYP_END};
	static const DWORD FrontErr7[] = {WOTYP_BINOP,	WOTYP_FUNC,		WOTYP_NEGA,		WOTYP_LBRACK,	WOTYP_END};
	static const DWORD *FrontErr[] = {FrontErr0, FrontErr1, FrontErr2, FrontErr3, FrontErr4, FrontErr5, FrontErr6, FrontErr7};	// 之前不应出现的类型

	const DWORD *typep;
	for (typep = FrontErr[id]; *typep != WOTYP_END; typep++)
		if (*typep == *type)	// 匹配到不应出现的类型
			return TRUE;	// 检查到错误
	*type = id;
	return FALSE;	// 没有错误
}

//	测试表达式关键字，并取得关键字
DWORD ChkKeyErr(const char **expre, DWORD *id, const char *variable[], DWORD varn)
{
	DWORD i;
	// 测试已有关键字
	for (i = 0; i < KEYWORDCOU; i++)
	{
		const char *curkey = Keyword[i], *cure = *expre;
		while (*curkey)
			if (*cure++ != *curkey++)
				goto nxtkey;
		*id = i;
		*expre = cure;
		return FALSE;
nxtkey:	continue;
	}
	// 测试变量关键字
	for (i = 0; i < varn; i++)
	{
		const char *curkey = variable[i], *cure = *expre;
		while (*curkey)
			if (*cure++ != *curkey++)
				goto nxtvar;
		*id = i + KEYWORDCOU;
		*expre = cure;
		return FALSE;
nxtvar:	continue;
	}
	// 测试数字
	if (((**expre) >= '0' && (**expre) <= '9') || (**expre) == '.')
	{
		*id = (~0);
		return FALSE;
	}
	return TRUE;
}

// 进行字符串表达式解析计算
DWORD workout(const char *expression,	// 输入：表达式字符串
			  const char *variable[],	// 输入：变量名称数组，varn为0时可为NULL
			  const double value[],		// 输入：变量值数组，varn为0时可为NULL
			  DWORD varn,				// 输入：变量数量
			  double *res)				// 输出：计算结果
{
	WONODE *wol = NULL, *curwo = NULL;	// 表达式链表，当前计算节点
	DWORD type = WOTYP_START;	// 当前运算类型
	DWORD lev = 0;	// 当前运算级别
	WONODE wobuf[128], *wop;

	if (expression == NULL || res == NULL)
		return WOERR_ARGUMENT;
	if (varn && (variable == NULL || value == NULL))
		return WOERR_ARGUMENT;
	wop = wobuf;
	while (*expression)
	{
		DWORD id;
		WONODE *tmpwo;

		if (ChkKeyErr(&expression, &id, variable, varn))	// 匹配关键字
			return WOERR_VARIABLE;
		// 控制字符处理
		if (id == 0)	// (
		{
			if (ChkFmtErr(WOTYP_LBRACK, &type))
				return WOERR_FORMAT;
			lev += LEVCOU;
			continue;
		}
		if (id == 1)	// )
		{
			if (ChkFmtErr(WOTYP_RBRACK, &type))
				return WOERR_FORMAT;
			lev -= LEVCOU;
			if (lev < 0)
				return WOERR_BRACKETS;
			continue;
		}
		if (id == 2)	// 空格
			continue;
		// 分配运算节点
		tmpwo = wop++;
		tmpwo->nxt = NULL;
		if (wol == NULL)
			wol = tmpwo;
		else
			curwo->nxt = tmpwo;
		curwo = tmpwo;
		// 运算字符处理
		if (id == NEGAID && (type == WOTYP_START || type == WOTYP_LBRACK))	// 判断负号
		{
			if (ChkFmtErr(WOTYP_NEGA, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.Nega.lev = lev + NEGALEV;
			continue;
		}
		if (id < CTRLCOU + BINOPCOU)	// 判断二元运算
		{
			if (ChkFmtErr(WOTYP_BINOP, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.BinOp.fun = WoBinOp[id - CTRLCOU];
			tmpwo->node.BinOp.lev = lev + WoBinLev[id - CTRLCOU];
			continue;
		}
		if (id < CTRLCOU + BINOPCOU + FUNCCOU)	// 判断函数
		{
			if (ChkFmtErr(WOTYP_FUNC, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.Func.fun = WoFunc[id - CTRLCOU - BINOPCOU];
			tmpwo->node.Func.lev = lev + FUNCLEV;
			continue;
		}
		if (id < CTRLCOU + BINOPCOU + FUNCCOU + varn)	// 判断变量数值
		{
			if (ChkFmtErr(WOTYP_NUM, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.Num = value[id - CTRLCOU - BINOPCOU - FUNCCOU];
			continue;
		}
		if (ChkFmtErr(WOTYP_NUM, &type))	// 判断常数数值
			return WOERR_FORMAT;
		tmpwo->type = type;
		tmpwo->node.Num = atof(expression);
		while ((*expression >= '0' && *expression <= '9') || *expression == '.')
			expression++;
	}
	if (ChkFmtErr(WOTYP_END, &type))	// 检查结尾状态
		return WOERR_FORMAT;
	if (lev)	// 检查括号匹配状况
		return WOERR_BRACKETS;

	while (wol->nxt)	// 进行链表收缩
	{
		DWORD prelev = 0;
		WONODE *prewo = NULL;
		curwo = wol;
		while (curwo)
		{
			switch(curwo->type)
			{
			case WOTYP_BINOP:
				lev = curwo->node.BinOp.lev;
				break;
			case WOTYP_FUNC:
				lev = curwo->node.Func.lev;
				break;
			case WOTYP_NEGA:
				lev = curwo->node.Nega.lev;
				break;
			case WOTYP_NUM:
				prewo = curwo;
				curwo = curwo->nxt;
				continue;
			}
			if (lev >= prelev && lev >= NxtLev(curwo->nxt))	// 检查是否比前后运算级别高
			{
				DWORD ErrCode;
				switch(curwo->type)
				{
				case WOTYP_BINOP:
					ErrCode = curwo->node.BinOp.fun(&prewo->node.Num, curwo->nxt->node.Num);
					if (ErrCode != WOERR_NOERR)
						return ErrCode;
					curwo = prewo->nxt = curwo->nxt->nxt;
					break;
				case WOTYP_FUNC:
				case WOTYP_NEGA:
					if (curwo->type == WOTYP_NEGA)
						curwo->node.Num = -curwo->nxt->node.Num;
					else
					{
						ErrCode = curwo->node.Func.fun(&curwo->nxt->node.Num);
						if (ErrCode != WOERR_NOERR)
							return ErrCode;
						curwo->node.Num = curwo->nxt->node.Num;
					}
					curwo->type = WOTYP_NUM;
					prewo = curwo;
					curwo = prewo->nxt = curwo->nxt->nxt;
					break;
				}
				prelev = lev;
			}
			else
			{
				prewo = curwo;
				curwo = curwo->nxt;
			}
		}
	}
	*res = wol->node.Num;

	return WOERR_NOERR;
}

const char *WoErrStr[] = {
	NULL,
	"参数错误",
	"内存错误",
	"变量名错误或含有非法字符",
	"表达式格式错误",
	"括号不匹配",
	"被除数为零",
	"整数运算被模数为零",
	"浮点运算被模数为零",
	"负数的小数次方产生复数",
	"反余弦定义域错误",
	"反正弦定义域错误",
	"自然对数定义域错误",
	"自然对数定义域错误",
	"开方函数定义域错误"
};

/*双精度转化为数字*/
char *ftoa(char *buf, double n)
{
	char *p, *q;
	DWORD intn;

	if (n < 0.0)
	{
		*buf++ = '-';
		n = -n;
	}
	intn = (DWORD)n;	/*处理整数部分*/
	n -= intn;
	q = buf;
	do
	{
		*buf++ = intn % 10 + '0';
		intn /= 10;
	}
	while (intn);
	p = buf - 1;
	while (p > q)	/*翻转整数字符串*/
	{
		char c = *q;
		*q++ = *p;
		*p-- = c;
	}
	if (n >= 1e-16)
	{
		*buf++ = '.';
		q = buf;
		do
		{
			n *= 10.0;
			intn = (DWORD)n;
			n -= intn;
			*buf++ = intn + '0';
		}
		while (buf - q < 16 && n >= 1e-16);
	}
	*buf = '\0';
	return buf;
}

#define WND_WIDTH	244
#define WND_HEIGHT	244
#define EDT_WIDTH	236
#define EDT_HEIGHT	20
#define BTN_WIDTH	36
#define BTN_HEIGHT	24
#define GRA_WIDTH	288
#define GRA_HEIGHT	216
#define SIDE_X		3	/*左侧与客户区边距*/
#define SIDE_Y		4	/*上侧与客户区边距*/

CTRL_WND *wnd;
CTRL_SEDT *edt;
CTRL_BTN *btns[42];
long CurGraMode;
UDI_AREA GraArea;
long p2d[GRA_WIDTH];
#define P3D_WID	51
float p3d[P3D_WID][P3D_WID], a, b;
long x0, y0;
#define P4D_LEN	101
float p4d[P4D_LEN][P3D_WID][P3D_WID];

static const char *findvar(const char *str, char c)
{
	while (*str)
	{
		if (*str == c && (*(str + 1) < 'a' || *(str + 1) > 'z'))
			return str;
		str++;
	}
	return NULL;
}

void SetGraMode(long GraMode)
{
	long i;
	if (GraMode > GRA_MODE_NONE)
	{
		GCBtnSetText(btns[4], "<<");
		GCWndSetSize(wnd, wnd->obj.x, wnd->obj.y, WND_WIDTH + GRA_WIDTH + 4, WND_HEIGHT);
		GCSetArea(&GraArea, GRA_WIDTH, GRA_HEIGHT, &wnd->client, WND_WIDTH - 1, SIDE_Y);
	}
	else
	{
		GCBtnSetText(btns[4], ">>");
		GCWndSetSize(wnd, wnd->obj.x, wnd->obj.y, WND_WIDTH, WND_HEIGHT);
	}
	CurGraMode = GraMode;
	GCSetArea(&edt->obj.uda, EDT_WIDTH, EDT_HEIGHT, &wnd->obj.uda, edt->obj.x, edt->obj.y);
	GCSedtDefDrawProc(edt);
	for (i = 0; i < 42; i++)
	{
		GCSetArea(&(btns[i]->obj.uda), BTN_WIDTH, BTN_HEIGHT, &wnd->obj.uda, btns[i]->obj.x, btns[i]->obj.y);
		GCBtnDefDrawProc(btns[i]);
	}
	GUIpaint(wnd->obj.gid, 4, 24, WND_WIDTH - 4 * 2, WND_HEIGHT - 20 - 4 * 2);	/*绘制完后提交*/
}

/*长整形转化为数字*/
char *Itoa(char *buf, long n)
{
	char *p, *q;

	if (n < 0)
	{
		n = -n;
		*buf++ = '-';
	}
	q = p = buf;
	do
	{
		*p++ = '0' + (char)(n % 10);
		n /= 10;
	}
	while (n);
	buf = p;	/*确定字符串尾部*/
	*p-- = '\0';
	while (p > q)	/*翻转字符串*/
	{
		char c = *q;
		*q++ = *p;
		*p-- = c;
	}
	return buf;
}

void Calc2D()
{
	static const char *varnam[] = {"x"};
	double x, y;
	long i;
	for (i = 0; i < GRA_WIDTH; i++)
	{
		x = 0.05 * (i - GRA_WIDTH / 2);	//x缩小转换
		if (workout(edt->text, varnam, &x, 1, &y) == WOERR_NOERR)	//代入未知数计算
			p2d[i] = (GRA_HEIGHT / 2) - (long)(20.0 * y);	//y放大转换
		else
			p2d[i] = -1;
	}
}

void Draw2D()
{
	long i;
	GCFillRect(&GraArea, 0, 0, GRA_WIDTH, GRA_HEIGHT, 0xFFFFFF);	// 清屏
	for (i = 4; i < GRA_WIDTH; i += 20)
		GCFillRect(&GraArea, i, 0, 1, GRA_HEIGHT, 0xFFFF00);
	for (i = 8; i < GRA_HEIGHT; i += 20)
		GCFillRect(&GraArea, 0, i, GRA_WIDTH, 1, 0xFFFF00);
	GCFillRect(&GraArea, GRA_WIDTH / 2, 0, 1, GRA_HEIGHT, 0xFF0000);
	GCFillRect(&GraArea, 0, GRA_HEIGHT / 2, GRA_WIDTH, 1, 0xFF0000);
	for (i = -7; i <= 7; i++)
	{
		char buf[4];
		Itoa(buf, i);
		GCDrawStr(&GraArea, i * 20 + GRA_WIDTH / 2 - 6, GRA_HEIGHT / 2 + 1, buf, 0xFF0000);
	}
	for (i = -5; i <= 5; i++)
	{
		char buf[4];
		Itoa(buf, i);
		GCDrawStr(&GraArea, GRA_WIDTH / 2 - 6, i * 20 + GRA_HEIGHT / 2 + 1, buf, 0xFF0000);
	}
	GCDrawAscii(&GraArea, GRA_WIDTH - 6, GRA_HEIGHT / 2 + 1, 'X', 0);
	GCDrawAscii(&GraArea, GRA_WIDTH / 2 - 6, 1, 'Y', 0);
	for (i = 1; i < GRA_WIDTH; i++)	//点连点画出函数图像
	{
		if (p2d[i - 1] != -1 && p2d[i] != -1)	//异常点不画
			GCDrawLine(&GraArea, i - 1, p2d[i - 1], i, p2d[i], 0);
	}
	GUIpaint(wnd->obj.gid, WND_WIDTH, 20 + SIDE_Y, GRA_WIDTH, GRA_HEIGHT);	/*绘制完后提交*/
}

void Calc3D()
{
	static const char *varnam[] = {"x", "y"};
	double val[2], z;
	long i, j;
	for (j = 0; j < P3D_WID; j++)
	{
		val[1] = 0.2 * j - 5.0;	//y缩小转换
		for (i = 0; i < P3D_WID; i++)
		{
			val[0] = 0.2 * i - 5.0;	//x缩小转换
			if (workout(edt->text, varnam, val, 2, &z) == WOERR_NOERR)	//代入未知数计算
				p3d[i][j] = (float)(5.0 * z);	//z放大转换
			else
				p3d[i][j] = 999999.f;
		}
	}
}

void Draw3D()
{
	long i, j;
	long x3d[P3D_WID][P3D_WID], y3d[P3D_WID][P3D_WID];
	float sa, ca, sb, cb;
	sa = (float)sin(a);
	ca = (float)cos(a);
	sb = (float)sin(b);
	cb = (float)cos(b);
	for (j = 0; j < P3D_WID; j++)	//三维变换
	{
		for (i = 0; i < P3D_WID; i++)
		{
			float x, y, z;
			x = (i - P3D_WID / 2) * ca + (j - P3D_WID / 2) * sa;
			y = (j - P3D_WID / 2) * ca - (i - P3D_WID / 2) * sa;
			z = p3d[i][j] * cb + x * sb;
			x = (x * cb - p3d[i][j] * sb) * 10.f;
			x3d[i][j] = GRA_WIDTH / 2 + (long)(6000.f * y / (1200.f - x));
			y3d[i][j] = GRA_HEIGHT / 2 - (long)(6000.f * z / (1200.f - x));
		}
	}
	GCFillRect(&GraArea, 0, 0, GRA_WIDTH, GRA_HEIGHT, 0xFFFFFF);	// 清屏
	for (j = 0; j < P3D_WID; j++)	//点连点画出函数图像
	{
		for (i = 1; i < P3D_WID; i++)
		{
			if (p3d[i][j] == 999999.f)	//异常点不画
				continue;
			if (p3d[i - 1][j] != 999999.f)
				GCDrawLine(&GraArea, x3d[i - 1][j], y3d[i - 1][j], x3d[i][j], y3d[i][j], 0);
			if (p3d[j][i - 1] != 999999.f)
				GCDrawLine(&GraArea, x3d[j][i - 1], y3d[j][i - 1], x3d[j][i], y3d[j][i], 0);
		}
	}
	GUIpaint(wnd->obj.gid, WND_WIDTH, 20 + SIDE_Y, GRA_WIDTH, GRA_HEIGHT);	/*绘制完后提交*/
}

void Calc4D()
{
	static const char *varnam[] = { "x", "y", "t" };
	double val[3], z;
	long i, j, k;
	for (k = 0; k < P4D_LEN; k++)
	{
		val[2] = 0.1 * k - 5.0;	//t缩小转换
		for (j = 0; j < P3D_WID; j++)
		{
			val[1] = 0.2 * j - 5.0;	//y缩小转换
			for (i = 0; i < P3D_WID; i++)
			{
				val[0] = 0.2 * i - 5.0;	//x缩小转换
				if (workout(edt->text, varnam, val, 3, &z) == WOERR_NOERR)	//代入未知数计算
					p4d[k][i][j] = (float)(5.0 * z);	//z放大转换
				else
					p4d[k][i][j] = 999999.f;
			}
		}
	}
}

void Draw4D()
{
	char buf[8];
	long i, j, k;
	long x3d[P3D_WID][P3D_WID], y3d[P3D_WID][P3D_WID];
	float sa, ca, sb, cb;
	strcpy(buf, "t=");
	sa = (float)sin(a);
	ca = (float)cos(a);
	sb = (float)sin(b);
	cb = (float)cos(b);
	for (k = 0; k < P4D_LEN; k++)
	{
		for (j = 0; j < P3D_WID; j++)	//三维变换
		{
			for (i = 0; i < P3D_WID; i++)
			{
				float x, y, z;
				x = (i - P3D_WID / 2) * ca + (j - P3D_WID / 2) * sa;
				y = (j - P3D_WID / 2) * ca - (i - P3D_WID / 2) * sa;
				z = p4d[k][i][j] * cb + x * sb;
				x = (x * cb - p4d[k][i][j] * sb) * 10.f;
				x3d[i][j] = GRA_WIDTH / 2 + (long)(6000.f * y / (1200.f - x));
				y3d[i][j] = GRA_HEIGHT / 2 - (long)(6000.f * z / (1200.f - x));
			}
		}
		GCFillRect(&GraArea, 0, 0, GRA_WIDTH, GRA_HEIGHT, 0xFFFFFF);	// 清屏
		for (j = 0; j < P3D_WID; j++)	//点连点画出函数图像
		{
			for (i = 1; i < P3D_WID; i++)
			{
				if (p4d[k][i][j] == 999999.f)	//异常点不画
					continue;
				if (p4d[k][i - 1][j] != 999999.f)
					GCDrawLine(&GraArea, x3d[i - 1][j], y3d[i - 1][j], x3d[i][j], y3d[i][j], 0);
				if (p4d[k][j][i - 1] != 999999.f)
					GCDrawLine(&GraArea, x3d[j][i - 1], y3d[j][i - 1], x3d[j][i], y3d[j][i], 0);
			}
		}
		ftoa(buf + 2, 0.1 * k - 5.0);
		GCDrawStr(&GraArea, 0, GraArea.height - GCCharHeight, buf, 0);
		GUIpaint(wnd->obj.gid, WND_WIDTH, 20 + SIDE_Y, GRA_WIDTH, GRA_HEIGHT);	/*绘制完后提交*/
	}
}

/*编辑框回车处理*/
void EdtEnterProc(CTRL_SEDT *edt)
{
	if (findvar(edt->text, 't'))	/*有变量t,绘制四维图像(动画)*/
	{
		SetGraMode(GRA_MODE_4DGRA);
		Calc4D();
		Draw4D();
	}
	else if (findvar(edt->text, 'y'))	/*有变量y,绘制三维图像*/
	{
		SetGraMode(GRA_MODE_3DGRA);
		Calc3D();
		Draw3D();
	}
	else if (findvar(edt->text, 'x'))	/*有变量x,绘制二维图像*/
	{
		SetGraMode(GRA_MODE_2DGRA);
		Calc2D();
		Draw2D();
	}
	else	/*没有变量,计算数值*/
	{
		char buf[128];
		double wores;
		DWORD res;

		if (CurGraMode > GRA_MODE_NONE)
			SetGraMode(GRA_MODE_NONE);
		if ((res = workout(edt->text, NULL, NULL, 0, &wores)) != WOERR_NOERR)
			strcpy(buf, WoErrStr[res]);
		else
			ftoa(buf, wores);
		GCSedtSetText(edt, buf);
	}
}

/*等号按钮处理*/
void EquPressProc(CTRL_BTN *btn)
{
	EdtEnterProc(edt);
}

/*退格按钮处理*/
void BksPressProc(CTRL_BTN *btn)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];

	ptid.ProcID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_KEY;
	data[1] = '\b';
	data[GUIMSG_GOBJ_ID] = (DWORD)edt;
	data[MSG_RES_ID] = NO_ERROR;
	edt->obj.MsgProc(ptid, data);
}

/*清除按钮处理*/
void ClrPressProc(CTRL_BTN *btn)
{
	GCSedtSetText(edt, NULL);
}

/*退出按钮处理*/
void QutPressProc(CTRL_BTN *btn)
{
	KExitProcess(NO_ERROR);
}

/*图像按钮处理*/
void GrpPressProc(CTRL_BTN *btn)
{
	SetGraMode(-CurGraMode);
	if (CurGraMode == GRA_MODE_2DGRA)
		Draw2D();
	else if (CurGraMode == GRA_MODE_3DGRA)
		Draw3D();
	else if (CurGraMode == GRA_MODE_4DGRA)
		Draw4D();
}

/*其余按钮处理*/
void OthPressProc(CTRL_BTN *btn)
{
	GCSedtAddText(edt, btn->text);
}

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	static const char *btnam[] =
	{
		"=",	"退格",	"清除",	"退出",	">>",	"t",
		"sin",	"cos",	"tan",	"asin",	"acos",	"atan",
		"sinh",	"cosh",	"tanh",	"exp",	"log",	"ln",
		"7",	"8",	"9",	"/",	"abs",	"sqr",
		"4",	"5",	"6",	"*",	"%",	"#",
		"1",	"2",	"3",	"-",	"(",	")",
		"0",	".",	"^",	"+",	"x",	"y",
	};
	static void (*BtnPressProc[5])(CTRL_BTN *btn) =
	{EquPressProc, BksPressProc, ClrPressProc, QutPressProc, GrpPressProc};
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			long i, CliX, CliY;
			CTRL_ARGS args;
			GCWndGetClientLoca(wnd, &CliX, &CliY);
			args.width = EDT_WIDTH;
			args.height = EDT_HEIGHT;
			args.x = CliX + SIDE_X;
			args.y = CliY + SIDE_Y;
			args.style = 0;
			args.MsgProc = NULL;
			GCSedtCreate(&edt, &args, wnd->obj.gid, &wnd->obj, NULL, EdtEnterProc);
			args.width = BTN_WIDTH;
			args.height = BTN_HEIGHT;
			args.y = CliY + EDT_HEIGHT + 8;
			for (i = 0; i < 5; i++)
			{
				args.x = CliX + SIDE_X + i * (BTN_WIDTH + 4);
				GCBtnCreate(&btns[i], &args, wnd->obj.gid, &wnd->obj, btnam[i], NULL, BtnPressProc[i]);
			}
			for (; i < 42; i++)
			{
				args.x = CliX + SIDE_X + (i % 6) * (BTN_WIDTH + 4);
				args.y = CliY + EDT_HEIGHT + 8 + (i / 6) * (BTN_HEIGHT + 4);
				GCBtnCreate(&btns[i], &args, wnd->obj.gid, &wnd->obj, btnam[i], NULL, OthPressProc);
			}
		}
		break;
	case GM_MOUSEENTER:
		if (!(data[1] & MUS_STATE_LBUTTON))
			break;
	case GM_LBUTTONDOWN:
		if (CurGraMode == GRA_MODE_3DGRA)
		{
			x0 = (short)(data[5] & 0xFFFF);
			y0 = (short)(data[5] >> 16);
		}
		break;
	case GM_MOUSEMOVE:
		if (data[1] & MUS_STATE_LBUTTON && CurGraMode == GRA_MODE_3DGRA)
		{
			a += (x0 - (short)(data[5] & 0xFFFF)) * 0.007f;
			b += (y0 - (short)(data[5] >> 16)) * 0.007f;
			x0 = (short)(data[5] & 0xFFFF);
			y0 = (short)(data[5] >> 16);
			Draw3D();
		}
		break;
	}
	return GCWndDefMsgProc(ptid, data);
}

int main()
{
	CTRL_ARGS args;
	long res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*设置16MB堆内存*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = WND_WIDTH;
	args.height = WND_HEIGHT;
	args.x = 128;
	args.y = 128;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "可视函数计算器");

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*处理GUI消息*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)wnd)	/*销毁主窗体,退出程序*/
				break;
		}
	}
	return NO_ERROR;
}
