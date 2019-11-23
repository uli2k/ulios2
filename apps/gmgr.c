/*	gmgr.c for ulios application
	作者：孙亮
	功能：图形化资源管理器程序
	最后修改日期：2012-02-13
*/

#include "../lib/string.h"
#include "../fs/fsapi.h"
#include "../lib/malloc.h"
#include "../lib/gclient.h"

CTRL_WND *MainWnd;	/*主窗口*/
CTRL_SEDT *DirSedt;	/*地址栏*/
CTRL_LST *PartList;	/*分区列表*/
CTRL_LST *FileList;	/*文件列表*/
CTRL_BTN *ParBtn;	/*父目录按钮*/
CTRL_BTN *CutBtn;	/*剪切按钮*/
CTRL_BTN *CopyBtn;	/*复制按钮*/
CTRL_BTN *PasteBtn;	/*粘贴按钮*/
CTRL_BTN *DelBtn;	/*删除按钮*/
CTRL_BTN *DirBtn;	/*创建目录按钮*/
CTRL_BTN *FileBtn;	/*创建文件按钮*/
CTRL_BTN *RenBtn;	/*重命名按钮*/
CTRL_SEDT *NameEdt;	/*名称编辑框*/
DWORD op;	/*操作*/
char PathBuf[MAX_PATH];	/*复制或剪切路径缓冲*/

#define OP_NONE	0	/*无*/
#define OP_CUT	1	/*剪切*/
#define OP_COPY	2	/*复制*/
#define OP_DIR	3	/*创建目录*/
#define OP_FILE	4	/*创建文件*/
#define OP_REN	5	/*重命名*/

#define WND_WIDTH	400	/*窗口最小宽度,高度*/
#define WND_HEIGHT	300
#define SIDE		2	/*控件边距*/
#define EDT_HEIGHT	16	/*地址栏高度*/
#define PART_WIDTH	64	/*分区列表宽度*/
#define BTN_WIDTH	56	/*按钮宽度,高度*/
#define BTN_HEIGHT	20

/*填充文件列表*/
void FillFileList()
{
	FILE_INFO fi;
	long dh;
	LIST_ITEM *item;
	char buf[MAX_PATH];

	if ((dh = FSOpenDir("")) < 0)
		return;
	FSGetCwd(buf, MAX_PATH);
	GCSedtSetText(DirSedt, buf);
	GCLstDelAllItem(FileList);
	item = NULL;
	while (FSReadDir(dh, &fi) == NO_ERROR)
	{
		char buf[MAX_PATH];
		sprintf(buf, "%s %s", (fi.attr & FILE_ATTR_DIREC) ? "[]" : "==", fi.name);
		GCLstInsertItem(FileList, item, buf, &item);
	}
	FSclose(dh);
}

/*地址栏回车处理*/
void DirSedtEnterProc(CTRL_SEDT *edt)
{
	if (FSChDir(edt->text) == NO_ERROR)
		FillFileList();
}

/*分区列表选取处理*/
void PartListSelProc(CTRL_LST *lst)
{
	if (lst->SelItem)	/*选中盘符*/
	{
		if (FSChDir(lst->SelItem->text) == NO_ERROR)
			FillFileList();
	}
}

/*分区列表消息处理*/
long PartListMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_LST *lst = (CTRL_LST*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			DWORD pid;
			LIST_ITEM *item;

			pid = 0;
			item = NULL;
			while (FSEnumPart(&pid) == NO_ERROR)
			{
				char buf[4];
				sprintf(buf, "/%u", pid);
				GCLstInsertItem(lst, item, buf, &item);
				pid++;
			}
		}
		break;
	}
	return GCLstDefMsgProc(ptid, data);
}

/*文件列表选取处理*/
void FileListSelProc(CTRL_LST *lst)
{
	if (lst->SelItem)	/*选中文件*/
	{
		GCBtnSetDisable(CutBtn, FALSE);
		GCBtnSetDisable(CopyBtn, FALSE);
		GCBtnSetDisable(DelBtn, FALSE);
		GCBtnSetDisable(RenBtn, FALSE);
	}
	else
	{
		GCBtnSetDisable(CutBtn, TRUE);
		GCBtnSetDisable(CopyBtn, TRUE);
		GCBtnSetDisable(DelBtn, TRUE);
		GCBtnSetDisable(RenBtn, TRUE);
	}
}

