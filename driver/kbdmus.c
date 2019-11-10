/*	kbdmus.c for ulios driver
	作者：孙亮
	功能：8042键盘鼠标控制器驱动服务程序
	最后修改日期：2010-05-18
*/

#include "basesrv.h"

#define KBD_IRQ	0x1	/*键盘中断请求号*/
#define MUS_IRQ	0xC	/*鼠标中断请求号*/

const BYTE KeyMap[] = {0,	/*美国英语键盘主键码表*/
	27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,	/*(1-14)ESC--BACKSPACE*/
	9,  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,	/*(15-28)TAB--ENTER*/
	0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'',			/*(29-40)CTRL--'*/
	'`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,	/*(41-54)`--RSHIFT*/
	0,   0,  ' ',  0,													/*(55-58)PrtSc--CapsLock*/
	2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   0,   0,			/*(59-70)F1--ScrLock*/
	1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   1,		/*(71-83)HOME--DEL*/
	0,   0,   0,   2,   2,   0,   0,   2,   2,   2						/*(84-93)F11--APPS*/
};

const BYTE KeyMapEx[] = {0,	/*美国英语键盘扩展键码表*/
	27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8,	/*(1-14)ESC--BACKSPACE*/
	9,  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13,	/*(15-28)TAB--ENTER*/
	0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"',			/*(29-40)<CTRL>*/
	'~', 0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,	/*(41-54)<LSHIFT><RSHIFT>*/
	0,   0,  ' ',  0,													/*(55-58)<ALT><CapsLock>*/
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,			/*(59-70)<NumLock><ScrLock>*/
	'7','8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',		/*(71-83)HOME--DEL*/
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0						/*(84-93)F11--APPS*/
};

#define RECVPTID_LEN	0x300	/*接收消息线程ID栈长度*/

/*向8042发送命令*/
static inline void Cmd8042(BYTE b)
{
	DWORD i;

	i = 0x2000;
	while ((inb(0x64) & 0x02) && --i);
	outb(0x64, b);
}

/*读8042数据*/
static inline BYTE Read8042()
{
	DWORD i;

	i = 0x2000;
	while (!(inb(0x64) & 0x01) && --i);
	return inb(0x60);
}

/*写8042数据*/
static inline void Write8042(BYTE b)
{
	DWORD i;

	i = 0x2000;
	while ((inb(0x64) & 0x02) && --i);
	outb(0x60, b);
}

/*等待8042回复*/
static inline void Ack8042()
{
	DWORD i;

	i = 0x2000;
	while ((inb(0x60) != 0xFA) && --i);
}

/*向键盘发送命令*/
void OutKeyboard(BYTE b)
{
	Write8042(b);
	Ack8042();
}

/*向鼠标发送命令*/
void OutMouse(BYTE b)
{
	Cmd8042(0xD4);	/*通知8042数据将要发向鼠标*/
	Write8042(b);
	Ack8042();
}

