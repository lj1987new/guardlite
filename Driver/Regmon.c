/*
 *	注册表监控模块
 */
#include <Ntifs.h>
#include "GuardLite.h"
#include "Regmon.h"
#include "Public.h"

fnRealRegSetValueKey		RealRegSetValueKey		= NULL;
PSRVTABLE					ServiceTable			= NULL;

#define MAXROOTLEN      128
#define MAXPATHLEN		1024


GUARDPATH		RegGuardPath[]		= {
	{MASK_SYSTEM_AUTORUN, L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKCU\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows", L"load", 0}
	, {MASK_SYSTEM_AUTORUN, L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", L"Userinit", 0}
	, {MASK_SYSTEM_AUTORUN, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RunServicesOnce", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunServicesOnce", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RunServices", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunServices", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\Setup", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\Setup", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", NULL, 0}
	, {MASK_SYSTEM_AUTORUN, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RUNSERVICESONCE", NULL, 0}
	, {MASK_SYSTEM_SCREEN, L"HKCU\\Control Panel\\Desktop", L"Scrnsave.exe", 0}
};

ROOTKEY CurrentUser[] = {
	{ L"\\REGISTRY\\USER\\S", L"HKCU", 0 },
	{ L"HKU\\S", L"HKCU", 0 }
};

ROOTKEY RootKey[] = {
	{ L"\\REGISTRY\\USER", L"HKU", 0 }
	, { L"\\REGISTRY\\MACHINE", L"HKLM", 0 }
	, { L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT", 
		L"HKCC", 0 }
	, { L"\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES", L"HKCR", 0 }
};

PAGED_LOOKASIDE_LIST			gRegMonLooaside;
//////////////////////////////////////////////////////////////////////////
NTSTATUS	RegmonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	int				i;

	ServiceTable = KeServiceDescriptorTable;
	RealRegSetValueKey = (fnRealRegSetValueKey) SYSCALL( ZwSetValueKey );
	// 更改SSDT表
	SETSYSCALL( ZwSetValueKey, RegSetValueKey );

	for(i = 0; i < arrayof(CurrentUser); i++)
		CurrentUser[i].RootNameLen = wcslen(CurrentUser[i].RootName);
	for(i = 0; i < arrayof(RootKey); i++)
		RootKey[i].RootNameLen = wcslen(RootKey[i].RootName);
	for(i = 0; i < arrayof(RegGuardPath); i++)
		RegGuardPath[i].ulPathHash = GetHashUprPath(RegGuardPath[i].pGuardPath);
	// 初始化内存分配器
	ExInitializePagedLookasideList(&gRegMonLooaside, NULL, NULL, 0
		, MAXPATHLEN * 2 + 2 * sizeof(ULONG), PAGE_DEBUG, 0);

	return STATUS_SUCCESS;
}

void		RegmonUnload()
{
	// 还原SSDT表
	SETSYSCALL( ZwSetValueKey, RealRegSetValueKey );
}

NTSTATUS	RegmonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
void EnableServiceTable()
{
	__asm{
		push eax 
			mov	 eax, cr0 
			and	 eax, 0fffeffffh 
			mov  cr0, eax 
			pop  eax

	}
}

void DisableServiceTable()
{
	__asm{ 
		push eax 
			mov  eax, cr0 
			or	 eax, not 0fffeffffh 
			mov  cr0, eax 
			pop  eax 
	} 
}

//////////////////////////////////////////////////////////////////////////
VOID ConvertToUpper( PWCHAR Dest, PWCHAR Source, ULONG Len )
{
	ULONG   i;

	if(-1 == Len)
		Len = wcslen(Source);
	for( i = 0; i < Len; i++ ) {
		if( Source[i] >= L'a' && Source[i] <= L'z' ) {

			Dest[i] = Source[i] - L'a' + L'A';

		} else {

			Dest[i] = Source[i];
		}
	}
}

void ConvertKeyPath(LPWSTR pOut, LPWSTR pIn, int nLen)
{
	int		i						= 0;
	WCHAR	cmpname[MAXROOTLEN]		= {0};
	LPWSTR	nameptr					= NULL;

	for( i = 0; i < arrayof(CurrentUser); i++ ) 
	{
		ConvertToUpper( cmpname, pIn, CurrentUser[i].RootNameLen );
		if( !wcsncmp( cmpname, CurrentUser[i].RootName,	CurrentUser[i].RootNameLen )) 
		{
			nameptr = pIn + CurrentUser[i].RootNameLen;
			while( *nameptr && *nameptr != L'\\' ) nameptr++;
			wcscpy( pOut, CurrentUser[i].RootShort );
			wcsncpy( &pOut[4],nameptr, nLen-4);
			//wcsncat( , pOut, nameptr );
			return;
		}
	}     

	for( i = 0; i < arrayof(RootKey); i++ ) 
	{
		ConvertToUpper( cmpname, pIn, RootKey[i].RootNameLen );
		if( !wcsncmp( cmpname, RootKey[i].RootName, 
			RootKey[i].RootNameLen )) 
		{
			nameptr = pIn + RootKey[i].RootNameLen;
			wcscpy( pOut, RootKey[i].RootShort );
			//wcscat( pOut, nameptr );
			wcsncpy( &pOut[4],nameptr, nLen-4);
			return;
		}
	}

	wcscpy( pOut, pIn );
}
/*
 *	查看路径是否监控的路径
 */
BOOLEAN		IsRegGuardPath(PCWSTR pPath, PCWSTR pSubPath, LONG* pSubType)
{
	ULONG			ulHash			= GetHashUprPath(pPath);
	int				i;

	for(i = 0; i < arrayof(RegGuardPath); i++)
	{
		if(ulHash != RegGuardPath[i].ulPathHash)
			continue;
		/*if(_wcsicmp(pPath, RegGuardPath[i].szGuardPath) == 0)*/

		*pSubType = RegGuardPath[i].nSubType;
		if(NULL == RegGuardPath[i].pSubPath)
			return TRUE;
		if(0 == _wcsicmp(pSubPath, RegGuardPath[i].pSubPath))
			return TRUE;
		
		return FALSE;
	}
	return FALSE;
}

NTSTATUS RegSetValueKey( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName,
						IN ULONG TitleIndex, IN ULONG Type, 
						IN PVOID Data, IN ULONG DataSize )
{
	WCHAR			szFullPath[MAXPATHLEN]	= {0};		
	PVOID			pKeyObj					= NULL;
	ULONG			ulRet					= 0;
	PUNICODE_STRING	fullUniName				= NULL;
	int				i;
	ULONG			nAllowd					= 1;
	WCHAR			szValueName[256]		= {0};
	WCHAR			szValue[512]			= {0};

	if(FALSE == IsGuardStart())
		goto allowed;
	if(STATUS_SUCCESS == ObReferenceObjectByHandle(KeyHandle, 0, NULL, KernelMode, &pKeyObj, NULL))
 	{
		PINNERPACK_LIST			pList;
		LONG					nSubType		= 0;

 		fullUniName = ExAllocateFromPagedLookasideList(&gRegMonLooaside);
		if(NULL == fullUniName)
			goto allowed;

 		fullUniName->MaximumLength = MAXPATHLEN * 2;
 		ObQueryNameString(pKeyObj, (POBJECT_NAME_INFORMATION)fullUniName, MAXPATHLEN, &ulRet);
 		ObDereferenceObject(pKeyObj);
		// 转换路径
		ConvertKeyPath(szFullPath, fullUniName->Buffer, MAXPATHLEN);
		ExFreeToPagedLookasideList(&gRegMonLooaside, fullUniName);
		// 复制路径
		wcsncpy(szValueName, (NULL != ValueName)?ValueName->Buffer:L""
			, (NULL != ValueName)?ValueName->Length:0);
		// 比较路径
		if(FALSE == IsRegGuardPath(szFullPath, szValueName, &nSubType))
			goto allowed;
		if(REG_SZ == Type)
		{
			wcsncpy(szValue, Data, arrayof(szValueName));
		}
		// 到用户求请
		if(FALSE != CheckRequestIsAllowed(MAKEGUARDTYPE(MASK_GUARDLITE_REGMON, nSubType)
			, szFullPath, szValueName, szValue))
		{
			goto allowed;
		}
	}
	return STATUS_ACCESS_DENIED;
allowed:
	return RealRegSetValueKey(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
}
