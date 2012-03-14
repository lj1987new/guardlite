/*
 *	TDI 扩展功能
 */
#include "EhomeNet.h"
#include "EhomeDevCtl.h"
#include <TdiKrnl.h>

NTSTATUS EHomeClientEventReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
								 , IN ULONG  ReceiveFlags, IN ULONG  BytesIndicated, IN ULONG  BytesAvailable
								 , OUT ULONG  *BytesTaken, IN PVOID  Tsdu, OUT PIRP  *IoRequestPacket);
NTSTATUS EHomeClientEventChainedReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
								 , IN ULONG  ReceiveFlags, IN ULONG  ReceiveLength, IN ULONG  StartingOffset
								 , IN PMDL  Tsdu, IN PVOID  TsduDescriptor);

// 设置事件句柄
NTSTATUS		EHomeTDISetEventHandler(PIRP pIrp, PIO_STACK_LOCATION pStack)
{
	PTDI_REQUEST_KERNEL_SET_EVENT			pTdiEvent			= NULL;
	PTDI_SOCKET_CONTEXT						pSocketContext		= NULL;

	pTdiEvent = (PTDI_REQUEST_KERNEL_SET_EVENT)&pStack->Parameters;
	pSocketContext = TdiSocketContextGet(pStack->FileObject, TRUE);
	if(NULL == pSocketContext)
		return STATUS_SUCCESS;

	if(TDI_EVENT_RECEIVE == pTdiEvent->EventType)
	{
		pSocketContext->event_receive_handler = pTdiEvent->EventHandler;
		pSocketContext->event_receive_context = pTdiEvent->EventContext;
		//return STATUS_INVALID_PARAMETER;
		if(NULL != pTdiEvent->EventHandler)
		{
			pTdiEvent->EventHandler = /*TdiDefaultReceiveHandler*/EHomeClientEventReceive;
			pTdiEvent->EventContext = pSocketContext;
		}
	}
	else if(TDI_EVENT_CHAINED_RECEIVE == pTdiEvent->EventType)
	{
		pSocketContext->event_chained_handler = pTdiEvent->EventHandler;
		pSocketContext->event_chained_context = pTdiEvent->EventContext;
		return STATUS_INVALID_PARAMETER;
 		if(NULL != pTdiEvent->EventHandler)
		{
			pTdiEvent->EventHandler = EHomeClientEventChainedReceive;
			pTdiEvent->EventContext = pSocketContext;
 		}
	}

	return STATUS_SUCCESS;
}

// 接收客户端
NTSTATUS EHomeClientEventReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
								 , IN ULONG  ReceiveFlags, IN ULONG  BytesIndicated, IN ULONG  BytesAvailable
								 , OUT ULONG  *BytesTaken, IN PVOID  Tsdu, OUT PIRP  *IoRequestPacket)
{
	PTDI_SOCKET_CONTEXT						pSocketContext		= NULL;
	NTSTATUS								status				= STATUS_SUCCESS;
	char*									pData				= NULL;

	pSocketContext = TdiSocketContextGet((PFILE_OBJECT)TdiEventContext, FALSE);
	if(NULL == pSocketContext)
		return STATUS_SUCCESS;
	if(NULL == pSocketContext->event_receive_handler)
		return STATUS_SUCCESS;
	if(pSocketContext->bStop)
	{
		int			i = 0;
	}
	KdPrint(("[EHomeClientEventReceive] file: %x (%d.%d.%d.%d:%d)(%x, %x, %d, %d, %d, %x, %s, %x)\n"
		, pSocketContext->fileobj
		, (pSocketContext->IPAdd >> 24) & 0xff
		, (pSocketContext->IPAdd >> 16) & 0xff
		, (pSocketContext->IPAdd >> 8) & 0xff
		, pSocketContext->IPAdd & 0xff
		, (int)pSocketContext->Port
		, TdiEventContext, ConnectionContext
		, ReceiveFlags, BytesIndicated, BytesAvailable
		, BytesTaken, Tsdu, IoRequestPacket));
	// 调用原来的接收例程
	status = ((PTDI_IND_RECEIVE)pSocketContext->event_receive_handler)
		/*TdiDefaultReceiveHandler*/(pSocketContext->event_receive_context
		, ConnectionContext
		, ReceiveFlags
		, BytesIndicated
		, BytesAvailable
		, BytesTaken
		, Tsdu
		, IoRequestPacket);

	return status;
}

