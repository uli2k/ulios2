// 无额外管理表的内存管理伙伴算法

#include <stdio.h>
#include <stdlib.h>

typedef unsigned char		BYTE;	// 8位
typedef unsigned short		WORD;	// 16位
typedef unsigned long		DWORD;	// 32位
typedef unsigned long		BOOL;
typedef unsigned long long	QWORD;	// 64位

#define NULL	((void *)0)
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

////////////////////////////////////////////////
#define MAX_AREA	16ul	// 最大分区数
#define MIN_SHIFT	5ul		// 最小块(32B)字节数位移

#define BUDDY_ATTR_FREE	0x62756479
#define BUDDY_ATTR_USED	0x97465726

typedef struct _BUDDY_NODE
{
	struct _BUDDY_NODE *pre, *nxt;
	DWORD idx, attr;
} BUDDY_NODE;	// 管理结构尺寸为16字节，可保证分配内存地址16字节对齐

BYTE *BaseAddr;
BUDDY_NODE *BuddyList[MAX_AREA];

// 获取size占用整块尺寸位移数
static inline DWORD GetSizeShift(DWORD size)
{
	DWORD shift = 8ul * sizeof(size) - __builtin_clzl(size);	// __builtin_clzl前导0个数
	if (likely(shift > __builtin_ctzl(size) + 1ul))	// __builtin_ctzl后缀0个数
		return shift;
	else
		return shift - 1ul;
}

int BuddyInit(void *addr, DWORD size)
{
	if (unlikely(addr == NULL || ((DWORD)addr) & 0xF != 0))
		return -1;
	if (unlikely(size == 0))
		return -1;
	DWORD idx = GetSizeShift(size) - MIN_SHIFT;
	if (unlikely(idx >= MAX_AREA))
		return -1;
	BaseAddr = (BYTE *)addr;
	BUDDY_NODE *NewNode;
	NewNode = BuddyList[idx] = (BUDDY_NODE *)addr;
	NewNode->nxt = NewNode->pre = NULL;
	NewNode->idx = idx;
	NewNode->attr = BUDDY_ATTR_FREE;
	return 0;
}

BUDDY_NODE *AllocBuddy(DWORD idx)
{
	if (unlikely(idx >= MAX_AREA))	// 级别索引错误
		return NULL;
	BUDDY_NODE *node;
	if ((node = BuddyList[idx]) != NULL)	// 本级链中有空余块
	{
		if ((BuddyList[idx] = node->nxt) != NULL)
			node->nxt->pre = NULL;
	}
	else if ((node = AllocBuddy(idx + 1)) != NULL)	// 向上级申请到空余块
	{
		BuddyList[idx] = node;
		node->nxt = node->pre = NULL;
		node->idx = idx;
		node->attr = BUDDY_ATTR_FREE;
		node = (BUDDY_NODE *)(((BYTE *)node) + (1ul << (MIN_SHIFT + idx)));
		node->idx = idx;
	}
	else
		return NULL;
	node->attr = BUDDY_ATTR_USED;
	return node;
}

void FreeBuddy(BUDDY_NODE *node)
{
	DWORD idx = node->idx;
	DWORD size = (1ul << (MIN_SHIFT + idx));
	BUDDY_NODE *par, *bdy;
	if (((BYTE *)node - BaseAddr) & size)	// 伙伴块地址在释放块之前
		par = bdy = (BUDDY_NODE *)(((BYTE *)node) - size);
	else	// 伙伴块地址在释放块之后
	{
		bdy = (BUDDY_NODE *)(((BYTE *)node) + size);
		par = node;
	}
	if (bdy->idx == idx && bdy->attr == BUDDY_ATTR_FREE)	// 伙伴块为同级空余块
	{
		if (BuddyList[idx] == bdy)
			BuddyList[idx] = bdy->nxt;
		if (bdy->pre)
			bdy->pre->nxt = bdy->nxt;
		if (bdy->nxt)
			bdy->nxt->pre = bdy->pre;
		par->idx++;
		FreeBuddy(par);	// 释放上级空余块
	}
	else	// 释放当前空余块
	{
		node->pre = NULL;
		node->nxt = BuddyList[idx];
		node->attr = BUDDY_ATTR_FREE;
		if (BuddyList[idx])
			BuddyList[idx]->pre = node;
		BuddyList[idx] = node;
	}
}

void *balloc(DWORD size)
{
	if (unlikely(size == 0))
		return NULL;
	DWORD idx = GetSizeShift(size + sizeof(BUDDY_NODE)) - MIN_SHIFT;
	if (unlikely(idx >= MAX_AREA))
		return NULL;
	BUDDY_NODE *node;
	if (unlikely((node = AllocBuddy(idx)) == NULL))
		return NULL;
	return (void *)(node + 1);
}

int bfree(void *addr)
{
	if (unlikely(addr == NULL || ((DWORD)addr) & 0xF != 0))
		return -1;
	BUDDY_NODE *node = ((BUDDY_NODE *)addr) - 1;
	if (unlikely(node->attr != BUDDY_ATTR_USED))
		return -1;
	FreeBuddy(node);
	return 0;
}

void ShowList()
{
	int i;
	for (i = 0; i < MAX_AREA; i++)
	{
		BUDDY_NODE *CurNode;
		printf("[%d:0x%X]", i, 1u << (MIN_SHIFT + i));
		for (CurNode = BuddyList[i]; CurNode; CurNode = CurNode->nxt)
			printf("->0x%X", ((BYTE *)CurNode) - BaseAddr);
		printf("\n");
	}
}

#define BUF_SIZE	0x100000
#define ADDR_LEN	0x400

int main()
{
	BYTE buf[BUF_SIZE];
	void *addr[ADDR_LEN];

	int i, ec1 = 0, ec2 = 0;
	for (i = 0; i < BUF_SIZE; i++)
		buf[i] = 0;
	BuddyInit(buf, BUF_SIZE);
	for (i = 0; i < ADDR_LEN; i++)
		addr[i] = NULL;
	for (i = 0; i < 100000000; i++)
	{
		int idx = rand() % ADDR_LEN;
		if (rand() % 1000 >= 499)
		{
			if (addr[idx])
				bfree(addr[idx]);
			addr[idx] = balloc(1ul << (rand() % 14));
			if (addr[idx] == NULL)
				ec1++;
		}
		else
		{
			if (addr[idx])
			{
				if (bfree(addr[idx]) != 0)
					ec2++;
				addr[idx] = NULL;
			}
		}
//		ShowList();
	}
	for (i = 0; i < ADDR_LEN; i++)
		if (addr[i])
			bfree(addr[i]);
	ShowList();
//	 for (i = 0; i < BUF_SIZE; i += sizeof(DWORD))
//		printf("%8X\t", *((DWORD*)&buf[i]));
	printf("AllocErr:%d\tFreeErr:%d\n", ec1, ec2);
}
