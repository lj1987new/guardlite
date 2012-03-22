/*
 *	TDI的TCP连接操作的HASH操作
 */
#include <ntddk.h>
#include <tdi.h>
#include "TdiFileObjectContext.h"

#define HASH_SIZE				0x1000
#define CALC_HASH(fileobj)		(((ULONG)(fileobj) >> 5) % HASH_SIZE)

#pragma data_seg()
PLIST_ENTRY						g_tdi_foc_HashTable		= NULL;

#pragma data_seg()
NPAGED_LOOKASIDE_LIST			g_tdi_foc_HashLookaside	= {0};

#pragma data_seg()
KSPIN_LOCK						g_tdi_foc_HashSpinLock		= {0};

/*
 *	初始化TABLE
 */
BOOLEAN tdi_foc_Init()
{
	int			i;

	ExInitializeNPagedLookasideList(&g_tdi_foc_HashLookaside, NULL, NULL
		, 0, sizeof(tdi_foc), 'ehom', 0);
	g_tdi_foc_HashTable = (PLIST_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY) * HASH_SIZE, 'ehom');
	if(NULL == g_tdi_foc_HashTable)
	{
		KdPrint(("[tdi_foc_Init] ExAllocatePool failed.\n"));
		return FALSE;
	}
	// 初始化自旋锁
	KeInitializeSpinLock(&g_tdi_foc_HashSpinLock);
	// 初始化每个链表
	for(i = 0; i < HASH_SIZE; i++)
	{
		InitializeListHead(&g_tdi_foc_HashTable[i]);
	}

	return TRUE;
}

/*
 *	获取结构体
 */
tdi_foc_ptr tdi_foc_Get(PFILE_OBJECT pAddressFileObj)
{
	int						nIndex		= CALC_HASH(pAddressFileObj);
	tdi_foc_ptr		pRet		= NULL;
	tdi_foc_ptr		pTemp		= NULL;
	KIRQL					irql;
	PLIST_ENTRY				pList		= NULL;

	if(NULL == g_tdi_foc_HashTable || NULL == pAddressFileObj)
		return NULL;
	KeAcquireSpinLock(&g_tdi_foc_HashSpinLock, &irql);
	// 在现有的TABLE里查找
	for(pList = g_tdi_foc_HashTable[nIndex].Blink
		; pList != &g_tdi_foc_HashTable[nIndex]
		; pList = pList->Blink)
	{
		pTemp = CONTAINING_RECORD(pList, tdi_foc, list);
		if(pAddressFileObj == pTemp->pAddressFileObj)
		{
			pRet = pTemp;
			break;
		}
	}
	KeReleaseSpinLock(&g_tdi_foc_HashSpinLock, irql);

	return pRet;
}
/*
 *	获取地址对像
 */
tdi_foc_ptr tdi_foc_GetAddress(PFILE_OBJECT pAddressFileObj, BOOLEAN bCreate)
{
	tdi_foc_ptr			pSocketContext		= tdi_foc_Get(pAddressFileObj);

	if(NULL == pSocketContext && FALSE != bCreate)
	{
		int						nIndex				= CALC_HASH(pAddressFileObj);

		pSocketContext = (tdi_foc_ptr)ExAllocateFromNPagedLookasideList(&g_tdi_foc_HashLookaside);
		if(NULL == pSocketContext)
		{
			KdPrint(("[tdi_foc_Get] ExAllocateFromNPagedLookasideList Failed %x to %d\n", pAddressFileObj, nIndex));
			return NULL;
		}
		RtlZeroMemory(pSocketContext, sizeof(tdi_foc));
		pSocketContext->pAddressFileObj = pAddressFileObj;
		pSocketContext->bIsAddressFileObj = TRUE;
		ExInterlockedInsertHeadList(&g_tdi_foc_HashTable[nIndex], &pSocketContext->list, &g_tdi_foc_HashSpinLock);
		KdPrint(("[tdi_foc_GetConnection] insert %x to %d\n", pAddressFileObj, nIndex));
	}
	else if(NULL != pSocketContext)
	{
		if(FALSE == pSocketContext->bIsAddressFileObj)
			return NULL;
	}
	return pSocketContext;
}
/*
 *	获取连接对像
 */
