/*	ime.c for ulios driver
	作者：孙亮
	功能：输入法(全拼汉字输入法)
	最后修改日期：2011-11-07
*/

#include "../lib/malloc.h"
#include "../lib/gclient.h"

#define IN_BUF_LEN	8	/*输入缓冲长度*/
#define HZ_PRE_PARG	10	/*每页汉字数*/
#define MIN(a, b)	((a) < (b) ? (a) : (b))

typedef struct _PYNODE
{
	char pystr[IN_BUF_LEN];	/*拼音字符串*/
	DWORD hzid;		/*首字内码ID*/
}PYNODE;

#define PYTAB_LEN	397

const PYNODE pytab[PYTAB_LEN] =
{
	{"a",		1410},	{"ai",		1412},	{"an",		1425},	{"ang",		1434},
	{"ao",		1437},	{"ba",		1446},	{"bai",		1464},	{"ban",		1472},
	{"bang",	1487},	{"bao",		1499},	{"bei",		1516},	{"ben",		1531},
	{"beng",	1535},	{"bi",		1541},	{"bian",	1565},	{"biao",	1577},
	{"bie",		1581},	{"bin",		1585},	{"bing",	1591},	{"bo",		1600},
	{"bu",		1621},	{"ca",		1630},	{"cai",		1631},	{"can",		1642},
	{"cang",	1649},	{"cao",		1654},	{"ce",		1659},	{"ceng",	1664},
	{"cha",		1666},	{"chai",	1677},	{"chan",	1680},	{"chang",	1690},
	{"chao",	1703},	{"che",		1712},	{"chen",	1718},	{"cheng",	1728},
	{"chi",		1743},	{"chong",	1759},	{"chou",	1764},	{"chu",		1776},
	{"chuai",	1792},	{"chuan",	1793},	{"chuang",	1800},	{"chui",	1806},
	{"chun",	1811},	{"chuo",	1818},	{"ci",		1820},	{"cong",	1832},
	{"cou",		1838},	{"cu",		1839},	{"cuan",	1843},	{"cui",		1846},
	{"cun",		1854},	{"cuo",		1857},	{"da",		1863},	{"dai",		1869},
	{"dan",		1881},	{"dang",	1896},	{"dao",		1901},	{"de",		1913},
	{"deng",	1916},	{"di",		1923},	{"dian",	1942},	{"diao",	1958},
	{"die",		1967},	{"ding",	1974},	{"diu",		1983},	{"dong",	1984},
	{"dou",		1994},	{"du",		2001},	{"duan",	2016},	{"dui",		2022},
	{"dun",		2026},	{"duo",		2035},	{"e",		2047},	{"en",		2060},
	{"er",		2061},	{"fa",		2069},	{"fan",		2077},	{"fang",	2094},
	{"fei",		2105},	{"fen",		2117},	{"feng",	2132},	{"fo",		2147},
	{"fou",		2148},	{"fu",		2149},	{"ga",		2194},	{"gai",		2196},
	{"gan",		2202},	{"gang",	2213},	{"gao",		2222},	{"ge",		2232},
	{"gei",		2249},	{"gen",		2250},	{"geng",	2252},	{"gong",	2259},
	{"gou",		2274},	{"gu",		2283},	{"gua",		2301},	{"guai",	2307},
	{"guan",	2310},	{"guang",	2321},	{"gui",		2324},	{"gun",		2340},
	{"guo",		2343},	{"ha",		2349},	{"hai",		2350},	{"han",		2357},
	{"hang",	2376},	{"hao",		2379},	{"he",		2388},	{"hei",		2406},
	{"hen",		2408},	{"heng",	2412},	{"hong",	2417},	{"hou",		2426},
	{"hu",		2433},	{"hua",		2451},	{"huai",	2460},	{"huan",	2465},
	{"huang",	2479},	{"hui",		2493},	{"hun",		2514},	{"huo",		2520},
	{"ji",		2530},	{"jia",		2583},	{"jian",	2600},	{"jiang",	2640},
	{"jiao",	2653},	{"jie",		2681},	{"jin",		2708},	{"jing",	2728},
	{"jiong",	2753},	{"jiu",		2755},	{"ju",		2772},	{"juan",	2797},
	{"jue",		2804},	{"jun",		2814},	{"ka",		2825},	{"kai",		2829},
	{"kan",		2834},	{"kang",	2840},	{"kao",		2847},	{"ke",		2851},
	{"ken",		2866},	{"keng",	2870},	{"kong",	2872},	{"kou",		2876},
	{"ku",		2880},	{"kua",		2887},	{"kuai",	2892},	{"kuan",	2896},
	{"kuang",	2898},	{"kui",		2906},	{"kun",		2917},	{"kuo",		2921},
	{"la",		2925},	{"lai",		2932},	{"lan",		2935},	{"lang",	2950},
	{"lao",		2957},	{"le",		2966},	{"lei",		2968},	{"leng",	2979},
	{"li",		2982},	{"lia",		3016},	{"lian",	3017},	{"liang",	3031},
	{"liao",	3042},	{"lie",		3055},	{"lin",		3060},	{"ling",	3072},
	{"liu",		3086},	{"long",	3097},	{"lou",		3106},	{"lu",		3112},
	{"lv",		3132},	{"luan",	3146},	{"lue",		3152},	{"lun",		3154},
	{"luo",		3161},	{"ma",		3173},	{"mai",		3182},	{"man",		3188},
	{"mang",	3197},	{"mao",		3203},	{"me",		3215},	{"mei",		3216},
	{"men",		3232},	{"meng",	3235},	{"mi",		3243},	{"mian",	3257},
	{"miao",	3266},	{"mie",		3274},	{"min",		3276},	{"ming",	3282},
	{"miu",		3288},	{"mo",		3289},	{"mou",		3306},	{"mu",		3309},
	{"na",		3324},	{"nai",		3331},	{"nan",		3336},	{"nang",	3339},
	{"nao",		3340},	{"ne",		3345},	{"nei",		3346},	{"nen",		3348},
	{"neng",	3349},	{"ni",		3350},	{"nian",	3361},	{"niang",	3368},
	{"niao",	3370},	{"nie",		3372},	{"nin",		3379},	{"ning",	3380},
	{"niu",		3386},	{"nong",	3390},	{"nu",		3394},	{"nv",		3397},
	{"nuan",	3398},	{"nue",		3399},	{"nuo",		3401},	{"o",		3405},
	{"ou",		3406},	{"pa",		3413},	{"pai",		3419},	{"pan",		3425},
	{"pang",	3433},	{"pao",		3438},	{"pei",		3445},	{"pen",		3454},
	{"peng",	3456},	{"pi",		3470},	{"pian",	3487},	{"piao",	3491},
	{"pie",		3495},	{"pin",		3497},	{"ping",	3502},	{"po",		3511},
	{"pu",		3520},	{"qi",		3535},	{"qia",		3571},	{"qian",	3574},
	{"qiang",	3596},	{"qiao",	3604},	{"qie",		3619},	{"qin",		3624},
	{"qing",	3635},	{"qiong",	3648},	{"qiu",		3650},	{"qu",		3658},
	{"quan",	3671},	{"que",		3682},	{"qun",		3690},	{"ran",		3692},
	{"rang",	3696},	{"rao",		3701},	{"re",		3704},	{"ren",		3706},
	{"reng",	3716},	{"ri",		3718},	{"rong",	3719},	{"rou",		3729},
	{"ru",		3732},	{"ruan",	3742},	{"rui",		3744},	{"run",		3747},
	{"ruo",		3749},	{"sa",		3751},	{"sai",		3754},	{"san",		3758},
	{"sang",	3762},	{"sao",		3765},	{"se",		3769},	{"sen",		3772},
	{"seng",	3773},	{"sha",		3774},	{"shai",	3783},	{"shan",	3785},
	{"shang",	3801},	{"shao",	3809},	{"she",		3820},	{"shen",	3832},
	{"sheng",	3848},	{"shi",		3859},	{"shou",	3906},	{"shu",		3916},
	{"shua",	3949},	{"shuai",	3951},	{"shuan",	3955},	{"shuang",	3957},
	{"shui",	3960},	{"shun",	3964},	{"shuo",	3968},	{"si",		3972},
	{"song",	3988},	{"sou",		3996},	{"su",		3999},	{"suan",	4012},
	{"sui",		4015},	{"sun",		4026},	{"suo",		4029},	{"ta",		4037},
	{"tai",		4046},	{"tan",		4055},	{"tang",	4073},	{"tao",		4086},
	{"te",		4097},	{"teng",	4098},	{"ti",		4102},	{"tian",	4117},
	{"tiao",	4125},	{"tie",		4130},	{"ting",	4133},	{"tong",	4143},
	{"tou",		4156},	{"tu",		4160},	{"tuan",	4171},	{"tui",		4173},
	{"tun",		4179},	{"tuo",		4182},	{"wa",		4193},	{"wai",		4200},
	{"wan",		4202},	{"wang",	4219},	{"wei",		4229},	{"wen",		4262},
	{"weng",	4272},	{"wo",		4275},	{"wu",		4284},	{"xi",		4313},
	{"xia",		4348},	{"xian",	4361},	{"xiang",	4387},	{"xiao",	4407},
	{"xie",		4425},	{"xin",		4446},	{"xing",	4456},	{"xiong",	4471},
	{"xiu",		4478},	{"xu",		4487},	{"xuan",	4506},	{"xue",		4516},
	{"xun",		4522},	{"ya",		4536},	{"yan",		4552},	{"yang",	4585},
	{"yao",		4602},	{"ye",		4617},	{"yi",		4632},	{"yin",		4685},
	{"ying",	4701},	{"yo",		4719},	{"yong",	4720},	{"you",		4735},
	{"yu",		4756},	{"yuan",	4800},	{"yue",		4820},	{"yun",		4830},
	{"za",		4842},	{"zai",		4845},	{"zan",		4852},	{"zang",	4856},
	{"zao",		4859},	{"ze",		4873},	{"zei",		4877},	{"zen",		4878},
	{"zeng",	4879},	{"zha",		4883},	{"zhai",	4897},	{"zhan",	4903},
	{"zhang",	4920},	{"zhao",	4935},	{"zhe",		4945},	{"zhen",	4955},
	{"zheng",	4971},	{"zhi",		4986},	{"zhong",	5029},	{"zhou",	5040},
	{"zhu",		5054},	{"zhua",	5080},	{"zhuai",	5082},	{"zhuan",	5083},
	{"zhuang",	5089},	{"zhui",	5096},	{"zhun",	5102},	{"zhuo",	5104},
	{"zi",		5115},	{"zong",	5130},	{"zou",		5137},	{"zu",		5141},
	{"zuan",	5149},	{"zui",		5151},	{"zun",		5155},	{"zuo",		5157},
	{"",		5165}
};

