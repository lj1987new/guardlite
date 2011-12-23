/*  
// //////////////////////////////////////////////////////////////////////////////////////////
//             EHomeNet 
//             Edit by Chao Ding 2010 08 20
*/


#include <Ntifs.h>
#include <ntddk.h>
#include "DevCtl.h"
#include "EhomeProc.h"
#include <portcls.h>
#include "Process.h"


//NTSTATUS PsLookupProcessByProcessId(IN   HANDLE   ulProcId,   OUT   PEPROCESS   *   pEProcess);
NTSTATUS ZwQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass
										  , PVOID ProcessInformation, ULONG ProcessInformationLength
										  ,	PULONG ReturnLength);
NTSTATUS	GetProcessFullName(HANDLE hPid, WCHAR* pPath, LONG nLen);
NTSTATUS	GetProcessFullName3(HANDLE hPid, WCHAR* pPath, LONG nLen);
NTSTATUS	KillProcess(ULONG64 nPID);
NTSTATUS	MmUnmapViewOfSection(IN PEPROCESS Process, IN PVOID BaseAddress);
NTSTATUS	InjectionCodetoProcess(HANDLE ProcessID, PEPROCESS Epro);
PVOID		GetNtdllBaseAddress();

typedef struct _CallbackInfoList{
	LIST_ENTRY			ListEntry;
	CALLBACK_INFO		cbInfo;
}CALLBACKINFO_LIST, *PCALLBACKINFO_LIST;

BOOL 			ProcMonitOn=FALSE;
// PKMUTEX			nameMutex=NULL;
PDEVICE_OBJECT	g_pDeviceObject;
int 			NamePos=0;


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING ustrDevName;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;
	UNICODE_STRING ustrLinkName;

	PEPROCESS epro=PsGetCurrentProcess();
	for(NamePos=0;NamePos<1024;NamePos++)
	{
		if(!strcmp((char*)epro+NamePos,"System"))
			break;
	}
	
	
	pDriverObj->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	pDriverObj->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
// #if defined(DBG)
// 	pDriverObj->DriverUnload = DriverUnload;
// #endif

	
	RtlInitUnicodeString(&ustrDevName, DEVICE_NAME);
	// 创建设备对象
	
	status = IoCreateDevice(pDriverObj, 
				sizeof(DEVICE_EXTENSION), // 为设备扩展结构申请空间
				&ustrDevName, 
				FILE_DEVICE_UNKNOWN,
				0,
				FALSE,
				&pDevObj);
	if(!NT_SUCCESS(status))
	{
		return status;
	}
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	
	InitializeListHead(&pDevExt->ListHeader);
	ExInitializeNPagedLookasideList(&pDevExt->ListLookAside, NULL, NULL, 0, sizeof(CALLBACKINFO_LIST), 'mohe', 0);
	KeInitializeSpinLock(&pDevExt->ListLock);

	RtlInitUnicodeString(&ustrLinkName, LINK_NAME);
	status = IoCreateSymbolicLink(&ustrLinkName, &ustrDevName);  
	if(!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);  
		return status;
	}


	g_pDeviceObject = pDevObj;


    status = PsSetCreateProcessNotifyRoutine(ProcessCallback, FALSE);
	if(!NT_SUCCESS(status))
	{
		IoDeleteSymbolicLink(&ustrLinkName);
		IoDeleteDevice(pDevObj);
	}

	return status;
}


void DriverUnload(PDRIVER_OBJECT pDriverObj)
{
	UNICODE_STRING strLink;
	
    PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);
	RtlInitUnicodeString(&strLink, LINK_NAME);
	IoDeleteSymbolicLink(&strLink);

	IoDeleteDevice(pDriverObj->DeviceObject);
}


NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}
NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PDEVICE_EXTENSION		pDevExt;
	PLIST_ENTRY				pList			= NULL;
	
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	ProcMonitOn=FALSE;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
//	ObDereferenceObject(devExt->ProcessEvent);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	// 清除缓存队列
	do
	{
		pList = ExInterlockedRemoveHeadList(&pDevExt->ListHeader, &pDevExt->ListLock);
	}while(NULL != pList && pList != &pDevExt->ListHeader);
	return STATUS_SUCCESS;
}


NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{

	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

	PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);


	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;


	ULONG uIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	char* inbuf=(char*)pIrp->AssociatedIrp.SystemBuffer;

	PCALLBACK_INFO pCallbackInfo = (PCALLBACK_INFO)pIrp->AssociatedIrp.SystemBuffer;
	ULONG uInSize = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG uOutSize = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch(uIoControlCode)
	{
	case IOCTL_NTPROCDRV_GET_PROCINFO:  
		{
			int						i;
			PLIST_ENTRY				pListEntry		= NULL;
			PCALLBACKINFO_LIST		pcbInfo			= NULL;
			ULONG					nCpSize			= 0;

			for(i = 0; i < (int)(uOutSize / sizeof(CALLBACK_INFO)); i++)
			{
				// 取出一个值
				pListEntry = ExInterlockedRemoveHeadList(&pDevExt->ListHeader, &pDevExt->ListLock);
				if(NULL == pListEntry || pListEntry == &pDevExt->ListHeader)
					break;
				// 复制值
				pcbInfo = CONTAINING_RECORD(pListEntry, CALLBACKINFO_LIST, ListEntry);
				RtlCopyMemory(pCallbackInfo, &pcbInfo->cbInfo, sizeof(CALLBACK_INFO));
				nCpSize += sizeof(CALLBACK_INFO);
				KdPrint(("get one info(%d) %I64d, %I64d, %S, %d\n", i, pCallbackInfo->hProcessId, pCallbackInfo->hParentId, pCallbackInfo->szImagePath, pCallbackInfo->bCreate));
				// 释放内存
				ExFreeToNPagedLookasideList(&pDevExt->ListLookAside, pcbInfo);
				pCallbackInfo = (PCALLBACK_INFO)((char*)pCallbackInfo + sizeof(CALLBACK_INFO));
			}
			if(FALSE == IsListEmpty(&pDevExt->ListHeader))
				KeSetEvent(pDevExt->ProcessEvent, 0, FALSE);	// 如果链表不为空就重设事件
			uOutSize = nCpSize;
			KdPrint(("get info count(%d) size: %d:%d\n", uOutSize / sizeof(CALLBACK_INFO), uOutSize, sizeof(CALLBACK_INFO)));
			status = STATUS_SUCCESS;
// 			if(uOutSize >= sizeof(CALLBACK_INFO))
// 			{
// 				KeWaitForSingleObject(nameMutex,Executive,KernelMode,FALSE,NULL);
// 				pCallbackInfo->hParentId  = (ULONGLONG)pDevExt->hPParentId;
//                 pCallbackInfo->hProcessId = (ULONGLONG)pDevExt->hPProcessId;
//                 pCallbackInfo->bCreate    = pDevExt->bPCreate;                          
// 				
// 				strcpy(pCallbackInfo->proname,pDevExt->proname);
// 				KeReleaseMutex(nameMutex,FALSE);
// 
//                 status = STATUS_SUCCESS;
// 			}
			break;
		}
	case IOCTL_SET_EVENT:
		{
		    HANDLE		EventHandle		= NULL;
			
			if(8 == uInSize)
				EventHandle = (HANDLE)*((PULONGLONG)inbuf);
			else
				EventHandle = (HANDLE)(__int64)*((PULONG)inbuf);
			status=ObReferenceObjectByHandle(EventHandle,SYNCHRONIZE,*ExEventObjectType,pIrp->RequestorMode,(PVOID*)&pDevExt->ProcessEvent,NULL);
			if(NT_SUCCESS(status))
				ProcMonitOn=TRUE;

			break;
		}
	case IOCTL_NTPROCDRV_STOP_PROC_MONITOR:
		{

			KdPrint(("Enter stop proc monitor\n"));
			ProcMonitOn=FALSE;
//			PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);
			status=STATUS_SUCCESS;
			break;
		}
	case IOCTL_KILL_PROCESS:
		{
			if(8 <= uInSize)
			{
				status = KillProcess(*(PULONG64)inbuf);
			}
			break;
		}
		
	}

	if(status == STATUS_SUCCESS)
		pIrp->IoStatus.Information = uOutSize;
	else
		pIrp->IoStatus.Information = 0;

	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}


