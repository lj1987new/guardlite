/*
 *	关键相关函数
 */
#include "EhomeNet.h"
#include "Keyword.h"

#define MAX_INDEX					256

typedef struct 
{
	LIST_ENTRY		list;
	ULONG			nSize;
} keyword_item, *keyword_item_ptr;

typedef struct 
{
	BOOLEAN			iskey;
	LIST_ENTRY		list;
} keyword_index, *keyword_index_ptr;

KMUTEX				g_keyword_Mutex			= {0};
keyword_index_ptr	g_keyword_Index			= NULL;

/* 初始化关键字 */
void keyword_Init()
{
	int			i		= 0;

	KeInitializeMutex(&g_keyword_Mutex, 0);
	g_keyword_Index = (keyword_index_ptr)ExAllocatePoolWithTag(NonPagedPool, sizeof(keyword_index) * MAX_INDEX, 'ehom');
	ASSERT(NULL != g_keyword_Index);
	if(NULL == g_keyword_Index)
		return;
	for(i = 0; i < MAX_INDEX; i++)
	{
		InitializeListHead(&g_keyword_Index[i].list);
		g_keyword_Index[i].iskey = FALSE;
	}
}

/* 销毁关键字 */
void keyword_Release()
{
	int					i				= 0;
	PLIST_ENTRY			pList			= NULL;
	keyword_item_ptr		pKeywordItem	= NULL;

	KeWaitForSingleObject(&g_keyword_Mutex, Executive, KernelMode, FALSE, NULL); 
	if(NULL != g_keyword_Index)
	{
		for(i = 0; i < MAX_INDEX; i++)
		{
			for(pList = g_keyword_Index[i].list.Blink
				; pList != &g_keyword_Index[i].list
			; pList = pList->Blink)
			{
				pKeywordItem = CONTAINING_RECORD(pList, keyword_item, list);
				pList->Blink->Flink = pList->Flink;
				pList->Flink->Blink = pList->Blink;
				ExFreePoolWithTag(pKeywordItem, 'ehom');
			}
		}
		ExFreePoolWithTag(g_keyword_Index, 'ehom');
		g_keyword_Index = NULL;
	}
	KeReleaseMutex(&g_keyword_Mutex, FALSE);
}

/* 清除Keyword */
void keyword_Clear()
{
	int					i				= 0;
	PLIST_ENTRY			pList			= NULL;
	keyword_item_ptr		pKeywordItem	= NULL;

	KeWaitForSingleObject(&g_keyword_Mutex, Executive, KernelMode, FALSE, NULL); 
	if(NULL != g_keyword_Index)
	{
		for(i = 0; i < MAX_INDEX; i++)
		{
			for(pList = g_keyword_Index[i].list.Blink
				; pList != &g_keyword_Index[i].list
				; pList = pList->Blink)
			{
				pKeywordItem = CONTAINING_RECORD(pList, keyword_item, list);
				pList->Blink->Flink = pList->Flink;
				pList->Flink->Blink = pList->Blink;
				ExFreePoolWithTag(pKeywordItem, 'ehom');
			}
			g_keyword_Index[i].iskey = FALSE;
		}
	}
	KeReleaseMutex(&g_keyword_Mutex, FALSE);
}

/* 添加一个KEY */
void keyword_Add(char* pKeyword, ULONG nLen)
{
	keyword_item_ptr		pKeyItem	= NULL;
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
	KeWaitForSingleObject(&g_keyword_Mutex, Executive, KernelMode, FALSE, NULL);
	if(NULL != g_keyword_Index)
	{
		UCHAR			uchar		= (UCHAR)pKeyword[0];
		BOOL			bAdd		= TRUE;

		for(pList = g_keyword_Index[uchar].list.Blink
			; pList != &g_keyword_Index[uchar].list
			; pList = pList->Blink)
		{
			pKeyItem = CONTAINING_RECORD(pList, keyword_item, list);
			if(nLen != pKeyItem->nSize)
				continue;
			if(memcmp((char *)pKeyItem + sizeof(keyword_item), pKeyword, nLen) == 0)
			{
				bAdd = FALSE;
				break;
			}
		}
		if(bAdd)
		{
			pKeyItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(keyword_item) + nLen, 'ehom');
			if(NULL != pKeyItem)
			{
				pKeyItem->nSize = nLen;
				memcpy((char*)pKeyItem + sizeof(keyword_item), pKeyword, nLen);
				InsertHeadList(&g_keyword_Index[uchar].list, &pKeyItem->list);
				g_keyword_Index[uchar].iskey = TRUE;
			}
		}
	}
	KeReleaseMutex(&g_keyword_Mutex, FALSE);
}

/* 查找关键字 */
BOOLEAN keyword_Find(IN char* pData, IN int nLenData, OUT char** ppKeyWord, OUT int* pLenKeyWord)
{
	int				i			= 0;
	BOOLEAN			bFind		= FALSE;

	KeWaitForSingleObject(&g_keyword_Mutex, Executive, KernelMode, FALSE, NULL);
	for(i = 0; i < nLenData && FALSE == bFind; i++)
	{
		UCHAR				uchar		= (UCHAR)pData[i];
		PLIST_ENTRY			pList;
		keyword_item_ptr		pKeyItem;

		if(FALSE == g_keyword_Index[uchar].iskey)
			continue;	// 快束查找
		for(pList = g_keyword_Index[uchar].list.Blink
			; pList != &g_keyword_Index[uchar].list
			; pList = pList->Blink)
		{
			pKeyItem = CONTAINING_RECORD(pList, keyword_item, list);
			if(memcmp(pData+i, (char*)pKeyItem+sizeof(keyword_item), pKeyItem->nSize) == 0)
			{
				bFind = TRUE;
				*ppKeyWord = pData + i;
				*pLenKeyWord = pKeyItem->nSize;
				break;
			}
		}
	}
	KeReleaseMutex(&g_keyword_Mutex, FALSE);
	return bFind;
}