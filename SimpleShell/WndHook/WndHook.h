#pragma once

#ifdef _WINDLL
#define DLLAPI		extern "C" _declspec(dllexport)
#else
#define DLLAPI		extern "C" _declspec(dllimport)
#endif

/*
 *	获取最后错误信息
 */
DLLAPI int WndHookGetLastError();

/*
 *	初始化窗口钩子
 */
DLLAPI BOOL WndHookInit();

/*
 *	注销窗口钩子
 */
DLLAPI BOOL WndHookDestroy();

#define ERROR_WNDHOOK_SUCCESS				0
#define ERROR_WNDHOOK_ALREADINIT			-1
#define ERROR_WNDHOOK_NOTINIT				-2