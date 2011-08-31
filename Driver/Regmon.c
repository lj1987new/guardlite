/*
 *	×¢²á±í¼à¿ØÄ£¿é
 */
#include "GuardLite.h"


NTSTATUS	RegmonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	return STATUS_SUCCESS;
}

void		RegmonUnload()
{

}

NTSTATUS	RegmonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	return STATUS_SUCCESS;
}
