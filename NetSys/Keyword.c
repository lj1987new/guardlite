/*
 *	关键相关函数
 */
#include "EhomeNet.h"
#include "Keyword.h"

#define MAX_INDEX					256

typedef struct _KeywordItem{
	LIST_ENTRY		list;
	ULONG			nSize;
}KEYWORDITEM, *PKEYWORDITEM;

typedef struct _KeywordIndex{
	BOOLEAN			iskey;
	LIST_ENTRY		list;
}KEYWORDINDEX, *PKEYWORDINDEX;

KMUTEX			gKeywordMutex			= {0};
PKEYWORDINDEX	gKeywordIndex			= NULL;

/* 初始化关键字 */
void KeywordInit()
{
	int			i		= 0;

	KeInitializeMutex(&gKeywordMutex, 0);
	gKeywordIndex = (PKEYWORDINDEX)ExAllocatePoolWithTag(NonPagedPool, sizeof(KEYWORDINDEX) * MAX_INDEX, 'ehom');
	ASSERT(NULL != gKeywordIndex);
	if(NULL == gKeywordIndex)
		return;
	for(i = 0; i < MAX_INDEX; i++)
	{
		InitializeListHead(&gKeywordIndex[i].list);
		gKeywordIndex[i].iskey = FALSE;
	}
}

/* 销毁关键字 */
void KeywordDestroy()
{
	int					i				= 0;
	PLIST_ENTRY			pList			= NULL;
	PKEYWORDITEM		pKeywordItem	= NULL;

	KeWaitForSingleObject(&gKeywordMutex, Executive, KernelMode, FALSE, NULL); 
	if(NULL != gKeywordIndex)
	{
		for(i = 0; i < MAX_INDEX; i++)
		{
			for(pList = gKeywordIndex[i].list.Blink
				; pList != &gKeywordIndex[i].list
			; pList = pList->Blink)
			{
				pKeywordItem = CONTAINING_RECORD(pList, KEYWORDITEM, list);
				pList->Blink->Flink = pList->Flink;
				pList->Flink->Blink = pList->Blink;
				ExFreePoolWithTag(pKeywordItem, 'ehom');
			}
		}
		ExFreePoolWithTag(gKeywordIndex, 'ehom');
		gKeywordIndex = NULL;
	}
	KeReleaseMutex(&gKeywordMutex, FALSE);
}

/* 清除Keyword */
void KeywordClear()
{
	int					i				= 0;
	PLIST_ENTRY			pList			= NULL;
	PKEYWORDITEM		pKeywordItem	= NULL;

	KeWaitForSingleObject(&gKeywordMutex, Executive, KernelMode, FALSE, NULL); 
	if(NULL != gKeywordIndex)
	{
		for(i = 0; i < MAX_INDEX; i++)
		{
			for(pList = gKeywordIndex[i].list.Blink
				; pList != &gKeywordIndex[i].list
				; pList = pList->Blink)
			{
				pKeywordItem = CONTAINING_RECORD(pList, KEYWORDITEM, list);
				pList->Blink->Flink = pList->Flink;
				pList->Flink->Blink = pList->Blink;
				ExFreePoolWithTag(pKeywordItem, 'ehom');
			}
			gKeywordIndex[i].iskey = FALSE;
		}
	}
	KeReleaseMutex(&gKeywordMutex, FALSE);
}

/* 添加一个KEY */
void KeywordAdd(char* pKeyword, ULONG nLen)
{
	PKEYWORDITEM		pKeyItem	= NULL;
	PLIST_ENTRY			pList		= NULL;
	int					i;

	if(NULL == pKeyword || 0 == nLen)
		return;
	// 去除0关键字
	for(i = (int)nLen-1; i >= 0; i--)
	{
		if('\x0' == pKeyword[i])
			nLen--;
		else
			break;
	}
	if(0 == nLen)
		return;
	// 开始添加
	KeWaitForSingleObject(&gKeywordMutex, Executive, KernelMode, FALSE, NULL);
	if(NULL != gKeywordIndex)
	{
		UCHAR			uchar		= (UCHAR)pKeyword[0];
		BOOL			bAdd		= TRUE;

		for(pList = gKeywordIndex[uchar].list.Blink
			; pList != &gKeywordIndex[uchar].list
			; pList = pList->Blink)
		{
			pKeyItem = CONTAINING_RECORD(pList, KEYWORDITEM, list);
			if(nLen != pKeyItem->nSize)
				continue;
			if(memcmp((char *)pKeyItem + sizeof(KEYWORDITEM), pKeyword, nLen) == 0)
			{
				bAdd = FALSE;
				break;
			}
		}
		if(bAdd)
		{
			pKeyItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEYWORDITEM) + nLen, 'ehom');
			if(NULL != pKeyItem)
			{
				pKeyItem->nSize = nLen;
				memcpy((char*)pKeyItem + sizeof(KEYWORDITEM), pKeyword, nLen);
				InsertHeadList(&gKeywordIndex[uchar].list, &pKeyItem->list);
				gKeywordIndex[uchar].iskey = TRUE;
			}
		}
	}
	KeReleaseMutex(&gKeywordMutex, FALSE);
}

/* 查找关键字 */
BOOLEAN KeywordFind(IN char* pData, IN int nLenData, OUT char** ppKeyWord, OUT int* pLenKeyWord)
{
	int				i			= 0;
	BOOLEAN			bFind		= FALSE;

	KeWaitForSingleObject(&gKeywordMutex, Executive, KernelMode, FALSE, NULL);
	for(i = 0; i < nLenData && FALSE == bFind; i++)
	{
		UCHAR				uchar		= (UCHAR)pData[i];
		PLIST_ENTRY			pList;
		PKEYWORDITEM		pKeyItem;

		if(FALSE == gKeywordIndex[uchar].iskey)
			continue;	// 快束查找
		for(pList = gKeywordIndex[uchar].list.Blink
			; pList != &gKeywordIndex[uchar].list
			; pList = pList->Blink)
		{
			pKeyItem = CONTAINING_RECORD(pList, KEYWORDITEM, list);
			if(memcmp(pData+i, (char*)pKeyItem+sizeof(KEYWORDITEM), pKeyItem->nSize) == 0)
			{
				bFind = TRUE;
				*ppKeyWord = pData + i;
				*pLenKeyWord = pKeyItem->nSize;
				break;
			}
		}
	}
	KeReleaseMutex(&gKeywordMutex, FALSE);
	return bFind;
}