/*文件列表消息处理*/
long FileListMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_LST *lst = (CTRL_LST*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		FillFileList();
		break;
	case GM_LBUTTONDBCLK:
		if (lst->SelItem)	/*选中目录或文件*/
		{
			if (lst->SelItem->text[0] == '[')	/*打开目录*/
			{
				if (FSChDir(lst->SelItem->text + 3) == NO_ERROR)
					FillFileList();
			}
			else	/*执行程序*/
			{
				THREAD_ID ptid;
				KCreateProcess(0, lst->SelItem->text + 3, NULL, &ptid);
			}
		}
		break;
	}
	return GCLstDefMsgProc(ptid, data);
}

/*上级目录按钮处理*/
void ParBtnPressProc(CTRL_BTN *btn)
{
	if (FSChDir("..") == NO_ERROR)
		FillFileList();
}

/*剪切按钮处理*/
void CutBtnPressProc(CTRL_BTN *btn)
{
	if (FileList->SelItem)	/*选中文件*/
	{
		char buf[MAX_PATH];
		FSGetCwd(buf, MAX_PATH);
		sprintf(PathBuf, "%s/%s", buf, FileList->SelItem->text + 3);
		op = OP_CUT;
		GCBtnSetDisable(PasteBtn, FALSE);
	}
}

/*复制按钮处理*/
void CopyBtnPressProc(CTRL_BTN *btn)
{
	if (FileList->SelItem)	/*选中文件*/
	{
		char buf[MAX_PATH];
		FSGetCwd(buf, MAX_PATH);
		sprintf(PathBuf, "%s/%s", buf, FileList->SelItem->text + 3);
		op = OP_COPY;
		GCBtnSetDisable(PasteBtn, FALSE);
	}
}

/*递归删除目录*/
void DelTree(char *path)
{
	if (FSChDir(path) == NO_ERROR)
	{
		FILE_INFO fi;
		long dh;
		if ((dh = FSOpenDir("")) < 0)
		{
			FSChDir("..");
			return;
		}
		while (FSReadDir(dh, &fi) == NO_ERROR)
			if (!(fi.name[0] == '.' && (fi.name[1] == '\0' || (fi.name[1] == '.' && fi.name[2] == '\0'))))	/*单点和双点目录不处理*/
				DelTree(fi.name);
		FSclose(dh);
		FSChDir("..");
	}
	FSremove(path);
}

/*递归复制目录到当前目录*/
void CopyTree(char *path)
{
	char *end, *name;
	FILE_INFO fi;
	long inh;

	end = path;	/*分离路径和名称*/
	while (*end)
		end++;
	name = end;
	while (name > path && *name != '/')
		name--;
	if (*name == '/')
		name++;
	if ((inh = FSOpenDir(path)) >= 0)	/*尝试打开目录*/
	{
		if (FSMkDir(name) == NO_ERROR)	/*创建新目录*/
		{
			FSChDir(name);
			while (FSReadDir(inh, &fi) == NO_ERROR)
				if (!(fi.name[0] == '.' && (fi.name[1] == '\0' || (fi.name[1] == '.' && fi.name[2] == '\0'))))	/*单点和双点目录不处理*/
				{
					*end = '/';
					strcpy(end + 1, fi.name);
					CopyTree(path);
					*end = '\0';
				}
			FSChDir("..");
		}
		FSclose(inh);
	}
	else if ((inh = FSopen(path, FS_OPEN_READ)) >= 0)	/*尝试打开文件*/
	{
		char buf[4096];
		long outh, siz;

		if ((outh = FScreat(name)) >= 0)	/*创建新文件*/
		{
			while ((siz = FSread(inh, buf, sizeof(buf))) > 0)	/*复制数据*/
				FSwrite(outh, buf, siz);
			FSclose(outh);
		}
		FSclose(inh);
	}
}

