#include <ntddk.h>
#include "NetProc.h"
#include "tdifw/tdi_fw_lib.h"

NTSTATUS DriverEntry(IN PDRIVER_OBJECT theDriverObject,
						   IN PUNICODE_STRING theRegistryPath)
{
	return STATUS_SUCCESS;
}

VOID OnUnload(IN PDRIVER_OBJECT DriverObject)
{

}

NTSTATUS DeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
	return STATUS_SUCCESS;
}