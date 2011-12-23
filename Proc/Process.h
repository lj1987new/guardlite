/*
 *	获取内核态的EXPLORER结构的
 */
#ifndef _PROCESS_HANDLE_
#define _PROCESS_HANDLE_

typedef struct _EPROCESS_OFFSET{
	ULONG32 Pcb              				;//: _KPROCESS
	ULONG32 ExitStatus       				;//: Int4B
	ULONG32 LockEvent        				;//: _KEVENT
	ULONG32 LockCount        				;//: Uint4B
	ULONG32 CreateTime       				;//: _LARGE_INTEGER
	ULONG32 ExitTime         				;//: _LARGE_INTEGER
	ULONG32 LockOwner        				;//: Ptr32 _KTHREAD
	ULONG32 UniqueProcessId  				;//: Ptr32 Void
	ULONG32 ActiveProcessLinks 				;//: _LIST_ENTRY
	ULONG32 QuotaPeakPoolUsage 				;//: [2] Uint4B
	ULONG32 QuotaPoolUsage   				;//: [2] Uint4B
	ULONG32 PagefileUsage    				;//: Uint4B
	ULONG32 CommitCharge     				;//: Uint4B
	ULONG32 PeakPagefileUsage 				;//: Uint4B
	ULONG32 PeakVirtualSize  				;//: Uint4B
	ULONG32 VirtualSize      				;//: Uint4B
	ULONG32 Vm               				;//: _MMSUPPORT
	ULONG32 SessionProcessLinks 			;//: _LIST_ENTRY
	ULONG32 DebugPort        				;//: Ptr32 Void
	ULONG32 ExceptionPort    				;//: Ptr32 Void
	ULONG32 ObjectTable      				;//: Ptr32 _HANDLE_TABLE
	ULONG32 Token            				;//: Ptr32 Void
	ULONG32 WorkingSetLock   				;//: _FAST_MUTEX
	ULONG32 WorkingSetPage   				;//: Uint4B
	ULONG32 ProcessOutswapEnabled 			;//: UChar
	ULONG32 ProcessOutswapped 				;//: UChar
	ULONG32 AddressSpaceInitialized 		;//: UChar
	ULONG32 AddressSpaceDeleted 			;//: UChar
	ULONG32 AddressCreationLock 			;//: _FAST_MUTEX
	ULONG32 HyperSpaceLock   				;//: Uint4B
	ULONG32 ForkInProgress   				;//: Ptr32 _ETHREAD
	ULONG32 VmOperation      				;//: Uint2B
	ULONG32 ForkWasSuccessful 				;//: UChar
	ULONG32 MmAgressiveWsTrimMask 			;//: UChar
	ULONG32 VmOperationEvent 				;//: Ptr32 _KEVENT
	ULONG32 PaeTop           				;//: Ptr32 Void
	ULONG32 LastFaultCount   				;//: Uint4B
	ULONG32 ModifiedPageCount				;// : Uint4B
	ULONG32 VadRoot          				;//: Ptr32 Void
	ULONG32 VadHint          				;//: Ptr32 Void
	ULONG32 CloneRoot        				;//: Ptr32 Void
	ULONG32 NumberOfPrivatePages			;// : Uint4B
	ULONG32 NumberOfLockedPages 			;//: Uint4B
	ULONG32 NextPageColor    				;//: Uint2B
	ULONG32 ExitProcessCalled				;// : UChar
	ULONG32 CreateProcessReported 			;//: UChar
	ULONG32 SectionHandle    				;//: Ptr32 Void
	ULONG32 Peb              				;//: Ptr32 _PEB
	ULONG32 SectionBaseAddress 				;//: Ptr32 Void
	ULONG32 QuotaBlock       				;//: Ptr32 _EPROCESS_QUOTA_BLOCK
	ULONG32 LastThreadExitStatus 			;//: Int4B
	ULONG32 WorkingSetWatch  				;//: Ptr32 _PAGEFAULT_HISTORY
	ULONG32 Win32WindowStation 				;//: Ptr32 Void
	ULONG32 InheritedFromUniqueProcessId 	;//: Ptr32 Void
	ULONG32 GrantedAccess    				;//: Uint4B
	ULONG32 DefaultHardErrorProcessing 		;//: Uint4B
	ULONG32 LdtInformation   				;//: Ptr32 Void
	ULONG32 VadFreeHint      				;//: Ptr32 Void
	ULONG32 VdmObjects       				;//: Ptr32 Void
	ULONG32 DeviceMap        				;//: Ptr32 Void
	ULONG32 SessionId        				;//: Uint4B
	ULONG32 PhysicalVadList  				;//: _LIST_ENTRY
	ULONG32 PageDirectoryPte 				;//: _HARDWARE_PTE_X86
	ULONG32 Filler           				;//: Uint8B
	ULONG32 PaePageDirectoryPage 			;//: Uint4B
	ULONG32 ImageFileName    				;//: [16] UChar
	ULONG32 VmTrimFaultValue 				;//: Uint4B
	ULONG32 SetTimerResolution 				;//: UChar
	ULONG32 PriorityClass    				;//: UChar
	ULONG32 SubSystemMinorVersion 			;//: UChar
	ULONG32 SubSystemMajorVersion 			;//: UChar
	ULONG32 SubSystemVersion 				;//: Uint2B
	ULONG32 Win32Process     				;//: Ptr32 Void
	ULONG32 Job              				;//: Ptr32 _EJOB
	ULONG32 JobStatus        				;//: Uint4B
	ULONG32 JobLinks         				;//: _LIST_ENTRY
	ULONG32 LockedPagesList  				;//: Ptr32 Void
	ULONG32 SecurityPort     				;//: Ptr32 Void
	ULONG32 Wow64Process     				;//: Ptr32 _WOW64_PROCESS
	ULONG32 ReadOperationCount 				;//: _LARGE_INTEGER
	ULONG32 WriteOperationCount				;// : _LARGE_INTEGER
	ULONG32 OtherOperationCount				;// : _LARGE_INTEGER
	ULONG32 ReadTransferCount 				;//: _LARGE_INTEGER
	ULONG32 WriteTransferCount 				;//: _LARGE_INTEGER
	ULONG32 OtherTransferCount 				;//: _LARGE_INTEGER
	ULONG32 CommitChargeLimit 				;//: Uint4B
	ULONG32 CommitChargePeak 				;//: Uint4B
	ULONG32 ThreadListHead   				;//: _LIST_ENTRY
	ULONG32 VadPhysicalPagesBitMap 			;//: Ptr32 _RTL_BITMAP
	ULONG32 VadPhysicalPages 				;//: Uint4B
	ULONG32 AweLock          				;//: Uint4B
	ULONG32 pImageFileName   				;//: Ptr32 _UNICODE_STRING
	ULONG32 Session          				;//: Ptr32 Void
	ULONG32 Flags            				;//: Uint4B
}EPROCESS_OFFSET, *PEPROCESS_OFFSET;

// 获取EPROCESS指针的索引
PEPROCESS_OFFSET		GetEProcessOffset();
// 获取PEB
BOOLEAN EPROCESS_PPEB(PEPROCESS pEproc, PPEB* pPeb);

#endif //#ifndef _PROCESS_HANDLE_