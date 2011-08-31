#pragma once

typedef struct _guardpath {
	WCHAR				szGuardPath[512];
	ULONG				ulPathHash;
} GUARDPATH, *PGUARDPATH;

// ¹«¹²º¯Êý
NTSTATUS				AddIrpToQueue(PIRP pIrp);
NTSTATUS				ResponseToQueue(PIRP pIrp);
PINNERPACK_LIST			AddPackToQueue(ULONG ulType, LPCWSTR lpPath, LPCWSTR lpSubPath);
void					RemovePackToQueue(PINNERPACK_LIST pQuery);
void					SetPackForQuery(ULONG nWaitID, BOOLEAN Access);
NTSTATUS				DealIrpAndPackQueue();
LONG					IrpReadStackPush(PIRP pIrp);
PIRP					IrpReadStackPop();
ULONG					GetHashUprPath(LPCWSTR lpPath);