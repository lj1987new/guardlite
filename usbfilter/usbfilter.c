#include "usbfilter.h"
#include <srb.h>
#include <scsi.h>
#include <usbdi.h>
#include <usbdlib.h>

/*
 *	驱动入口函数
 */
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	int				i;

	KdPrint(("usbfilter.sys!!! DriverEnter\n"));

	pDriverObj->DriverUnload = ufd_unload;
	pDriverObj->DriverExtension->AddDevice = ufd_add_device;
	
	for(i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriverObj->MajorFunction[i] = ufd_dispatch_default;
	}

	pDriverObj->MajorFunction[IRP_MJ_POWER] = ufd_dispatch_power;
	pDriverObj->MajorFunction[IRP_MJ_PNP] = ufd_dispatch_pnp;
	pDriverObj->MajorFunction[IRP_MJ_SCSI] = ufd_dispatch_scsi;

	return STATUS_SUCCESS;
}

/*
 *	御载函数
 */
void ufd_unload(IN PDRIVER_OBJECT DriverObject)
{
	KdPrint(("usbfilter.sys!!! uf_driver_unload\n"));
}

/*
 *	添加新设备
 */
NTSTATUS ufd_add_device(IN PDRIVER_OBJECT DriverObject, 
						IN PDEVICE_OBJECT pdo)
{
	NTSTATUS							status;
	PDEVICE_OBJECT						fido;
	device_extension_ptr				pdx;
	PDEVICE_OBJECT						fdo;
// 	HANDLE								handle;
// 	URB									urb;
// 	//PUSB_STRING_DESCRIPTOR				pusd;
// 	PUSB_DEVICE_DESCRIPTOR				pudd;
// 	LONG								size;
// 	
// 	//获取设备信息
// 	pudd = ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_DEVICE_DESCRIPTOR), 'ehom');
// 	pudd->bLength = sizeof(USB_DEVICE_DESCRIPTOR);
// 	pudd->bDescriptorType = USB_STRING_DESCRIPTOR_TYPE;
// 	UsbBuildGetDescriptorRequest(&urb, 
// 		(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
// 		USB_STRING_DESCRIPTOR_TYPE, 0, 0, pudd, NULL, 
// 		 sizeof(USB_DEVICE_DESCRIPTOR), NULL);
// 	status = ufd_CallUSBD(pdo, &urb);
// 	KdPrint(("get urb retrun: %x\n", status));
// 	if( NT_SUCCESS(status) )
// 	{
// // 		size = pusd->bLength * sizeof(WCHAR) + sizeof(PUSB_STRING_DESCRIPTOR);
// // 		pusd = ExAllocatePoolWithTag(NonPagedPool, size, 'ehom');
// // 		RtlZeroMemory(pusd, size);
// // 		pusd->bLength = (size - sizeof(PUSB_STRING_DESCRIPTOR)) / 2;
// // 		pusd->bDescriptorType = USB_STRING_DESCRIPTOR_TYPE;
// // 		UsbBuildGetDescriptorRequest(&urb, 
// // 			(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
// // 			USB_STRING_DESCRIPTOR_TYPE, 0, 0, pusd, NULL, 
// // 			size, NULL);
// // 		status = ufd_CallUSBD(pdo, &urb);
// // 		if( NT_SUCCESS(status) )
// // 		{
// // 			KdPrint(("Get string Descriptor: %S\n", pusd->bString));
// // 		}
// 
// // 		size = pucd->wTotalLength;
// // 		ExFreePoolWithTag(pucd, 'ehom');
// // 		pucd = ExAllocatePoolWithTag(NonPagedPool, size, 'ehom');
// // 		UsbBuildGetDescriptorRequest(&urb, 
// // 			(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
// // 			USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pucd, NULL, 
// // 			size, NULL);
// // 		status = ufd_CallUSBD(pdo, &urb);
// 		
// 	}
// 
// 	ExFreePoolWithTag(pudd, 'ehom');
	// 创建设备
	KdPrint(("usbfilter.sys!!! ufd_add_device enter[%wZ]\n", &DriverObject->DriverName));
	status = IoCreateDevice(DriverObject, sizeof(device_extension), NULL,
		ufd_get_device_type(pdo), 0, FALSE, &fido);
	if( !NT_SUCCESS(status) )
	{
		KdPrint(("usbfilter.sys!!! create device failed: %d\n", status));
		return status;
	}

	pdx = (device_extension_ptr)fido->DeviceExtension;
	do 
	{
		IoInitializeRemoveLock(&pdx->remove_lock, 0, 0, 0);
		pdx->device_object = fido;
		pdx->pdo = pdo;
		
		fdo = IoAttachDeviceToDeviceStack(fido, pdo);
		if(!fdo)
		{
			KdPrint(("usbfilter.sys!!! attach to device failed\n"));
			status = STATUS_DEVICE_REMOVED;
			break;
		}

		pdx->lower_device_object = fdo;

		fido->Flags |= fdo->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO | DO_POWER_PAGABLE);
		fido->Flags &= ~DO_DEVICE_INITIALIZING;
	} while (FALSE);

	// 附加失败删除操作
	if( !NT_SUCCESS(status) )
	{
		if( pdx->lower_device_object )
		{
			IoDetachDevice(pdx->lower_device_object);
		}

		IoDeleteDevice(fido);
	}

	return status;
}
/*
 *	默认的分发函数
 */
