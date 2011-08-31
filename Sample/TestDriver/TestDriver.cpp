// TestDriver.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "TestDriver.h"
#include "TestDriverDlg.h"
#include "../../common/GuardLiteCtrl.h"
#include <Psapi.h>

#pragma comment(lib, "Psapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestDriverApp

BEGIN_MESSAGE_MAP(CTestDriverApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CTestDriverApp 构造

CTestDriverApp::CTestDriverApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CTestDriverApp 对象

CTestDriverApp theApp;


// CTestDriverApp 初始化

BOOL CTestDriverApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));
	//////////////////////////////////////////////////////////////////////////
	// 打开驱动
	HANDLE			hGuardLite			= NULL;
	DWORD			dwRead				= 0;

	hGuardLite = CreateFile(_T("\\\\.\\GuardLite")
		, GENERIC_READ|GENERIC_WRITE
		, 0
		, NULL
		, OPEN_EXISTING
		, FILE_ATTRIBUTE_NORMAL
		, NULL);
	if(NULL == hGuardLite || INVALID_HANDLE_VALUE == hGuardLite)
	{
		CString				str;

		str.Format(_T("打开服务失败: %d"), GetLastError());
		MessageBox(NULL, str.GetBuffer(), _T("错误"), MB_ICONERROR | MB_OK);
		return FALSE;
	}
	// 开启服务
	DeviceIoControl(hGuardLite, GUARDLITE_CTRL_START, NULL, 0, NULL, 0, &dwRead, NULL);

	while(true)
	{
		GUARDLITEREQUEST			request		= {0};
		GUARDLITERERESPONSE			response	= {0};

		if(FALSE == DeviceIoControl(hGuardLite, GUARDLITE_CTRL_REQUEST
			, NULL, 0, &request, sizeof(request), &dwRead, NULL))
		{
			CString				str;

			str.Format(_T("读取数据失败: %d"), GetLastError());
			MessageBox(NULL, str.GetBuffer(), _T("错误"), MB_ICONERROR | MB_OK);
			break;
		}
		response.dwReuestID = request.dwRequestID;
		// 显示提示框
		CTestDriverDlg			dlg;
		HANDLE					hProc;
		TCHAR					szPath[MAX_PATH]		= {0};
		TCHAR					szType[32]				= {0};

		switch(request.dwMonType)
		{
		case 0x1:
			_tcscpy(szType, _T("注册表"));
			break;
		case 0x2:
			_tcscpy(szType, _T("文件"));
			break;
		case 0x4:
			_tcscpy(szType, _T("服务"));
			break;
		default:
			_tcscpy(szType, _T("未知"));
			break;
		}

		hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, request.dwProcessID);
		GetProcessImageFileName(hProc, szPath, MAX_PATH);
		CloseHandle(hProc);
		//m_pMainWnd = &dlg;

		dlg.m_strInfo.Format(_T("监控类型: %s\r\n进程名: %s\r\n监控路径: %s\r\n监控子目录: %s\r\n")
			, szType
			, szPath
			, request.szPath
			, request.szSubPath);

		INT_PTR nResponse = dlg.DoModal();
		//m_pMainWnd = NULL;
		if (nResponse == IDOK)
		{
			response.bAllowed = TRUE;
		}
		
		DeviceIoControl(hGuardLite, GUARDLITE_CTRL_RESPONSE, &response, sizeof(response)
			, NULL, 0, &dwRead, NULL);
	}
	
	CloseHandle(hGuardLite);
	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}
