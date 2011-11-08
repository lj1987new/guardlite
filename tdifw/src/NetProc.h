#pragma once


NTSTATUS		DriverEntry(IN PDRIVER_OBJECT DriverObject
							, IN PUNICODE_STRING RegistryPath);
VOID			OnUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS		DeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);
