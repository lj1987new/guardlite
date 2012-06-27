#pragma once
/*
 *	foc 即 FileObject Context的首字母简写
 */

/* 连接对像使用结构体 */
typedef struct
{
	BOOLEAN					bChecked;
	BOOLEAN					bInline;
	USHORT					Port;
	ULONG					IPAdd;
	ULONGLONG				pid;
	CONNECTION_CONTEXT		pConnectContext;
	CHAR*					pHost;
} tdi_foc_connection, *tdi_foc_connection_ptr;

/* 地址对像使作结构体 */
typedef struct  
{
	PVOID					event_receive_handler;
	PVOID					event_receive_context;
	PVOID					event_chained_handler;
	PVOID					event_chained_context;
	char*					pRedirectHeader;
} tdi_foc_address;

/* TDI结构体 */
typedef struct {
	LIST_ENTRY				list;
	BOOLEAN					bSelf;					/* 是否自己 */
	BOOLEAN					bIsAddressFileObj;		/* 是否本地端口 */
	PFILE_OBJECT			pAddressFileObj;		/* 本地地址对像 */
	PFILE_OBJECT			pConnectFileObj;		/* 连接端口对像 */
	BOOLEAN					bIsHttp;				/* 是否HTTP协议 */
	BOOLEAN					bStopOption;			/* 是否阻止连接 */
	union
	{
		tdi_foc_address			address;			/* 地址对像扩展数据 */
		tdi_foc_connection		connecation;		/* 连接对像扩展数据 */
	};
} tdi_foc, *tdi_foc_ptr;

// 基础函数
BOOLEAN						tdi_foc_Init();
tdi_foc_ptr					tdi_foc_Get(PFILE_OBJECT pAddressFileObj);
tdi_foc_ptr					tdi_foc_GetAddress(PFILE_OBJECT pAddressFileObj, BOOLEAN bCreate);
tdi_foc_ptr					tdi_foc_GetConnection(PFILE_OBJECT pConnectFileObj, BOOLEAN bCreate);
void						tdi_foc_Erase(PFILE_OBJECT pAddressFileObj);
void						tdi_foc_Release();
// 扩展函数
BOOLEAN						tdi_foc_CheckConnection(PFILE_OBJECT fileObj, tdi_foc_connection_ptr* pAddress, tdi_foc_ptr* ppSockConnect);
