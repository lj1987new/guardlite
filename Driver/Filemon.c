/*
 *	文件监控模块
 */
#include "GuardLite.h"
#include "Filemon.h"
#include "Public.h"

// 驱动所在目录
WCHAR			gGuardPath[MAX_PATH]		= {0};
// 要过滤的目录
GUARDPATH		FileGuardPath[]				= {
	{MASK_SYSTEM_AUTORUN, L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", NULL, 0, -1}
	, {MASK_SYSTEM_AUTORUN, L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", NULL, 0, -1}
};
//////////////////////////////////////////////////////////////////////////
const FLT_OPERATION_REGISTRATION FltCallbacks[]  = {
	{
		IRP_MJ_CREATE
		, 0
		, FilemonPreCreate
		, FilemonPostCreate
	}
	, {
		IRP_MJ_DIRECTORY_CONTROL
		, 0
		, FilemonPreCreate
		, FilemonPostCreate
	}
	, { IRP_MJ_OPERATION_END }
};

const FLT_CONTEXT_REGISTRATION FltContextRegistration[] = {
	{ 
		FLT_STREAMHANDLE_CONTEXT
		, 0
		, NULL
		, sizeof(SCANNER_STREAM_HANDLE_CONTEXT)
		, 'etil'/*'chBS'*/ 
	}
	
	, { FLT_CONTEXT_END }
};

const FLT_REGISTRATION FilterRegistration = {

	sizeof( FLT_REGISTRATION )         //  Size
	, FLT_REGISTRATION_VERSION           //  Version
	, 0                                  //  Flags
	, FltContextRegistration                //  Context Registration.
	, FltCallbacks                          //  Operation callbacks
	, FltUnload                      //  FilterUnload
	, FltInstanceSetup               //  InstanceSetup
	, FltQueryTeardown               //  InstanceQueryTeardown
	, NULL                               //  InstanceTeardownStart
	, NULL                               //  InstanceTeardownComplete
	, NULL                               //  GenerateFileName
	, NULL                               //  GenerateDestinationFileName
	, NULL                                //  NormalizeNameComponent
};

#pragma PAGEDCODE
FLT_FILEMON_DATA		FltData		= {0};

//////////////////////////////////////////////////////////////////////////
NTSTATUS	FilemonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS			status;

	FltData.Filter = NULL;
	status = FltRegisterFilter(pDriverObject, &FilterRegistration, &FltData.Filter);
	if(!NT_SUCCESS(status))
		return status;
	status = FltStartFiltering(FltData.Filter);
	if(!NT_SUCCESS(status))
	{
		FltUnregisterFilter(FltData.Filter);
		return status;
	}
	return STATUS_SUCCESS;
}

void		FilemonUnload()
{
	if(NULL != FltData.Filter)
		FltUnregisterFilter(FltData.Filter);
}

NTSTATUS	FilemonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS FilemonPreCreate (__inout PFLT_CALLBACK_DATA Data
											, __in PCFLT_RELATED_OBJECTS FltObjects
											, __deref_out_opt PVOID *CompletionContext)
{
	PFLT_FILE_NAME_INFORMATION		nameInfo;
	NTSTATUS						status;
	LONG							nType;


	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( CompletionContext );

	PAGED_CODE();

	if (FALSE == IsGuardStart()) 
	{
		KdPrint(( "!!! scanner.sys -- allowing create for trusted process \n" ));
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (!NT_SUCCESS( Data->IoStatus.Status ) ||
		(STATUS_REPARSE == Data->IoStatus.Status)) 
	{
		KdPrint(("[ScannerPreCreate] Data->IoStatus.Status:%d.\n", Data->IoStatus.Status));
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}


	status = FltGetFileNameInformation(Data
		, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT
		, &nameInfo);

	if (!NT_SUCCESS( status )) 
		return FLT_POSTOP_FINISHED_PROCESSING;
	
	FltParseFileNameInformation( nameInfo );
	KdPrint(("[ScannerPreCreate] monitoer:%wZ \n", &nameInfo->Name));
	if(IsFilemonGuardPath(nameInfo->Name.Buffer
		, FlagOn(FILE_DIRECTORY_FILE, Data->Iopb->Parameters.Create.Options) > 0
		, &nType))
	{
		WCHAR		szPath[MAX_PATH]		= {0};

		wcsncpy(szPath, nameInfo->Name.Buffer, min(MAX_PATH-1, nameInfo->Name.Length));
		FltReleaseFileNameInformation( nameInfo );
		// 获取通过与否
		if(FALSE != CheckRequestIsAllowed(MAKEGUARDTYPE(MASK_GUARDLITE_FILEMON, nType)
			, szPath
			, L""
			, L""))
		{
			return FLT_PREOP_SUCCESS_NO_CALLBACK;
		}
		// 未通过
		if(FlagOn( (FILE_CREATE<<24), Data->Iopb->Parameters.Create.Options ))
		{
			Data->IoStatus.Information = 0;
			Data->IoStatus.Status = STATUS_ACCESS_DENIED;
			return FLT_PREOP_COMPLETE;
		}
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}
	
	FltReleaseFileNameInformation( nameInfo );
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS FilemonPostCreate (__inout PFLT_CALLBACK_DATA Data
											  , __in PCFLT_RELATED_OBJECTS FltObjects
											  , __in_opt PVOID CompletionContext
											  , __in FLT_POST_OPERATION_FLAGS Flags)
{
	UNREFERENCED_PARAMETER( CompletionContext );
	UNREFERENCED_PARAMETER( Flags );

	if (!NT_SUCCESS( Data->IoStatus.Status ) ||
		(STATUS_REPARSE == Data->IoStatus.Status)) 
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	if(FltObjects->FileObject->WriteAccess)
	{
		FltCancelFileOpen( FltObjects->Instance, FltObjects->FileObject );
		KdPrint(("[ScannerPreCreate] CancelFile(0x%X):%wZ.\n", FltObjects->Instance, &FltObjects->FileObject->FileName));

		Data->IoStatus.Status = STATUS_ACCESS_DENIED;
		Data->IoStatus.Information = 0;

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	return STATUS_SUCCESS;
}
// flt的御载
NTSTATUS FltUnload (__in FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER( Flags );
	KdPrint(("[ScannerUnload] Enter.\n"));

	if(NULL != FltData.Filter)
		FltUnregisterFilter(FltData.Filter);
	
	return STATUS_SUCCESS;
}

NTSTATUS FltQueryTeardown (__in PCFLT_RELATED_OBJECTS FltObjects
						   , __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( Flags );

	KdPrint(("[ScannerQueryTeardown] Enter.\n"));

	return STATUS_SUCCESS;
}

NTSTATUS FltInstanceSetup (__in PCFLT_RELATED_OBJECTS FltObjects
						   , __in FLT_INSTANCE_SETUP_FLAGS Flags
						   , __in DEVICE_TYPE VolumeDeviceType
						   , __in FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( Flags );
	UNREFERENCED_PARAMETER( VolumeFilesystemType );

	PAGED_CODE();

	ASSERT( FltObjects->Filter == FltData.Filter );

	KdPrint(("[ScannerInstanceSetup] Enter.\n"));
	//
	//  Don't attach to network volumes.
	//
	if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {

		return STATUS_FLT_DO_NOT_ATTACH;
	}

	//FltGetVolumeName(FltObjects->Volume, &usVolumn, NULL); 
	KdPrint(("[ScannerInstanceSetup] Attach volumn[0x%x].\n", FltObjects->Volume));
	return STATUS_SUCCESS;
}

/*
 *	判断是不是工作目录
 */
BOOLEAN			IsFilemonGuardPath(PWSTR pPath, BOOLEAN isDir, LONG* pSubType)
{
	ULONG			ulHashs[MAX_PATH]	= {0};
	LONG			nCheck				= 0;
	WCHAR*			pCheck				= NULL;
	ULONG			ulPathHash			= 0;
	LONG			i;

	if(NULL == pPath)
		return FALSE;

	pCheck = wcsrchr(pPath, L'\\');
	if(NULL == pCheck)
		return FALSE;
	
	nCheck = pCheck - pPath;
	// 如果长度不对，就返回
	if(nCheck <= 0)
		return FALSE;
	// 获取HASH
	ulPathHash = GetHashUprPath(pPath, ulHashs);
	// 开始比较
	for(i = 0; i < arrayof(FileGuardPath); i++)
	{
		if(NULL != pSubType)
			*pSubType = FileGuardPath[i].nSubType;
		// 目录处理
		if(FALSE != isDir && ulPathHash == FileGuardPath[i].ulPathHash)
			return TRUE;
		
		if(FileGuardPath[i].nPathLen >= 0 && FileGuardPath[i].nPathLen <= nCheck)
		{
			if(ulHashs[ FileGuardPath[i].nPathLen ] != FileGuardPath[i].ulPathHash)
				continue;

			if( pPath[ FileGuardPath[i].nPathLen ] != L'\\')
				return FALSE;
			return TRUE;
		}
		else
		{
			if(ulPathHash != FileGuardPath[i].ulPathHash)
				continue;

			return TRUE;
		}
	}

	return FALSE;
}