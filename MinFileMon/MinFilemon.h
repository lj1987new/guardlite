#pragma once

#include <fltKernel.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <ntddk.h>
#ifdef __cplusplus
};
#endif
// 界面交互
#include "MinFilemonCtrl.h"

#define SFLT_POOL_TAG 'tlFS' 

#define PAGEDCODE			code_seg("PAGE")
#define LOCKEDCODE			code_seg()
#define INITCODE			code_seg("INIT")

#define PAGEDDATA			data_seg("PAGE")
#define LOCKEDDATA			data_seg()
#define INITDATA			data_seg("INIT")

#define arrayof(p)		( sizeof(p) / sizeof((p)[0]) )

typedef struct _DEVICE_EXTENSION{
	PDEVICE_OBJECT			pDevice;
	PDEVICE_OBJECT			AttachedToDeviceObject;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
VOID DDKMinFilemonUnload (__in PDRIVER_OBJECT DriverObject);

NTSTATUS QuerySymbolicLink(PUNICODE_STRING SymbolicLinkName,  OUT PUNICODE_STRING LinkTarget);
//////////////////////////////////////////////////////////////////////////
NTSTATUS ScannerInstanceSetup (__in PCFLT_RELATED_OBJECTS FltObjects
							   , __in FLT_INSTANCE_SETUP_FLAGS Flags, __in DEVICE_TYPE VolumeDeviceType
							   , __in FLT_FILESYSTEM_TYPE VolumeFilesystemType);
NTSTATUS ScannerUnload ( __in FLT_FILTER_UNLOAD_FLAGS Flags);

FLT_PREOP_CALLBACK_STATUS 
ScannerPreCreate (__inout PFLT_CALLBACK_DATA Data
				  , __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID *CompletionContext);

FLT_PREOP_CALLBACK_STATUS 
ScannerPreSetInformation (__inout PFLT_CALLBACK_DATA Data
						  , __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID *CompletionContext);

NTSTATUS ScannerQueryTeardown (__in PCFLT_RELATED_OBJECTS FltObjects,
							   __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);

FLT_POSTOP_CALLBACK_STATUS ScannerPostCreate (__inout PFLT_CALLBACK_DATA Data
											  , __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext
											  , __in FLT_POST_OPERATION_FLAGS Flags);
FLT_POSTOP_CALLBACK_STATUS ScannerPostSetInformation (__inout PFLT_CALLBACK_DATA Data
													  , __in PCFLT_RELATED_OBJECTS FltObjects
													  , __in_opt PVOID CompletionContext
													  , __in FLT_POST_OPERATION_FLAGS Flags);
// 检测目录是否我们的目录
LONG		CheckPathIsProc(UNICODE_STRING* pusPath, LONG bDir); // 返回 0 表示与监控目录无关, 1 表是监控目录, -1 表示监控目录的子目录
BOOLEAN		CheckIsSkip(PEPROCESS process);
VOID		AddToSkipProcess(PEPROCESS process, BOOLEAN bAdd);

typedef struct _SCANNER_STREAM_HANDLE_CONTEXT {

	BOOLEAN RescanRequired;

} SCANNER_STREAM_HANDLE_CONTEXT, *PSCANNER_STREAM_HANDLE_CONTEXT;
