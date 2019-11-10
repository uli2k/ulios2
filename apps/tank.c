/*	tank.c for ulios application
	作者：孙亮
	功能：坦克游戏
	最后修改日期：2017-06-29
*/

#include "../lib/malloc.h"
#include "../lib/gclient.h"

// 方向
#define DIR_UP 0	// 上
#define DIR_DOWN 1  // 下
#define DIR_LEFT 2  // 左
#define DIR_RIGHT 3 // 右

// 行动
#define ACT_IDLE 15 // 静止
#define ACT_FIRE 16 // 发射

// 坦克状态
#define STATE_INIT 0	// 初始
#define STATE_IDLE 1	// 空闲
#define STATE_FROZ 2	// 冻结
#define STATE_DEAD 3	// 阵亡
#define STATE_SUPER 4	// 无敌

// 地图属性
#define ATTR_SPACE 0	// 空地
#define ATTR_ROAD 1		// 马路
#define ATTR_WALL 2		// 砖墙
#define ATTR_IRON 3		// 铁墙
#define ATTR_RIVER 4	// 河流
#define ATTR_GRASS 5	// 草地
#define ATTR_HOME 6		// 巢穴

#define PRICE_SUPER 0	// 无敌
#define PRICE_ENERGY 1	// 增加能量
#define PRICE_ARMOR 2	// 增加护甲
#define PRICE_NUM 3		// 增加数量
#define PRICE_BOMB 4	// 全爆机器人
#define PRICE_HOME 5	// 加固巢

typedef struct // 炮弹
{
	short x, y;		// 位置
	BYTE clock;		// 计时器
	BYTE dir;		// 方向
	BYTE speed;		// 速度
	BYTE energy;	// 能量
} SHELL;

typedef struct // 坦克
{
	short x, y;		// 位置
	BYTE clock;		// 计时器
	BYTE dir;		// 方向
	BYTE speed;		// 速度
	BYTE armor;		// 护甲
	BYTE state;		// 状态
	BYTE action;	// 行动
	BYTE s_speed;	// 炮弹速度
	BYTE s_energy;	// 炮弹能量
	SHELL shell;	// 炮弹
} TANK;

typedef struct // 地图块
{
	BYTE attr; // 属性
} MAP;

// 游戏设置
#define ALIGN_SIZE 8		// 行进对齐尺寸
#define ALIGN_MASK 0xFFF8	// 行进对齐模板
#define BLOCK_WIDTH 16		// 图形块大小
#define MAP_WIDTH 40		// 地图宽度
#define MAP_HEIGHT 30		// 地图高度
#define TANK_NUM 6			// 坦克数量
#define HOME_X 21			// 巢位置
#define HOME_Y 29
#define INIT_X 19			// 初生位置
#define INIT_Y 29
#define INIT_DIR DIR_UP		// 初生方向
#define RBT_INIT_Y 0		// 机器人初生行位置

// 时间控制
#define CLK_INIT 16			// 初始冻结时间
#define CLK_INIT_SUPER 64	// 初始无敌状态时间
#define CLK_SUPER 250		// 初始无敌状态时间
#define CLK_DEAD 16			// 阵亡状态时间

// 全局数据
DWORD *ImageBuf;
TANK *tanks;
MAP *map;
BYTE MyNum, RbtNum;		// 我方和敌方坦克剩余数量
BYTE PriceAttr;			// 奖励属性
short PriceX, PriceY;	// 奖励位置
THREAD_ID GLPtid;

