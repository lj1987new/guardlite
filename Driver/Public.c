/*
 *	公共服务模块
 */
#include "GuardLite.h"

// 添加IRP队列
NTSTATUS		AddIrpToQueue(PIRP pIrp)
{
	NTSTATUS			status		= STATUS_SUCCESS;
	PIRP_LIST			irpList		= NULL;
	PPACK_LIST			packList	= NULL;
	PLIST_ENTRY			pList		= NULL;
	VOID*				pData		= 0;
	LONG				nLen		= 0;
	PIO_STACK_LOCATION	stack;

	// 验证IRP数据大小
	stack = IoGetCurrentIrpStackLocation(pIrp);
	nLen = stack->Parameters.Read.Length;
	if(nLen < sizeof(GUARDLITEPACK))
	{
		status = STATUS_BUFFER_OVERFLOW;
		return status;
	}
	// 进入IRP获取区域
	KeWaitForSingleObject(&gIrpPackMutex, Executive, KernelMode, FALSE, NULL);
	if(IsListEmpty(&gPackQueue.list))
	{
		// 没有要请求的数据
		status = STATUS_PENDING;
		irpList = (PIRP_LIST)ExAllocateFromNPagedLookasideList(&gIrpQueue.lookaside);
		if(NULL == irpList)
		{
			status = STATUS_BUFFER_OVERFLOW;
			pIrp->IoStatus.Information = 0;
			pIrp->IoStatus.Status = status;
		}
		else
		{
			IoMarkIrpPending(pIrp);
			irpList->pIrp = pIrp;
			InsertTailList(&gIrpQueue.list, &irpList->list);
		}
	}
	else
	{
		status = STATUS_SUCCESS;
		pList = RemoveHeadList(&gPackQueue.list);
		packList = CONTAINING_RECORD(pList, PACK_LIST, list);
		pData = pIrp->AssociatedIrp.SystemBuffer;
		RtlCopyMemory(pData, &packList->query.Pack, sizeof(GUARDLITEPACK));
		InterlockedIncrement(&gPackWaitID);
		packList->query.ulID = gPackWaitID;
		InsertHeadList(&gPackWaitList, pList);
		// 处理其它队列
		DealIrpAndPackQueue();
	}
	KeReleaseMutex(&gIrpPackMutex, FALSE);
	return status;
}
// 添加PACK队列
NTSTATUS		AddPackToQueue(PGUARDLITEQUERY pQuery)
{
	KeWaitForSingleObject(&gIrpPackMutex, Executive, KernelMode, FALSE, NULL);
	KeReleaseMutex(&gIrpPackMutex, FALSE);
	return STATUS_SUCCESS;
}
// 处理队列
NTSTATUS		DealIrpAndPackQueue()
{
	return STATUS_SUCCESS;
}