/*粘贴按钮处理*/
void PasteBtnPressProc(CTRL_BTN *btn)
{
	if (op == OP_CUT || op == OP_COPY)	/*已剪切或复制文件*/
	{
		CopyTree(PathBuf);
		if (op == OP_CUT)
		{
			char buf[MAX_PATH];
			FSGetCwd(buf, MAX_PATH);
			DelTree(PathBuf);
			FSChDir(buf);
		}
		op = OP_NONE;
		GCBtnSetDisable(PasteBtn, TRUE);
		FillFileList();
	}
}

/*删除按钮处理*/
void DelBtnPressProc(CTRL_BTN *btn)
{
	if (FileList->SelItem)	/*选中文件*/
	{
		DelTree(FileList->SelItem->text + 3);
		FillFileList();
	}
}

/*名称输入框回车处理*/
void NameEdtEnterProc(CTRL_SEDT *edt)
{
	switch (op)
	{
	case OP_DIR:
		FSMkDir(edt->text);
		break;
	case OP_FILE:
		FSclose(FScreat(edt->text));
		break;
	case OP_REN:
		FSrename(FileList->SelItem->text + 3, edt->text);
		break;
	}
	GCFillRect(&edt->obj.uda, 0, 0, edt->obj.uda.width, edt->obj.uda.height, 0xCCCCCC);
	GUIdestroy(edt->obj.gid);
	NameEdt = NULL;
	GCBtnSetDisable(DirBtn, FALSE);
	GCBtnSetDisable(FileBtn, FALSE);
	GCBtnSetDisable(RenBtn, FALSE);
	GCBtnSetDisable(ParBtn, FALSE);
	FillFileList();
}

/*名称输入框消息处理*/
long NameEdtMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_SEDT *edt = (CTRL_SEDT*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		GUISetFocus(edt->obj.gid);
		break;
	case GM_KEY:
		if ((data[1] & 0xFF) == 27)	/*按下ESC*/
		{
			GCFillRect(&edt->obj.uda, 0, 0, edt->obj.uda.width, edt->obj.uda.height, 0xCCCCCC);
			GUIdestroy(edt->obj.gid);
			NameEdt = NULL;
			GCBtnSetDisable(DirBtn, FALSE);
			GCBtnSetDisable(FileBtn, FALSE);
			GCBtnSetDisable(RenBtn, FALSE);
			GCBtnSetDisable(ParBtn, FALSE);
			return NO_ERROR;
		}
		break;
	}
	return GCSedtDefMsgProc(ptid, data);
}

/*创建名称输入框*/
void CreateNameEdt(const char *text)
{
	CTRL_ARGS args;

	if (NameEdt == NULL)
	{
		args.width = BTN_WIDTH;
		args.height = EDT_HEIGHT;
		args.x = ParBtn->obj.x;
		args.y = ParBtn->obj.y + BTN_HEIGHT * 8 + SIDE * 8;
		args.style = 0;
		args.MsgProc = NameEdtMsgProc;
		GCSedtCreate(&NameEdt, &args, MainWnd->obj.gid, &MainWnd->obj, text, NameEdtEnterProc);
	}
	GCBtnSetDisable(PasteBtn, TRUE);
	GCBtnSetDisable(DirBtn, TRUE);
	GCBtnSetDisable(FileBtn, TRUE);
	GCBtnSetDisable(RenBtn, TRUE);
}

/*创建目录按钮处理*/
void DirBtnPressProc(CTRL_BTN *btn)
{
	CreateNameEdt(NULL);
	op = OP_DIR;
}

/*创建文件按钮处理*/
void FileBtnPressProc(CTRL_BTN *btn)
{
	CreateNameEdt(NULL);
	op = OP_FILE;
}

/*重命名按钮处理*/
void RenamBtnPressProc(CTRL_BTN *btn)
{
	if (FileList->SelItem)	/*选中文件*/
	{
		CreateNameEdt(FileList->SelItem->text + 3);
		GCBtnSetDisable(ParBtn, TRUE);
		op = OP_REN;
	}
}

