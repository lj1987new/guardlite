/*
 *	注册表监控
 */
#include "MinFileMon.h"

PFLT_FILTER		gProcDirFilter			= NULL;
KSPIN_LOCK		gLockSkipProcess;
WCHAR			gProcDir[512]			= {0};

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

	{ FLT_STREAMHANDLE_CONTEXT,
	0,
	NULL,
	sizeof(SCANNER_STREAM_HANDLE_CONTEXT),
	'EHom' },

	{ FLT_CONTEXT_END }
};

const FLT_OPERATION_REGISTRATION Callbacks[] = {

	{ IRP_MJ_CREATE,
	0,
	ScannerPreCreate,
	ScannerPostCreate},

	{ IRP_MJ_DIRECTORY_CONTROL,
	0,
	ScannerPreSetInformation,
	ScannerPostSetInformation} ,

	{ IRP_MJ_OPERATION_END}
};

const FLT_REGISTRATION FilterRegistration = {

	sizeof( FLT_REGISTRATION ),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags
	ContextRegistration,                //  Context Registration.
	Callbacks,                          //  Operation callbacks
	ScannerUnload,                      //  FilterUnload
	ScannerInstanceSetup,               //  InstanceSetup
	ScannerQueryTeardown,               //  InstanceQueryTeardown
	NULL,                               //  InstanceTeardownStart
	NULL,                               //  InstanceTeardownComplete
	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent
};
//////////////////////////////////////////////////////////////////////////
// 入口函数
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS					status				= STATUS_SUCCESS;
	int							i					= 0;
	PFAST_IO_DISPATCH			fastIoDispatch		= NULL;
	UNICODE_STRING				usLink				= RTL_CONSTANT_STRING(L"\\??\\C:");
	UNICODE_STRING				usProcDir;

	pDriverObject->DriverUnload = DDKMinFilemonUnload;
	// 设置过滤目录
	usProcDir.Buffer = gProcDir;
	usProcDir.Length = 0;
	usProcDir.MaximumLength = sizeof(gProcDir);
	QuerySymbolicLink(&usLink, &usProcDir);
	// 由于时间的原因， 暂时在这里设置一个固定值
	wcscat(gProcDir, L"\\xxj\\");
	KdPrint(("[MinFilemon.sys] proc dir: %S\n", gProcDir));
	// 开启文件过滤
	status = FltRegisterFilter( pDriverObject,
		&FilterRegistration,
		&gProcDirFilter );
	if( !NT_SUCCESS(status) )
	{
		KdPrint(("!!! FltRegisterFilter failed: 0x%X\n", status));
		return status;
	}
	status = FltStartFiltering(gProcDirFilter);
	if( !NT_SUCCESS(status) )
	{
		KdPrint(("!!! FltStartFiltering failed: 0x%X\n", status));
		return status;
	}
	KeInitializeSpinLock(&gLockSkipProcess);

	return status;
}

