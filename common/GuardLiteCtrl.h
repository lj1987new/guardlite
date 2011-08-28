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
 *  输入LONG, 控制类型flag，如果没有相应的FLAG开启监控将失败
 *  输出LONG，实际开启的flag位
 */
#define GUARDLITE_CTRL_START			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *	关闭控制监控
 *
 *  输入LONG, 控制类型flag，如果没有相应的FLAG关闭监控将不做处理，如果为0表示只查询
 *  输出LONG，关闭的监控后，实际开启的flag位
 */
#define GUARDLITE_CTRL_STOP				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 注册表监控
#define MASK_GUARDLITE_REGMON			0x1
// 文件监控
#define MASK_GUARDLITE_FILEMON			0x2
// 服务监控
#define MASK_GUARDLITE_SERVICESMON		0x4

#pragma pack(push, 1)
typedef struct _GuardLitePack{
	ULONG		dwMonType;			// 监控类型
	ULONG		dwPackID;			// 此次监控的ID标识
	ULONG		dwProcessID;		// 目标进程ID
	WCHAR		szPath[256];		// 监控目录
	WCHAR		szSubPath[512];		// 监控子目录
}GUARDLITEPACK;
#pragma pack(pop)

