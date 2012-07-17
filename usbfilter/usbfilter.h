#include <wdm.h>
#include <usb.h>

typedef struct _device_extension
{
	IO_REMOVE_LOCK			remove_lock;
	PDEVICE_OBJECT			device_object;
	PDEVICE_OBJECT			pdo;
	PDEVICE_OBJECT			lower_device_object;
}device_extension, *device_extension_ptr;

void		ufd_unload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS	ufd_add_device(IN PDRIVER_OBJECT DriverObject, 
						   IN PDEVICE_OBJECT pdo);

NTSTATUS	ufd_dispatch_default(IN PDEVICE_OBJECT device_object, IN PIRP irp);
NTSTATUS	ufd_dispatch_power(IN PDEVICE_OBJECT device_object, IN PIRP irp);
NTSTATUS	ufd_dispatch_pnp(IN PDEVICE_OBJECT device_object, IN PIRP irp);
NTSTATUS	ufd_dispatch_pnp_start_device(IN PDEVICE_OBJECT device_object, 
										  IN PIRP irp);
NTSTATUS	ufd_dispatch_scsi(IN PDEVICE_OBJECT device_object, IN PIRP irp);
NTSTATUS	ufd_dispatch_internal_device_control(IN PDEVICE_OBJECT device_object, 
												 IN PIRP irp);

NTSTATUS	ufd_completion_usage_notification(IN PDEVICE_OBJECT device_object, 
											  IN PIRP irp, IN PVOID Context);
NTSTATUS	ufd_completion_start_device(IN PDEVICE_OBJECT device_object, 
										IN PIRP irp, IN PVOID Context);
NTSTATUS	ufd_completion_scsi(IN PDEVICE_OBJECT device_object, 
								IN PIRP irp, IN PVOID Context);
NTSTATUS	ufd_completion_internal_device_control(IN PDEVICE_OBJECT device_object, 
								IN PIRP irp, IN PVOID Context);

NTSTATUS	ufd_get_usb_info(PDEVICE_OBJECT fdo, USHORT* pvid, USHORT* ppid, 
							 WCHAR* pmanuf, WCHAR* pproduct, WCHAR* psn);
NTSTATUS	ufd_get_usb_class(PDEVICE_OBJECT fdo, UCHAR* pclass);
NTSTATUS	ufd_check_usb_class(UCHAR uclass);
NTSTATUS	ufd_check_usb_skip(USHORT vid, USHORT pid, WCHAR* pmanuf, 
							   WCHAR* pproduct, WCHAR* psn);

NTSTATUS	ufd_CallUSBD(IN PDEVICE_OBJECT fdo, IN PURB Urb);
void		ufd_driver_removedevice(IN PDEVICE_OBJECT device_object);
ULONG		ufd_get_device_type(PDEVICE_OBJECT pdo);



