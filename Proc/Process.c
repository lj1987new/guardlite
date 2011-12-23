/*
 *	获取内核态的EXPLORER结构的
 */
#include <ntddk.h>
#include "Process.h"

// 获系统编译版本
ULONG	GetBuildNumber()
{
	static ULONG	ulBuild		= 0;

	if(0 == ulBuild)
	{
		PsGetVersion(NULL, NULL, &ulBuild, NULL);
	}
	return ulBuild;
}
// 获取Windows 2000的offset指针
PEPROCESS_OFFSET		GetEProcessOffset_2195(PEPROCESS_OFFSET pOffset)
{
	if(NULL == pOffset)
		return NULL;
	pOffset->Pcb              				      = +0x000 ;//Pcb              : _KPROCESS
	pOffset->ExitStatus       				      = +0x06c ;//ExitStatus       : Int4B
	pOffset->LockEvent        				      = +0x070 ;//LockEvent        : _KEVENT
	pOffset->LockCount        				      = +0x080 ;//LockCount        : Uint4B
	pOffset->CreateTime       				      = +0x088 ;//CreateTime       : _LARGE_INTEGER
	pOffset->ExitTime         				      = +0x090 ;//ExitTime         : _LARGE_INTEGER
	pOffset->LockOwner        				      = +0x098 ;//LockOwner        : Ptr32 _KTHREAD
	pOffset->UniqueProcessId  				      = +0x09c ;//UniqueProcessId  : Ptr32 Void
	pOffset->ActiveProcessLinks 			      = +0x0a0 ;//ActiveProcessLinks : _LIST_ENTRY
	pOffset->QuotaPeakPoolUsage 			      = +0x0a8 ;//QuotaPeakPoolUsage : [2] Uint4B
	pOffset->QuotaPoolUsage   				      = +0x0b0 ;//QuotaPoolUsage   : [2] Uint4B
	pOffset->PagefileUsage    				      = +0x0b8 ;//PagefileUsage    : Uint4B
	pOffset->CommitCharge     				      = +0x0bc ;//CommitCharge     : Uint4B
	pOffset->PeakPagefileUsage 			        = +0x0c0 ;//PeakPagefileUsage : Uint4B
	pOffset->PeakVirtualSize  				      = +0x0c4 ;//PeakVirtualSize  : Uint4B
	pOffset->VirtualSize      				      = +0x0c8 ;//VirtualSize      : Uint4B
	pOffset->Vm               				      = +0x0d0 ;//Vm               : _MMSUPPORT
	pOffset->SessionProcessLinks 		        = +0x118 ;//SessionProcessLinks : _LIST_ENTRY
	pOffset->DebugPort        				      = +0x120 ;//DebugPort        : Ptr32 Void
	pOffset->ExceptionPort    				      = +0x124 ;//ExceptionPort    : Ptr32 Void
	pOffset->ObjectTable      				      = +0x128 ;//ObjectTable      : Ptr32 _HANDLE_TABLE
	pOffset->Token            				      = +0x12c ;//Token            : Ptr32 Void
	pOffset->WorkingSetLock   				      = +0x130 ;//WorkingSetLock   : _FAST_MUTEX
	pOffset->WorkingSetPage   				      = +0x150 ;//WorkingSetPage   : Uint4B
	pOffset->ProcessOutswapEnabled 	        = +0x154 ;//ProcessOutswapEnabled : UChar
	pOffset->ProcessOutswapped 			        = +0x155 ;//ProcessOutswapped : UChar
	pOffset->AddressSpaceInitialized        = +0x156 ;//AddressSpaceInitialized : UChar
	pOffset->AddressSpaceDeleted 		        = +0x157 ;//AddressSpaceDeleted : UChar
	pOffset->AddressCreationLock 		        = +0x158 ;//AddressCreationLock : _FAST_MUTEX
	pOffset->HyperSpaceLock   				      = +0x178 ;//HyperSpaceLock   : Uint4B
	pOffset->ForkInProgress   				      = +0x17c ;//ForkInProgress   : Ptr32 _ETHREAD
	pOffset->VmOperation      				      = +0x180 ;//VmOperation      : Uint2B
	pOffset->ForkWasSuccessful 			        = +0x182 ;//ForkWasSuccessful : UChar
	pOffset->MmAgressiveWsTrimMask 	        = +0x183 ;//MmAgressiveWsTrimMask : UChar
	pOffset->VmOperationEvent 				      = +0x184 ;//VmOperationEvent : Ptr32 _KEVENT
	pOffset->PaeTop           				      = +0x188 ;//PaeTop           : Ptr32 Void
	pOffset->LastFaultCount   				      = +0x18c ;//LastFaultCount   : Uint4B
	pOffset->ModifiedPageCount				      = +0x190 ;//ModifiedPageCount : Uint4B
	pOffset->VadRoot          				      = +0x194 ;//VadRoot          : Ptr32 Void
	pOffset->VadHint          				      = +0x198 ;//VadHint          : Ptr32 Void
	pOffset->CloneRoot        				      = +0x19c ;//CloneRoot        : Ptr32 Void
	pOffset->NumberOfPrivatePages		        = +0x1a0 ;//NumberOfPrivatePages : Uint4B
	pOffset->NumberOfLockedPages 		        = +0x1a4 ;//NumberOfLockedPages : Uint4B
	pOffset->NextPageColor    				      = +0x1a8 ;//NextPageColor    : Uint2B
	pOffset->ExitProcessCalled				      = +0x1aa ;//ExitProcessCalled : UChar
	pOffset->CreateProcessReported 	        = +0x1ab ;//CreateProcessReported : UChar
	pOffset->SectionHandle    				      = +0x1ac ;//SectionHandle    : Ptr32 Void
	pOffset->Peb              				      = +0x1b0 ;//Peb              : Ptr32 _PEB
	pOffset->SectionBaseAddress 			      = +0x1b4 ;//SectionBaseAddress : Ptr32 Void
	pOffset->QuotaBlock       				      = +0x1b8 ;//QuotaBlock       : Ptr32 _EPROCESS_QUOTA_BLOCK
	pOffset->LastThreadExitStatus 		      = +0x1bc ;//LastThreadExitStatus : Int4B
	pOffset->WorkingSetWatch  				      = +0x1c0 ;//WorkingSetWatch  : Ptr32 _PAGEFAULT_HISTORY
	pOffset->Win32WindowStation 				    = +0x1c4 ;//Win32WindowStation : Ptr32 Void
	pOffset->InheritedFromUniqueProcessId 	= +0x1c8 ;//InheritedFromUniqueProcessId : Ptr32 Void
	pOffset->GrantedAccess    				      = +0x1cc ;//GrantedAccess    : Uint4B
	pOffset->DefaultHardErrorProcessing 	  = +0x1d0 ;//DefaultHardErrorProcessing : Uint4B
	pOffset->LdtInformation   				      = +0x1d4 ;//LdtInformation   : Ptr32 Void
	pOffset->VadFreeHint      				      = +0x1d8 ;//VadFreeHint      : Ptr32 Void
	pOffset->VdmObjects       				      = +0x1dc ;//VdmObjects       : Ptr32 Void
	pOffset->DeviceMap        				      = +0x1e0 ;//DeviceMap        : Ptr32 Void
	pOffset->SessionId        				      = +0x1e4 ;//SessionId        : Uint4B
	pOffset->PhysicalVadList  				      = +0x1e8 ;//PhysicalVadList  : _LIST_ENTRY
	pOffset->PageDirectoryPte 				      = +0x1f0 ;//PageDirectoryPte : _HARDWARE_PTE_X86
	pOffset->Filler           				      = +0x1f0 ;//Filler           : Uint8B
	pOffset->PaePageDirectoryPage 		      = +0x1f8 ;//PaePageDirectoryPage : Uint4B
	pOffset->ImageFileName    				      = +0x1fc ;//ImageFileName    : [16] UChar
	pOffset->VmTrimFaultValue 				      = +0x20c ;//VmTrimFaultValue : Uint4B
	pOffset->SetTimerResolution 			      = +0x210 ;//SetTimerResolution : UChar
	pOffset->PriorityClass    				      = +0x211 ;//PriorityClass    : UChar
	pOffset->SubSystemMinorVersion 	        = +0x212 ;//SubSystemMinorVersion : UChar
	pOffset->SubSystemMajorVersion 	        = +0x213 ;//SubSystemMajorVersion : UChar
	pOffset->SubSystemVersion 				      = +0x212 ;//SubSystemVersion : Uint2B
	pOffset->Win32Process     				      = +0x214 ;//Win32Process     : Ptr32 Void
	pOffset->Job              				      = +0x218 ;//Job              : Ptr32 _EJOB
	pOffset->JobStatus        				      = +0x21c ;//JobStatus        : Uint4B
	pOffset->JobLinks         				      = +0x220 ;//JobLinks         : _LIST_ENTRY
	pOffset->LockedPagesList  				      = +0x228 ;//LockedPagesList  : Ptr32 Void
	pOffset->SecurityPort     				      = +0x22c ;//SecurityPort     : Ptr32 Void
	pOffset->Wow64Process     				      = +0x230 ;//Wow64Process     : Ptr32 _WOW64_PROCESS
	pOffset->ReadOperationCount 			      = +0x238 ;//ReadOperationCount : _LARGE_INTEGER
	pOffset->WriteOperationCount			      = +0x240 ;//WriteOperationCount : _LARGE_INTEGER
	pOffset->OtherOperationCount			      = +0x248 ;//OtherOperationCount : _LARGE_INTEGER
	pOffset->ReadTransferCount 			        = +0x250 ;//ReadTransferCount : _LARGE_INTEGER
	pOffset->WriteTransferCount 			      = +0x258 ;//WriteTransferCount : _LARGE_INTEGER
	pOffset->OtherTransferCount 			      = +0x260 ;//OtherTransferCount : _LARGE_INTEGER
	pOffset->CommitChargeLimit 			        = +0x268 ;//CommitChargeLimit : Uint4B
	pOffset->CommitChargePeak 				      = +0x26c ;//CommitChargePeak : Uint4B
	pOffset->ThreadListHead   				      = +0x270 ;//ThreadListHead   : _LIST_ENTRY
	pOffset->VadPhysicalPagesBitMap 	      = +0x278 ;//VadPhysicalPagesBitMap : Ptr32 _RTL_BITMAP
	pOffset->VadPhysicalPages 				      = +0x27c ;//VadPhysicalPages : Uint4B
	pOffset->AweLock          				      = +0x280 ;//AweLock          : Uint4B
	pOffset->pImageFileName   				      = +0x284 ;//pImageFileName   : Ptr32 _UNICODE_STRING
	pOffset->Session          				      = +0x288 ;//Session          : Ptr32 Void
	pOffset->Flags            				      = +0x28c ;//Flags            : Uint4B

	return pOffset;
}
// 获取指针
PEPROCESS_OFFSET		GetEProcessOffset()
{
	static EPROCESS_OFFSET		EprocessOffset		= {0};

	if(2195 == GetBuildNumber())
	{
		// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible
		return GetEProcessOffset_2195(&EprocessOffset);
	}
	return NULL;
}
// 获取PEB结构
BOOLEAN EPROCESS_PPEB(PEPROCESS pEproc, PPEB* pPeb)
{
	BOOLEAN			bRet	= TRUE;


	return FALSE;
}