VOID ProcessCallback(IN HANDLE  hParentId, IN HANDLE  hProcessId, IN BOOLEAN bCreate)
{
	PEPROCESS		Epro;
	NTSTATUS		status;

	PsLookupProcessByProcessId(hProcessId, &Epro);
/*	char* proname=(char*)Epro+NamePos;
	if(stricmp(proname,"Ehome.exe")==0&&!bCreate)
	{

		ProcMonitOn=FALSE;
		return;
	}*/
	if(ProcMonitOn)
	{
		PDEVICE_EXTENSION		pDevExt			=  (PDEVICE_EXTENSION)g_pDeviceObject->DeviceExtension;
		PCALLBACKINFO_LIST		pcbInfo			= (PCALLBACKINFO_LIST)ExAllocateFromNPagedLookasideList(&pDevExt->ListLookAside);
		WCHAR					szImageName[260]	= {0};

		if(NULL != pcbInfo)
		{
			pcbInfo->cbInfo.hParentId = (__int64)hParentId;
			pcbInfo->cbInfo.hProcessId = (__int64)hProcessId;
			pcbInfo->cbInfo.bCreate = bCreate;
			RtlZeroMemory(pcbInfo->cbInfo.szImagePath, sizeof(pcbInfo->cbInfo.szImagePath));
			KeAttachProcess(Epro);
			status = GetProcessFullName3(hProcessId, szImageName, 259);
			if(!NT_SUCCESS(status))
				status = GetProcessFullName(hProcessId, szImageName, 259);
			
			KeDetachProcess();
			// InjectionCodetoProcess(hProcessId, Epro);
			if(0 == status)
			{
				wcsncpy(pcbInfo->cbInfo.szImagePath, szImageName, 260);
			}
			else
			{
				KdPrint(("GetProcessFullName failed(%s): %d\n", (char *)Epro+NamePos, status));
			}
			ExInterlockedInsertTailList(&pDevExt->ListHeader, &pcbInfo->ListEntry, &pDevExt->ListLock);
			KdPrint(("Find one ProcEvent: %I64d, %I64d, %S, %d\n", (__int64)hProcessId, (__int64)hParentId, pcbInfo->cbInfo.szImagePath, bCreate));
		}

// 	    KeWaitForSingleObject(nameMutex,Executive,KernelMode,FALSE,NULL);
//         pDevExt->hPParentId  = (__int64)hParentId;
//         pDevExt->hPProcessId = (__int64)hProcessId;
//         pDevExt->bPCreate    = bCreate;
// 	    
// 
// 
// 	
// 	    strcpy(pDevExt->proname,(char*)Epro+NamePos);
// 	    KeReleaseMutex(nameMutex,FALSE);

// 	    KdPrint(("process is:%s  %d\n",pDevExt->proname,bCreate));
        KeSetEvent(pDevExt->ProcessEvent, 0, FALSE);
	}
	KdPrint(("Leave Processcallback\n"));
	ObDereferenceObject(Epro);

}