BOOL LoadImage()
{
	DWORD width, height;
	if (GCLoadBmp("tank.bmp", NULL, 0, &width, &height) != NO_ERROR)
	{
		CUIPutS("Read tank.bmp error!");
		return FALSE;
	}
	if (width != BLOCK_WIDTH || height != 28 * BLOCK_WIDTH)
	{
		CUIPutS("Image size error!");
		return FALSE;
	}
	if ((ImageBuf = (DWORD *)malloc((32 + 20) * BLOCK_WIDTH * BLOCK_WIDTH * sizeof(DWORD))) == NULL)
	{
		CUIPutS("Out of memory!");
		return FALSE;
	}
	GCLoadBmp("mouse.bmp", ImageBuf, 28 * BLOCK_WIDTH * BLOCK_WIDTH, NULL, NULL);
	memcpy32(&ImageBuf[32 * BLOCK_WIDTH * BLOCK_WIDTH], &ImageBuf[8 * BLOCK_WIDTH * BLOCK_WIDTH], 20 * BLOCK_WIDTH * BLOCK_WIDTH); // 搬移除坦克以外的图片
	for (DWORD *SrcImg = ImageBuf; SrcImg < &ImageBuf[8 * BLOCK_WIDTH * BLOCK_WIDTH]; SrcImg += BLOCK_WIDTH * BLOCK_WIDTH)		   // 产生每个方向的坦克图片
	{
		DWORD *DestImgDown = ImageBuf + 8 * BLOCK_WIDTH * BLOCK_WIDTH;
		DWORD *DestImgLeft = ImageBuf + 16 * BLOCK_WIDTH * BLOCK_WIDTH;
		DWORD *DestImgRight = ImageBuf + 24 * BLOCK_WIDTH * BLOCK_WIDTH;
		for (DWORD y = 0; y < BLOCK_WIDTH; y++)
			for (DWORD x = 0; x < BLOCK_WIDTH; x++)
			{
				DWORD color = SrcImg[x + y * BLOCK_WIDTH];
				DestImgDown[x + (BLOCK_WIDTH - 1 - y) * BLOCK_WIDTH] = color;
				DestImgLeft[y + x * BLOCK_WIDTH] = color;
				DestImgRight[y + (BLOCK_WIDTH - 1 - x) * BLOCK_WIDTH] = color;
			}
	}
	return TRUE;
}

void hua(ui x, ui y, uc n, uc pic[][16][16], uc spic[][4][4])
{
	int i, j;
	uc *p = pic[n % 8];
	x += y * 320;
	switch (n / 8)
	{
	case 1:
		p += 256;
		for (i = 15; i >= 0; i--)
		{
			p -= 16;
			for (j = 0; j < 16; j++)
				if (y = *(p + j))
					pokeb(0xa000, x + j, y);
			x += 320;
		}
		break;
	case 2:
		for (i = 0; i < 16; i++)
		{
			for (j = 0; j < 16; j++)
				if (y = *(p + j * 16 + i))
					pokeb(0xa000, x + j, y);
			x += 320;
		}
		break;
	case 3:
		for (i = 0; i < 16; i++)
		{
			for (j = 0; j < 16; j++)
				if (y = *(p + (15 - j) * 16 + i))
					pokeb(0xa000, x + j, y);
			x += 320;
		}
		break;
	default:
		if (n < 51)
		{
			if (n >= 32)
				p = pic[n - 24];
			for (i = 0; i < 16; i++)
			{
				for (j = 0; j < 16; j++)
					if (y = *(p + j))
						pokeb(0xa000, x + j, y);
				x += 320;
				p += 16;
			}
		}
		else
		{
			p = spic[n - 51];
			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 4; j++)
					if (y = *(p + j))
						pokeb(0xa000, x + j, y);
				x += 320;
				p += 4;
			}
		}
	}
}

#define KEY_UP 0x4800
#define KEY_LEFT 0x4B00
#define KEY_RIGHT 0x4D00
#define KEY_DOWN 0x5000

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND *)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_SETFOCUS:
		if (data[1])
			wnd->obj.style |= WND_STATE_FOCUS;
		else
			wnd->obj.style &= (~WND_STATE_FOCUS);
		break;
	case GM_LBUTTONDOWN: /*鼠标按下*/
		if (!(wnd->obj.style & WND_STATE_FOCUS))
			GUISetFocus(wnd->obj.gid);
		break;
	case GM_KEY: /*按键消息*/
		switch (data[1] & 0xFFFF)
		{
		case KEY_LEFT:
			tanks[0].action = DIR_LEFT;
			break;
		case KEY_RIGHT:
			tanks[0].action = DIR_RIGHT;
			break;
		case KEY_UP:
			tanks[0].action = DIR_UP;
			break;
		case KEY_DOWN:
			tanks[0].action = DIR_DOWN;
			break;
		}
		if (data[1] & KBD_STATE_LCTRL)
			tanks[0].action |= ACT_FIRE;
		break;
	}
	return GCWndDefMsgProc(ptid, data);
}

