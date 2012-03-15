/*
 *	TDI 扩展功能
 */
#include "EhomeNet.h"
#include "EhomeDevCtl.h"
#include <TdiKrnl.h>
#include "Keyword.h"
#include "TdiFileObjectContext.h"

typedef struct _tdi_client_irp_ctx {
	PIO_COMPLETION_ROUTINE	completion;
	PVOID					context;
	UCHAR					old_control;
	PFILE_OBJECT            connobj;
}tdi_client_irp_ctx;

NTSTATUS EHomeClientEventReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
								 , IN ULONG  ReceiveFlags, IN ULONG  BytesIndicated, IN ULONG  BytesAvailable
								 , OUT ULONG  *BytesTaken, IN PVOID  Tsdu, OUT PIRP  *IoRequestPacket);
NTSTATUS EHomeClientEventChainedReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
								 , IN ULONG  ReceiveFlags, IN ULONG  ReceiveLength, IN ULONG  StartingOffset
								 , IN PMDL  Tsdu, IN PVOID  TsduDescriptor);

void EHomeReplaceKeyword(IN PVOID pData, IN ULONG nLen);
NTSTATUS tdi_close_connect(PFILE_OBJECT pFileObject);

// 设置事件句柄
NTSTATUS		EHomeTDISetEventHandler(PIRP pIrp, PIO_STACK_LOCATION pStack)
{
	PTDI_REQUEST_KERNEL_SET_EVENT			pTdiEvent			= NULL;
	tdi_foc_ptr						pSocketContext		= NULL;

	pTdiEvent = (PTDI_REQUEST_KERNEL_SET_EVENT)&pStack->Parameters;
	pSocketContext = tdi_foc_GetAddress(pStack->FileObject, TRUE);
	if(NULL == pSocketContext || NULL == pTdiEvent)
	{
		KdPrint(("[EHomeTDISetEventHandler] pSocketContext: %d, pTdiEvent: %d\n", pSocketContext, pTdiEvent));
		return STATUS_SUCCESS;
	}

	if(TDI_EVENT_RECEIVE == pTdiEvent->EventType)
	{
		pSocketContext->event_receive_handler = pTdiEvent->EventHandler;
		pSocketContext->event_receive_context = pTdiEvent->EventContext;
		if(NULL != pTdiEvent->EventHandler)
		{
			pTdiEvent->EventHandler = EHomeClientEventReceive;
			pTdiEvent->EventContext = pStack->FileObject;
		}
		KdPrint(("[EHomeTDISetEventHandler] TDI_EVENT_RECEIVE pTdiEvent->EventHandler: %d\n", pTdiEvent->EventHandler));
	}
	else if(TDI_EVENT_CHAINED_RECEIVE == pTdiEvent->EventType)
	{
		pSocketContext->event_chained_handler = pTdiEvent->EventHandler;
		pSocketContext->event_chained_context = pTdiEvent->EventContext;
 		if(NULL != pTdiEvent->EventHandler)
		{
			pTdiEvent->EventHandler = EHomeClientEventChainedReceive;
			pTdiEvent->EventContext = pStack->FileObject;
 		}
		KdPrint(("[EHomeTDISetEventHandler] TDI_EVENT_CHAINED_RECEIVE pTdiEvent->EventHandler: %d\n", pTdiEvent->EventHandler));
	}

	return STATUS_SUCCESS;
}

