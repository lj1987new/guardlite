#include <ntddk.h>
#include "NetProc.h"
#include "tdifw/tdi_fw_lib.h"

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
						   IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS		status;
	int				i;

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = DeviceDispatch;

	status = tdifw_DriverEntry(DriverObject, RegistryPath);
	if(!NT_SUCCESS(status))
		return status;

#if defined(DBG)
	DriverObject->DriverUnload = OnUnload;
#endif
// 	i = 0;
// 	i = 254 / i;
// 	KdPrint(("[DriverEntry] %d", i));
	return STATUS_SUCCESS;
}

VOID OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	tdifw_OnUnload(DriverObject);
}

NTSTATUS DeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
	BOOLEAN		bDispath		= FALSE;
	NTSTATUS	status;

	// 调用tdifw的事件处理函数
	status = tdifw_DeviceDispatch(DeviceObject, irp, &bDispath);
	if(FALSE != bDispath)
		return status;
	// 自己的处理函数

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}
/*
 *	事件处理回调函数 
 */
int		tdifw_filter(struct flt_request *request)
{
	int			i;

	if(NULL == request)
		return FILTER_ALLOW;
	if(TYPE_RECV == request->type)
	{
		__try{
			KdPrint(("[receive] %s\n", request->data.pdata));
			for(i = 0; i < request->data.len; i++)
			{
				if(strncmp("xxj", (char*)request->data.pdata + i, 3) == 0)
				{
					memcpy((char *)request->data.pdata + i, "***", 3);
					i += 2;
				}
			}
 		}
		__except(1)
		{
			KdPrint(("tdifw_filter bad receive.\n"));
			return FILTER_ALLOW;
		}
	}
	return FILTER_ALLOW;
}