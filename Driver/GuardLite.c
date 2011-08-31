/*
 *	驱动主模块
 */
#include "GuardLite.h"
#include "Regmon.h"
#include "Public.h"

#pragma LOCKEDDATA
PACK_QUEUE		gPackQueue;
#pragma LOCKEDDATA
IRP_READ_STACK	gIrpReadStack;
#pragma LOCKEDDATA
LONG			gGuardStatus;
#pragma LOCKEDDATA
PEPROCESS		gOptProcess;

// 驱动主函数
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS			status;
	int					i;
	UNICODE_STRING		usDeviceName		= RTL_CONSTANT_STRING(GUARDLITE_DEVICE);
	UNICODE_STRING		usLinkName			= RTL_CONSTANT_STRING(GUARDLITE_LINK);
	PDEVICE_OBJECT		pCtrlDev			= NULL;
	PDEVICE_EXTENSION	pDevExt				= NULL;

	for(i = 0; i < arrayof(pDriverObject->MajorFunction); i++)
	{
		pDriverObject->MajorFunction[i] = DriverDispatchRoutine;
	}
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControlRuntine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCloseRuntine;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateRuntine;
	pDriverObject->DriverUnload = DriverUnload;
	// 创建管理设备
	status = IoCreateDevice(pDriverObject
		, sizeof(DEVICE_EXTENSION)
		, &usDeviceName
		, FILE_DEVICE_UNKNOWN
		, 0
		, TRUE
		, &pCtrlDev);
	if(!NT_SUCCESS(status))
	{
		IoDeleteDevice(pCtrlDev);
		return status;
	}
	// 创建连接
	status = IoCreateSymbolicLink(&usLinkName, &usDeviceName);
	if(!NT_SUCCESS(status))
	{
		IoDeleteDevice(pCtrlDev);
		return status;
	}
	// 设置设备属性
	pDevExt = (PDEVICE_EXTENSION)pCtrlDev->DeviceExtension;
	pCtrlDev->Flags |= DO_BUFFERED_IO;
	pDevExt->DeviceType = GLDT_Main;
	RtlCopyUnicodeString(&pDevExt->LinkName, &usLinkName);
	pDevExt->StartMask = 0L;
	// 注册监控函数
	gGuardStatus = 0L;
	gOptProcess = NULL;
	if(STATUS_SUCCESS != FilemonEntry(pDriverObject, pRegistryPath))
		goto failed;
	if(STATUS_SUCCESS != RegmonEntry(pDriverObject, pRegistryPath))
		goto failed;
	if(STATUS_SUCCESS != ServicesEntry(pDriverObject, pRegistryPath))
		goto failed;
	if(STATUS_SUCCESS != ProcmonEntry(pDriverObject, pRegistryPath))
		goto failed;
	goto successed;
failed:
	FilemonUnload();
	RegmonUnload();
	ServicesUnload();
	ProcmonUnload();
	IoDeleteSymbolicLink(&usLinkName);
	IoDeleteDevice(pCtrlDev);
	return STATUS_ABANDONED; 
	// 初始化PACK链表
successed:
	InitializeListHead(&gPackQueue.list);
	ExInitializeNPagedLookasideList(&gPackQueue.lookaside, NULL, NULL, 0, sizeof(INNERPACK_LIST), PAGE_DEBUG, 0);
	gPackQueue.ulWaitID = 0L;
	KeInitializeMutex(&gPackQueue.mutex, 0);
	// 初始化IRP栈变量
	RtlZeroMemory(&gIrpReadStack, sizeof(gIrpReadStack));
	gIrpReadStack.lPos = -1L;
	KeInitializeSpinLock(&gIrpReadStack.spinkLock);

	return status;
}

// 默认例程
NTSTATUS DriverDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// 打开例程
NTSTATUS DriverCreateRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// 关闭例程
NTSTATUS DriverCloseRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	CancelReadIrp();

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// 控制例程
NTSTATUS DriverDeviceControlRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PIO_STACK_LOCATION			pStack;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	switch(pStack->Parameters.DeviceIoControl.IoControlCode)
	{
	case GUARDLITE_CTRL_START:
		gOptProcess = PsGetCurrentProcess();
		gGuardStatus = 1L;
		break;
	case GUARDLITE_CTRL_STOP:
		gGuardStatus = 0L;
		gOptProcess = NULL;
		break;
	case GUARDLITE_CTRL_STATUS:
		{
			if(pStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ULONG))
			{
				RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &gGuardStatus, sizeof(ULONG));
				pIrp->IoStatus.Information = sizeof(ULONG);
			}
			else
			{
				pIrp->IoStatus.Information = 0;
				pIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
			}
		}
		break;
	case GUARDLITE_CTRL_REQUEST:
		return AddIrpToQueue(pIrp);
	case GUARDLITE_CTRL_RESPONSE:
		return ResponseToQueue(pIrp);
	}

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// 监控是否开启
BOOLEAN IsGuardStart()
{
	return FALSE != gGuardStatus && gOptProcess != PsGetCurrentProcess();
}

// 御载驱动
void DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT			pNextDev;

	// 删除各种监控
	FilemonUnload();
	RegmonUnload();
	ServicesUnload();
	ProcmonUnload();
	// 删除设备
	pNextDev = pDriverObject->DeviceObject;
	while(NULL != pNextDev)
	{
		PDEVICE_OBJECT		pCurDev		= pNextDev;
		PDEVICE_EXTENSION	pDevExt;
		
		pNextDev = pNextDev->NextDevice;
		pDevExt = (PDEVICE_EXTENSION)pCurDev->DeviceExtension;
		IoDeleteSymbolicLink(&pDevExt->LinkName);
		IoDeleteDevice(pCurDev);
	}
}

// 删除未完成的IRP
void CancelReadIrp()
{
	PIRP			pReadIrp;

	gGuardStatus = 0L;
	gOptProcess = NULL;
	do 
	{
		pReadIrp = IrpReadStackPop();
		if(NULL == pReadIrp)
			break;
		pReadIrp->IoStatus.Information = 0;
		pReadIrp->IoStatus.Status = STATUS_CANCELLED;
		IoCompleteRequest(pReadIrp, IO_NO_INCREMENT);
	} while (TRUE);
}