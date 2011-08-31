/*
 *	公共服务模块
 */
#include "GuardLite.h"
#include "Public.h"

#define REQUEST_TIME_OUT			30		// 请求超时时间

// 添加IRP队列
NTSTATUS		AddIrpToQueue(PIRP pIrp)
{
	PIO_STACK_LOCATION		pStack			= NULL;

	// 是否已经停止监控
	if(0 == gGuardStatus)
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_CANCELLED;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return pIrp->IoStatus.Status;
	}
	// 验证接收缓冲区
	pStack = IoGetCurrentIrpStackLocation(pIrp);
	if(pStack->Parameters.DeviceIoControl.OutputBufferLength < (LONG)sizeof(GUARDLITEREQUEST))
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return pIrp->IoStatus.Status;
	}
	// 插入IRP_READ_STACK
	if(0 > IrpReadStackPush(pIrp))
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_STACK_OVERFLOW;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return pIrp->IoStatus.Status;
	}
	pIrp->IoStatus.Status = STATUS_PENDING;
	IoMarkIrpPending(pIrp);
	// 读取处理队列
	DealIrpAndPackQueue();

	return pIrp->IoStatus.Status;
}
// 添加PACK队列
BOOLEAN		CheckRequestIsAllowed(ULONG ulType, LPCWSTR lpPath, LPCWSTR lpSubPath, LPCWSTR lpValue)
{
	PINNERPACK_LIST			pList			= NULL;
	LARGE_INTEGER			timeout;
	NTSTATUS				status;
	BOOLEAN					bRet			= FALSE;

	// 分配内存空间
	pList = ExAllocateFromNPagedLookasideList(&gPackQueue.lookaside);
	if(NULL == pList)
		return TRUE;
	RtlZeroMemory(&pList->innerPack, sizeof(GUARDLITEINNERPACK));
	pList->innerPack.Read = FALSE;
	pList->innerPack.Timeout = FALSE;
	pList->innerPack.Access = FALSE;
	KeInitializeEvent(&pList->innerPack.Event, NotificationEvent, FALSE);
	pList->innerPack.Pack.dwRequestID = InterlockedDecrement(&gPackQueue.ulWaitID);
	pList->innerPack.Pack.dwProcessID = (ULONG)PsGetCurrentProcessId();
	pList->innerPack.Pack.dwGuardType = ulType;
	wcsncpy(pList->innerPack.Pack.szPath, lpPath, arrayof(pList->innerPack.Pack.szPath));
	wcsncpy(pList->innerPack.Pack.szSubPath, lpSubPath, arrayof(pList->innerPack.Pack.szSubPath));
	wcsncpy(pList->innerPack.Pack.szValue, lpValue, arrayof(pList->innerPack.Pack.szValue));
	// 插入队列
	KeWaitForSingleObject(&gPackQueue.mutex, Executive, KernelMode, FALSE, NULL);
	InsertTailList(&gPackQueue.list, &pList->list);
	KeReleaseMutex(&gPackQueue.mutex, FALSE);
	// 处理队列
	DealIrpAndPackQueue();
	// 等待事件
	timeout = RtlConvertLongToLargeInteger(-REQUEST_TIME_OUT * 1000 * 10000);
	KeWaitForSingleObject(&pList->innerPack.Event, Executive, KernelMode, FALSE, &timeout);
	bRet = (BOOLEAN)pList->innerPack.Access;
	// 删除空间
	EraseFromQueue(pList);

	return bRet;
}
// 处理队列
NTSTATUS		DealIrpAndPackQueue()
{
	PINNERPACK_LIST			pPackList		= NULL;
	PLIST_ENTRY				pCurList		= gPackQueue.list.Flink;
	PIRP					pIrp			= NULL;
	
	KeWaitForSingleObject(&gPackQueue.mutex, Executive, KernelMode, FALSE, NULL);
	for(; pCurList != &gPackQueue.list; pCurList = pCurList->Flink)
	{
		pPackList = CONTAINING_RECORD(pCurList, INNERPACK_LIST, list);
		if(FALSE != pPackList->innerPack.Read || FALSE != pPackList->innerPack.Timeout)
			continue;
		pIrp = IrpReadStackPop();
		if(NULL == pIrp)
			break;
		RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &pPackList->innerPack.Pack, sizeof(GUARDLITEREQUEST));
		pPackList->innerPack.Read = TRUE;
		// 完成IRP
		pIrp->IoStatus.Information = (ULONG)sizeof(GUARDLITEREQUEST);
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}
	KeReleaseMutex(&gPackQueue.mutex, FALSE);

	return STATUS_SUCCESS;
}
// 应答控制
NTSTATUS	ResponseToQueue(PIRP pIrp)
{
	PIO_STACK_LOCATION				pStack;
	PGUARDLITERERESPONSE			pResponse;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	if(pStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(GUARDLITERERESPONSE))
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return pIrp->IoStatus.Status;
	}
	pResponse = (PGUARDLITERERESPONSE)pIrp->AssociatedIrp.SystemBuffer;
	
	SetPackForQuery(pResponse->dwReuestID, (BOOLEAN)pResponse->bAllowed);

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return pIrp->IoStatus.Status;
}
// 删除Pack
void		EraseFromQueue(PINNERPACK_LIST pQuery)
{
	PINNERPACK_LIST			pPackList		= NULL;
	PLIST_ENTRY				pCurList		= gPackQueue.list.Flink;
	PIRP					pIrp			= NULL;

	pQuery->innerPack.Timeout = TRUE;
	KeWaitForSingleObject(&gPackQueue.mutex, Executive, KernelMode, FALSE, NULL);
	for(; pCurList != &gPackQueue.list; pCurList = pCurList->Flink)
	{
		pPackList = CONTAINING_RECORD(pCurList, INNERPACK_LIST, list);
		if(pQuery != pPackList)
			continue;
		// 从队列中删除
		pCurList->Blink->Flink = pCurList->Flink;
		pCurList->Flink->Blink = pCurList->Blink;
		ExFreeToNPagedLookasideList(&gPackQueue.lookaside, pPackList);
		break;
	}
	KeReleaseMutex(&gPackQueue.mutex, FALSE);
}
// 设置是否通过队列
void		SetPackForQuery(ULONG nWaitID, BOOLEAN Access)
{
	PINNERPACK_LIST			pPackList		= NULL;
	PLIST_ENTRY				pCurList		= gPackQueue.list.Flink;
	PIRP					pIrp			= NULL;

	KeWaitForSingleObject(&gPackQueue.mutex, Executive, KernelMode, FALSE, NULL);
	for(; pCurList != &gPackQueue.list; pCurList = pCurList->Flink)
	{
		pPackList = CONTAINING_RECORD(pCurList, INNERPACK_LIST, list);
		if(nWaitID != pPackList->innerPack.Pack.dwRequestID)
			continue;
		// 设置事件
		pPackList->innerPack.Access = Access;
		KeSetEvent(&pPackList->innerPack.Event, IO_NO_INCREMENT, FALSE);
		break;
	}
	KeReleaseMutex(&gPackQueue.mutex, FALSE);
}
// IRP队列入栈
LONG		IrpReadStackPush(PIRP pIrp)
{
	KIRQL			irql;
	LONG			lRet		= -1L;

	KeAcquireSpinLock(&gIrpReadStack.spinkLock, &irql);
	if(gIrpReadStack.lPos < (LONG)(arrayof(gIrpReadStack.irp) - 1))
	{
		gIrpReadStack.irp[++gIrpReadStack.lPos] = pIrp;
		lRet = gIrpReadStack.lPos;
	}
	KeReleaseSpinLock(&gIrpReadStack.spinkLock, irql);

	return lRet;
}
// IRP队列出栈
PIRP		IrpReadStackPop()
{
	KIRQL			irql;
	PIRP			pIrp		= NULL;

	KeAcquireSpinLock(&gIrpReadStack.spinkLock, &irql);
	if(gIrpReadStack.lPos > -1L)
	{
		pIrp = gIrpReadStack.irp[gIrpReadStack.lPos--];
	}
	KeReleaseSpinLock(&gIrpReadStack.spinkLock, irql);

	return pIrp;
}
/*
 *	得到HASH过的路径
 *
 *  AP Hash
 */
ULONG					GetHashUprPath(LPCWSTR lpPath)
{
	ULONG			ul			= 0;
	ULONG			i;
	ULONG			uSize;
	WCHAR			wc;

	uSize = wcslen(lpPath);
	for(i = 0; i < uSize; i++)
	{
		wc = lpPath[i];
		if(wc >= L'a' && wc <= L'z')
			wc = wc - L'a' + L'A';

		if( (i & 1) == 0)
		{
			ul ^= ( (ul << 7) ^ wc ^ (ul >> 3) );
		}
		else
		{
			ul ^= ( ~((ul << 11) ^ wc ^ (ul >> 5)) );
		}
	}

	return ( ul & 0x7FFFFFFF );
}