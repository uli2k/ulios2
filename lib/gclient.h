/*	gclient.h for ulios graphical user interface
作者：孙亮
功能：GUI客户端功能库定义
最后修改日期：2011-08-15
*/

#ifndef	_GCLIENT_H_
#define	_GCLIENT_H_

#include "../gui/guiapi.h"

/**********绘图功能定义**********/

#define GC_ERR_LOCATION			-2688	/*坐标错误*/
#define GC_ERR_AREASIZE			-2689	/*尺寸错误*/
#define GC_ERR_OUT_OF_MEM		-2690	/*内存不足*/
#define GC_ERR_INVALID_GUIMSG	-2691	/*非法GUI消息*/
#define GC_ERR_WRONG_ARGS		-2692	/*参数错误*/

typedef struct _UDI_IMAGE
{
	DWORD width, height;	/*尺寸*/
	DWORD buf[];			/*位图缓冲,扩展到width*height大小*/
}UDI_IMAGE;	/*GUI位图*/

typedef struct _UDI_AREA
{
	DWORD width, height;	/*尺寸*/
	DWORD *vbuf;			/*绘图缓冲*/
	struct _UDI_AREA *root;	/*根绘图区*/
}UDI_AREA;	/*GUI绘图区域*/

extern const BYTE *GCfont;
extern DWORD GCwidth, GCheight;
extern DWORD GCCharWidth, GCCharHeight;

/*初始化GC库*/
long GCinit();

/*撤销GC库*/
void GCrelease();

/*设置绘图区域的参数和缓存,第一次设置无根绘图区时先清空uda->vbuf*/
long GCSetArea(UDI_AREA *uda, DWORD width, DWORD height, const UDI_AREA *par, long x, long y);

/*回收绘图区域缓存*/
void GCFreeArea(UDI_AREA *uda);

/*画点*/
long GCPutPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD c);

/*取点*/
long GCGetPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD *c);

/*贴图*/
long GCPutImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h);

/*去背景色贴图*/
long GCPutBCImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h, DWORD bc);

/*截图*/
long GCGetImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h);

/*填充矩形*/
long GCFillRect(UDI_AREA *uda, long x, long y, long w, long h, DWORD c);

/*画线*/
long GCDrawLine(UDI_AREA *uda, long x1, long y1, long x2, long y2, DWORD c);

/*画圆*/
long GCcircle(UDI_AREA *uda, long cx, long cy, long r, DWORD c);

/*绘制汉字*/
long GCDrawHz(UDI_AREA *uda, long x, long y, DWORD hz, DWORD c);

/*绘制ASCII字符*/
long GCDrawAscii(UDI_AREA *uda, long x, long y, DWORD ch, DWORD c);

/*绘制字符串*/
long GCDrawStr(UDI_AREA *uda, long x, long y, const char *str, DWORD c);

/*加载BMP图像文件*/
long GCLoadBmp(char *path, DWORD *buf, DWORD len, DWORD *width, DWORD *height);

/**********GUI客户端自定义消息**********/

#define GM_CLOSE		0x8000	/*关闭窗体消息*/

/**********控件基类**********/

/*控件类型代码*/
#define GC_CTRL_TYPE_DESKTOP	0		/*桌面*/
#define GC_CTRL_TYPE_WINDOW		1		/*窗口*/
#define GC_CTRL_TYPE_BUTTON		2		/*按钮*/
#define GC_CTRL_TYPE_TEXT		3		/*静态文本框*/
#define GC_CTRL_TYPE_SLEDIT		4		/*单行编辑框*/
#define GC_CTRL_TYPE_MLEDIT		5		/*多行编辑框*/
#define GC_CTRL_TYPE_SCROLL		6		/*滚动条*/
#define GC_CTRL_TYPE_LIST		7		/*列表框*/

typedef long (*MSGPROC)(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);	/*窗体消息处理函数*/

typedef struct _CTRL_ARGS
{
	DWORD width, height;	/*窗体尺寸*/
	long x, y;				/*窗体位置*/
	WORD type, style;		/*类型,样式*/
	MSGPROC MsgProc;		/*消息处理函数*/
}CTRL_ARGS;	/*控件参数*/

typedef struct _CTRL_GOBJ
{
	UDI_AREA uda;			/*窗体绘图区域*/
	DWORD gid;				/*GUI服务端对象ID*/
	struct _CTRL_GOBJ *pre, *nxt;	/*兄/弟控件链指针*/
	struct _CTRL_GOBJ *par, *chl;	/*父/子控件链指针*/
	long x, y;				/*窗体位置*/
	WORD type, style;		/*类型,样式*/
	MSGPROC MsgProc;		/*消息处理函数*/
	void (*DrawProc)(struct _CTRL_GOBJ *gobj);	/*绘制处理函数*/
}CTRL_GOBJ;	/*控件基类*/

