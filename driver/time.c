/*	time.c for ulios driver
	作者：孙亮
	功能：时间服务
	最后修改日期：2010-04-19
*/

#include "basesrv.h"

#define BCD2BIN(val)	((val) = ((val) & 15) + ((val) >> 4) * 10)
#define MINUTE			60
#define HOUR			(60 * MINUTE)
#define DAY				(24 * HOUR)
#define YEAR			(365 * DAY)

/*从bios中读参数*/
static inline BYTE ReadCmos(BYTE addr)
{
	outb(0x70, 0x80 | addr);
	return inb(0x71);
}

void Sec2Tm(DWORD sec, TM *tm)	/*1970以来年经过的秒数转换为时间结构*/
{
	DWORD i, day;
	tm->sec = sec % 60;	sec /= 60;	/*秒*/
	tm->min = sec % 60;	sec /= 60;	/*分*/
	tm->hor = sec % 24;	sec /= 24;	/*时*/
	tm->wday = (sec + 4) % 7;
	i = sec / 365;	/*粗略年,此处乃是牛头人向博想到的超强算法*/
	day = ((i + 1) >> 2) - (i + 69) / 100 + (i + 369) / 400;	/*粗略闰年计数*/
	i = (sec - day) / 365;	/*精确年*/
	day = ((i + 1) >> 2) - (i + 69) / 100 + (i + 369) / 400;	/*精确闰年计数*/
	sec -= i * 365 + day;	/*剩余日*/
	tm->yday = sec;
	tm->yer = i + 1970;	/*年*/
	for (i = 1, day = 31; sec >= day;)	/*循环递减法*/
	{
		sec -= day;
		if (++i == 2)
			day = (((tm->yer & 3) == 0 && tm->yer % 100 != 0) || tm->yer % 400 == 0) ? 29 : 28;
		else if (i < 8)
			day = (i & 1) ? 31 : 30;
		else
			day = (i & 1) ? 30 : 31;
	}
	tm->mon = i;	/*月*/
	tm->day = sec + 1;	/*日*/
}

long Tm2Sec(DWORD *sec, const TM *tm)
{
	static DWORD month[12] =
	{
		0,
		DAY * (31),
		DAY * (31 + 29),
		DAY * (31 + 29 + 31),
		DAY * (31 + 29 + 31 + 30),
		DAY * (31 + 29 + 31 + 30 + 31),
		DAY * (31 + 29 + 31 + 30 + 31 + 30),
		DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31),
		DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31),
		DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
		DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
		DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)
	};
	DWORD mon, yer, tmsec;

	if (tm->sec > 59 ||
		tm->min > 59 ||
		tm->hor > 23 ||
		tm->day == 0 || tm->day > 31 ||
		tm->mon == 0 || tm->mon > 12 ||
		tm->yer < 1970)
		return TIME_ERR_WRONG_TM;
	mon = tm->mon - 1;
	yer = tm->yer - 1970;
	tmsec = YEAR * yer + DAY * ((yer + 1) / 4);
	tmsec += month[mon];
	if (mon > 1 && ((yer + 2) % 4))
		tmsec -= DAY;
	tmsec += DAY * (tm->day - 1);
	tmsec += HOUR * tm->hor;
	tmsec += MINUTE * tm->min;
	tmsec += tm->sec;
	*sec = tmsec;
	return NO_ERROR;
}

int main()
{
	TM tm;
	DWORD bootsec, clk, RandSeed;	/*启动时的1970秒数*/
	long res;	/*返回结果*/

	if ((res = KRegKnlPort(SRV_TIME_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	do	/*读取BIOS时间*/
	{
		tm.sec = ReadCmos(0);
		tm.min = ReadCmos(2);
		tm.hor = ReadCmos(4);
		tm.day = ReadCmos(7);
		tm.mon = ReadCmos(8);
		tm.yer = ReadCmos(9);
		if ((res = KGetClock(&clk)) != NO_ERROR)
			return res;
	}
	while (tm.sec != ReadCmos(0));
	BCD2BIN(tm.sec);
	BCD2BIN(tm.min);
	BCD2BIN(tm.hor);
	BCD2BIN(tm.day);
	BCD2BIN(tm.mon);
	BCD2BIN(tm.yer);
	if (tm.yer < 70)
		tm.yer += 2000;
	else
		tm.yer += 1900;
	if ((res = Tm2Sec(&bootsec, &tm)) != NO_ERROR)
		return res;
	RandSeed = bootsec;
	bootsec -= clk / 100;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_TIME)	/*应用请求消息*/
		{
			switch (data[MSG_API_ID] & MSG_API_MASK)
			{
			case TIME_API_CURSECOND:	/*取得1970年经过的秒*/
				KGetClock(&clk);
				data[1] = bootsec + clk / 100;
				KSendMsg(&ptid, data, 0);
				break;
			case TIME_API_CURTIME:		/*取得当前时间*/
				KGetClock(&clk);
				Sec2Tm(bootsec + clk / 100, (TM*)&data[1]);
				((TM*)&data[1])->mil = (clk % 100) * 10;
				KSendMsg(&ptid, data, 0);
				break;
			case TIME_API_MKTIME:		/*TM结构转换为秒*/
				data[MSG_RES_ID] = Tm2Sec(&data[1], (TM*)&data[1]);
				KSendMsg(&ptid, data, 0);
				break;
			case TIME_API_LOCALTIME:	/*秒转换为TM结构*/
				Sec2Tm(data[1], (TM*)&data[1]);
				((TM*)&data[1])->mil = 0;
				KSendMsg(&ptid, data, 0);
				break;
			case TIME_API_GETRAND:	/*取得随机数*/
				data[1] = RandSeed = RandSeed * 1103515245 + 12345;
				KSendMsg(&ptid, data, 0);
				break;
			}
		}
	}
	KUnregKnlPort(SRV_TIME_PORT);
	return res;
}
