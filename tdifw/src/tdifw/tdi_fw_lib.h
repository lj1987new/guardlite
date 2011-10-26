/*
 *	添加目录，把原TDIFW转化为LIB文件，供以后使用
 */
#pragma once
#include <ipc.h>

// 原DriverEntry改为tdifw_DriverEntry
NTSTATUS tdifw_DriverEntry(IN PDRIVER_OBJECT theDriverObject,
				  IN PUNICODE_STRING theRegistryPath);


// 原OnUnload改为tdifw_OnUnload
VOID tdifw_OnUnload(IN PDRIVER_OBJECT DriverObject);

// 原DeviceDispatch改为tdifw_DeviceDispatch
NTSTATUS tdifw_DeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);

// 新添加事件
int		tdifw_filter(struct flt_request *request);