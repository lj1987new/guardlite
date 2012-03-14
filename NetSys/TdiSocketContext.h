#pragma once

/* 地正信息 */
typedef struct _ASSOCIATE_ADDRESS
{
	ULONG				IPAdd;
	USHORT				Port;
	BOOLEAN				bChecked;
}ASSOCIATE_ADDRESS, *PASSOCIATE_ADDRESS;
/* replaced context */
typedef struct {
	LIST_ENTRY				list;
	BOOLEAN					bIsAddressFileObj;		/* 是否本地端口 */
	PFILE_OBJECT			pAddressFileObj;		/* 本地地址对像 */
	PFILE_OBJECT			pConnectFileObj;		/* 连接端口对像 */
	BOOLEAN					bIsHttp;				/* 是否HTTP协议 */
	PVOID					event_receive_handler;
	PVOID					event_receive_context;
	PVOID					event_chained_handler;
	PVOID					event_chained_context;
	ASSOCIATE_ADDRESS		address;				/* 地址信息 */
} TDI_SOCKET_CONTEXT, *PTDI_SOCKET_CONTEXT;


BOOLEAN				TdiSocketContextInit();
PTDI_SOCKET_CONTEXT	TdiSocketContextGet(PFILE_OBJECT pAddressFileObj);
PTDI_SOCKET_CONTEXT TdiSocketContextGetAddress(PFILE_OBJECT pAddressFileObj, BOOLEAN bCreate);
PTDI_SOCKET_CONTEXT	TdiSocketContextGetConnect(PFILE_OBJECT pConnectFileObj, BOOLEAN bCreate);
void				TdiSocketContextErase(PFILE_OBJECT pAddressFileObj);
void				TdiSocketContextRelease();
BOOLEAN				IsTcpRequest(PFILE_OBJECT fileObj, PASSOCIATE_ADDRESS* pAddress, PTDI_SOCKET_CONTEXT* ppSockConnect);
