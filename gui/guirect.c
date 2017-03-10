/*	guirect.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面GUI剪切矩形基本操作
	最后修改日期：2010-10-05
*/

#include "gui.h"

CLIPRECT *ClipRectt, *FstClipRect;	/*剪切矩形管理表指针*/

/*分配剪切矩形结构*/
static CLIPRECT *AllocClipRect()
{
	CLIPRECT *clip;

	if (FstClipRect == NULL)
		return NULL;
	FstClipRect = (clip = FstClipRect)->nxt;
	return clip;
}

/*释放剪切矩形结构*/
static void FreeClipRect(CLIPRECT *clip)
{
	clip->nxt = FstClipRect;
	FstClipRect = clip;
}

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

/*掩盖窗体的矩形外部*/
long CoverRectExter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend)
{
	CLIPRECT *PreClip, *CurClip;

	if (xpos <= 0 && xend >= gobj->rect.xend - gobj->rect.xpos &&	/*完全覆盖区域*/
		ypos <= 0 && yend >= gobj->rect.yend - gobj->rect.ypos)
		return GUI_ERR_NOCHG_CLIPRECT;
	for (PreClip = NULL, CurClip = gobj->ClipList; CurClip;)	/*处理父窗体*/
	{
		if (xend > CurClip->rect.xpos && xpos < CurClip->rect.xend &&
			yend > CurClip->rect.ypos && ypos < CurClip->rect.yend)	/*有重叠区域*/
		{
			if (CurClip->rect.xpos < xpos)
				CurClip->rect.xpos = xpos;
			if (CurClip->rect.ypos < ypos)
				CurClip->rect.ypos = ypos;
			if (CurClip->rect.xend > xend)
				CurClip->rect.xend = xend;
			if (CurClip->rect.yend > yend)
				CurClip->rect.yend = yend;
			CurClip = (PreClip = CurClip)->nxt;	/*继续处理后续节点*/
		}
		else
		{
			/*删除原剪切矩形*/
			if (PreClip)
			{
				PreClip->nxt = CurClip->nxt;
				FreeClipRect(CurClip);
				CurClip = PreClip->nxt;
			}
			else
			{
				gobj->ClipList = CurClip->nxt;
				FreeClipRect(CurClip);
				CurClip = gobj->ClipList;
			}
		}
	}
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*递归处理子窗体*/
		CoverRectExter(gobj, xpos - gobj->rect.xpos, ypos - gobj->rect.ypos, xend - gobj->rect.xpos, yend - gobj->rect.ypos);

	return NO_ERROR;
}

