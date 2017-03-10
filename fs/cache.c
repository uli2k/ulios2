/*	cache.c for ulios file system
	作者：孙亮
	功能：高速缓冲管理，属于文件系统的底层功能
	最后修改日期：2009-06-09
*/

#include "fs.h"

CACHE_DESC *bmt;
void *cache;
DWORD cahl;	/*缓冲管理锁*/

/*缓冲读写*/
long RwCache(DWORD drv, BOOL isWrite, DWORD sec, DWORD cou, void *buf)
{
	lock(&cahl);
	for (cou += sec; sec < cou; sec++, buf += BLK_SIZ)	/*cou用做结束块ID*/
	{
		CACHE_DESC *CurBmd;
		void *cachep;

		CurBmd = &bmt[sec & BLKATTR_MASK];
		cachep = cache + ((sec & BLKATTR_MASK) << BLK_SHIFT);
		if ((CurBmd->BlkID >> BLKID_SHIFT) == (sec >> BLKID_SHIFT) && CurBmd->DrvID == drv)	/*缓冲区中存在*/
		{
			if (isWrite)	/*写入*/
			{
				memcpy32(cachep, buf, BLK_SIZ / sizeof(DWORD));
				CurBmd->BlkID |= CACHE_ATTR_DIRTY;	/*标记为脏*/
			}
			else	/*读出*/
				memcpy32(buf, cachep, BLK_SIZ / sizeof(DWORD));
			if ((CurBmd->BlkID & CACHE_ATTR_TIMES) < CACHE_ATTR_TIMES)	/*访问次数增加*/
				CurBmd->BlkID++;
		}
		else	/*缓冲区中不存在就要访问设备*/
		{
			long res;
			CACHE_DESC *PreBmd;	/*预先处理块*/

			if (CurBmd->BlkID & CACHE_ATTR_DIRTY)	/*当前缓冲块脏,不可丢弃,扫描可预先保存的脏块*/
			{
				for (PreBmd = CurBmd + 1; PreBmd - CurBmd < MAXPROC_COU && PreBmd < &bmt[BMT_LEN] && (PreBmd->BlkID & CACHE_ATTR_DIRTY) && (PreBmd->BlkID >> BLKID_SHIFT) == (CurBmd->BlkID >> BLKID_SHIFT) && PreBmd->DrvID == CurBmd->DrvID; PreBmd++)
					PreBmd->BlkID &= ~CACHE_ATTR_DIRTY;
				if ((res = HDWriteSector(CurBmd->DrvID, (CurBmd->BlkID & BLKID_MASK) | (sec & BLKATTR_MASK), PreBmd - CurBmd, cachep)) != NO_ERROR)	/*保存脏块*/
				{
					ulock(&cahl);
					return res;
				}
			}
			if (isWrite)	/*写操作,先写入缓存*/
			{
				memcpy32(cachep, buf, BLK_SIZ / sizeof(DWORD));
				CurBmd->BlkID = (sec & BLKID_MASK) | CACHE_ATTR_DIRTY | 1;	/*标记为访问过1次的脏块*/
			}
			else	/*读操作,读入缓存并复制*/
			{
				for (PreBmd = CurBmd + 1; PreBmd - CurBmd < MAXPROC_COU && PreBmd < &bmt[BMT_LEN] && !(PreBmd->BlkID & CACHE_ATTR_DIRTY) && (PreBmd->BlkID & CACHE_ATTR_TIMES) <= 1; PreBmd++)
				{
					PreBmd->DrvID = drv;
					PreBmd->BlkID = sec & BLKID_MASK;
				}
				if ((res = HDReadSector(drv, (sec & BLKID_MASK) | (sec & BLKATTR_MASK), PreBmd - CurBmd, cachep)) != NO_ERROR)	/*预读取*/
				{
					ulock(&cahl);
					return res;
				}
				memcpy32(buf, cachep, BLK_SIZ / sizeof(DWORD));
				CurBmd->BlkID = (sec & BLKID_MASK) | 1;
			}
			CurBmd->DrvID = drv;
		}
	}
	ulock(&cahl);
	return NO_ERROR;
}

/*保存脏数据*/
long SaveCache()
{
	long res;
	CACHE_DESC *CurBmd, *PreBmd;

	lock(&cahl);
	for (CurBmd = bmt; CurBmd < &bmt[BMT_LEN];)
	{
		if (CurBmd->BlkID & CACHE_ATTR_DIRTY)
		{
			for (PreBmd = CurBmd + 1; PreBmd - CurBmd < MAXPROC_COU && PreBmd < &bmt[BMT_LEN] && (PreBmd->BlkID & CACHE_ATTR_DIRTY) && (PreBmd->BlkID >> BLKID_SHIFT) == (CurBmd->BlkID >> BLKID_SHIFT) && PreBmd->DrvID == CurBmd->DrvID; PreBmd++)
				PreBmd->BlkID &= ~CACHE_ATTR_DIRTY;
			if ((res = HDWriteSector(CurBmd->DrvID, (CurBmd->BlkID & BLKID_MASK) | (CurBmd - bmt), PreBmd - CurBmd, cache + ((CurBmd - bmt) << BLK_SHIFT))) != NO_ERROR)	/*保存脏块*/
			{
				ulock(&cahl);
				return res;
			}
			CurBmd->BlkID &= ~CACHE_ATTR_DIRTY;
			CurBmd = PreBmd;
		}
		else
			CurBmd++;
	}
	ulock(&cahl);
	return NO_ERROR;
}
