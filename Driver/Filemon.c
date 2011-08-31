/*
 *	ÎÄ¼þ¼à¿ØÄ£¿é
 */
#include "GuardLite.h"

NTSTATUS	FilemonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	return STATUS_SUCCESS;
}

void		FilemonUnload()
{

}

NTSTATUS	FilemonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	return STATUS_SUCCESS;
}
