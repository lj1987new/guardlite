#include "EhomeNet.h"
#include "Keyword.h"

extern URLINFO					HostInfo;
extern ULONG					IsHttpAllow;
extern PKEVENT					UrlAllowOrNotEvent;
extern PKEVENT					EhomeUsrEvent;
extern ULONG					IsHttpFilterOn;
extern PEPROCESS				gControlPID;
extern CTRLNETWORK*				gpCtrlNetWork;
extern URLINFO					storageurl;
extern CHAR*					gNetRedirectHead;
extern PKEVENT					gEHomeNewworkEvent;
/*
 *	得到请求的信息
 *  接收数据： (URLINFO) 一个URLINFO结构体
 */
NTSTATUS EHomeCtrlGetDnsInfo(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							 , IN PVOID pInBuff, IN ULONG nInSize
							 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	*pOutSize = min(*pOutSize, sizeof(HostInfo));
	memcpy(pOutBuff, &HostInfo, *pOutSize);

	return STATUS_SUCCESS;
}
/*
 *	控制永允或拒绝
 *  传入数据: (LONG) 0(阻止)或1(放行)
 */
NTSTATUS EHomeCtrlSetDnsAllowOrNot(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							 , IN PVOID pInBuff, IN ULONG nInSize
							 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	IsHttpAllow= *((ULONG*)pInBuff);
	KeSetEvent(UrlAllowOrNotEvent, 0, FALSE);

	return STATUS_SUCCESS;
}
/*
 *	设置监控事件
 *  传入数据: (HANDEL) 外部事件句柄
 */
NTSTATUS EHomeCtrlSetDnsEvent(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
								   , IN PVOID pInBuff, IN ULONG nInSize
								   , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	ULONGLONG		EventHandle			= 0;
	NTSTATUS		status;

	if(8 == nInSize)
	{
		EventHandle = *((PULONGLONG)pInBuff);
	}
	else
	{
		EventHandle = (ULONGLONG)*((PULONG)pInBuff);
	}

	status = ObReferenceObjectByHandle((PVOID)EventHandle, SYNCHRONIZE, *ExEventObjectType
		, pIrp->RequestorMode, (PVOID*)&EhomeUsrEvent, NULL);
	IsHttpFilterOn = TRUE;
	gControlPID = PsGetCurrentProcess();
	if(!NT_SUCCESS(status))
	{
		IsHttpFilterOn = FALSE;
	}

	return STATUS_SUCCESS;
}
/*
 *	新增断网事件
 *  传入参数: (CTRLNETWORK)
 *  传出参数: (LONG) 返回断网状态, 0:处理断网状态, 1:处理连接状态
 */
NTSTATUS EHomeCtrlSetNetwork(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							  , IN PVOID pInBuff, IN ULONG nInSize
							  , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	CTRLNETWORK*	pCtrl		= ((CTRLNETWORK *)pInBuff);

	if(nInSize < sizeof(CTRLNETWORK))
	{
		return STATUS_INVALID_BLOCK_LENGTH;
	}
	if(pCtrl->code > 0)
	{
		gpCtrlNetWork->code = 1;
	}
	else if(0 == pCtrl->code)
	{
		strncpy(gpCtrlNetWork->szPaseProc, pCtrl->szPaseProc, 1020);
		_strlwr(gpCtrlNetWork->szPaseProc);
		gpCtrlNetWork->code = 0;
	}
	if(*pOutSize >= 4)
	{
		*((LONG *)pOutBuff) = (0 != gpCtrlNetWork->code);
		*pOutSize = 4;
	}
	else
	{
		*pOutSize = 0;
	}
	return STATUS_SUCCESS;
}
/*
 *	新增方法，清除缓存
 */
NTSTATUS EHomeCtrlClearCache(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							 , IN PVOID pInBuff, IN ULONG nInSize
							 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	memset(&storageurl, 0, sizeof(storageurl));

	return STATUS_SUCCESS;
}
/*
 *	设置关键字过滤规则
 *  <0, 发现关键字断开网络
 *  >0, 发现关键字替换为*号
 *  0, 停止关键字过滤, 默认是关闭的
 */
NTSTATUS EHomeCtrlSetFilterRule(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							 , IN PVOID pInBuff, IN ULONG nInSize
							 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	if(nInSize >= 4)
	{
		gEHomeFilterRule.rule = *((int *)pInBuff);
	}

	return STATUS_SUCCESS;
}
/*
 *	添加关键字过滤
 *  设置要过滤的关键字
 */
NTSTATUS EHomeCtrlAddKeyword(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
								, IN PVOID pInBuff, IN ULONG nInSize
								, OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	if(nInSize > 0)
	{
		keyword_Add((char *)pInBuff, nInSize);
	}

	return STATUS_SUCCESS;
}
/*
 *	清除规则
 */
NTSTATUS EHomeCtrlClearKeyword(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							 , IN PVOID pInBuff, IN ULONG nInSize
							 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	keyword_Clear();

	return STATUS_SUCCESS;
}
/*
 *	设置关键字断网提示事件
 *  传入数据: (HANDEL) 外部事件句柄
 */