NTSTATUS ufd_dispatch_default(IN PDEVICE_OBJECT device_object, IN PIRP irp)
{
	device_extension_ptr			pdx;
	PIO_STACK_LOCATION				stack;
	NTSTATUS						status;

	pdx = (device_extension_ptr)device_object->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(irp);
	KdPrint(("usbfilter.sys!!! [0x%x]ufd_dispatch_default: MJ(0x%x), MN(0x%x)\n",
		device_object, stack->MajorFunction, stack->MinorFunction));

	status = IoAcquireRemoveLock(&pdx->remove_lock, irp);
	if( !NT_SUCCESS(status) )
	{
		irp->IoStatus.Information = 0;
		irp->IoStatus.Status = status;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}

	IoSkipCurrentIrpStackLocation(irp);
	status = IoCallDriver(pdx->lower_device_object, irp);
	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return status;
}
/*
 *	电源管理分发函数
 */
NTSTATUS ufd_dispatch_power(IN PDEVICE_OBJECT device_object, IN PIRP irp)
{
	device_extension_ptr		pdx;
	NTSTATUS					status;

	KdPrint(("usbfilter.sys!!! [0x%x]ufd_dispatch_power enter\n", device_object));
	pdx = (device_extension_ptr)device_object->DeviceExtension;
	PoStartNextPowerIrp(irp);
	status = IoAcquireRemoveLock(&pdx->remove_lock, irp);
	if( !NT_SUCCESS(status) )
	{
		irp->IoStatus.Status = status;
		irp->IoStatus.Information = 0;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}

	IoSkipCurrentIrpStackLocation(irp);
	status = PoCallDriver(pdx->lower_device_object, irp);

	IoReleaseRemoveLock(&pdx->remove_lock, irp);
	return status;
}
/*
 *	对即插即用IRP进行处理
 */
