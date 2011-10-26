#include <ntddk.h>
#include "NetProc.h"
#include "tdifw/tdi_fw_lib.h"

NTSTATUS DriverEntry(IN PDRIVER_OBJECT theDriverObject,
						   IN PUNICODE_STRING theRegistryPath)
{
	tdifw_DriverEntry(theDriverObject, theRegistryPath);
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


	return STATUS_SUCCESS;
}
/*
 *	事件处理回调函数 
 */
int		tdifw_filter(struct flt_request *request)
{
	return FILTER_ALLOW;
}