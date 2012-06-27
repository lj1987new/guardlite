#pragma once

#include <ntddk.h>
#include "EhomeDevCtl.h"
#include "tdi.h"
#include "TdiKrnl.h"
#include "TdiFileObjectContext.h"

#define NO_UNLOAD

typedef int BOOL;
#define UDPDEVNAME			L"\\Device\\Udp"
#define EHOMETCPDEVNAME		L"\\Device\\EHomeNetFltDev"
#define EHOMELINKNAME		L"\\??\\EHomeNetDev"
#define TCPDEVNAME			L"\\Device\\Tcp"
#define UDPDEVNAME			L"\\Device\\Udp"
#define HOSTNAMEOFFSET 13
/*#define NAMELENGTH     50*/
#define MAXEHOMELIST   300

/* 设备类型 */
typedef enum _DeviceType{
	DT_EHOME = 0
	, DT_FILTER_TCP
	, DT_FILTER_UDP
}DEVICETYPE;

typedef struct _DEVICE_EXTENTION
{
	PDEVICE_OBJECT			LowTcpDev;
	UNICODE_STRING			linkName;
	DEVICETYPE				DeviceType;	
}DEVICE_EXTENTION, *PDEVICE_EXTENTION;

typedef struct _EHOME_KEYWORK_LIST{
	LIST_ENTRY			list;
}EHOME_KEYWORK_LIST, *PEHOME_KEYWORK_LIST;

typedef struct _EHOME_FILTER_RULE{
	int				rule;		/* 0: 停止过滤, >0: 发现关键字替换, <0: 发现关键字断开*/
}EHOME_FILTER_RULE, *PEHOME_FILTER_RULE;

typedef struct _tdi_client_irp_ctx {
	PIO_COMPLETION_ROUTINE	completion;
	PVOID					context;
	UCHAR					old_control;
	PFILE_OBJECT            addrobj;
}tdi_client_irp_ctx;

typedef struct _filter_keyword_block{
	LIST_ENTRY				list;
	FILTERKEYWORDBLOCK		fkl;
} FILTER_KEYWORD_BLOCK, *PFILTER_KEYWORD_BLOCK;

typedef struct _ehome_filter_keyword{
	LIST_ENTRY				headlist;
	NPAGED_LOOKASIDE_LIST	lookaside;
	KSPIN_LOCK				spinlock;
	PKEVENT					noticeevent;
} EHOME_FILTER_KEYWORD;

NTSTATUS	Ehomedisp(PDEVICE_OBJECT pDevObj,PIRP irp);
NTSTATUS	EhomeCreate(PDEVICE_OBJECT pDevObj,PIRP irp);
NTSTATUS	EhomeDevCtl(PDEVICE_OBJECT pDevObj,PIRP irp);
NTSTATUS	EhomeCloseCleanup(PDEVICE_OBJECT pDevObj,PIRP irp);
NTSTATUS	EhomeInternalDevCtl(PDEVICE_OBJECT pDevObj,PIRP irp);
void		EHomeTcpOpen(PIRP pIrp, PIO_STACK_LOCATION pIrps);

USHORT my_ntohs(USHORT uPort);
ULONG my_ntohl(ULONG ip);
NTSTATUS
  EhomeConnectComRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  irp,
    IN PVOID  Context
    );
// VOID TdiDisConnect(PIRP irp,PIO_STACK_LOCATION stack);
BOOL EhomeTDISend(PIRP irp,PIO_STACK_LOCATION stack);
BOOL IsSkipDisnetwork(char* pProcName);		// 是否跳过的进程
// 驱动功能
typedef NTSTATUS (*FnEHomeControl)(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps, IN PVOID pInBuff, IN ULONG nInSize, OUT PVOID pOutBuff, IN OUT ULONG *pOutSize);
typedef struct _EHOME_CONTROL_CALLBACK{
	ULONG				code;
	FnEHomeControl		fn;
}EHOME_CONTROL_CALLBACK;
// 设置事件句柄
NTSTATUS		EHomeTDISetEventHandler(PIRP pIrp, PIO_STACK_LOCATION pStack);
// 御载功能
void		EhomeUnload(PDRIVER_OBJECT pDriverObject);
void		EhomeClear();
NTSTATUS	DispatchRoutineComplate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
// 断网功能
NTSTATUS	CheckNetwork(PIO_STACK_LOCATION pStack, PDEVICE_EXTENTION pDevExt, char* pProcName);	
NTSTATUS	CheckUrl(char* pHttpPacket, int nHttpLen, tdi_foc_connection_ptr pAddress, BOOLEAN* pIsHttp);
void		HttpRequestEraseFlag(char* pHttpRequest, int nHttpLen);

// 处理关键字过滤
NTSTATUS tdi_client_irp_complete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);

extern EHOME_FILTER_RULE		gEHomeFilterRule;
extern EHOME_FILTER_KEYWORD		gEHomeKeyword;
extern EHOME_CONTROL_CALLBACK	gEHomeControlCallback[];