BOOL isPassable(short mapx, short mapy)
{
	BYTE attr = map[mapx + mapy * MAP_WIDTH].attr;
	return attr == ATTR_SPACE || attr == ATTR_ROAD || attr == ATTR_GRASS;
}

void GameLoopThread()
{
	// 机器人随机决策
	for (TANK *tank = &tanks[1]; tank < &tanks[TANK_NUM]; tank++)
	{
		switch (TMGetRand() % 25)
		{
		case 0:
			tank->action = DIR_UP;
			break;
		case 1:
			tank->action = DIR_DOWN;
			break;
		case 2:
			tank->action = DIR_LEFT;
			break;
		case 3:
			tank->action = DIR_RIGHT;
			break;
		case 4:
			tank->action |= ACT_FIRE;
			break;
		}
	}
	// 坦克行动
	for (TANK *tank = tanks; tank < &tanks[TANK_NUM]; tank++)
	{
		if (tank->clock) // 检查自动状态是否到期
			tank->clock--;
		else // 到期切换状态
		{
			switch (tank->state)
			{
			case STATE_INIT: // 初始切换为无敌
				tank->state = STATE_SUPER;
				tank->clock = CLK_INIT_SUPER;
				break;
			case STATE_FROZ: // 冻结或无敌失效
			case STATE_SUPER:
				tank->state = STATE_IDLE;
				break;
			case STATE_DEAD:	   // 阵亡后复活或结束
				if (tank == tanks) // 是否是我方
				{
					if (MyNum)
						MyNum--;
				}
				else
				{
					if (RbtNum)
						RbtNum--;
				}
				tank->state = STATE_INIT;
				tank->clock = CLK_INIT;
				break;
			}
		}
		if (tank->state == STATE_IDLE || tank->state == STATE_SUPER) // 空闲和无敌状态都是可行动状态
		{
			short mapx, mapy;
			switch (tank->action & ACT_IDLE)
			{
			case DIR_UP: // 向上行进
				tank->dir = DIR_UP;
				mapx = (tank->x + BLOCK_WIDTH - 1) / BLOCK_WIDTH;
				mapy = (tank->y - 1) / BLOCK_WIDTH;
				if (isPassable(tank->x / BLOCK_WIDTH, (tank->y - 1) / BLOCK_WIDTH))
				{
					if (isPassable(mapx, mapy))
					{
						tank->y -= tank->speed;
						if (tank->y < 0)
							tank->y = 0;
					}
					else
						tank->x &= ALIGN_MASK;
				}
				else if (isPassable(mapx, mapy))
					tank->x = ((tank->x - 1) & ALIGN_MASK) + ALIGN_SIZE;
				break;
			case DIR_DOWN: // 向下行进
				tank->dir = DIR_DOWN;
				mapx = (tank->x + BLOCK_WIDTH - 1) / BLOCK_WIDTH;
				mapy = (tank->y + BLOCK_WIDTH) / BLOCK_WIDTH;
				if (isPassable(tank->x / BLOCK_WIDTH, (tank->y + BLOCK_WIDTH) / BLOCK_WIDTH))
				{
					if (isPassable(mapx, mapy))
					{
						tank->y += tank->speed;
						if (tank->y > (MAP_HEIGHT - 1) * BLOCK_WIDTH)
							tank->y = (MAP_HEIGHT - 1) * BLOCK_WIDTH;
					}
					else
						tank->x &= ALIGN_MASK;
				}
				else if (isPassable(mapx, mapy))
					tank->x = ((tank->x - 1) & ALIGN_MASK) + ALIGN_SIZE;
				break;
			case DIR_LEFT: // 向左行进
				tank->dir = DIR_LEFT;
				mapx = (tank->x - 1) / BLOCK_WIDTH;
				mapy = (tank->y + BLOCK_WIDTH - 1) / BLOCK_WIDTH;
				if (isPassable((tank->x - 1) / BLOCK_WIDTH, tank->y / BLOCK_WIDTH))
				{
					if (isPassable(mapx, mapy))
					{
						tank->x -= tank->speed;
						if (tank->x < 0)
							tank->x = 0;
					}
					else
						tank->y &= ALIGN_MASK;
				}
				else if (isPassable(mapx, mapy))
					tank->y = ((tank->y - 1) & ALIGN_MASK) + ALIGN_SIZE;
				break;
			case DIR_RIGHT: // 向右行进
				tank->dir = DIR_RIGHT;
				mapx = (tank->x + BLOCK_WIDTH) / BLOCK_WIDTH;
				mapy = (tank->y + BLOCK_WIDTH - 1) / BLOCK_WIDTH;
				if (isPassable((tank->x + BLOCK_WIDTH) / BLOCK_WIDTH, tank->y / BLOCK_WIDTH))
				{
					if (isPassable(mapx, mapy))
					{
						tank->x += tank->speed;
						if (tank->x > (MAP_WIDTH - 1) * BLOCK_WIDTH)
							tank->x = (MAP_WIDTH - 1) * BLOCK_WIDTH;
					}
					else
						tank->y &= ALIGN_MASK;
				}
				else if (isPassable(mapx, mapy))
					tank->y = ((tank->y - 1) & ALIGN_MASK) + ALIGN_SIZE;
				break;
			}
			if ((tank->action & ACT_FIRE) && tank->shell.speed == 0) // 开火
			{
				tank->shell.x = tank->x;
				tank->shell.y = tank->y;
				tank->shell.clock = 0;
				tank->shell.dir = tank->dir;
				tank->shell.speed = tank->s_speed;
				tank->shell.energy = tank->s_energy;
			}
		}
		// 奖励检测
		if (PriceAttr != 0xFF)
		{
			if (tanks->x > PriceX - BLOCK_WIDTH && tanks->x < PriceX + BLOCK_WIDTH &&
				tanks->y > PriceY - BLOCK_WIDTH && tanks->y < PriceY + BLOCK_WIDTH)
			{
				short mapx, mapy;
				switch (PriceAttr)
				{
				case PRICE_SUPER:
					tanks->state = STATE_SUPER;
					tanks->clock = CLK_SUPER;
					break;
				case PRICE_ENERGY:
					if (tanks->s_speed < 8)
						tanks->s_speed++;
					if (tanks->s_energy < 4)
						tanks->s_energy++;
					break;
				case PRICE_ARMOR:
					if (tanks->speed < 4)
						tanks->speed++;
					if (tanks->armor < 4)
						tanks->armor++;
					break;
				case PRICE_NUM:
					MyNum++;
					break;
				case PRICE_BOMB:
					for (TANK *tank = &tanks[1]; tank < &tanks[TANK_NUM]; tank++)
					{
						tank->state = STATE_DEAD;
						tank->clock = CLK_DEAD;
					}
					RbtNum -= 5;
					if (RbtNum > 250)
						RbtNum = 0;
					break;
				case PRICE_HOME:
					for (mapy = HOME_Y - 1; mapy < HOME_Y + 1; mapy++)
						for (mapx = HOME_X - 1; mapx < HOME_X + 1; mapx++)
							if (mapx >= 0 && mapx < MAP_WIDTH && mapy >= 0 && mapy < MAP_HEIGHT)
								map[mapx + mapy * MAP_WIDTH].attr = ATTR_IRON;
					m[HOME_X + HOME_Y * MAP_WIDTH].attr = ATTR_HOME;
					break;
				}
			}
		}
	}

	KExitThread(NO_ERROR);
}

int main()
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	CTRL_ARGS args;
	long res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR) /*设置16MB堆内存*/
		return res;
	if (!LoadImage())
		return 1;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = BLOCK_WIDTH * MAP_WIDTH;
	args.height = BLOCK_WIDTH * MAP_HEIGHT;
	args.x = 200;
	args.y = 100;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "坦克大战");

	KCreateThread(GameLoopThread, 0x4000, NULL, &GLPtid);
	for (;;)
	{
		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR) /*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR) /*处理GUI消息*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)wnd) /*销毁主窗体,退出程序*/
				break;
		}
	}
	KKillThread(GLPtid.ThedID);
	return NO_ERROR;
}