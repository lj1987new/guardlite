#pragma once

#include <ntddk.h>
#include "../common/GuardLiteCtrl.h"

#define arrayof(p)		( sizeof(p) / sizeof((p)[0]) )

#define PAGEDCODE			code_seg("PAGE")
#define LOCKEDCODE			code_seg()
#define INITCODE			code_seg("INIT")

#define PAGEDDATA			data_seg("PAGE")
#define LOCKEDDATA			data_seg()
#define INITDATA			data_seg("INIT")

#define GUARDLITE_DEVICE		L"\\Device\\DDKGuardLite"
#define GUARDLITE_LINK			L"\\??\\GuardLite"

#define PAGE_DEBUG			'etiL'


// 设备类型
typedef enum _GuardLiteDeviceType
{
	GLDT_Main = 0x0
	, GLDT_File
}GuardLiteDeviceType;

// IRP队列
typedef struct _IRP_LIST{
	PIRP				pIrp;
	LIST_ENTRY			list;
}IRP_LIST, *PIRP_LIST;

// 检测到一个包的数据申名
typedef struct _GuardLiteQuery{
	GUARDLITEPACK			Pack;
	KEVENT					Event;
	ULONG					Timeout;
	ULONG					ulID;
}GUARDLITEQUERY, *PGUARDLITEQUERY;

// PACK队列
typedef struct _PACK_LIST{
	GUARDLITEQUERY		query;
	LIST_ENTRY			list;
}PACK_LIST, *PPACK_LIST;

// Pack的Lookaside结构
typedef struct _PACK_QUEUE{
	LIST_ENTRY					list;
	PNPAGED_LOOKASIDE_LIST		lookaside;
} PACK_QUEUE;
// IRP队列
typedef struct _IRP_QUEUE{
	IRP_LIST					list;
	PNPAGED_LOOKASIDE_LIST		lookaside;	
} IRP_QUEUE;

// 设备扩展定义
typedef struct _DEVICE_EXTENSION
{
	// 设备类型
	GuardLiteDeviceType			DeviceType;
	// 连接名
	UNICODE_STRING				LinkName;
	// 开启监控的Mask
	ULONG						StartMask;

}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//////////////////////////////////////////////////////////////////////////
// 全局变量
extern PACK_QUEUE			gPackQueue;
extern LIST_ENTRY			gPackWaitList;
extern IRP_QUEUE			gIrpQueue;
extern KMUTEX				gIrpPackMutex;
extern ULONG				gPackWaitID;
//////////////////////////////////////////////////////////////////////////
// 主驱动模块函数
NTSTATUS	DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		DriverOnload();
NTSTATUS	DriverDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS	DriverDeviceControlRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS	DriverCloseRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS	DriverCreateRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp);

//////////////////////////////////////////////////////////////////////////
// 文件监控模块函数
NTSTATUS	FilemonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		FilemonUnload();
NTSTATUS	FilemonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);


//////////////////////////////////////////////////////////////////////////
// 注册表监控模块函数
NTSTATUS	RegmonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		RegmonUnload();
NTSTATUS	RegmonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);


//////////////////////////////////////////////////////////////////////////
// 服务监控模块
NTSTATUS	ServicesEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		ServicesUnload();
NTSTATUS	ServicesDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);


//////////////////////////////////////////////////////////////////////////
// 进程监控
NTSTATUS	ProcmonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		ProcmonUnload();
NTSTATUS	ProcmonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);


//////////////////////////////////////////////////////////////////////////
// 公共函数
NTSTATUS		AddIrpToQueue(PIRP pIrp);
NTSTATUS		AddPackToQueue(PGUARDLITEQUERY pQuery);
NTSTATUS		DealIrpAndPackQueue();
