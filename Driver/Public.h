#pragma once

typedef struct _guardpath {
	ULONG				nSubType;
	WCHAR*				pGuardPath;
	WCHAR*				pSubPath;
	ULONG				ulPathHash;
	LONG				nPathLen;
} GUARDPATH, *PGUARDPATH;

// ¹«¹²º¯Êý
NTSTATUS				AddIrpToQueue(PIRP pIrp);
NTSTATUS				ResponseToQueue(PIRP pIrp);
BOOLEAN					CheckRequestIsAllowed(ULONG ulType, LPCWSTR lpPath, LPCWSTR lpSubPath, LPCWSTR lpValue);
void					EraseFromQueue(PINNERPACK_LIST pQuery);
void					SetPackForQuery(ULONG nWaitID, BOOLEAN Access);
NTSTATUS				DealIrpAndPackQueue();
LONG					IrpReadStackPush(PIRP pIrp);
PIRP					IrpReadStackPop();
ULONG					GetHashUprPath(LPCWSTR lpPath, ULONG* pLenHash);
LONG					mywcsicmp(LPCWSTR pstr1, LPCWSTR pstr2);