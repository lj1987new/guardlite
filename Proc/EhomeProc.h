






NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp);
void DriverUnload(PDRIVER_OBJECT pDriverObj);
NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDevObj, PIRP pIrp);
VOID ProcessCallback(IN HANDLE  hParentId, IN HANDLE  hProcessId, IN BOOLEAN bCreate);

#define DEVICE_NAME		L"\\Device\\devNTProcDrv"
#define LINK_NAME		L"\\DosDevices\\slNTProcDrv"


typedef int BOOL;

typedef struct _DEVICE_EXTENSION 
{
    PKEVENT						ProcessEvent;
	LIST_ENTRY					ListHeader;
	NPAGED_LOOKASIDE_LIST		ListLookAside;
	KSPIN_LOCK					ListLock;
//     ULONGLONG		hPParentId;	
//     ULONGLONG		hPProcessId;
//     BOOLEAN			bPCreate;
// 	char proname[NAMELENGTH];
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