NTSTATUS ufd_dispatch_pnp(IN PDEVICE_OBJECT device_object, IN PIRP irp)
{
	device_extension_ptr			pdx;
	PIO_STACK_LOCATION				stack;
	NTSTATUS						status;

	pdx = (device_extension_ptr)device_object->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(irp);
	KdPrint(("usbfilter.sys!!! [0xx%x]ufd_dispatch_pnp: MN(0x%x)\n", 
		device_object, stack->MinorFunction));
	
	status = IoAcquireRemoveLock(&pdx->remove_lock, irp);
	if( !NT_SUCCESS(status) )
	{
		irp->IoStatus.Information = 0;
		irp->IoStatus.Status = status;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		
		return status;
	}

	if(IRP_MN_START_DEVICE == stack->MinorFunction)
	{
		status = ufd_dispatch_pnp_start_device(device_object, irp);
		if( !NT_SUCCESS(status) )
		{
			IoReleaseRemoveLock(&pdx->remove_lock, irp);
			return status;
		}
	}

	if(IRP_MN_DEVICE_USAGE_NOTIFICATION == stack->MinorFunction)
	{
		if(!device_object->AttachedDevice || 
			(device_object->AttachedDevice->Flags & DO_POWER_PAGABLE))
		{
			device_object->Flags |= DO_POWER_PAGABLE;
		}

		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, ufd_completion_usage_notification, 
			(PVOID)pdx, TRUE, TRUE, TRUE);

		return IoCallDriver(pdx->lower_device_object, irp);
	}

	if(IRP_MN_START_DEVICE == stack->MinorFunction)
	{
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, ufd_completion_start_device,
			(PVOID)pdx, TRUE, TRUE, TRUE);
		
		return IoCallDriver(pdx->lower_device_object, irp);
	}

	if(IRP_MN_REMOVE_DEVICE == stack->MinorFunction)
	{
		IoSkipCurrentIrpStackLocation(irp);
		status = IoCallDriver(pdx->lower_device_object, irp);
		IoReleaseRemoveLockAndWait(&pdx->remove_lock, irp);

		ufd_driver_removedevice(device_object);
		return status;
	}

	IoSkipCurrentIrpStackLocation(irp);
	status = IoCallDriver(pdx->lower_device_object, irp);
	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return status;
}
/*
 *	开启设备
 */