NTSTATUS EHomeClientEventChainedReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
										, IN ULONG  ReceiveFlags, IN ULONG  ReceiveLength, IN ULONG  StartingOffset
										, IN PMDL  Tsdu, IN PVOID  TsduDescriptor)
{
	PTDI_SOCKET_CONTEXT						pSocketContext		= NULL;
	NTSTATUS								status				= STATUS_SUCCESS;
	char*									pData				= NULL;

	pSocketContext = TdiSocketContextGet((PFILE_OBJECT)TdiEventContext, FALSE);
	if(NULL == pSocketContext)
		return STATUS_SUCCESS;
	if(NULL == pSocketContext->event_receive_handler)
		return STATUS_SUCCESS;
	if(pSocketContext->bStop)
	{
		int			i = 0;
	}
	if(NULL != Tsdu)
	{
		pData = (char *)MmGetSystemAddressForMdlSafe(Tsdu, NormalPagePriority);
		KdPrint(("[EHomeClientEventChainedReceive] %s \n", pData));
	}
	KdPrint(("[EHomeClientEventChainedReceive] file:%x (%d.%d.%d.%d:%d)(%x, %x, %d, %d, %d, %s, %s)\n"
		, pSocketContext->fileobj
		, (pSocketContext->IPAdd >> 24) & 0xff
		, (pSocketContext->IPAdd >> 16) & 0xff
		, (pSocketContext->IPAdd >> 8) & 0xff
		, pSocketContext->IPAdd & 0xff
		, (int)pSocketContext->Port
		, TdiEventContext, ConnectionContext
		, ReceiveFlags, ReceiveLength, StartingOffset
		, pData, TsduDescriptor));
	// 调用原来的接收例程
	status = ((PTDI_IND_CHAINED_RECEIVE)pSocketContext->event_chained_handler)
		/*TdiDefaultChainedReceiveHandler*/(
		pSocketContext->event_chained_context
		, ConnectionContext
		, ReceiveFlags
		, ReceiveLength
		, StartingOffset
		, Tsdu
		, TsduDescriptor);

	return status;
}
// 调用驱动
NTSTATUS TdiCall (IN PIRP pIrp, IN PDEVICE_OBJECT pDeviceObject, IN OUT PIO_STATUS_BLOCK pIoStatusBlock )
{
	KEVENT kEvent;                                                    // signaling event
	NTSTATUS dStatus = STATUS_INSUFFICIENT_RESOURCES;                 // local status

	KeInitializeEvent ( &kEvent, NotificationEvent, FALSE );          // reset notification event
	pIrp->UserEvent = &kEvent;                                        // pointer to event
	pIrp->UserIosb = pIoStatusBlock;                               // pointer to status block
	dStatus = IoCallDriver ( pDeviceObject, pIrp );                   // call next driver
	if ( dStatus == STATUS_PENDING )                                  // make all request synchronous
	{
		(void)KeWaitForSingleObject (
			(PVOID)&kEvent,                                 // signaling object
			Executive,                                      // wait reason
			KernelMode,                                     // wait mode
			TRUE,                                           // alertable
			NULL );                                         // timeout
	}
	dStatus = pIoStatusBlock->Status;                          
	return ( dStatus );                                               // return with status
}

