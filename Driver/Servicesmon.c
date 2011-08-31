/*
 *	·þÎñ¼à¿ØÄ£¿é
 */
#include "GuardLite.h"

NTSTATUS	ServicesEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	return STATUS_SUCCESS;
}

void		ServicesUnload()
{

}

NTSTATUS	ServicesDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	return STATUS_SUCCESS;
}