// 接收客户端
NTSTATUS EHomeClientEventReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
								 , IN ULONG  ReceiveFlags, IN ULONG  BytesIndicated, IN ULONG  BytesAvailable
								 , OUT ULONG  *BytesTaken, IN PVOID  Tsdu, OUT PIRP  *IoRequestPacket)
{
	tdi_foc_ptr						pSocketContext		= NULL;
	NTSTATUS								status				= STATUS_SUCCESS;
	char*									pData				= NULL;
	BOOLEAN									bContinue			= TRUE;

	pSocketContext = tdi_foc_GetAddress((PFILE_OBJECT)TdiEventContext, FALSE);
	if(NULL == pSocketContext || NULL == pSocketContext->event_receive_handler)
	{
		KdPrint(("[EHomeClientEventReceive] pSocketContext: %d\n", pSocketContext));
		return STATUS_SUCCESS;
	}
	KdPrint(("[EHomeClientEventReceive] len:%d, data: %s\n", BytesIndicated, Tsdu));
	if( FALSE != pSocketContext->bIsHttp && 0 != gEHomeFilterRule.rule )
	{
		EHomeFilterRecvData( Tsdu, BytesIndicated, &bContinue );
		if( FALSE == bContinue )
			return STATUS_SUCCESS;
	}
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
	// 设置完成端口
	if(NULL != *IoRequestPacket)
	{
		PIO_STACK_LOCATION		irps = IoGetCurrentIrpStackLocation(*IoRequestPacket);
		tdi_client_irp_ctx*		new_ctx = (tdi_client_irp_ctx *)ExAllocatePoolWithTag(NonPagedPool, sizeof(tdi_client_irp_ctx), 'ehom');

		if(NULL != new_ctx)
		{
			new_ctx->connobj = pSocketContext->pAddressFileObj;

			if (irps->CompletionRoutine != NULL) {
				new_ctx->completion = irps->CompletionRoutine;
				new_ctx->context = irps->Context;
				new_ctx->old_control = irps->Control;

			} else {

				// we don't use IoSetCompletionRoutine because it uses next not current location

				new_ctx->completion = NULL;
				new_ctx->context = NULL;

			}

			irps->CompletionRoutine = tdi_client_irp_complete;
			irps->Context = new_ctx;
			irps->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
		}
	}
	return status;
}

NTSTATUS tdi_client_irp_complete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	tdi_client_irp_ctx *ctx = (tdi_client_irp_ctx *)Context;
	NTSTATUS status;

	if (Irp->IoStatus.Status == STATUS_SUCCESS) 
	{
		PVOID		pData			= MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
		BOOLEAN		bContinue		= TRUE;	
		
		KdPrint(("[tdi_client_irp_complete] len:%d, data: %s\n", Irp->IoStatus.Information, pData));
		EHomeFilterRecvData(pData, Irp->IoStatus.Information, &bContinue);
		if(FALSE == bContinue)
		{
			Irp->IoStatus.Information = 0;
			Irp->IoStatus.Status = STATUS_INVALID_CONNECTION;
			return STATUS_SUCCESS;
		}
	}
	if(NULL == ctx)
		return STATUS_SUCCESS;	// 没有上下文时退出
	// call original completion
	if (ctx->completion != NULL) 
	{
		// call old completion (see the old control)
		BOOLEAN b_call = FALSE;

		if (Irp->Cancel) 
		{
			// cancel
			if (ctx->old_control & SL_INVOKE_ON_CANCEL)
				b_call = TRUE;
		} 
		else 
		{
			if (Irp->IoStatus.Status >= STATUS_SUCCESS) 
			{
				// success
				if (ctx->old_control & SL_INVOKE_ON_SUCCESS)
					b_call = TRUE;
			} else 
			{
				// error
				if (ctx->old_control & SL_INVOKE_ON_ERROR)
					b_call = TRUE;
			}
		}

		if (b_call) 
		{
			status = (ctx->completion)(DeviceObject, Irp, ctx->context);

			KdPrint(("[tdi_flt] tdi_client_irp_complete: original handler: 0x%x; status: 0x%x\n",
				ctx->completion, status));
		} 
		else
		{
			status = STATUS_SUCCESS;
		}
	}

	ExFreePoolWithTag(ctx, 'ehom');
	return status;
}

