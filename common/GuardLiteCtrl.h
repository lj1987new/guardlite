/*
 *	设备符号名: \\??\\GuardLite
 */
#pragma once

#ifndef _NTDDK_
#include <winioctl.h>
#endif

/*
 *	开启控制监控
 *
 *  输入LONG, 无
 *  输出LONG，无
 */
#define GUARDLITE_CTRL_START			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *	关闭控制监控
 *
 *  输入LONG, 无
 *  输出LONG，无
 */
#define GUARDLITE_CTRL_STOP				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *	查询驱动状态
 *  
 *  输入: 无
 *  输出: 0，未开启监控  1, 关闭监控
 */
#define GUARDLITE_CTRL_STATUS			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *	获取监控到的请求
 *
 *  输入: 无
 *  输出: GUARDLITEREQUEST 结构
 */
#define GUARDLITE_CTRL_REQUEST			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *	设置监控到的请求
 *
 *  输入: GUARDLITERERESPONSE 结构
 *  输出: 无
 */
#define GUARDLITE_CTRL_RESPONSE			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 注册表监控
#define MASK_GUARDLITE_REGMON			0x1
// 文件监控
#define MASK_GUARDLITE_FILEMON			0x2
// 服务监控
#define MASK_GUARDLITE_SERVICESMON		0x4

#pragma pack(push, 1)

// Read结构体
typedef struct _GuardLiteRequest{
	ULONG		dwMonType;			// 监控类型
	ULONG		dwRequestID;		// 此次监控的ID标识
	ULONG		dwProcessID;		// 目标进程ID
	WCHAR		szPath[256];		// 监控目录
	WCHAR		szSubPath[512];		// 监控子目录
}GUARDLITEREQUEST, PGUARDLITEREQUEST;
// Write结构体
typedef struct _GuardLiteResponse{
	ULONG		dwReuestID;			// 请求的ID
	ULONG		bAllowed;			// 是否允许
}GUARDLITERERESPONSE, *PGUARDLITERERESPONSE;

#pragma pack(pop)

