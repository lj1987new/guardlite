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
// 获取PEB结构
BOOLEAN EPROCESS__PPEB(PEPROCESS pEproc, PPEB* pPeb)
{
	ULONG			nPos	= 0;

	if(NULL == pEproc)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
		nPos = 0x1B0;
		break;
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x1A8;
		break;
	default:
		return FALSE;
	}
	*pPeb = (PPEB)*( (ULONG*)((char*)pEproc + nPos) );
	return TRUE;
}
BOOLEAN EPROCESS__ThreadListHead(PEPROCESS pEproc, PLIST_ENTRY* pThreadList)
{
	ULONG			nPos	= 0;

	if(NULL == pEproc)
		return FALSE;
	switch(GetBuildNumber())
	{
	case 2600: // Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
		nPos = 0x190;
		break;
	default:
		return FALSE;
	}
	*pThreadList = (PLIST_ENTRY)((char*)pEproc + nPos);
	return TRUE;
}
// 获取Imagebase地址
BOOLEAN PEB__ImageBaseAddress(PPEB pPeb, PVOID* pBase)
{
	ULONG			nPos	= 0;

	if(NULL == pPeb)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x8;
		break;
	default:
		return FALSE;
	}
	*pBase = (PPEB)*( (ULONG*)((char*)pPeb + nPos) );
	return TRUE;
}
// 获取Ldr _PEB_LDR_DATA
BOOLEAN PEB__Ldr(PPEB pPeb, PVOID* pLdr)
{
	ULONG			nPos	= 0;

	if(NULL == pPeb)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0xC;
		break;
	default:
		return FALSE;
	}
	*pLdr = (PVOID)*( (ULONG*)((char*)pPeb + nPos) );
	return TRUE;
}
// 获取PEB->ProcessParameters
BOOLEAN PEB__ProcessParameters(PPEB pPeb, PVOID* pParam)
{
	ULONG			nPos	= 0;

	if(NULL == pPeb)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x10;
		break;
	default:
		return FALSE;
	}
	*pParam = (PVOID)*( (ULONG*)((char*)pPeb + nPos) );
	return TRUE;
}

BOOLEAN RTL_USER_PROCESS_PARAMETERS__DllPath(PVOID pParam, PUNICODE_STRING* pDllPath)
{
	ULONG			nPos	= 0;

	if(NULL == pParam)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x30;
		break;
	default:
		return FALSE;
	}
	*pDllPath = (PUNICODE_STRING)((char*)pParam + nPos);
	return TRUE;
}

BOOLEAN RTL_USER_PROCESS_PARAMETERS__ImagePathName(PVOID pParam, PUNICODE_STRING* pPathName)
{
	ULONG			nPos	= 0;

	if(NULL == pParam)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x38;
		break;
	default:
		return FALSE;
	}
	*pPathName = (PUNICODE_STRING)((char*)pParam + nPos);
	return TRUE;
}

BOOLEAN RTL_USER_PROCESS_PARAMETERS__CommandLine(PVOID pParam, PUNICODE_STRING* pCommandLine)
{
	ULONG			nPos	= 0;

	if(NULL == pParam)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x40;
		break;
	default:
		return FALSE;
	}
	*pCommandLine = (PUNICODE_STRING)((char*)pParam + nPos);
	return TRUE;
}
// 获取PEB->Ldr->InLoadOrderModuleList
BOOLEAN PEB_LDR_DATA__InLoadOrderModuleList(PVOID pLdr, PLIST_ENTRY* pEntry)
{
	ULONG			nPos	= 0;

	if(NULL == pLdr)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0xC;
		break;
	default:
		return FALSE;
	}
	*pEntry = (PLIST_ENTRY)((char*)pLdr + nPos);
	return TRUE;
}
BOOLEAN CONTAINING_RECORD__LDR_DATA_TABLE(PVOID pInload, PVOID* pLdr)
{
	ULONG			nPos	= 0;

	if(NULL == pInload)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x0;
		break;
	default:
		return FALSE;
	}
	*pLdr = (PVOID)((char*)pInload + nPos);
	return TRUE;
}

BOOLEAN LDR_DATA_TABLE_ENTRY__DllBase(PVOID pLdr, PVOID* pBase)
{
	ULONG			nPos	= 0;

	if(NULL == pLdr)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x18;
		break;
	default:
		return FALSE;
	}
	*pBase = (PVOID)*( (ULONG*)((char*)pLdr + nPos) );
	return TRUE;
}

BOOLEAN LDR_DATA_TABLE_ENTRY__EntryPoint(PVOID pLdr, PVOID* pPoint)
{
	ULONG			nPos	= 0;

	if(NULL == pLdr)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x1C;
		break;
	default:
		return FALSE;
	}
	*pPoint = (PVOID)*( (ULONG*)((char*)pLdr + nPos) );
	return TRUE;
}