NTSTATUS QuerySymbolicLink(PUNICODE_STRING SymbolicLinkName,  OUT PUNICODE_STRING LinkTarget)
{
	OBJECT_ATTRIBUTES			oa;
	NTSTATUS					status;
	HANDLE						h;

	InitializeObjectAttributes(&oa, SymbolicLinkName, OBJ_CASE_INSENSITIVE,
		0, 0);

	status = ZwOpenSymbolicLinkObject(&h, GENERIC_READ, &oa);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = ZwQuerySymbolicLinkObject(h, LinkTarget, NULL);
	ZwClose(h);

	return status;
}

void VolumePathToDosPath(PUNICODE_STRING pusPath, WCHAR* pPath, LONG nLen)
{
	NTSTATUS					status;
	PFILE_OBJECT				FileObject;
	PDEVICE_OBJECT				DeviceObject;
	UNICODE_STRING				usDevice			= {0};
	UNICODE_STRING				usPath				= {0};
	WCHAR*						pSplit				= NULL;

	// 分离设备名
	pSplit = wcschr(pusPath->Buffer, L'\\');
	if(NULL == pSplit)
		goto vptd_end;
	pSplit = wcschr(pSplit+1, L'\\');
	if(NULL == pSplit)
		goto vptd_end;
	pSplit = wcschr(pSplit+1, L'\\');
	if(NULL == pSplit)
		goto vptd_end;
	usDevice.Buffer = pusPath->Buffer;
	usDevice.Length = (pSplit - pusPath->Buffer) * sizeof(WCHAR);
	usDevice.MaximumLength = usDevice.Length;
	// 打开设备
	do{
		UNICODE_STRING				usLinkName			= {0};
		WCHAR						szLinkName[]		= {L"\\??\\C:"};
		WCHAR						i					= L'C';

		usLinkName.Buffer = szLinkName;
		usLinkName.MaximumLength = usLinkName.Length = 6 * sizeof(WCHAR);
		for(; i <= L'Z'; i++)
		{
			UNICODE_STRING				usLinkDevice		= {0};
			WCHAR						szLinkDevice[128]	= {0};
			
			usLinkDevice.Buffer = szLinkDevice;
			usLinkDevice.MaximumLength = 128 * sizeof(WCHAR);
			szLinkName[4] = i;
			status = QuerySymbolicLink(&usLinkName, &usLinkDevice);
			if(RtlEqualUnicodeString(&usLinkDevice, &usDevice, FALSE))
				break;
		}
		if(i > L'Z')
			break;
		pPath[0] = i;
		pPath[1] = L':';
		wcsncpy(&pPath[2], pSplit, nLen - 2);
	}while(0);
	// 返回
vptd_end:
	if(0 == pPath[0])
		RtlCopyMemory(pPath, pusPath->Buffer, nLen);
}

NTSTATUS GetProcessFullName(HANDLE hPid, WCHAR* pPath, LONG nLen)
{
	HANDLE					hProcess		= NULL;
	NTSTATUS				status;
	OBJECT_ATTRIBUTES		objattr			= {0};
	CLIENT_ID				cid				= {0};
	PUNICODE_STRING			pusPath			= {0};
	WCHAR					szBuff[sizeof(UNICODE_STRING) / sizeof(WCHAR) + 260]		= {0};

	objattr.Length = sizeof(objattr);
	cid.UniqueProcess = hPid;
	status = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &objattr, &cid);
	if(!NT_SUCCESS(status))
		return status;
	pusPath = (PUNICODE_STRING)szBuff;
	pusPath->MaximumLength = 260 * sizeof(WCHAR);
	pusPath->Buffer = szBuff + ( sizeof(UNICODE_STRING) / sizeof(WCHAR) );
	status = ZwQueryInformationProcess(hProcess, ProcessImageFileName, pusPath, sizeof(szBuff)/*sizeof(usDevice)*/, NULL);
	//RtlCopyMemory(pPath, pusPath->Buffer, nLen);
	ZwClose(hProcess);
	// 转换文件路径
	VolumePathToDosPath(pusPath, pPath, nLen);
	return status;
}