/*主窗口消息处理*/
long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			CTRL_ARGS args;

			GCWndGetClientLoca(wnd, &args.x, &args.y);
			args.width = wnd->client.width - SIDE * 2;
			args.height = EDT_HEIGHT;
			args.x += SIDE;
			args.y += SIDE;
			args.style = 0;
			args.MsgProc = NULL;
			GCSedtCreate(&DirSedt, &args, wnd->obj.gid, &wnd->obj, NULL, DirSedtEnterProc);
			args.width = PART_WIDTH;
			args.height = wnd->client.height - EDT_HEIGHT - SIDE * 3;
			args.y += EDT_HEIGHT + SIDE;
			args.style = 0;
			args.MsgProc = PartListMsgProc;
			GCLstCreate(&PartList, &args, wnd->obj.gid, &wnd->obj, PartListSelProc);
			args.width = wnd->client.width - PART_WIDTH - BTN_WIDTH - SIDE * 4;
			args.x += PART_WIDTH + SIDE;
			args.MsgProc = FileListMsgProc;
			GCLstCreate(&FileList, &args, wnd->obj.gid, &wnd->obj, FileListSelProc);
			args.x += args.width + SIDE;
			args.width = BTN_WIDTH;
			args.height = BTN_HEIGHT;
			args.MsgProc = NULL;
			GCBtnCreate(&ParBtn, &args, wnd->obj.gid, &wnd->obj, "上级目录", NULL, ParBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			args.style = BTN_STYLE_DISABLED;
			GCBtnCreate(&CutBtn, &args, wnd->obj.gid, &wnd->obj, "剪切", NULL, CutBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&CopyBtn, &args, wnd->obj.gid, &wnd->obj, "复制", NULL, CopyBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&PasteBtn, &args, wnd->obj.gid, &wnd->obj, "粘贴", NULL, PasteBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&DelBtn, &args, wnd->obj.gid, &wnd->obj, "删除", NULL, DelBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			args.style = 0;
			GCBtnCreate(&DirBtn, &args, wnd->obj.gid, &wnd->obj, "创建目录", NULL, DirBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&FileBtn, &args, wnd->obj.gid, &wnd->obj, "创建文件", NULL, FileBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			args.style = BTN_STYLE_DISABLED;
			GCBtnCreate(&RenBtn, &args, wnd->obj.gid, &wnd->obj, "重命名", NULL, RenamBtnPressProc);
		}
		break;
	case GM_SIZE:
		{
			long x, y;
			DWORD width, height;

			GCWndGetClientLoca(wnd, &x, &y);
			width = wnd->client.width - SIDE * 2;
			height = EDT_HEIGHT;
			x += SIDE;
			y += SIDE;
			GCGobjSetSize(&DirSedt->obj, x, y, width, height);
			width = PART_WIDTH;
			height = wnd->client.height - EDT_HEIGHT - SIDE * 3;
			y += EDT_HEIGHT + SIDE;
			GCLstSetSize(PartList, x, y, width, height);
			width = wnd->client.width - PART_WIDTH - BTN_WIDTH - SIDE * 4;
			x += PART_WIDTH + SIDE;
			GCLstSetSize(FileList, x, y, width, height);
			x += width + SIDE;
			GCGobjMove(&ParBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&CutBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&CopyBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&PasteBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&DelBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&DirBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&FileBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&RenBtn->obj, x, y);
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
	args.x = (GCwidth - args.width) / 2;	/*居中*/
	args.y = (GCheight - args.height) / 2;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN | WND_STYLE_MAXBTN | WND_STYLE_MINBTN | WND_STYLE_SIZEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&MainWnd, &args, 0, NULL, "资源管理器");
	MainWnd->MinWidth = WND_WIDTH;
	MainWnd->MinHeight = WND_HEIGHT;

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*处理GUI消息*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)MainWnd)	/*销毁主窗体,退出程序*/
				break;
		}
	}
	return NO_ERROR;
}