NTSTATUS EHomeRecive(PDEVICE_OBJECT pDeviceObject, PFILE_OBJECT pFileObject)
{
	PTDI_SOCKET_CONTEXT			pSocket;
	char						szData[]		= {"HTTP/1.1 302 Found\r\n"
		"Location: http://www.google.com.hk/\r\n"
		"Content-Length: 0\r\n"
		"\r\n"};
	ULONG						len				= sizeof(szData);
	NTSTATUS					status			= 0;

	pSocket = TdiSocketContextGet(pFileObject, FALSE);
	if(NULL == pSocket)
		return STATUS_INSUFFICIENT_RESOURCES;
	if(NULL == pSocket->event_receive_handler)
		return STATUS_INSUFFICIENT_RESOURCES;
	
	status = ((PTDI_IND_RECEIVE)pSocket->event_receive_handler)(
		pSocket->event_receive_context
		, pSocket->fileobj
		, TDI_RECEIVE_NORMAL
		, len
		, len
		, &len
		, szData
		, NULL);

	return status;
}
// 重新连接
NTSTATUS EHomeReconnect(PDEVICE_OBJECT pDeviceObject, PFILE_OBJECT pFileObject)
{
	PIRP					pIrp				= NULL;
	IO_STATUS_BLOCK			IoStatusBlock		= {0};

	return EHomeRecive(pDeviceObject, pFileObject);
	// 断开原来的连接
	pIrp = TdiBuildInternalDeviceControlIrp(TDI_DISCONNECT, pDeviceObject, pFileObject, NULL, NULL);
	if(NULL == pIrp)
		return STATUS_INSUFFICIENT_RESOURCES;
	TdiBuildDisconnect(pIrp, pDeviceObject, pFileObject, NULL, NULL, NULL
		, TDI_DISCONNECT_ABORT, NULL, NULL);
	if(STATUS_SUCCESS != TdiCall(pIrp, pDeviceObject, &IoStatusBlock))
		return STATUS_INSUFFICIENT_RESOURCES;
	// 连接新地址
	pIrp = TdiBuildInternalDeviceControlIrp(TDI_CONNECT, pDeviceObject, pFileObject, NULL, NULL);
	if(NULL == pIrp)
		return STATUS_INSUFFICIENT_RESOURCES;
	{
		IO_STATUS_BLOCK					IoStatus;
		TA_IP_ADDRESS					RemoteAddr;
		TDI_CONNECTION_INFORMATION		RequestInfo;
		PTDI_ADDRESS_IP					pTdiAddressIp;


		//// build connection packet
		pIrp = TdiBuildInternalDeviceControlIrp(TDI_CONNECT, pDeviceObject, FileObject, NULL, &IoStatus);
		if (pIrp == 0)
			return STATUS_INSUFFICIENT_RESOURCES;

		// Initialize controller data
		RemoteAddr.TAAddressCount = 1;
		RemoteAddr.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
		RemoteAddr.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
		/*pTdiAddressIp = (TDI_ADDRESS_IP *)RemoteAddr.Address[0].Address;
		pTdiAddressIp->sin_port=Port;
		pTdiAddressIp->in_addr=Addr; */
			RemoteAddr.Address[0].Address[0].sin_port = ((80 << 8) & 0xff00);
		RemoteAddr.Address[0].Address[0].in_addr = ( (((UCHAR)222 << 24) & 0xff000000) | ((76 << 16) & 0x00ff0000) 
			| (((UCHAR)216 << 8) & 0x0000ff00) | ((88) & 0x000000ff) );

		RequestInfo.Options=0;
		RequestInfo.OptionsLength=0;
		RequestInfo.UserData=0;
		RequestInfo.UserDataLength=0;
		RequestInfo.RemoteAddress=&RemoteAddr;
		RequestInfo.RemoteAddressLength=sizeof(RemoteAddr);

		TdiBuildConnect(pIrp, pDeviceObject, pFileObject, NULL, NULL, NULL, &RequestInfo, NULL);
		if(STATUS_SUCCESS != TdiCall(pIrp, pDeviceObject, &IoStatusBlock))
			return -1;
	}
	// 发送数据
	{
		PMDL				pMdl		= NULL;
		CHAR				szData[]	= {"GET /\r\nHost: www.txtws.com\r\n\r\n"};

		pIrp = TdiBuildInternalDeviceControlIrp(TDI_SEND, pDeviceObject, pFileObject, NULL, NULL);
		if(NULL == pIrp)
			return STATUS_INSUFFICIENT_RESOURCES;
		pMdl = IoAllocateMdl(szData, strlen(szData)+1, FALSE, FALSE, NULL);
		if(NULL == pMdl)
			return STATUS_INSUFFICIENT_RESOURCES;

		__try {
			MmProbeAndLockPages(pMdl, KernelMode, IoModifyAccess);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			IoFreeMdl(pMdl);
			pMdl = NULL;
		};

		TdiBuildSend(pIrp, pDeviceObject, pFileObject, NULL, NULL, pMdl, 0, strlen(szData)+1);
		if(STATUS_SUCCESS != TdiCall(pIrp, pDeviceObject, &IoStatusBlock))
			return STATUS_INSUFFICIENT_RESOURCES;
	}

	return STATUS_SUCCESS;
}