NTSTATUS ufd_dispatch_pnp_start_device(IN PDEVICE_OBJECT device_object, 
									   IN PIRP irp)
{
	PDRIVER_OBJECT						pDriver;
	UNICODE_STRING						usName;
	device_extension_ptr				pdx;
	NTSTATUS							status;
	HANDLE								handle;
	URB									urb;
	PUSB_STRING_DESCRIPTOR				pusd;
	PUSB_DEVICE_DESCRIPTOR				pudd;
	LONG								size;

	pdx = (device_extension_ptr)device_object->DeviceExtension;
	//获取设备信息
	size = sizeof(USB_DEVICE_DESCRIPTOR);
	pudd = ExAllocatePoolWithTag(NonPagedPool, size, 'ehom');
	UsbBuildGetDescriptorRequest(&urb, 
		(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, pudd, NULL, 
		size, NULL);
 	status = ufd_CallUSBD(pdx->lower_device_object, &urb);
	KdPrint(("get urb retrun: %x\n", status));
	if( NT_SUCCESS(status) )
	{
		size = 1024;
		pusd = ExAllocatePoolWithTag(NonPagedPool, size, 'ehom');
		pusd->bLength = size;
		UsbBuildGetDescriptorRequest(&urb, 
			(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
			USB_STRING_DESCRIPTOR_TYPE, pudd->iSerialNumber, 936, pusd, NULL, 
			size, NULL);
		status = ufd_CallUSBD(pdx->lower_device_object, &urb);
		KdPrint(("serial number: %S\n", pusd->bString));
		ExFreePoolWithTag(pusd, 'ehom');
		// 取设备类型　
		{
			PUSB_INTERFACE_DESCRIPTOR		puid;
			PUSB_CONFIGURATION_DESCRIPTOR	pucd;

			size = sizeof(USB_CONFIGURATION_DESCRIPTOR);
			pucd = ExAllocatePoolWithTag(NonPagedPool, size, 'ehom');
			UsbBuildGetDescriptorRequest(&urb, 
				(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
				USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pucd, NULL, 
				size, NULL);
			status = ufd_CallUSBD(pdx->lower_device_object, &urb);
			if( NT_SUCCESS(status) )
			{
				size = pucd->wTotalLength;
				ExFreePoolWithTag(pucd, 'ehom');
				pucd = ExAllocatePoolWithTag(NonPagedPool, size, 'ehom');
				UsbBuildGetDescriptorRequest(&urb, 
					(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
					USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pucd, NULL, 
					size, NULL);
				status = ufd_CallUSBD(pdx->lower_device_object, &urb);
				if( NT_SUCCESS(status) )
				{
#define INTERFACENUMBER 0  
#define ALTERNATESETTING 1  
					puid = USBD_ParseConfigurationDescriptorEx(pucd, pucd, 
						INTERFACENUMBER,
						0/*ALTERNATESETTING*/,
						-1, -1, -1);
				}
			}
			ExFreePoolWithTag(pucd, 'ehom');
		}
	}

	ExFreePoolWithTag(pudd, 'ehom');
	// 过滤设置
	pDriver = pdx->lower_device_object->DriverObject;
	RtlInitUnicodeString(&usName, L"\\driver\\usbstor");
	KdPrint(("usbfilter.sys!!! IRP_MN_START_DEVICE: %wZ <=> %wZ\n", 
		&pDriver->DriverName, &usName));
	if( 0 == RtlCompareUnicodeString(&usName, &pDriver->DriverName, TRUE) )
	{
		// 获取设备名
		irp->IoStatus.Information = 0;
		irp->IoStatus.Status = STATUS_UNSUCCESSFUL;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}
/*
 *	SCSI例程
 */
NTSTATUS ufd_dispatch_scsi(IN PDEVICE_OBJECT device_object, IN PIRP irp)
{
	device_extension_ptr		pdx;
	PIO_STACK_LOCATION			stack;
	NTSTATUS					status;

	KdPrint(("usbfilter.sys!!! [0x%x]ufd_dispatch_scsi enter\n", device_object));
	pdx = (device_extension_ptr)device_object->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(irp);
	status = IoAcquireRemoveLock(&pdx->remove_lock, irp);
	if( !NT_SUCCESS(status) )
	{
		irp->IoStatus.Status = status;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}

	IoCopyCurrentIrpStackLocationToNext(irp);
	IoSetCompletionRoutine(irp, ufd_completion_scsi,
		NULL, TRUE, TRUE, TRUE);
	status = IoCallDriver(pdx->lower_device_object, irp);
	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return status;
}
/*
 *	IRP_MN_DEVICE_USAGE_NOTIFICATION
 */
NTSTATUS ufd_completion_usage_notification(IN PDEVICE_OBJECT device_object,
										   IN PIRP irp, IN PVOID Context)
{
	device_extension_ptr		pdx;

	pdx = (device_extension_ptr)Context;

	if(irp->PendingReturned)
	{
		IoMarkIrpPending(irp);
	}

	if(!(pdx->lower_device_object->Flags & DO_POWER_PAGABLE))
	{
		device_object->Flags &= ~DO_POWER_PAGABLE;
	}

	IoReleaseRemoveLock(&pdx->remove_lock, irp);
	return STATUS_SUCCESS;
}
/*
 *	IRP_MN_START_DEVICE
 */
NTSTATUS ufd_completion_start_device(IN PDEVICE_OBJECT device_object, 
									 IN PIRP irp, IN PVOID Context)
{
	device_extension_ptr		pdx;

	pdx = (device_extension_ptr)Context;
	if(irp->PendingReturned)
	{
		IoMarkIrpPending(irp);
	}

	if(pdx->lower_device_object->Characteristics & FILE_REMOVABLE_MEDIA)
	{
		device_object->Characteristics |= FILE_REMOVABLE_MEDIA;
	}

	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return STATUS_SUCCESS;
}
/*
 *	IRP_MJ_SCSI
 */
NTSTATUS ufd_completion_scsi(IN PDEVICE_OBJECT device_object, 
							 IN PIRP irp, IN PVOID Context)
{
	device_extension_ptr		pdx;
	PIO_STACK_LOCATION			stack;
	PSCSI_REQUEST_BLOCK			srb;
	PCDB						cdb;
	CHAR						code;

	pdx = (device_extension_ptr)device_object->DeviceExtension;
	IoAcquireRemoveLock(&pdx->remove_lock, irp);
	stack = IoGetCurrentIrpStackLocation(irp);
	
	srb = stack->Parameters.Scsi.Srb;
	cdb = (PCDB)srb->Cdb;
	code = cdb->CDB6GENERIC.OperationCode;
	
	KdPrint(("usbfilter.sys!!! [0x%x]ufd_completion_scsi code=0x%x\n", 
		device_object, code));
	if(SCSIOP_MODE_SENSE == code && srb->DataBuffer
		&& srb->DataTransferLength >= sizeof(MODE_PARAMETER_HEADER))
	{
		PMODE_PARAMETER_HEADER		mode;

		KdPrint(("usbfilter.sys!!! read only!\n"));
		// U盘只读
		mode = (PMODE_PARAMETER_HEADER)srb->DataBuffer;
		mode->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
	}

	if(irp->PendingReturned)
	{
		IoMarkIrpPending(irp);
	}

	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return irp->IoStatus.Status;
}
/*
 *	移除设备
 */
void ufd_driver_removedevice(IN PDEVICE_OBJECT device_object)
{
	device_extension_ptr		pdx;
	
	KdPrint(("usbfilter.sys!!! ufd_driver_removedevice\n"));
	pdx = (device_extension_ptr)device_object->DeviceExtension;
	if( pdx->lower_device_object )
	{
		IoDetachDevice(pdx->lower_device_object);
	}

	IoDeleteDevice(device_object);
}
/*
 *	获取设备的类型　
 */
ULONG ufd_get_device_type(PDEVICE_OBJECT pdo)
{
	PDEVICE_OBJECT			ldo;
	ULONG					type;

	ldo = IoGetAttachedDeviceReference(pdo);
	if(!ldo)
	{
		return FILE_DEVICE_UNKNOWN;
	}

	type = ldo->DeviceType;
	ObDereferenceObject(ldo);

	return type;
}

NTSTATUS ufd_CallUSBD(IN PDEVICE_OBJECT fdo, IN PURB Urb)
{  
   NTSTATUS					ntStatus, status = STATUS_SUCCESS;  
   PIRP						irp;  
   KEVENT					event;  
   IO_STATUS_BLOCK			ioStatus;  
   PIO_STACK_LOCATION		nextStack;  
  
     // issue a synchronous request (see notes above)  
   KeInitializeEvent(&event, NotificationEvent, FALSE);  
  
   irp = IoBuildDeviceIoControlRequest(  
             IOCTL_INTERNAL_USB_SUBMIT_URB,  
             fdo,  
             NULL,  
             0,  
             NULL,  
             0,  
             TRUE, /* INTERNAL */  
             &event,  
             &ioStatus);  
  
   // Prepare for calling the USB driver stack  
   nextStack = IoGetNextIrpStackLocation(irp);  
   ASSERT(nextStack != NULL);  
  
   // Set up the URB ptr to pass to the USB driver stack  
   nextStack->Parameters.Others.Argument1 = Urb;  
  
    ntStatus = IoCallDriver(fdo, irp);  
    
   if (ntStatus == STATUS_PENDING)  
   {  
      status = KeWaitForSingleObject(  
                    &event,  
                    Suspended,  
                    KernelMode,  
                    FALSE,  
                    NULL);  
   }  
   else  
   {  
      ioStatus.Status = ntStatus;  
   }  
  
   ntStatus = ioStatus.Status;  
  
   if (NT_SUCCESS(ntStatus))  
   {  
      if (!(USBD_SUCCESS(Urb->UrbHeader.Status)))  
         ntStatus = STATUS_UNSUCCESSFUL;  
   } 
  
   return ntStatus;  
}