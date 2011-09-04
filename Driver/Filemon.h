#pragma once

//////////////////////////////////////////////////////////////////////////
// 文件监控模块函数
NTSTATUS	FilemonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		FilemonUnload();
NTSTATUS	FilemonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);

//////////////////////////////////////////////////////////////////////////
FLT_PREOP_CALLBACK_STATUS FilemonPreCreate (__inout PFLT_CALLBACK_DATA Data
											, __in PCFLT_RELATED_OBJECTS FltObjects
											, __deref_out_opt PVOID *CompletionContext);

FLT_POSTOP_CALLBACK_STATUS FilemonPostCreate (__inout PFLT_CALLBACK_DATA Data
											  , __in PCFLT_RELATED_OBJECTS FltObjects
											  , __in_opt PVOID CompletionContext
											  , __in FLT_POST_OPERATION_FLAGS Flags);

NTSTATUS FltUnload (__in FLT_FILTER_UNLOAD_FLAGS Flags);

NTSTATUS FltQueryTeardown (__in PCFLT_RELATED_OBJECTS FltObjects
						   , __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);

NTSTATUS FltInstanceSetup (__in PCFLT_RELATED_OBJECTS FltObjects
						   , __in FLT_INSTANCE_SETUP_FLAGS Flags
						   , __in DEVICE_TYPE VolumeDeviceType
						   , __in FLT_FILESYSTEM_TYPE VolumeFilesystemType);


BOOLEAN			IsFilemonGuardPath(PWSTR pPath, BOOLEAN isDir, LONG* pSubType);

//////////////////////////////////////////////////////////////////////////
typedef struct _FLT_FILEMON_DATA {
	PDRIVER_OBJECT		DriverObject;
	PFLT_FILTER			Filter;
	// 模块所在目录
	UNICODE_STRING		usModulePath;
	UNICODE_STRING		usVolumePath;
	// 模块所在组
	PFLT_VOLUME			fltVolume;
	HANDLE				hSysFile;

} FLT_FILEMON_DATA, *PFLT_FILEMON_DATA;

typedef struct _SCANNER_STREAM_HANDLE_CONTEXT {

	BOOLEAN RescanRequired;

} SCANNER_STREAM_HANDLE_CONTEXT, *PSCANNER_STREAM_HANDLE_CONTEXT;
