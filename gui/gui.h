/*	gui.h for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面相关结构、常量定义
	最后修改日期：2010-10-03
*/

#ifndef	_GUI_H_
#define	_GUI_H_

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"
#include "guiapi.h"

/**********图形用户界面结构定义**********/

typedef struct _RECT
{
	long xpos, ypos, xend, yend;
}RECT;	/*矩形结构*/

#define CLIPRECTT_LEN	0x2000		/*剪切矩形管理表长度*/

typedef struct _CLIPRECT
{
	RECT rect;						/*剪切矩形*/
	struct _CLIPRECT *nxt;			/*后续指针*/
}CLIPRECT;	/*可视剪切矩形节点,用于窗体的重叠控制*/

#define GOBJT_LEN		0x4000		/*窗体描述符管理表长度*/

typedef struct _GOBJ_DESC
{
	THREAD_ID ptid;					/*所属线程ID*/
	DWORD ClientSign;				/*客户识别符,方便客户端快速定位对象*/
	RECT rect;						/*相对父窗体的位置*/
	DWORD *vbuf;					/*可视内存缓冲,NULL表示使用父辈窗体的缓冲*/
	struct _GOBJ_DESC *pre, *nxt;	/*兄/弟对象链指针*/
	struct _GOBJ_DESC *par, *chl;	/*父/子对象链指针*/
	CLIPRECT *ClipList;				/*自身剪切矩形列表*/
}GOBJ_DESC;	/*GUI对象(窗体)描述符*/

/**********剪切矩形管理相关**********/

/*掩盖窗体的矩形外部*/
long CoverRectExter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*掩盖窗体的矩形内部*/
long CoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*显露窗体的矩形内部*/
long DiscoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*被祖父和祖伯父窗体覆盖*/
void CoverRectByPar(GOBJ_DESC *gobj);

/*删除剪切矩形链表*/
long DeleteClipList(GOBJ_DESC *gobj);

/*绘制窗体矩形内部*/
void DrawGobj(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend, long AbsXpos, long AbsYpos, BOOL isDrawNoRoot);

/**********窗体管理相关**********/

/*取得窗体的绝对坐标*/
void GetGobjPos(GOBJ_DESC *gobj, long *AbsXpos, long *AbsYpos);

/*根据绝对坐标查找窗体*/
GOBJ_DESC *FindGobj(long *AbsXpos, long *AbsYpos);

/*创建主桌面*/
long CreateDesktop(THREAD_ID ptid, DWORD ClientSign, DWORD *vbuf, DWORD len);

/*删除主桌面*/
long DeleteDesktop(GOBJ_DESC *gobj);

/*创建窗体*/
long CreateGobj(GOBJ_DESC *ParGobj, THREAD_ID ptid, DWORD ClientSign, long xpos, long ypos, long width, long height, DWORD *vbuf, DWORD len, GOBJ_DESC **pgobj);

/*删除窗体*/
long DeleteGobj(GOBJ_DESC *gobj);

/*设置窗体的位置大小*/
long MoveGobj(GOBJ_DESC *gobj, long xpos, long ypos);

/*设置窗体的大小*/
long SizeGobj(GOBJ_DESC *gobj, long xpos, long ypos, long width, long height, DWORD *vbuf, DWORD len);

/*绘制窗体*/
long PaintGobj(GOBJ_DESC *gobj, long xpos, long ypos, long width, long height);

/*设置窗体在顶端*/
long SetTopGobj(GOBJ_DESC *gobj);

/*设置窗体焦点*/
long SetFocusGobj(GOBJ_DESC *gobj);

/**********功能库相关**********/

extern void *GDIvm;
extern DWORD GDIwidth, GDIheight, GDIPixBits, GDImode;

/*贴图*/
void GDIPutImage(long x, long y, DWORD *img, long w, long h);

/*截图*/
void GDIGetImage(long x, long y, DWORD *img, long w, long h);

/*去背景色贴图*/
void GDIPutBCImage(long x, long y, DWORD *img, long w, long h, DWORD bc);

/*GUI矩形块贴图*/
void GuiPutImage(long x, long y, DWORD *img, long memw, long w, long h);

/**********键盘鼠标功能相关**********/

#define MUSPIC_MAXLEN	1024		/*鼠标指针图片缓冲长度,最大32*32*/

/*键盘鼠标初始化*/
long InitKbdMus();

/*键盘消息处理*/
void KeyboardProc(DWORD key);

/*计算鼠标指针与区域是否重叠*/
BOOL CheckMousePos(long xpos, long ypos, long xend, long yend);

/*隐藏鼠标指针*/
void HideMouse();

/*显示鼠标指针*/
void ShowMouse();

/*设置鼠标指针*/
long SetMousePic(long x, long y, long width, long height, DWORD *src, DWORD len);

/*鼠标消息处理*/
void MouseProc(DWORD key, long x, long y, long z);

/*处理拖拽消息*/
long DragGobj(GOBJ_DESC *gobj, DWORD mode);

#endif
