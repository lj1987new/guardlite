/*
*	½ø³Ì¼à¿ØÄ£¿é
*/
#include "GuardLite.h"
#include "Regmon.h"

NTSTATUS	ProcmonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	return STATUS_SUCCESS;
}

void		ProcmonUnload()
{

}

NTSTATUS	ProcmonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	return STATUS_SUCCESS;
}