/*掩盖窗体的矩形内部*/
long CoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend)
{
	CLIPRECT *PreClip, *CurClip, *NewClip;

	if (xend <= 0 || xpos >= gobj->rect.xend - gobj->rect.xpos ||	/*没有重叠区域*/
		yend <= 0 || ypos >= gobj->rect.yend - gobj->rect.ypos)
		return GUI_ERR_NOCHG_CLIPRECT;
	for (PreClip = NULL, CurClip = gobj->ClipList; CurClip;)	/*处理父窗体*/
	{
		if (xend > CurClip->rect.xpos && xpos < CurClip->rect.xend &&
			yend > CurClip->rect.ypos && ypos < CurClip->rect.yend)	/*有重叠区域*/
		{
			/*左边的不重叠块(不包括左上和左下)*/
			if (xpos > CurClip->rect.xpos)
			{
				NewClip = AllocClipRect();
				NewClip->rect.xpos = CurClip->rect.xpos;
				NewClip->rect.ypos = MAX(ypos, CurClip->rect.ypos);
				NewClip->rect.xend = xpos;
				NewClip->rect.yend = MIN(yend, CurClip->rect.yend);
				NewClip->nxt = CurClip;
				if (PreClip)
					PreClip->nxt = NewClip;
				else
					gobj->ClipList = NewClip;
				PreClip = NewClip;
			}
			/*右边的不重叠块(不包括右上和右下)*/
			if (xend < CurClip->rect.xend)
			{
				NewClip = AllocClipRect();
				NewClip->rect.xpos = xend;
				NewClip->rect.ypos = MAX(ypos, CurClip->rect.ypos);
				NewClip->rect.xend = CurClip->rect.xend;
				NewClip->rect.yend = MIN(yend, CurClip->rect.yend);
				NewClip->nxt = CurClip;
				if (PreClip)
					PreClip->nxt = NewClip;
				else
					gobj->ClipList = NewClip;
				PreClip = NewClip;
			}
			/*上边的不重叠块(包括左上和右上)*/
			if (ypos > CurClip->rect.ypos)
			{
				NewClip = AllocClipRect();
				NewClip->rect.xpos = CurClip->rect.xpos;
				NewClip->rect.ypos = CurClip->rect.ypos;
				NewClip->rect.xend = CurClip->rect.xend;
				NewClip->rect.yend = ypos;
				NewClip->nxt = CurClip;
				if (PreClip)
					PreClip->nxt = NewClip;
				else
					gobj->ClipList = NewClip;
				PreClip = NewClip;
			}
			/*下边的不重叠块(包括左上和右上)*/
			if (yend < CurClip->rect.yend)
			{
				NewClip = AllocClipRect();
				NewClip->rect.xpos = CurClip->rect.xpos;
				NewClip->rect.ypos = yend;
				NewClip->rect.xend = CurClip->rect.xend;
				NewClip->rect.yend = CurClip->rect.yend;
				NewClip->nxt = CurClip;
				if (PreClip)
					PreClip->nxt = NewClip;
				else
					gobj->ClipList = NewClip;
				PreClip = NewClip;
			}
			/*删除原剪切矩形*/
			if (PreClip)
			{
				PreClip->nxt = CurClip->nxt;
				FreeClipRect(CurClip);
				CurClip = PreClip->nxt;
			}
			else
			{
				gobj->ClipList = CurClip->nxt;
				FreeClipRect(CurClip);
				CurClip = gobj->ClipList;
			}
		}
		else
			CurClip = (PreClip = CurClip)->nxt;	/*继续处理后续节点*/
	}
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*递归处理子窗体*/
		CoverRectInter(gobj, xpos - gobj->rect.xpos, ypos - gobj->rect.ypos, xend - gobj->rect.xpos, yend - gobj->rect.ypos);

	return NO_ERROR;
}

/*显露窗体的矩形内部*/
long DiscoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend)
{
	GOBJ_DESC *ChlGobj;
	CLIPRECT *NewClip;

	if (xend <= 0 || xpos >= gobj->rect.xend - gobj->rect.xpos ||	/*没有重叠区域*/
		yend <= 0 || ypos >= gobj->rect.yend - gobj->rect.ypos)
		return GUI_ERR_NOCHG_CLIPRECT;
	if (xpos < 0)	/*处理父窗体*/
		xpos = 0;
	if (ypos < 0)
		ypos = 0;
	if (xend > gobj->rect.xend - gobj->rect.xpos)
		xend = gobj->rect.xend - gobj->rect.xpos;
	if (yend > gobj->rect.yend - gobj->rect.ypos)
		yend = gobj->rect.yend - gobj->rect.ypos;
	NewClip = AllocClipRect();
	NewClip->rect.xpos = xpos;
	NewClip->rect.ypos = ypos;
	NewClip->rect.xend = xend;
	NewClip->rect.yend = yend;
	NewClip->nxt = gobj->ClipList;
	gobj->ClipList = NewClip;
	if (gobj->chl == NULL)
		return NO_ERROR;
	ChlGobj = gobj->chl;
	while (ChlGobj->nxt)	/*查找最底层的窗体*/
		 ChlGobj = ChlGobj->nxt;
	while (ChlGobj)	/*递归处理子窗体*/
	{
		if (ChlGobj->vbuf)	/*无根窗体不覆盖父窗体*/
			CoverRectInter(gobj, ChlGobj->rect.xpos, ChlGobj->rect.ypos, ChlGobj->rect.xend, ChlGobj->rect.yend);
		DiscoverRectInter(ChlGobj, xpos - ChlGobj->rect.xpos, ypos - ChlGobj->rect.ypos, xend - ChlGobj->rect.xpos, yend - ChlGobj->rect.ypos);
		ChlGobj = ChlGobj->pre;
	}

	return NO_ERROR;
}

