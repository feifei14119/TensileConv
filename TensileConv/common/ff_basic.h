#pragma once

#include <string>

namespace feifei
{
#ifdef _WIN32
#define	DIR_SPT ('\\')
#else
#define	DIR_SPT ('/')
#endif

	typedef enum DataTypeEnum
	{
		Float,
		Int,
		Fp32,
		Fp64,
		String,
		UInt8,
		Int8,
		Uint16,
		Int16,
		Int32
	} E_DataType;

	typedef enum class ReturnStateEnum
	{
		SUCCESS = 0,
		RTN_WARN = 1,	// 警告,继续执行
		RTN_ERR = 2,	// 错误, 抛出异常并退出此函数
		RTN_FATAL = 3	// 失败, 终止程序
	} E_ReturnState;

	typedef void(*PVoidFunc)();
	typedef E_ReturnState(*PRetFunc)();
	typedef E_ReturnState(*PRetFunc2)(void* param);

#define ChkErr(val) do{\
	E_ReturnState val_hold = val; \
	if(val_hold == E_ReturnState::RTN_ERR) return val;\
	if(val_hold == E_ReturnState::RTN_FATAL) exit(EXIT_FAILURE);\
}while(0)

}