int main()
{
	DWORD code;	/*端口取码*/
	DWORD KbdFlag, KbdState;	/*键码扫描码,前缀标志,控制键状态*/
	DWORD MusId, MusCou, MusKey;		/*鼠标ID,鼠标数据包计数,键状态*/
	long MusX, MusY, MusZ;	/*三维偏移*/
	THREAD_ID RecvPtid[RECVPTID_LEN], *CurRecv;	/*接收消息线程ID栈*/
	long res;	/*返回结果*/

	KDebug("booting... kbdmus driver\n");
	if ((res = KRegKnlPort(SRV_KBDMUS_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	/*键盘初始化*/
	OutKeyboard(0xED);	/*设置LED灯命令*/
	OutKeyboard(0x00);	/*设置LED状态全部关闭*/
	OutKeyboard(0xF4);	/*清空键盘缓冲*/
	/*鼠标初始化*/
	Cmd8042(0xA8);	/*允许鼠标接口*/
	OutMouse(0xF3);	/*设置鼠标采样率*/
	OutMouse(0xC8);	/*采样率200*/
	OutMouse(0xF3);	/*设置鼠标采样率*/
	OutMouse(0x64);	/*采样率100*/
	OutMouse(0xF3);	/*设置鼠标采样率*/
	OutMouse(0x50);	/*采样率80*/
	OutMouse(0xF2);	/*取得鼠标ID*/
	MusId = Read8042();
	if (MusId == 0xFA)
		MusId = 0;
	OutMouse(0xF4);	/*允许鼠标发数据*/
	/*打开中断位*/
	Cmd8042(0x60);	/*发往8042命令寄存器*/
	Write8042(0x47);	/*打开键盘鼠标中断位*/
	if ((res = KRegIrq(KBD_IRQ)) != NO_ERROR)	/*注册键盘中断请求号*/
		return res;
	if ((res = KRegIrq(MUS_IRQ)) != NO_ERROR)	/*注册鼠标中断请求号*/
		return res;
	KbdState = KbdFlag = 0;
	MusKey = MusCou = 0;
	MusZ = MusY = MusX = 0;
	CurRecv = RecvPtid - 1;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		switch (data[MSG_ATTR_ID] & MSG_ATTR_MASK)
		{
		case MSG_ATTR_IRQ:	/*中断请求消息*/
			code = inb(0x60);
			switch (data[1])
			{
			case KBD_IRQ:	/*键盘中断消息*/
				if (code == 0xE0)	/*前缀,后面跟1个键码*/
				{
					KbdFlag = 1;
					continue;
				}
				if (code == 0xE1)	/*前缀,后面跟2个键码*/
				{
					KbdFlag = 2;
					continue;
				}
				if (KbdFlag == 2)
				{
					if (code == 0xC5)	/*pause键按下*/
					{
						KbdState |= KBD_STATE_PAUSE;
						KbdFlag = 0;
					}
					continue;
				}
				if (KbdState & KBD_STATE_PAUSE)	/*pause键退出*/
				{
					KbdState &= (~KBD_STATE_PAUSE);
					continue;
				}
				if (code & 0x80)	/*松开键*/
				{
					code &= 0x7F;
					if (code < sizeof(KeyMap) && KeyMap[code] == 0)	/*松开控制键*/
					{
						switch (code)
						{
						case 29:
							if (KbdFlag)
								KbdState &= (~KBD_STATE_RCTRL);	/*右ctrl*/
							else
								KbdState &= (~KBD_STATE_LCTRL);	/*左ctrl*/
							break;
						case 54:
							KbdState &= (~KBD_STATE_RSHIFT);	/*右shift*/
							break;
						case 42:
							if (!KbdFlag)
								KbdState &= (~KBD_STATE_LSHIFT);	/*左shift*/
							break;
						case 56:
							if (KbdFlag)
								KbdState &= (~KBD_STATE_RALT);	/*右alt*/
							else
								KbdState &= (~KBD_STATE_LALT);	/*左alt*/
							break;
						case 55:
							KbdState &= (~KBD_STATE_PRTSC);	/*PrtSc*/
							break;
						case 82:
							KbdState &= (~(KBD_STATE_INSERT << 4));	/*Insert*/
							break;
						case 58:
							KbdState &= (~(KBD_STATE_CAPSLOCK << 4));	/*CapsLock*/
							break;
						case 69:
							KbdState &= (~(KBD_STATE_NUMLOCK << 4));	/*NumLock*/
							break;
						case 70:
							KbdState &= (~(KBD_STATE_SCRLOCK << 4));	/*ScrLock*/
							break;
						}
					}
				}
				else	/*按下键*/
				{
					DWORD KbdKey;

					if (code < sizeof(KeyMap))
					{
						if (KeyMap[code] > 2)	/*主键盘键*/
						{
							if (KeyMap[code] >= 'a' && KeyMap[code] <= 'z')	/*按了字母键*/
							{
								if (((KbdState & KBD_STATE_CAPSLOCK) != 0) ^ ((KbdState & (KBD_STATE_LSHIFT | KBD_STATE_RSHIFT)) != 0))	/*caps开或shift按下其一符合*/
									KbdKey = KeyMapEx[code];
								else
									KbdKey = KeyMap[code];
							}
							else if (KbdState & (KBD_STATE_LSHIFT | KBD_STATE_RSHIFT))	/*有shift按下*/
								KbdKey = KeyMapEx[code];
							else
								KbdKey = KeyMap[code];
						}
						else if (KeyMap[code] == 2)	/*功能键*/
							KbdKey = code << 8;
						else if (KeyMap[code] == 1)	/*小键盘*/
						{
							if (KbdState & KBD_STATE_NUMLOCK)	/*NumLock开*/
								KbdKey = KeyMapEx[code];
							else
								KbdKey = code << 8;
						}
						else	/*控制键*/
						{
							KbdKey = 0;
							switch (code)
							{
							case 29:
								if (KbdFlag)
									KbdState |= KBD_STATE_RCTRL;	/*右ctrl*/
								else
									KbdState |= KBD_STATE_LCTRL;	/*左ctrl*/
								break;
							case 54:
								KbdState |= KBD_STATE_RSHIFT;	/*右shift*/
								break;
							case 42:
								if (!KbdFlag)
									KbdState |= KBD_STATE_LSHIFT;	/*左shift*/
								break;
							case 56:
								if (KbdFlag)
									KbdState |= KBD_STATE_RALT;	/*右alt*/
								else
									KbdState |= KBD_STATE_LALT;	/*左alt*/
								break;
							case 55:	/*PrtSc*/
								KbdState |= KBD_STATE_PRTSC;
								KbdKey = code << 8;
								break;
							case 82:	/*Insert*/
								if (KbdState & KBD_STATE_NUMLOCK)	/*numlock开*/
									KbdKey = KeyMapEx[code];
								else if (!(KbdState & (KBD_STATE_INSERT << 4)))	/*没有按住*/
								{
									KbdState |= (KBD_STATE_INSERT << 4);
									KbdState ^= KBD_STATE_INSERT;
								}
								break;
							case 58:	/*CapsLock*/
								if (!(KbdState & (KBD_STATE_CAPSLOCK << 4)))
								{
									KbdState |= (KBD_STATE_CAPSLOCK << 4);
									KbdState ^= KBD_STATE_CAPSLOCK;
								}
								break;
							case 69:	/*NumLock*/
								if (!(KbdState & (KBD_STATE_NUMLOCK << 4)))
								{
									KbdState |= (KBD_STATE_NUMLOCK << 4);
									KbdState ^= KBD_STATE_NUMLOCK;
								}
								break;
							case 70:	/*ScrLock*/
								if (!(KbdState & (KBD_STATE_SCRLOCK << 4)))
								{
									KbdState |= (KBD_STATE_SCRLOCK << 4);
									KbdState ^= KBD_STATE_SCRLOCK;
								}
								break;
							}
						}
					}
					else
						KbdKey = code << 8;	/*不能处理的键直接放入队列*/
					KbdKey |= KbdState;
					data[MSG_ATTR_ID] = MSG_ATTR_KBD;
					data[1] = KbdKey;
					while (CurRecv >= RecvPtid)
					{
						res = KSendMsg(CurRecv, data, 0);
						if (res != KERR_PROC_NOT_EXIST && res != KERR_THED_NOT_EXIST)
							break;
						CurRecv--;	/*无法发送到栈中的线程则忽略该线程*/
					}
				}
				KbdFlag = 0;
				break;
			case MUS_IRQ:	/*鼠标中断消息*/
				switch (MusCou)
				{
				case 0:	/*收到第一字节,按键信息*/
					if (!(code & 0x08))
						continue;
					MusKey = code;
					MusCou++;
					continue;
				case 1:	/*收到第二字节,x的位移量*/
					if (MusKey & MUS_STATE_XSIGN)
						code |= 0xFFFFFF00;
					MusX = (long)code;
					MusCou++;
					continue;
				case 2:	/*收到第三字节,y的位移量*/
					if (MusKey & MUS_STATE_YSIGN)
						code |= 0xFFFFFF00;
					MusY = -(long)code;
					if (MusId)
					{
						MusCou++;
						continue;
					}
					MusCou = 0;
					break;
				case 3:	/*收到第四字节,滚轮位移量*/
					MusZ = (char)code;
					MusCou = 0;
					break;
				}
				data[MSG_ATTR_ID] = MSG_ATTR_MUS;
				data[1] = MusKey;
				data[2] = MusX;
				data[3] = MusY;
				data[4] = MusZ;
				while (CurRecv >= RecvPtid)
				{
					res = KSendMsg(CurRecv, data, 0);
					if (res != KERR_PROC_NOT_EXIST && res != KERR_THED_NOT_EXIST)
						break;
					CurRecv--;	/*无法发送到栈中的线程则忽略该线程*/
				}
				break;
			}
			break;
		case MSG_ATTR_KBDMUS:	/*应用请求消息*/
			if ((data[MSG_API_ID] & MSG_API_MASK) == KBDMUS_API_SETRECV)
				if (CurRecv < &RecvPtid[RECVPTID_LEN - 1])	/*PTID栈不满时注册接收键盘鼠标消息的线程*/
					*(++CurRecv) = ptid;
			break;
		}
	}
	KUnregIrq(KBD_IRQ);
	KUnregIrq(MUS_IRQ);
	KUnregKnlPort(SRV_KBDMUS_PORT);
	return NO_ERROR;
}