#define BASE_PEB_PROCESS_PARAMETER_OFFSET         0x0010
#define BASE_PROCESS_PARAMETER_FULL_IMAGE_NAME     0x003C

NTSTATUS GetProcessFullName3(HANDLE hPid, WCHAR* pPath, LONG nLen)
{
	HANDLE							hProcess		= NULL;
	NTSTATUS						status;
	OBJECT_ATTRIBUTES				objattr			= {0};
	CLIENT_ID						cid				= {0};
	PROCESS_BASIC_INFORMATION		pbinfo			= {0};
	PCWSTR							pTemp			= NULL;
	ULONG							dwAddress		= 0;

	// 打开进程的数据结构
	objattr.Length = sizeof(objattr);
	cid.UniqueProcess = hPid;
	status = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &objattr, &cid);
	if(!NT_SUCCESS(status))
		return status;
	status = ZwQueryInformationProcess(hProcess, ProcessBasicInformation, &pbinfo, sizeof(pbinfo)/*sizeof(usDevice)*/, NULL);
	ZwClose(hProcess);
	if(!NT_SUCCESS(status))
		return status;
	// 读取进程信息
	dwAddress = (ULONG)pbinfo.PebBaseAddress;
	// 取PEB->ProcessParameters
	__try
	{
		dwAddress += BASE_PEB_PROCESS_PARAMETER_OFFSET;
		if(0 == (dwAddress = *(ULONG*)dwAddress))
			return -1;
		dwAddress += BASE_PROCESS_PARAMETER_FULL_IMAGE_NAME;
		if(0 == (dwAddress = *(ULONG*)dwAddress))
			return -1;
		pTemp=(PCWSTR)dwAddress;
		if (wcslen(pTemp)>4)
		{
			if (pTemp[0]==L'\\'&&
				pTemp[1]==L'?'&&
				pTemp[2]==L'?'&&
				pTemp[3]==L'\\')
			{
				dwAddress+=8;
			}
			if (pTemp[0]==L'\\'&&
				pTemp[1]==L'\\'&&
				pTemp[2]==L'?'&&
				pTemp[3]==L'\\')
			{
				dwAddress+=8;
			}
		}
		wcsncpy(pPath, (WCHAR*)dwAddress, nLen);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return -1;
	}

	return status;
}

NTSTATUS MyZeroProcessMemory3(IN PEPROCESS EProcess) 
{
#if !defined(WIN32)
	return -5;	// 拒绝访问
#else
	KAPC_STATE			ApcState;
	NTSTATUS			status			= -5; 
	ULONG				dwFileSize		= 0;
	PPEB				ppeb			= NULL;
	PVOID				pLdr			= NULL;
	PLIST_ENTRY			pModuleList		= NULL;
	PVOID				pFirstModule	= NULL;
	PVOID				pImageBase		= NULL;
	PVOID				pFirstImageBase	= NULL;

	if(NULL == EProcess)
		return status;
	if(FALSE == EPROCESS__PPEB(EProcess, &ppeb))
		return status;

	KeStackAttachProcess (EProcess, &ApcState);
	__asm{
		push	eax
		cli
		mov		eax, cr0
		and		eax, not 10000h  ;清除cr0的WP位
		mov		cr0, eax
		pop		eax
	}
	__try { 
		// 获取基址
		if(FALSE == PEB__ImageBaseAddress(ppeb, &pImageBase))
			__leave;
		// 获取Dll链表
		if(FALSE == PEB__Ldr(ppeb, &pLdr))
			__leave;
		// 获取第一个模块链表
		if(FALSE == PEB_LDR_DATA__InLoadOrderModuleList(pLdr, &pModuleList))
			__leave;
		if(FALSE == CONTAINING_RECORD__LDR_DATA_TABLE(pModuleList, &pFirstModule))
			__leave;
		if(FALSE == LDR_DATA_TABLE_ENTRY__DllBase(pFirstModule, &pFirstImageBase))
			__leave;
		// 获取模块基址
		if(pImageBase != pFirstImageBase)
			__leave;
		LDR_DATA_TABLE_ENTRY__SizeOfImage(pFirstModule, &dwFileSize);
		// 清0
		ProbeForWrite((void *)pImageBase, (SIZE_T)dwFileSize, 1);
		RtlZeroMemory((void *)pImageBase, dwFileSize);
		status = STATUS_SUCCESS;
	} 
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
	} 
	__asm{
		push	eax
		mov		eax, cr0
		or		eax, 10000h  ;恢复cr0的WP位
		mov		cr0, eax
		sti
		pop		eax
	}
	KeUnstackDetachProcess (&ApcState); 
	return status;