// 关闭监控
VOID DDKMinFilemonUnload (__in PDRIVER_OBJECT DriverObject)
{
	if(NULL != gProcDirFilter)
	{
		FltUnregisterFilter(gProcDirFilter);
		gProcDirFilter = NULL;
		KdPrint(("[MinFilemon.sys] stop proc dir: %S", gProcDir));
	}
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

FLT_PREOP_CALLBACK_STATUS ScannerPreCreate (__inout PFLT_CALLBACK_DATA Data
											, __in PCFLT_RELATED_OBJECTS FltObjects
											, __deref_out_opt PVOID *CompletionContext)
{
	PFLT_FILE_NAME_INFORMATION nameInfo;
	NTSTATUS status;

	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( CompletionContext );

	PAGED_CODE();

	if (!NT_SUCCESS( Data->IoStatus.Status ) ||	(STATUS_REPARSE == Data->IoStatus.Status)) 
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	status = FltGetFileNameInformation( Data,
		FLT_FILE_NAME_NORMALIZED |
		FLT_FILE_NAME_QUERY_DEFAULT,
		&nameInfo );

	if (!NT_SUCCESS( status )) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	FltParseFileNameInformation( nameInfo );
	if( 1 == CheckPathIsProc(&nameInfo->Name
			, FlagOn(FILE_DIRECTORY_FILE, Data->Iopb->Parameters.Create.Options)) )
	{
		KdPrint(("!!! MinFileMon.sys [ScannerPreCreate] monitoer:%wZ(%d) \n"
			, &nameInfo->Name, Data->Iopb->Parameters.Create.Options));
		FltReleaseFileNameInformation( nameInfo );
		if((Data->Iopb->Parameters.Create.Options >> 24) != FILE_OPEN)
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

FLT_PREOP_CALLBACK_STATUS ScannerPreSetInformation(__inout PFLT_CALLBACK_DATA Data 
												   , __in PCFLT_RELATED_OBJECTS FltObjects
												   , __deref_out_opt PVOID *CompletionContext)
{
	PFLT_IO_PARAMETER_BLOCK			pIopb				= Data->Iopb;
	ULONG							IrpFlags			= Data->Iopb->IrpFlags;
	FILE_INFORMATION_CLASS			FileInformationClass;
	PFLT_FILE_NAME_INFORMATION		nameInfo;
	NTSTATUS						status;
	LONG							lCheck				= 0;

	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( CompletionContext );

// 	if ( FALSE != CheckIsSkip(IoThreadToProcess( Data->Thread )) ) 
// 	{
// 		KdPrint(( "!!! MinFileMon.sys -- allowing create for trusted process \n" ));
// 		return FLT_PREOP_SUCCESS_NO_CALLBACK;
// 	}

	if (!NT_SUCCESS( Data->IoStatus.Status ) ||	(STATUS_REPARSE == Data->IoStatus.Status)) 
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	status = FltGetFileNameInformation( Data,
		FLT_FILE_NAME_NORMALIZED |
		FLT_FILE_NAME_QUERY_DEFAULT,
		&nameInfo );

	if (!NT_SUCCESS( status )) {

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	FltParseFileNameInformation( nameInfo );
	lCheck = CheckPathIsProc(&nameInfo->Name, FALSE);
	if(0 != lCheck)
	{
		KdPrint(("!!! MinFileMon.sys [ScannerPreSetInformation] %wZ(%d).\n"
			, &nameInfo->Name, Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass));
	}
	FltReleaseFileNameInformation( nameInfo );
	if(-1 == lCheck)
	{
		//vista或win7返回的FileInformationClass结构不再是FileBothDirectoryInformation.
		//而是FileidBothDirectoryInformation
		if(FileBothDirectoryInformation == Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass
				|| FileIdBothDirectoryInformation == Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass)
		{
			return FLT_PREOP_SUCCESS_WITH_CALLBACK;
		}
	}
	else if(0 == lCheck)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// 如果是重命名就拒绝访问, 和拒绝枚举
	if(FileRenameInformation == Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass
		|| FileLinkInformation == Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass
		)
	{
		Data->IoStatus.Information = 0;
		Data->IoStatus.Status = STATUS_ACCESS_DENIED;
		return FLT_PREOP_COMPLETE;
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK; 
}

FLT_POSTOP_CALLBACK_STATUS ScannerPostCreate (__inout PFLT_CALLBACK_DATA Data
											  , __in PCFLT_RELATED_OBJECTS FltObjects
											  , __in_opt PVOID CompletionContext
											  , __in FLT_POST_OPERATION_FLAGS Flags)
{
	PSCANNER_STREAM_HANDLE_CONTEXT scannerContext;
	FLT_POSTOP_CALLBACK_STATUS returnStatus = FLT_POSTOP_FINISHED_PROCESSING;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	NTSTATUS status;
	BOOLEAN safeToOpen, scanFile;
	UNICODE_STRING			ustrLog		= RTL_CONSTANT_STRING( L"log" );

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
		KdPrint(("[ScannerPreCreate] CancelFile(0x%X):%wZ.\n"
			, FltObjects->Instance, &FltObjects->FileObject->FileName));

		Data->IoStatus.Status = STATUS_ACCESS_DENIED;
		Data->IoStatus.Information = 0;

		return FLT_POSTOP_FINISHED_PROCESSING;
	}
	if(FltObjects->FileObject->DeleteAccess)
	{
		FltCancelFileOpen( FltObjects->Instance, FltObjects->FileObject );
		Data->IoStatus.Status = STATUS_ACCESS_DENIED;
		Data->IoStatus.Information = 0;

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}

NTSTATUS ScannerUnload ( __in FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER( Flags );
	KdPrint(("[ScannerUnload] Enter.\n"));

	FltUnregisterFilter( gProcDirFilter );
	gProcDirFilter = NULL;

	return STATUS_SUCCESS;
}

NTSTATUS ScannerInstanceSetup (__in PCFLT_RELATED_OBJECTS FltObjects
							   , __in FLT_INSTANCE_SETUP_FLAGS Flags
							   , __in DEVICE_TYPE VolumeDeviceType
							   , __in FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( Flags );
	UNREFERENCED_PARAMETER( VolumeFilesystemType );

	PAGED_CODE();

	KdPrint(("[ScannerInstanceSetup] Enter.\n"));
	//
	//  Don't attach to network volumes.
	//
	if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {

		return STATUS_FLT_DO_NOT_ATTACH;
	}

	KdPrint(("[ScannerInstanceSetup] Attach volumn[0x%x].\n", FltObjects->Volume));
	return STATUS_SUCCESS;
}
NTSTATUS ScannerQueryTeardown (
							   __in PCFLT_RELATED_OBJECTS FltObjects,
							   __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
							   )
{
	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( Flags );

	DbgPrint("[ScannerQueryTeardown] Enter.\n");
	return STATUS_SUCCESS;
}
//////////////////////////////////////////////////////////////////////////
LONG		CheckPathIsProc(UNICODE_STRING* pusPath, LONG bIsDir)
{
	WCHAR*				pFile			= NULL;
	WCHAR				wcTmp			= 0;
	int					i				= 0;
	int					nLenProcDir		= wcslen(gProcDir);
	int					nLenPath		= pusPath->Length / sizeof(WCHAR);

	PAGED_CODE();

	if(0 == gProcDir[0] || NULL == pusPath 
		|| 0 == nLenPath || NULL == pusPath->Buffer)
	{
		KdPrint(("!!! MinFileMon.sys [CheckPathIsProc] can't chekc.\n"));
		return 0;
	}

	for(i = 0; i < nLenPath && i < nLenProcDir; i++)
	{
		if(pusPath->Buffer[i] == gProcDir[i])
			continue;
		wcTmp = gProcDir[i];
		if(wcTmp >= 'A' && wcTmp <= 'Z')
			wcTmp = wcTmp - 'A' + 'a';
		else if(wcTmp >= 'a' && wcTmp <= 'z')
			wcTmp = wcTmp - 'a' + 'A';
		else
		{
			return 0;
		}
		if(pusPath->Buffer[i] != wcTmp)
		{
			return 0;
		}
	}
	// 文件夹判断
	if( i == nLenPath && (i + 1) == nLenProcDir )
		return 1;

	if(i == nLenPath && i < nLenProcDir)
	{
		// 是否监控目录的父一级目录
		for(i++; i < (nLenProcDir-1); i++)
		{
			if(L'\\' == gProcDir[i])
				return 0;
		}
		return -1;
	}
	else if(i == nLenProcDir)
	{
		return 1;
	}
	return 0;
}

BOOLEAN GetHideDirectory(WCHAR* pDir, int nLen)
{
	WCHAR*		pP			= NULL;
	int			i			= 0;

	for(i = wcslen(gProcDir) - 2; i >= 0 && gProcDir[i]; i--)
	{
		if(L'\\' == gProcDir[i])
			break;
	}
	if(i < 0 || L'\\' != gProcDir[i])
		return FALSE;

	pP = &gProcDir[i];
	i = 0;
	for(pP++; *pP && L'\\' != *pP && i < (nLen-1); pP++, i++)
	{
		pDir[i] = *pP;
	}
	pDir[i] = 0;
	return TRUE;
}
// 列举完文件操作
FLT_POSTOP_CALLBACK_STATUS ScannerPostSetInformation(__inout PFLT_CALLBACK_DATA Data 
													 , __in PCFLT_RELATED_OBJECTS FltObjects
													 , __in_opt PVOID CompletionContext 
													 , __in FLT_POST_OPERATION_FLAGS Flags)
{
	ULONG								nextOffset					= 0;
	int									modified					= 0;
	int									removedAllEntries			= 1;
	WCHAR								szHideDir[128]				= {0};
	PVOID								SafeBuffer;
	UNICODE_STRING						fileName;
	PFILE_ID_BOTH_DIR_INFORMATION		vista_currentFileInfo		= 0;
	PFILE_ID_BOTH_DIR_INFORMATION		vista_nextFileInfo			= 0;
	PFILE_ID_BOTH_DIR_INFORMATION		vista_previousFileInfo		= 0;
	PFILE_BOTH_DIR_INFORMATION			xp_currentFileInfo			= 0;
	PFILE_BOTH_DIR_INFORMATION			xp_nextFileInfo				= 0;
	PFILE_BOTH_DIR_INFORMATION			xp_previousFileInfo			= 0;
	BOOLEAN								bUseXP						= TRUE;
	WCHAR								szFileName[128]				= {0};

	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( CompletionContext );

	KdPrint(("!!! MinFileMon.sys [ScannerPostSetInformation] %wZ.\n", FltObjects->FileObject->FileName));

	if( FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING ) 
		|| FALSE == GetHideDirectory(szHideDir, arrayof(szHideDir)))
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	//vista或win7返回的FileInformationClass结构不再是FileBothDirectoryInformation.
	//而是FileidBothDirectoryInformation
	if(FileIdBothDirectoryInformation == Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass)
		bUseXP = FALSE;

	if( Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY 
		|| Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length <= 0 
		|| !NT_SUCCESS(Data->IoStatus.Status))
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}
	// 查找枚举数据
	if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress != NULL)
	{
		SafeBuffer = MmGetSystemAddressForMdlSafe( 
			Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
			NormalPagePriority );            
	}
	else
	{
		SafeBuffer = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;             
	}     
	if(NULL == SafeBuffer)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	vista_previousFileInfo = vista_currentFileInfo = (PFILE_ID_BOTH_DIR_INFORMATION)SafeBuffer;
	xp_previousFileInfo = xp_currentFileInfo = (PFILE_BOTH_DIR_INFORMATION)SafeBuffer;

	do
	{
		//Byte offset of the next FILE_BOTH_DIR_INFORMATION entry
		if(bUseXP)
		{
			// XP以下系统分析方法
			nextOffset = xp_currentFileInfo->NextEntryOffset;
			xp_nextFileInfo = (PFILE_BOTH_DIR_INFORMATION)((PCHAR)(xp_currentFileInfo) + nextOffset);
			memset(szFileName, 0, sizeof(szFileName));
			memcpy(szFileName, xp_currentFileInfo->FileName, min(sizeof(szFileName)-2, xp_currentFileInfo->FileNameLength));
			KdPrint(("!!! MinFileMon.sys enum dir: %S, %S\n", szFileName, szHideDir));
			if(_wcsicmp(szFileName, szHideDir)==0)
			{                
				if( nextOffset == 0 )
					xp_previousFileInfo->NextEntryOffset = 0;
				else
					xp_previousFileInfo->NextEntryOffset = (ULONG)((PCHAR)xp_currentFileInfo - (PCHAR)xp_previousFileInfo) + nextOffset;

				modified = 1;                
			}
			else
			{
				removedAllEntries = 0;
				//前驱结点指针后移 
				xp_previousFileInfo = xp_currentFileInfo;                
			}
			//当前指针后移 
			xp_currentFileInfo = xp_nextFileInfo;
		}
		else
		{
			// Vista及以上系统分析方法
			nextOffset = vista_currentFileInfo->NextEntryOffset;
			vista_nextFileInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((PCHAR)(vista_currentFileInfo) + nextOffset);
			memset(szFileName, 0, sizeof(szFileName));
			memcpy(szFileName, vista_previousFileInfo->FileName, min(sizeof(szFileName)-2, vista_previousFileInfo->FileNameLength));
			KdPrint(("!!! MinFileMon.sys enum dir: %S, %S\n", szFileName, szHideDir));
			if(_wcsicmp(szFileName, szHideDir)==0)
			{                
				if( nextOffset == 0 )
					vista_previousFileInfo->NextEntryOffset = 0;
				else
					vista_previousFileInfo->NextEntryOffset = (ULONG)((PCHAR)vista_currentFileInfo - (PCHAR)vista_previousFileInfo) + nextOffset;

				modified = 1;                
			}
			else
			{
				removedAllEntries = 0;
				//前驱结点指针后移 
				vista_previousFileInfo = vista_currentFileInfo;                
			}
			//当前指针后移 
			vista_currentFileInfo = vista_nextFileInfo;
		}
	} while( nextOffset != 0 );

	// 与入修改标记
	if( modified )
	{
		if( removedAllEntries )
		{
			Data->IoStatus.Status = STATUS_NO_MORE_FILES;
		}
		else
		{
			FltSetCallbackDataDirty( Data );
		}
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}