/*根据输入字符串查找首字ID和末字ID*/
void findhzid(const char *instr, long cid, DWORD *beg, DWORD *end)
{
	DWORD bid, eid, mid;	/*折半查找开始,结束,中间位置*/

	bid = *beg;
	eid = *end;
	while (instr[cid])
	{
		char srcc;
		srcc = instr[cid];
		while (bid <= eid)	/*折半查找*/
		{
			char dstc;
			mid = (bid + eid) / 2;
			dstc = pytab[mid].pystr[cid];
			if (srcc > dstc)
				bid = mid + 1;
			else if (srcc < dstc)
				eid = mid - 1;
			else	/*找到*/
			{
				bid = mid;	/*查找首字ID*/
				while (srcc == pytab[bid - 1].pystr[cid])
					bid--;
				eid = mid;	/*查找末字ID*/
				while (srcc == pytab[eid + 1].pystr[cid])
					eid++;
				break;
			}
		}
		cid++;
	}
	*beg = bid;
	*end = eid;
}

/*填充编辑框字符串*/
void FillEdtBuf(char *EdtBuf, const char *title, const char *InBuf, DWORD BegID, DWORD EndID)
{
	char id;
	EdtBuf = strcpy(EdtBuf, title) - 1;
	EdtBuf = strcpy(EdtBuf, InBuf) - 1;
	id = '0';
	while (BegID < EndID)
	{
		*EdtBuf++ = ' ';
		*EdtBuf++ = id++;
		*EdtBuf++ = '.';
		*EdtBuf++ = (char)(BegID / 94 + 161);
		*EdtBuf++ = (char)(BegID % 94 + 161);
		BegID++;
	}
	*EdtBuf = '\0';
}

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_SEDT *edt = (CTRL_SEDT*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_LBUTTONDOWN:
		GUIdrag(edt->obj.gid, GM_DRAGMOD_MOVE);	/*按下时拖动*/
		break;
	}
	return GCSedtDefMsgProc(ptid, data);
}

