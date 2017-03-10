/*	ulidef.h for ulios
	作者：孙亮
	功能：ulios内核代码基本宏定义
	最后修改日期：2009-05-26
*/

#ifndef	_ULIDEF_H_
#define	_ULIDEF_H_

typedef unsigned char		BYTE;	/*8位*/
typedef unsigned short		WORD;	/*16位*/
typedef unsigned long		DWORD;	/*32位*/
typedef unsigned long		BOOL;
typedef unsigned long long	QWORD;	/*64位*/

typedef struct _THREAD_ID
{
	WORD ProcID;
	WORD ThedID;
}THREAD_ID;	/*进程线程ID*/

#define TRUE	1
#define FALSE	0
#define NULL	((void*)0)
#define INVALID	(~0)

/*32位内存设置*/
static inline void memset32(void *dest, DWORD d, DWORD n)
{
	register void *_dest;
	register DWORD _n;
	__asm__ __volatile__("cld;rep stosl": "=&D"(_dest), "=&c"(_n): "0"(dest), "a"(d), "1"(n): "flags", "memory");
}

/*32位内存复制*/
static inline void memcpy32(void *dest, const void *src, DWORD n)
{
	register void *_dest;
	register const void *_src;
	register DWORD _n;
	__asm__ __volatile__("cld;rep movsl": "=&D"(_dest), "=&S"(_src), "=&c"(_n): "0"(dest), "1"(src), "2"(n): "flags", "memory");
}

/*字符串复制*/
static inline void strcpy(BYTE *dest, const BYTE *src)
{
	BYTE *_dest;
	const BYTE *_src;
	__asm__ __volatile__
	(
		"cld\n"
		"1:\tlodsb\n"
		"stosb\n"
		"testb %%al, %%al\n"
		"jne 1b"
		: "=&D"(_dest), "=&S"(_src): "0"(dest), "1"(src): "flags", "al", "memory"
	);
}

/*关中断*/
static inline void cli()
{
	__asm__("cli");
}

/*开中断*/
static inline void sti()
{
	__asm__("sti");
}

/*端口输出字节*/
static inline void outb(WORD port, BYTE b)
{
	__asm__ __volatile__("outb %1, %w0":: "d"(port), "a"(b));
}

/*端口输入字节*/
static inline BYTE inb(WORD port)
{
	register BYTE b;
	__asm__ __volatile__("inb %w1, %0": "=a"(b): "d"(port));
	return b;
}

/*进程调度*/
void schedul();

/*锁变量锁定*/
static inline void lock(volatile DWORD *l)
{
	cli();
	while (*l)
		schedul();
	*l = TRUE;
	sti();
}

/*锁变量解锁*/
static inline void ulock(volatile DWORD *l)
{
	*l = FALSE;
}

/*WORD锁变量锁定*/
static inline void lockw(volatile WORD *l)
{
	cli();
	while (*l)
		schedul();
	*l = TRUE;
	sti();
}

/*WORD锁变量解锁*/
static inline void ulockw(volatile WORD *l)
{
	*l = FALSE;
}

/*锁定并赋值*/
static inline void lockset(volatile DWORD *l, DWORD val)
{
	cli();
	while (*l)
		schedul();
	*l = val;
	sti();
}

/*关中断并加1*/
static inline void cliadd(volatile DWORD *l)
{
	cli();
	(*l)++;
	sti();
}

/*关中断并减1*/
static inline void clisub(volatile DWORD *l)
{
	cli();
	(*l)--;
	sti();
}

/*检查锁变量并关中断*/
#define clilock(l) \
({\
	cli();\
	while (l)\
		schedul();\
})

#endif