NTSTATUS EHomeCtrlSetFilterEvent(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							   , IN PVOID pInBuff, IN ULONG nInSize
							   , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	ULONGLONG		EventHandle			= 0;
	NTSTATUS		status;

	if(8 == nInSize)
		EventHandle = *((PULONGLONG)pInBuff);
	else
		EventHandle = (ULONGLONG)*((PULONG)pInBuff);

	status = ObReferenceObjectByHandle((PVOID)EventHandle, SYNCHRONIZE, *ExEventObjectType
		, pIrp->RequestorMode, (PVOID*)&gEHomeKeyword.noticeevent, NULL);
	if( !NT_SUCCESS(status) )
	{
		gEHomeKeyword.noticeevent = NULL;
	}

	return STATUS_SUCCESS;
}
/*
 *	获取一个断网提示关键字
 *  传出参数: Filter_block结构
 */
NTSTATUS EHomeCtrlGetFilterBlock(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
								 , IN PVOID pInBuff, IN ULONG nInSize
								 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	PFILTER_KEYWORD_BLOCK		pfkb		= NULL;
	PLIST_ENTRY					plist		= NULL;
	KIRQL						oldIrql;

	plist = ExInterlockedRemoveHeadList(&gEHomeKeyword.headlist, &gEHomeKeyword.spinlock);
	if(NULL != plist)
	{
		pfkb = CONTAINING_RECORD(plist, FILTER_KEYWORD_BLOCK, list);
		*pOutSize = min(*pOutSize, sizeof(FILTERKEYWORDBLOCK));
		memcpy(pOutBuff, &pfkb->fkl, *pOutSize);
		ExFreeToNPagedLookasideList(&gEHomeKeyword.lookaside, pfkb);
	}
	else
	{
		*pOutSize = 0;
		return STATUS_SUCCESS;
	}
	// 判断是否还有数据未处理完
	KeAcquireSpinLock(&gEHomeKeyword.spinlock, &oldIrql);
	if(FALSE == IsListEmpty(&gEHomeKeyword.headlist))
	{
		KeSetEvent(gEHomeKeyword.noticeevent, 0, FALSE);
	}
	KeReleaseSpinLock(&gEHomeKeyword.spinlock, oldIrql);

	return STATUS_SUCCESS;
}
/*
 *	重定向阻止网页到指定的页面
 *  传出参数: 转向指定页的主机头
 */
NTSTATUS EHomeCtrlSetDnsRedirect(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
								 , IN PVOID pInBuff, IN ULONG nInSize
								 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	RtlCopyMemory(gNetRedirectHead, pInBuff, min(1023, nInSize));
	IsHttpAllow = FALSE;
	KeSetEvent(UrlAllowOrNotEvent, 0, FALSE);

	return STATUS_SUCCESS;
}
/*
 *	设置断网通知事件
 *  传入数据: (HANDEL) 外部事件句柄
 */
NTSTATUS EHomeCtrlSetNetworkEvent(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
								 , IN PVOID pInBuff, IN ULONG nInSize
								 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	ULONGLONG		EventHandle			= 0;

	if(8 == nInSize)
	{
		EventHandle = *((PULONGLONG)pInBuff);
	}
	else
	{
		EventHandle = (ULONGLONG)*((PULONG)pInBuff);
	}

	ObReferenceObjectByHandle((PVOID)EventHandle, SYNCHRONIZE, *ExEventObjectType
		, pIrp->RequestorMode, (PVOID*)&gEHomeNewworkEvent, NULL);

	return STATUS_SUCCESS;
}
//////////////////////////////////////////////////////////////////////////
// 定义控制代码回调函数
EHOME_CONTROL_CALLBACK	gEHomeControlCallback[] = {
	{ IOCTL_GET_DNS_INFO , EHomeCtrlGetDnsInfo }
	, { IOCTL_DNS_ALLOW_OR_NOT, EHomeCtrlSetDnsAllowOrNot }
	, { IOCTL_DNS_SETEVENT, EHomeCtrlSetDnsEvent }
	, { IOCTL_CONTROL_NETWORK, EHomeCtrlSetNetwork }
	, { IOCTL_CONTROL_CLEARCACHE, EHomeCtrlClearCache }
	, { IOCTL_CONTROL_FILTER_RULE, EHomeCtrlSetFilterRule }
	, { IOCTL_CONTROL_FILTER_ADDKEYWORD, EHomeCtrlAddKeyword }
	, { IOCTL_CONTROL_FILTER_CLEARKEYWORD, EHomeCtrlClearKeyword }
	, { IOCTL_CONTROL_FILTER_SETEVENT, EHomeCtrlSetFilterEvent }
	, { IOCTL_CONTROL_FILTER_GET_BLOCK, EHomeCtrlGetFilterBlock }
	, { IOCTL_DNS_REDIRECT, EHomeCtrlSetDnsRedirect }
	, { IOCTL_CONTROL_NETWORK_SETEVENT, EHomeCtrlSetNetworkEvent }
	, { 0, NULL }
};