tdi_foc_ptr tdi_foc_GetConnection(PFILE_OBJECT pConnectFileObj, BOOLEAN bCreate)
{
	tdi_foc_ptr			pSocketContext		= tdi_foc_Get(pConnectFileObj);

	if(NULL == pSocketContext && FALSE != bCreate)
	{
		int						nIndex				= CALC_HASH(pConnectFileObj);

		pSocketContext = (tdi_foc_ptr)ExAllocateFromNPagedLookasideList(&g_tdi_foc_HashLookaside);
		if(NULL == pSocketContext)
		{
			KdPrint(("[tdi_foc_Get] ExAllocateFromNPagedLookasideList Failed %x to %d\n", pConnectFileObj, nIndex));
			return NULL;
		}
		RtlZeroMemory(pSocketContext, sizeof(tdi_foc));
		pSocketContext->pAddressFileObj = pConnectFileObj;
		ExInterlockedInsertHeadList(&g_tdi_foc_HashTable[nIndex], &pSocketContext->list, &g_tdi_foc_HashSpinLock);
		KdPrint(("[tdi_foc_GetConnection] insert %x to %d\n", pConnectFileObj, nIndex));
	}
	else if(NULL != pSocketContext)
	{
		if(FALSE != pSocketContext->bIsAddressFileObj)
			return NULL;
	}
	return pSocketContext;
}
/*
 *	删除结构体
 */
void				tdi_foc_Erase(PFILE_OBJECT pAddressFileObj)
{
	int						nIndex		= CALC_HASH(pAddressFileObj);
	tdi_foc_ptr				pRet		= NULL;
	tdi_foc_ptr				pTemp		= NULL;
	KIRQL					irql;
	PLIST_ENTRY				pList		= NULL;

	if(NULL == g_tdi_foc_HashTable)
		return;
	KeAcquireSpinLock(&g_tdi_foc_HashSpinLock, &irql);
	// 在现有的TABLE里查找
	for(pList = g_tdi_foc_HashTable[nIndex].Blink
		; pList != &g_tdi_foc_HashTable[nIndex]
		; pList = pList->Blink)
	{
		pTemp = CONTAINING_RECORD(pList, tdi_foc, list);
		if(pAddressFileObj == pTemp->pAddressFileObj)
		{
			pList->Flink->Blink = pList->Blink;
			pList->Blink->Flink = pList->Flink;
			if(FALSE == pTemp->bIsAddressFileObj && NULL != pTemp->connecation.pHost)
			{
				ExFreePoolWithTag(pTemp->connecation.pHost, 'ehom');
				pTemp->connecation.pHost = NULL;
			}
			ExFreeToNPagedLookasideList(&g_tdi_foc_HashLookaside, pTemp);
			KdPrint(("[tdi_foc_Erase] delete %x from %d\n", pAddressFileObj, nIndex));
			break;
		}
	}
	
	KeReleaseSpinLock(&g_tdi_foc_HashSpinLock, irql);
}

/*
 *	释放结构体
 */
void		tdi_foc_Release()
{
	int						i;
	tdi_foc_ptr		pTemp		= NULL;
	KIRQL					irql;

	if(NULL == g_tdi_foc_HashTable)
		return;
	KeAcquireSpinLock(&g_tdi_foc_HashSpinLock, &irql);
	// 在现有的TABLE里查找
	for(i = 0; i < HASH_SIZE; i++)
	{
		while(FALSE == IsListEmpty(&g_tdi_foc_HashTable[i]))
		{
			pTemp = CONTAINING_RECORD(RemoveHeadList(&g_tdi_foc_HashTable[i]), tdi_foc, list);
			ExFreeToNPagedLookasideList(&g_tdi_foc_HashLookaside, pTemp);
		}
	}
	KeReleaseSpinLock(&g_tdi_foc_HashSpinLock, irql);
}

/* 是否HTTP请求 */
BOOLEAN tdi_foc_CheckConnection(PFILE_OBJECT fileObj, tdi_foc_connection_ptr* pAddress, tdi_foc_ptr* ppSockConnect)
{
	*pAddress = NULL;
	*ppSockConnect = tdi_foc_GetConnection(fileObj, FALSE);
	if(NULL != *ppSockConnect)
	{
		*pAddress = &( (*ppSockConnect)->connecation );
	}
	return NULL != *pAddress;
}