BOOLEAN LDR_DATA_TABLE_ENTRY__SizeOfImage(PVOID pLdr, UINT32* pSize)
{
	ULONG			nPos	= 0;

	if(NULL == pLdr)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x20;
		break;
	default:
		return FALSE;
	}
	*pSize = (UINT32)*( (ULONG*)((char*)pLdr + nPos) );
	return TRUE;
}

BOOLEAN LDR_DATA_TABLE_ENTRY__FullDllName(PVOID pLdr, PUNICODE_STRING* pFullName)
{
	ULONG			nPos	= 0;

	if(NULL == pLdr)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x24;
		break;
	default:
		return FALSE;
	}
	*pFullName = (PUNICODE_STRING)((char*)pLdr + nPos);
	return TRUE;
}

BOOLEAN LDR_DATA_TABLE_ENTRY__BaseDllName(PVOID pLdr, PUNICODE_STRING* pDllName)
{
	ULONG			nPos	= 0;

	if(NULL == pLdr)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2195:	// Windows 2000 Kernel Version 2195 (Service Pack 4) UP Free x86 compatible 
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
	case 7600: // Windows 7 Kernel Version 7600 MP (1 procs) Free x86 compatible
		nPos = 0x2C;
		break;
	default:
		return FALSE;
	}
	*pDllName = (PUNICODE_STRING)((char*)pLdr + nPos);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// 获取模块路径
PVOID	GetModuleBaseAddress(PEPROCESS pEproc, PUNICODE_STRING pDllName)
{
	PPEB					pPeb;
	PVOID					pLdr;
	PLIST_ENTRY				pModules;
	PLIST_ENTRY				pNext;
	PVOID					pBaseAddr		= NULL;

	if(FALSE == EPROCESS__PPEB(PsGetCurrentProcess(), &pPeb))
		return NULL;
	// 获取DLL链表
	if(FALSE == PEB__Ldr(pPeb, &pLdr))
		return NULL;
	// 获取链表头
	if(FALSE == PEB_LDR_DATA__InLoadOrderModuleList(pLdr, &pModules))
		return NULL;
	for(pNext = pModules->Flink; pNext != pModules; pNext = pNext->Flink)
	{
		PVOID				pLdrOne;
		PUNICODE_STRING		pBaseName;

		if(FALSE == CONTAINING_RECORD__LDR_DATA_TABLE(pNext, &pLdrOne))
			continue;
		if(FALSE == LDR_DATA_TABLE_ENTRY__BaseDllName(pLdrOne, &pBaseName))
			continue;
		if(0 != RtlCompareUnicodeString(pDllName, pBaseName, FALSE))
			continue;
		if(FALSE == LDR_DATA_TABLE_ENTRY__DllBase(pLdrOne, &pBaseAddr))
			return NULL;
		// 反回获取到的值
		return pBaseName;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
// 线程操作
BOOLEAN CONTAINING_RECORD__ETHREAD(PVOID pEntery, PETHREAD* pEThread)
{
	LONG		nPos		= 0;

	if(NULL == pEThread)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
		nPos = -0x22c;
		break;
	default:
		return FALSE;
	}
	*pEThread = (PETHREAD)((char*)pEntery + nPos);
	return TRUE;
}

BOOLEAN ETHREAD__Tcb(PETHREAD EThread, PKTHREAD* pTcb)
{
	LONG		nPos		= 0;

	if(NULL == EThread)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
		nPos = 0x0;
		break;
	default:
		return FALSE;
	}
	*pTcb = (PKTHREAD)((char*)EThread + nPos);
	return TRUE;
}

BOOLEAN KTHREAD__SuspendCount(PKTHREAD pTcb, PCHAR* pSuspendCount)
{
	LONG		nPos		= 0;

	if(NULL == pTcb)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
		nPos = 0x1b9;
		break;
	default:
		return FALSE;
	}
	*pSuspendCount = (PCHAR)((char*)pTcb + nPos);
	return TRUE;
}

BOOLEAN KTHREAD__SuspendApc(PKTHREAD pTcb, PKAPC* pSuspendApc)
{
	LONG		nPos		= 0;

	if(NULL == pTcb)
		return FALSE;

	switch(GetBuildNumber())
	{
	case 2600:	// Windows XP Kernel Version 2600 (Service Pack 2) UP Free x86 compatible
		nPos = 0x16c;
		break;
	default:
		return FALSE;
	}
	*pSuspendApc = (PKAPC)((char*)pTcb + nPos);
	return TRUE;
}