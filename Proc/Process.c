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
BOOLEAN EPROCESS_PPEB(PEPROCESS pEproc, PPEB* pPeb)
{
	ULONG			nPos	= 0;

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
// 获取Imagebase地址
BOOLEAN PEB_ImageBaseAddress(PPEB pPeb, PVOID* pBase)
{
	ULONG			nPos	= 0;

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
BOOLEAN PEB_Ldr(PPEB pPeb, PVOID* pLdr)
{
	ULONG			nPos	= 0;

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
BOOLEAN PEB_ProcessParameters(PPEB pPeb, PVOID* pParam)
{
	ULONG			nPos	= 0;

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

BOOLEAN RTL_USER_PROCESS_PARAMETERS_DllPath(PVOID pParam, PUNICODE_STRING* pDllPath)
{
	ULONG			nPos	= 0;

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

BOOLEAN RTL_USER_PROCESS_PARAMETERS_ImagePathName(PVOID pParam, PUNICODE_STRING* pPathName)
{
	ULONG			nPos	= 0;

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

BOOLEAN RTL_USER_PROCESS_PARAMETERS_CommandLine(PVOID pParam, PUNICODE_STRING* pCommandLine)
{
	ULONG			nPos	= 0;

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
BOOLEAN PEB_LDR_DATA_InLoadOrderModuleList(PVOID pLdr, PLIST_ENTRY* pEntry)
{
	ULONG			nPos	= 0;

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
BOOLEAN LDR_DATA_TABLEFromInLoadOrderModuleList(PVOID pInload, PVOID* pLdr)
{
	ULONG			nPos	= 0;

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

BOOLEAN LDR_DATA_TABLE_DllBase(PVOID pLdr, PVOID* pBase)
{
	ULONG			nPos	= 0;

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

BOOLEAN LDR_DATA_TABLE_EntryPoint(PVOID pLdr, PVOID* pPoint)
{
	ULONG			nPos	= 0;

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

BOOLEAN LDR_DATA_TABLE_SizeOfImage(PVOID pLdr, UINT32* pSize)
{
	ULONG			nPos	= 0;

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

BOOLEAN LDR_DATA_TABLE_FullDllName(PVOID pLdr, PUNICODE_STRING* pFullName)
{
	ULONG			nPos	= 0;

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

BOOLEAN LDR_DATA_TABLE_BaseDllName(PVOID pLdr, PUNICODE_STRING* pDllName)
{
	ULONG			nPos	= 0;

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
