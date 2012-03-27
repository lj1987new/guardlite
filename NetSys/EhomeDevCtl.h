#pragma once


//////////////////////////////////////////////////////////////////////////
// 控制命令
/*
 *	得到请求的信息
 *  接收数据： (URLINFO) 一个URLINFO结构体
 */
#define IOCTL_GET_DNS_INFO			CTL_CODE(FILE_DEVICE_UNKNOWN, \
			0x0800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
/*
 *	控制永允或拒绝
 *  传入数据: (LONG) 0(阻止)或1(放行)
 */
#define IOCTL_DNS_ALLOW_OR_NOT		CTL_CODE(FILE_DEVICE_UNKNOWN,\
            0x0801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
/*
 *	设置监控事件
 *  传入数据: (HANDEL) 外部事件句柄
 */
#define IOCTL_DNS_SETEVENT			CTL_CODE(FILE_DEVICE_UNKNOWN,\
            0x0802, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
/*
 *	新增断网事件
 *  传入参数: (CTRLNETWORK)
 *  传出参数: (LONG) 返回断网状态, 0:处理断网状态, 1:处理连接状态
 */
#define IOCTL_CONTROL_NETWORK		CTL_CODE(FILE_DEVICE_UNKNOWN,\
			0x0803, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
 *	新增方法，清除缓存
 */
#define IOCTL_CONTROL_CLEARCACHE	CTL_CODE(FILE_DEVICE_UNKNOWN,\
		0x0804, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
 *	设置关键字过滤规则
 *  <0, 发现关键字断开网络
 *  >0, 发现关键字替换为*号
 *  0, 停止关键字过滤, 默认是关闭的
 */
#define IOCTL_CONTROL_FILTER_RULE	CTL_CODE(FILE_DEVICE_UNKNOWN,\
		0x0805, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
 *	添加关键字过滤
 *  设置要过滤的关键字
 */
#define IOCTL_CONTROL_FILTER_ADDKEYWORD	CTL_CODE(FILE_DEVICE_UNKNOWN,\
		0x0806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
 *	清除规则
 */
#define IOCTL_CONTROL_FILTER_CLEARKEYWORD	CTL_CODE(FILE_DEVICE_UNKNOWN,\
		0x0807, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
 *	设置关键字断网提示事件
 *  传入数据: (HANDEL) 外部事件句柄
 */
#define IOCTL_CONTROL_FILTER_SETEVENT		CTL_CODE(FILE_DEVICE_UNKNOWN,\
		0x0808, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
 *	获取一个断网提示关键字
 *  传出参数: Filter_block结构
 */
#define IOCTL_CONTROL_FILTER_GET_BLOCK		CTL_CODE(FILE_DEVICE_UNKNOWN,\
		0x0809, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
 *	重定向阻止网页到指定的页面
 *  传出参数: 转向指定页的主机头
 */
#define IOCTL_DNS_REDIRECT					CTL_CODE(FILE_DEVICE_UNKNOWN,\
		0x0810, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
//////////////////////////////////////////////////////////////////////////
#define NAMELENGTH     128
#pragma pack(push, 1)
// 产生一个URL请求是的数据结构, IOCTL_GET_DNS_INFO接收数据
typedef struct _URLInfo{
	ULONGLONG	PID;					// 进程PID, 兼容64位
	ULONG		bHasInline;				// 是否内连接，0表示不是内连,1表示是内连
	ULONG		nPort;					// 端口号
	CHAR		szUrl[NAMELENGTH];		// HOST主机头
	CHAR		szUrlPath[1024];		// URL后面的中径
} URLINFO;

// 断网事件传入参数
typedef struct _CtrlNetwork{
	LONG		code;					// 0: 断开网络, 1: 连接网络, -1: 查询状态
	CHAR		szPaseProc[1];			// 跳过监控进程名字列表, 格式"|proc1.exe|proc2.exe|...\0"最大长度1020
} CTRLNETWORK;

// 关键字过滤结构
typedef struct _FilterKeywordBlock{
	LONG		rule;					// >0: 替换, <0: 阻止, 0: 无效
	LONG		bInline;				// 是否是内连， 不一定正确
	ULONGLONG	pid;					// 进程ID
	ULONG		nPort;					// 端口号
	CHAR		szHost[128];			// 截获的HOST
} FILTERKEYWORDBLOCK;
#pragma pack(pop)