/*被祖父和祖伯父窗体覆盖*/
void CoverRectByPar(GOBJ_DESC *gobj)
{
	GOBJ_DESC *ParGobj, *PreGobj;
	long xpos, ypos;

	ypos = xpos = 0;
	ParGobj = gobj;
	for (;;)
	{
		xpos -= ParGobj->rect.xpos;
		ypos -= ParGobj->rect.ypos;
		for (PreGobj = ParGobj->pre; PreGobj; PreGobj = PreGobj->pre)	/*被祖伯父窗体覆盖内部*/
			CoverRectInter(gobj, xpos + PreGobj->rect.xpos, ypos + PreGobj->rect.ypos, xpos + PreGobj->rect.xend, ypos + PreGobj->rect.yend);
		if (ParGobj->par == NULL)
			break;
		ParGobj = ParGobj->par;	/*被祖父窗体覆盖外部*/
		CoverRectExter(gobj, xpos, ypos, xpos + ParGobj->rect.xend - ParGobj->rect.xpos, ypos + ParGobj->rect.yend - ParGobj->rect.ypos);
	}
}

/*删除剪切矩形链表*/
long DeleteClipList(GOBJ_DESC *gobj)
{
	BOOL isNoChg = TRUE;

	if (gobj->ClipList)
	{
		CLIPRECT *CurClip;

		CurClip = gobj->ClipList;	/*查找链尾节点*/
		while (CurClip->nxt)
			CurClip = CurClip->nxt;
		CurClip->nxt = FstClipRect;
		FstClipRect = gobj->ClipList;
		gobj->ClipList = NULL;
		isNoChg = FALSE;
	}
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*递归处理子窗体*/
		if (DeleteClipList(gobj) == NO_ERROR)
			isNoChg = FALSE;
	if (isNoChg)
		return GUI_ERR_NOCHG_CLIPRECT;

	return NO_ERROR;
}

/*绘制窗体矩形内部*/
void DrawGobj(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend, long AbsXpos, long AbsYpos, BOOL isDrawNoRoot)
{
	CLIPRECT *CurClip;

	if (xend <= 0 || xpos >= gobj->rect.xend - gobj->rect.xpos ||	/*没有重叠区域*/
		yend <= 0 || ypos >= gobj->rect.yend - gobj->rect.ypos)
		return;
	if (gobj->vbuf || isDrawNoRoot)	/*窗体具有自身缓冲或强制绘制无根窗体时进行绘制*/
	{
		GOBJ_DESC *RootGobj;
		long RootXpos, RootYpos;
		for (RootGobj = gobj, RootYpos = RootXpos = 0; RootGobj->vbuf == NULL; RootGobj = RootGobj->par)	/*有显示缓冲则直接绘图,没有则使用祖父窗体的显示缓冲*/
		{
			RootXpos += RootGobj->rect.xpos;
			RootYpos += RootGobj->rect.ypos;
		}
		for (CurClip = gobj->ClipList; CurClip; CurClip = CurClip->nxt)	/*处理父窗体*/
		{
			long RectXpos, RectYpos, RectXend, RectYend;

			RectXpos = MAX(xpos, CurClip->rect.xpos);
			RectYpos = MAX(ypos, CurClip->rect.ypos);
			RectXend = MIN(xend, CurClip->rect.xend);
			RectYend = MIN(yend, CurClip->rect.yend);
			if (RectXpos < RectXend && RectYpos < RectYend)	/*有重叠区域*/
			{
				BOOL isRefreshMouse;

				isRefreshMouse = CheckMousePos(AbsXpos + RectXpos, AbsYpos + RectYpos, AbsXpos + RectXend, AbsYpos + RectYend);
				if (isRefreshMouse)
					HideMouse();
				GuiPutImage(AbsXpos + RectXpos, AbsYpos + RectYpos, RootGobj->vbuf + (RootXpos + RectXpos) + (RootYpos + RectYpos) * (RootGobj->rect.xend - RootGobj->rect.xpos), RootGobj->rect.xend - RootGobj->rect.xpos, RectXend - RectXpos, RectYend - RectYpos);
				if (isRefreshMouse)
					ShowMouse();
			}
		}
	}
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*递归处理子窗体*/
		DrawGobj(gobj, xpos - gobj->rect.xpos, ypos - gobj->rect.ypos, xend - gobj->rect.xpos, yend - gobj->rect.ypos, AbsXpos + gobj->rect.xpos, AbsYpos + gobj->rect.ypos, FALSE);
}
