

#include <ntddk.h>
#include <tdi.h>
#include <TdiKrnl.h>

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

typedef struct _ASSOCIATE_ADDRESS
{
	PFILE_OBJECT		fileObj;
	ULONG				IPAdd;
	USHORT				Port;
}ASSOCIATE_ADDRESS, *PASSOCIATE_ADDRESS;

typedef struct _EHOME_LIST
{
	LIST_ENTRY			plist;
	ASSOCIATE_ADDRESS	AddFileObj;
}EHOME_LIST, *PEHOME_LIST;


NTSTATUS	Ehomedisp(PDEVICE_OBJECT pDevObj,PIRP irp);
NTSTATUS	EhomeCreate(PDEVICE_OBJECT pDevObj,PIRP irp);
NTSTATUS	EhomeDevCtl(PDEVICE_OBJECT pDevObj,PIRP irp);
NTSTATUS	EhomeCloseCleanup(PDEVICE_OBJECT pDevObj,PIRP irp);
NTSTATUS	EhomeInternalDevCtl(PDEVICE_OBJECT pDevObj,PIRP irp);

USHORT my_ntohs(USHORT uPort);
ULONG my_ntohl(ULONG ip);
NTSTATUS
  EhomeConnectComRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  irp,
    IN PVOID  Context
    );
VOID TdiDisConnect(PIRP irp,PIO_STACK_LOCATION stack);
BOOL EhomeTDISend(PIRP irp,PIO_STACK_LOCATION stack);
BOOL IsHttpRequest(PFILE_OBJECT fileObj, PASSOCIATE_ADDRESS* pAddress);
BOOL IsSkipDisnetwork(char* pProcName);		// 是否跳过的进程

// 御载功能
void		EhomeUnload(PDRIVER_OBJECT pDriverObject);
void		EhomeClear();
NTSTATUS	DispatchRoutineComplate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
// 断网功能
NTSTATUS	CheckNetwork(PIO_STACK_LOCATION pStack, PDEVICE_EXTENTION pDevExt, char* pProcName);	
NTSTATUS	CheckUrl(char* pHttpPacket, int nHttpLen, PASSOCIATE_ADDRESS pAddress);