#endif
}

PVOID	GetNtdllBaseAddress()
{
	static PVOID		pNtdll		= NULL;

	if(NULL == pNtdll)
	{
		UNICODE_STRING			usNtdll;

		RtlInitUnicodeString(&usNtdll, L"ntdll.dll");
		pNtdll = GetModuleBaseAddress(PsGetCurrentProcess(), &usNtdll);
	}

	return pNtdll;
}

// 结束进程
NTSTATUS	KillProcess(ULONG64 nPID)
{
	PEPROCESS						Epro;
	NTSTATUS						status;
	HANDLE							hProcess		= NULL;
	OBJECT_ATTRIBUTES				objattr			= {0};
	CLIENT_ID						cid				= {0};
	PROCESS_BASIC_INFORMATION		pbinfo			= {0};

	status = PsLookupProcessByProcessId((HANDLE)nPID, &Epro);
	if(!NT_SUCCESS(status))
		return status;

	objattr.Length = sizeof(objattr);
	cid.UniqueProcess = (HANDLE)nPID;
 	// 普通的结束方法
//   	KeAttachProcess(Epro);
//  	status = ZwTerminateProcess(NULL, 1);
//  	KeDetachProcess();
//  
//  	// 加强的结束方法
//  	if(!NT_SUCCESS(status))
//  	{
//  		KAPC_STATE			ApcState;
//  		NTSTATUS			sttemp;
//  
//  		KeStackAttachProcess(Epro, &ApcState);
//  		sttemp = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &objattr, &cid);
//  		if(NT_SUCCESS(sttemp))
//  		{
//  			status = ZwTerminateProcess(hProcess, 1);
//  			ZwClose(hProcess);
//  		}
//  		KeUnstackDetachProcess(&ApcState);
//  	}
// 
// 	// 卸出ntdll方法
// 	if(!NT_SUCCESS(status))
// 	{
// 		//0x7c920000是ntdll.dll的基址, 经测试XP下可以结束360
// 		status = MmUnmapViewOfSection(Epro, (PVOID)GetNtdllBaseAddress());
// 		KdPrint(("!!!EHomeProc.sys KillProcess us unload ntdll.dll: %x.\n", status));
// 	}
// 	if(NT_SUCCESS(status))
// 	{
// 		ObDereferenceObject(Epro);
// 		return status;
// 	}
// 	KdPrint(("!!!EHomeProc.sys KillProcess us zero memory.\n"));
	// 如果以上两种方法都不行， 就出最具破坏力的办法，内存清0结束法
	status = MyZeroProcessMemory3(Epro);
	ObDereferenceObject(Epro);
	// 清理工作
	return status;
}
// 获取起始地址
NTSTATUS GetProcessStartBase(PPEB peb, PVOID* pEnterPoint, USHORT* pMachine)
{
	UCHAR*		pImageBase		= NULL;
	UCHAR*		pNtHeader		= NULL;

	// 找到ImageBase
	pImageBase = (UCHAR*)*( (ULONG32 *)((char*)peb + 0x8) );
	if( 'ZM' != *((USHORT*)pImageBase) )
		return STATUS_INVALID_ADDRESS;	// 验证是不是PE文件
	// 找到IMAGE_NT_HEADER地址
	pNtHeader = (UCHAR *)( (UCHAR*)pImageBase + *((ULONG32 *)((char *)pImageBase + 0x3C)) );
	if( 'EP' != *((USHORT*)pNtHeader) )
		return STATUS_INVALID_ADDRESS;	// 验证是不是NT文件
	// 判断是64位，还是32位
	*pMachine = *((USHORT *)(pNtHeader + 4));
	if( 0x014c/*IMAGE_FILE_MACHINE_I386*/ == *pMachine )
	{
		// 32位
		*pEnterPoint = (void *)( (UCHAR*)pImageBase + *((ULONG32 *)((char*)pNtHeader + 0x28)) );
	}
	else if( 0x8664/*IMAGE_FILE_MACHINE_AMD64*/ == *pMachine )
	{
		// AMD64位
		*pEnterPoint = (void *)( (UCHAR*)pImageBase + *((ULONG64 *)((char*)pNtHeader + 0x28)) );
	}
	else
	{
		return STATUS_NOT_SUPPORTED;	// 尚不支持
	}

	return 0;
}
// 插入代码到起始地址
NTSTATUS InjectionCodetoImageBase(HANDLE hProcess, void* pStartBase, USHORT wMachine)
{
	UCHAR*				pNewCode					= NULL;
	NTSTATUS			status;
	SIZE_T				nCodeSize					= 1024;
	UCHAR*				pStartCode					= NULL;

	if((0x14c != wMachine && 0x8664 != wMachine) || NULL == pStartBase)
		return STATUS_NOT_SUPPORTED;
	// 分配空间
	status = ZwAllocateVirtualMemory(NULL/*hProcess*/, (PVOID *)&pNewCode, 0,
		&nCodeSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(!NT_SUCCESS(status))
	{
		status = ZwAllocateVirtualMemory(hProcess, (PVOID *)&pNewCode, 0,
			&nCodeSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	}
	if(!NT_SUCCESS(status))
	{
		return status;
	}
	RtlZeroMemory(pNewCode, 1024);
	RtlCopyMemory(pNewCode, pStartBase, 16);
	// 写入代码
	if( 0x014c/*IMAGE_FILE_MACHINE_I386*/ == wMachine )
	{
		// 32位
		pStartCode = pNewCode + 16;
		// 设置原转跳地址
		*((UCHAR *)pStartBase) = 0xE9;	// JMP
		*((ULONG32 *)((UCHAR*)pStartBase+1)) = (ULONG32)pStartCode - (ULONG32)5 - (ULONG32)pStartBase;
		// 写目录代码
		*((UCHAR *)pStartCode) = 0xF4;	// nop
		*((UCHAR *)pStartCode + 1) = 0xE9; // JMP
		*((ULONG32 *)(pStartCode + 2)) = (ULONG32)pStartCode - (ULONG32)5 - (ULONG32)(pStartCode + 1);
		*((ULONG32 *)(pStartCode + 6)) = 0xcc; //int 3
		*((ULONG32 *)(pStartCode + 7)) = 0xcc; //int 3
		*((ULONG32 *)(pStartCode + 8)) = 0xcc; //int 3
		// 跳回原地址
		pStartCode = pNewCode + 32;
		*((UCHAR *)pStartCode) = 0xE9; // JMP
		*((ULONG32 *)(pStartCode + 1)) = (ULONG32)pStartBase - (ULONG32)5 - (ULONG32)pStartCode;   
	}
	else if( 0x8664/*IMAGE_FILE_MACHINE_AMD64*/ == wMachine )
	{
		// AMD64位
	}

	return STATUS_NOT_SUPPORTED;
}
// 插入代码到起始地址
NTSTATUS InjectionCodetoImageBase2(HANDLE hProcess, HANDLE SectionHandle)
{
	UCHAR*				pNewCode					= NULL;
	NTSTATUS			status;
	SIZE_T				nCodeSize					= 1024;
	UCHAR*				pStartCode					= NULL;
	UCHAR*				ImageBase					= NULL;
	SIZE_T				ViewSize					= 0;

	if(NULL == SectionHandle)
		return STATUS_OBJECT_PATH_NOT_FOUND;
	// 加载Section
	status = ZwMapViewOfSection(SectionHandle, hProcess
		, (PVOID*)&ImageBase, 0, 0, NULL
		, &ViewSize, ViewShare, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	return status;
}
// 注入DLL到进程
NTSTATUS InjectionCodetoProcess(HANDLE ProcessID, PEPROCESS Epro)
{
	HANDLE							hProcess		= NULL;
	OBJECT_ATTRIBUTES				objattr			= {0};
	CLIENT_ID						cid				= {0};
	NTSTATUS						status			= STATUS_NOT_SUPPORTED;
	PROCESS_BASIC_INFORMATION		pbinfo			= {0};
	UCHAR*							pStartBase		= NULL;	
	KAPC_STATE						ApcState;
	USHORT							wMachine		= 0;
	static HANDLE					SectionHandle	= NULL;

	objattr.Length = sizeof(objattr);
	cid.UniqueProcess = ProcessID;
	// 加载SectionHandle
	if(NULL == SectionHandle)
	{
		OBJECT_ATTRIBUTES			object;
		UNICODE_STRING				usDll;
		HANDLE						FileHandle			= NULL;
		IO_STATUS_BLOCK				Iosb				= {0};

		// 打开文件
		RtlInitUnicodeString(&usDll, L"\\??\\c:\\ehome.dll");
		InitializeObjectAttributes(&object, &usDll, OBJ_CASE_INSENSITIVE, NULL, NULL);
		status = ZwOpenFile(&FileHandle, FILE_READ_ACCESS
			, &object, &Iosb, FILE_SHARE_READ
			, FILE_SYNCHRONOUS_IO_NONALERT);
		if(!NT_SUCCESS(status))
			return status;
		// 加载到内核
		status = ZwCreateSection(&SectionHandle, SECTION_ALL_ACCESS
			, NULL,	NULL, PAGE_READWRITE, 0x1000000/*SEC_IMAGE*/ /*| 0x8000000SEC_COMMIT*/
			, FileHandle);
		ZwClose(FileHandle);
		if(!NT_SUCCESS(status))
		{
			SectionHandle = NULL;
			return status;
		}
	}
	// 附加到进程
	KeStackAttachProcess (Epro, &ApcState);
	status = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &objattr, &cid);
	if(!NT_SUCCESS(status))
	{
		KeUnstackDetachProcess (&ApcState);
		return status;
	}
	// 查询信息
	status = ZwQueryInformationProcess(hProcess, ProcessBasicInformation, &pbinfo, sizeof(pbinfo), NULL);
	// 找到起始代码
	status = GetProcessStartBase(pbinfo.PebBaseAddress, (PVOID *)&pStartBase, &wMachine);
	if(0 == status)
	{
		__asm{
			push	eax
			cli
			mov		eax, cr0
			and		eax, not 10000h  ;清除cr0的WP位
			mov		cr0, eax
			pop		eax
		}
//		status = InjectionCodetoImageBase(hProcess, pStartBase, wMachine);
		status = InjectionCodetoImageBase2(hProcess, SectionHandle);
		__asm{
			push	eax
			mov		eax, cr0
			or		eax, 10000h  ;恢复cr0的WP位
			mov		cr0, eax
			sti
			pop		eax
		}
	}
	ZwClose(hProcess);
	// 离开进程
	KeUnstackDetachProcess (&ApcState);
	if(!NT_SUCCESS(status))
	{
		status = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &objattr, &cid);
		status = InjectionCodetoImageBase2(hProcess, SectionHandle);
		ZwClose(hProcess);
	}
	return status;
}