NTSTATUS EHomeClientEventChainedReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
										, IN ULONG  ReceiveFlags, IN ULONG  ReceiveLength, IN ULONG  StartingOffset
										, IN PMDL  Tsdu, IN PVOID  TsduDescriptor)
{
	tdi_foc_ptr						pSocketContext		= NULL;
	NTSTATUS								status				= STATUS_SUCCESS;
	char*									pData				= NULL;
	BOOLEAN									bContinue			= TRUE;

	pSocketContext = tdi_foc_GetAddress((PFILE_OBJECT)TdiEventContext, FALSE);
	if(NULL == pSocketContext || NULL == pSocketContext->event_receive_handler)
	{
		KdPrint(("[EHomeClientEventChainedReceive] pSocketContext: %d\n", pSocketContext));
		return STATUS_SUCCESS;
	}
	if(NULL != Tsdu)
	{
		pData = (char *)MmGetSystemAddressForMdlSafe(Tsdu, NormalPagePriority);
		KdPrint(("[EHomeClientEventChainedReceive] %s \n", pData));
	}
	KdPrint(("[EHomeClientEventChainedReceive] len:%d, data: %s\n", ReceiveLength, (char *)pData + StartingOffset));
	if( FALSE != pSocketContext->bIsHttp && 0 != gEHomeFilterRule.rule )
	{
		EHomeFilterRecvData((char *)pData + StartingOffset, ReceiveLength, &bContinue);
		if( FALSE == bContinue )
		{
			tdi_close_connect(pSocketContext->pAddressFileObj);
			return STATUS_SUCCESS;
		}
	}
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

/*替换关键字*/
void EHomeReplaceKeyword(IN PVOID pData, IN ULONG nLen)
{
	__try
	{
		char*			pKeyWord		= (char *)pData;
		int				nKeywordLen		= 0;
		int				nSurplusLen		= nLen;
		int				i;

		while( keyword_Find(pKeyWord, nSurplusLen, &pKeyWord, &nKeywordLen) )
		{
			for(i = 0; i < nKeywordLen; i++)
				pKeyWord[i] = '*';
			pKeyWord += 4;
			nSurplusLen = nLen - ( pKeyWord - (char *)pData);
			if(nSurplusLen < 0)
				break;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("[EHomeReplaceKeyword] EXCEPTION_EXECUTE_HANDLER\n"));
	}
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

/*过滤关键字*/
void EHomeFilterRecvData(IN PVOID pData, IN ULONG nLen, OUT BOOLEAN* pbContinue)
{
	*pbContinue = TRUE;
	if(gEHomeFilterRule.rule > 0)
	{
		EHomeReplaceKeyword(pData, nLen);
	}
	else if(gEHomeFilterRule.rule < 0)
	{
		char*					pKeyword			= NULL;
		int						nKeywordLen			= 0;
		PIRP					pIrp				= NULL;
		IO_STATUS_BLOCK			IoStatusBlock		= {0};

		if( FALSE == keyword_Find(pData, nLen, &pKeyword, &nKeywordLen) )
			return;
 		// 断开连接
		*pbContinue = FALSE;
	}
}

/* 关闭连接 */
NTSTATUS tdi_close_connect(PFILE_OBJECT pFileObject)
{
// 	KEVENT						Event;
// 	NTSTATUS					Status;
// 	PDEVICE_OBJECT				DeviceObject;
// 	IO_STATUS_BLOCK				IoStatus			= {0}; 
// 	PIRP						Irp;
// 
// 	KeInitializeEvent(&Event, NotificationEvent, FALSE); 
// 	DeviceObject = IoGetRelatedDeviceObject(pFileObject); 
// 	Irp = TdiBuildInternalDeviceControlIrp(TDI_DISCONNECT, DeviceObject, pFileObject, NULL, NULL); 
// 	if (Irp == 0)
// 		return STATUS_INSUFFICIENT_RESOURCES; 
// 	TdiBuildDisconnect(Irp, DeviceObject, pFileObject, 0, 0, 0, TDI_DISCONNECT_RELEASE, 0, 0); 
// 	Status = Status = IoCallDriver(DeviceObject, Irp);
// 
// 	return Status == STATUS_SUCCESS ? IoStatus.Status : Status; 

	return STATUS_SUCCESS;
}