typedef void (*DRAWPROC)(CTRL_GOBJ *gobj);							/*窗体绘制处理函数*/

/*初始化CTRL_GOBJ结构*/
long GCGobjInit(CTRL_GOBJ *gobj, const CTRL_ARGS *args, MSGPROC MsgProc, DRAWPROC DrawProc, DWORD pid, CTRL_GOBJ *ParGobj);

/*删除窗体树*/
void GCGobjDelete(CTRL_GOBJ *gobj);

/*绘制窗体树*/
void GCGobjDraw(CTRL_GOBJ *gobj);

/*GUI客户端消息调度*/
long GCDispatchMsg(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

/*移动简单窗体*/
void GCGobjMove(CTRL_GOBJ *gobj, long x, long y);

/*设置简单窗体位置大小*/
void GCGobjSetSize(CTRL_GOBJ *gobj, long x, long y, DWORD width, DWORD height);

/**********桌面**********/

typedef struct _CTRL_DSK
{
	CTRL_GOBJ obj;
}CTRL_DSK;	/*桌面类*/

long GCDskCreate(CTRL_DSK **dsk, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj);

long GCDskDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCDskDefDrawProc(CTRL_DSK *dsk);

/**********按钮**********/

#define BTN_TXT_LEN			32

#define BTN_STYLE_DISABLED	0x0001	/*不可用*/

typedef struct _CTRL_BTN
{
	CTRL_GOBJ obj;
	char text[BTN_TXT_LEN];			/*按钮文本*/
	UDI_IMAGE *img;					/*按钮图片*/
	BOOL isPressDown;				/*是否已按下*/
	void (*PressProc)(struct _CTRL_BTN *btn);	/*点击处理函数*/
}CTRL_BTN;	/*按钮类*/

long GCBtnCreate(CTRL_BTN **btn, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text, UDI_IMAGE *img, void (*PressProc)(CTRL_BTN *btn));

long GCBtnDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCBtnDefDrawProc(CTRL_BTN *btn);

/*设置按钮文本*/
void GCBtnSetText(CTRL_BTN *btn, const char *text);

/*设置按钮为不可用样式*/
void GCBtnSetDisable(CTRL_BTN *btn, BOOL isDisable);

/**********窗口**********/

#define WND_CAP_LEN			128

#define WND_STYLE_CAPTION	0x0001	/*窗体标题栏*/
#define WND_STYLE_BORDER	0x0002	/*窗体边框*/
#define WND_STYLE_CLOSEBTN	0x0004	/*关闭按钮*/
#define WND_STYLE_MAXBTN	0x0008	/*最大化按钮*/
#define WND_STYLE_MINBTN	0x0010	/*最小化按钮*/
#define WND_STYLE_SIZEBTN	0x0020	/*拖动缩放按钮*/

#define WND_STATE_FOCUS		0x8000	/*焦点状态*/
#define WND_STATE_TOP		0x4000	/*顶端状态*/

typedef struct _CTRL_WND
{
	CTRL_GOBJ obj;
	char caption[WND_CAP_LEN];		/*标题文本*/
	CTRL_BTN *close;				/*关闭按钮*/
	CTRL_BTN *max;					/*最大化按钮*/
	CTRL_BTN *min;					/*最小化按钮*/
	CTRL_BTN *size;					/*缩放按钮*/
	UDI_AREA client;				/*客户绘图区*/
	DWORD MinWidth, MinHeight;		/*最小和最大大小*/
	DWORD MaxWidth, MaxHeight;
	long x0, y0;					/*正常位置,大小*/
	DWORD width0, height0;
}CTRL_WND;	/*窗口类*/

long GCWndCreate(CTRL_WND **wnd, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *caption);

long GCWndDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCWndDefDrawProc(CTRL_WND *wnd);

/*设置窗口标题文本*/
void GCWndSetCaption(CTRL_WND *wnd, const char *caption);

/*设置窗口位置大小*/
void GCWndSetSize(CTRL_WND *wnd, long x, long y, DWORD width, DWORD height);

/*取得窗口客户区位置*/
void GCWndGetClientLoca(CTRL_WND *wnd, long *x, long *y);

/**********静态文本框**********/

#define TXT_TXT_LEN			128

typedef struct _CTRL_TXT
{
	CTRL_GOBJ obj;
	char text[TXT_TXT_LEN];			/*文本框文本*/
}CTRL_TXT;	/*静态文本框类*/

long GCTxtCreate(CTRL_TXT **txt, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text);

long GCTxtDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCTxtDefDrawProc(CTRL_TXT *txt);

/*设置文本框文本*/
void GCTxtSetText(CTRL_TXT *txt, const char *text);

/**********单行编辑框**********/

#define SEDT_TXT_LEN		128

#define SEDT_STYLE_RDONLY	0x0001	/*只读*/

#define SEDT_STATE_FOCUS	0x8000	/*焦点状态*/
#define SEDT_STATE_IME		0x4000	/*开启输入法*/

typedef struct _CTRL_SEDT
{
	CTRL_GOBJ obj;
	char text[SEDT_TXT_LEN];		/*编辑框文本*/
	char *FstC, *CurC;				/*首字符位置,光标位置*/
	void (*EnterProc)(struct _CTRL_SEDT *edt);	/*回车处理函数*/
}CTRL_SEDT;	/*单行编辑框*/

long GCSedtCreate(CTRL_SEDT **edt, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text, void (*EnterProc)(CTRL_SEDT *edt));

long GCSedtDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCSedtDefDrawProc(CTRL_SEDT *edt);

/*设置单行编辑框文本*/
void GCSedtSetText(CTRL_SEDT *edt, const char *text);

/*追加单行编辑框文本*/
void GCSedtAddText(CTRL_SEDT *edt, const char *text);

/**********多行编辑框**********/

/**********滚动条**********/

#define SCRL_STYLE_HOR		0x0000	/*水平*/
#define SCRL_STYLE_VER		0x0001	/*竖直*/

typedef struct _CTRL_SCRL
{
	CTRL_GOBJ obj;
	long min, max;			/*最小最大值*/
	long pos, page;			/*位置值,页大小*/
	CTRL_BTN *sub;			/*减小按钮*/
	CTRL_BTN *add;			/*增大按钮*/
	CTRL_BTN *drag;			/*拖动按钮*/
	void (*ChangeProc)(struct _CTRL_SCRL *scl);	/*值改变处理函数*/
}CTRL_SCRL;	/*滚动条*/

long GCScrlCreate(CTRL_SCRL **scl, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, long min, long max, long pos, long page, void (*ChangeProc)(CTRL_SCRL *scl));

long GCScrlDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCScrlDefDrawProc(CTRL_SCRL *scl);

/*设置滚动条位置大小*/
void GCScrlSetSize(CTRL_SCRL *scl, long x, long y, DWORD width, DWORD height);

/*设置滚动条参数*/
long GCScrlSetData(CTRL_SCRL *scl, long min, long max, long pos, long page);

/**********列表框**********/

#define LSTITM_ATTR_SELECTED	0x00000001	/*被选中*/
#define LSTITM_TXT_LEN		128

typedef struct _LIST_ITEM
{
	struct _LIST_ITEM *pre, *nxt;	/*前后指针*/
	DWORD attr;			/*属性*/
	char text[LSTITM_TXT_LEN];	/*列表框单项文本*/
}LIST_ITEM;	/*列表框单项*/

typedef struct _CTRL_LST
{
	CTRL_GOBJ obj;
	DWORD ItemCou;		/*总项数*/
	DWORD CurPos;		/*当前项位置*/
	DWORD MaxWidth;		/*最长字符串像素宽度*/
	DWORD TextX;		/*文本横向偏移*/
	UDI_AREA cont;		/*内容绘图区*/
	LIST_ITEM *item;	/*项列表指针*/
	LIST_ITEM *CurItem;	/*当前项*/
	LIST_ITEM *SelItem;	/*被选项*/
	CTRL_SCRL *hscl;	/*横滚动条*/
	CTRL_SCRL *vscl;	/*纵滚动条*/
	void (*SelProc)(struct _CTRL_LST *lst);	/*被选项改变处理函数*/
}CTRL_LST;	/*列表框*/

long GCLstCreate(CTRL_LST **lst, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, void (*SelProc)(CTRL_LST *lst));

long GCLstDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCLstDefDrawProc(CTRL_LST *lst);

/*设置列表框位置大小*/
void GCLstSetSize(CTRL_LST *lst, long x, long y, DWORD width, DWORD height);

/*插入项*/
long GCLstInsertItem(CTRL_LST *lst, LIST_ITEM *pre, const char *text, LIST_ITEM **item);

/*删除项*/
long GCLstDeleteItem(CTRL_LST *lst, LIST_ITEM *item);

/*删除所有项*/
long GCLstDelAllItem(CTRL_LST *lst);

#endif