int main()
{
	char EdtBuf[64], InBuf[IN_BUF_LEN];	/*编辑框缓存,输入缓冲*/
	long InID, PageID;	/*输入计数器,选字页计数*/
	DWORD beg, end;	/*备选始末PYNODE结构ID*/
	CTRL_SEDT *edt;
	CTRL_ARGS args;

	long res;

	if ((res = KRegKnlPort(SRV_IME_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	if ((res = InitMallocTab(0x100000)) != NO_ERROR)	/*设置1MB堆内存*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	PageID = InID = 0;
	args.width = GCCharWidth * 60 + 2;
	args.height = GCCharHeight + 2;
	args.style = SEDT_STYLE_RDONLY;
	args.MsgProc = MainMsgProc;

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*处理GUI消息*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)edt)	/*销毁主窗体,退出程序*/
				edt = NULL;
			continue;
		}
		switch (data[MSG_ATTR_ID] & MSG_ATTR_MASK)
		{
		case MSG_ATTR_IME:	/*普通服务消息*/
			switch (data[MSG_API_ID] & MSG_API_MASK)
			{
			case IME_API_OPENBAR:	/*打开输入法工具条*/
				if (edt)
				{
					GUImove(edt->obj.gid, data[1], data[2]);
					GUISetTop(edt->obj.gid);
				}
				else
				{
					InID = 0;
					beg = PYTAB_LEN - 2;
					end = 0;
					args.x = data[1];
					args.y = data[2];
					GCSedtCreate(&edt, &args, 0, NULL, "拼音:", NULL);
				}
				break;
			case IME_API_CLOSEBAR:	/*关闭输入法工具条*/
				if (edt)
					GUIdestroy(edt->obj.gid);
				break;
			case IME_API_PUTKEY:	/*按键输入*/
				switch (data[1] & 0xFF)
				{
				case '\0':
					break;
				case '-':	/*向前翻页*/
					if (edt && PageID > 0)
					{
						DWORD hzid;
						PageID--;
						hzid = pytab[beg].hzid + PageID * HZ_PRE_PARG;
						FillEdtBuf(EdtBuf, "拼音:", InBuf, hzid, MIN(hzid + HZ_PRE_PARG, pytab[end + 1].hzid));
						GCSedtSetText(edt, EdtBuf);
					}
					break;
				case '=':	/*向后翻页*/
					if (edt && ((PageID + 1) * HZ_PRE_PARG) < pytab[end + 1].hzid - pytab[beg].hzid)
					{
						DWORD hzid;
						PageID++;
						hzid = pytab[beg].hzid + PageID * HZ_PRE_PARG;
						FillEdtBuf(EdtBuf, "拼音:", InBuf, hzid, MIN(hzid + HZ_PRE_PARG, pytab[end + 1].hzid));
						GCSedtSetText(edt, EdtBuf);
					}
					break;
				case ' ':	/*空格,回车输入首字*/
				case '\r':
					if (edt && beg <= end)
					{
						DWORD hzid;
						hzid = pytab[beg].hzid + PageID * HZ_PRE_PARG;
						data[0] = MSG_ATTR_GUI | GM_IMEPUTKEY;
						data[1] = hzid / 94 + 161 + ((hzid % 94 + 161) << 8);
						KSendMsg(&ptid, data, 0);
						GUIdestroy(edt->obj.gid);
					}
					break;
				case '\b':	/*退格,删一字符*/
					if (edt)
					{
						if (--InID == 0)
							GUIdestroy(edt->obj.gid);
						else
						{
							beg = 0;
							end = PYTAB_LEN - 2;
							PageID = 0;
							InBuf[InID] = '\0';
							findhzid(InBuf, 0, &beg, &end);
							FillEdtBuf(EdtBuf, "拼音:", InBuf, pytab[beg].hzid, MIN(pytab[beg].hzid + HZ_PRE_PARG, pytab[end + 1].hzid));
							GCSedtSetText(edt, EdtBuf);
						}
					}
					break;
				case '0':	/*数字键选字*/
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (edt && beg <= end)
					{
						DWORD hzid;
						hzid = pytab[beg].hzid + PageID * HZ_PRE_PARG + ((data[1] & 0xFF) - '0');
						if (hzid < pytab[end + 1].hzid)
						{
							data[0] = MSG_ATTR_GUI | GM_IMEPUTKEY;
							data[1] = hzid / 94 + 161 + ((hzid % 94 + 161) << 8);
							KSendMsg(&ptid, data, 0);
							GUIdestroy(edt->obj.gid);
						}
					}
					break;
				case 27:	/*ESC关闭工具条*/
					if (edt)
						GUIdestroy(edt->obj.gid);
					break;
				default:	/*一般字符,加入*/
					if (InID < IN_BUF_LEN - 1)	/*缓冲未满,继续输入*/
					{
						if (!edt)
						{
							InID = 0;
							GCSedtCreate(&edt, &args, 0, NULL, NULL, NULL);
							{
								DWORD tmp[MSG_DATA_LEN];
								ptid.ProcID = SRV_GUI_PORT;
								ptid.ThedID = INVALID;
								if (KRecvProcMsg(&ptid, tmp, INVALID) != NO_ERROR)	/*等待创建完成消息*/
									break;
								GCDispatchMsg(ptid, tmp);	/*创建完成后续处理*/
							}
						}
						if (InID == 0)
						{
							beg = 0;
							end = PYTAB_LEN - 2;
						}
						PageID = 0;
						InBuf[InID] = data[1] & 0xFF;
						InBuf[InID + 1] = '\0';
						findhzid(InBuf, InID, &beg, &end);
						InID++;
						FillEdtBuf(EdtBuf, "拼音:", InBuf, pytab[beg].hzid, MIN(pytab[beg].hzid + HZ_PRE_PARG, pytab[end + 1].hzid));
						GCSedtSetText(edt, EdtBuf);
					}
				}
				break;
			}
		}
	}
	KUnregKnlPort(SRV_IME_PORT);
	return NO_ERROR;
}
