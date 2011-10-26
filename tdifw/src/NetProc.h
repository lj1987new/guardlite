#pragma once


NTSTATUS		DriverEntry(IN PDRIVER_OBJECT theDriverObject
							, IN PUNICODE_STRING theRegistryPath);
VOID			OnUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS		DeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);
