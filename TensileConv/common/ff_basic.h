#pragma once

#include <string>

/************************************************************************/
/* 返回类型定义															*/
/************************************************************************/
typedef enum ReturnStateEnum
{
	SUCCESS = 0,
	FAIL = 1
} E_ReturnState;

static void checkFuncRet(E_ReturnState retVel, char const *const func, const char *const file, int const line)
{
	if (retVel != E_ReturnState::SUCCESS)
	{
		fprintf(stderr, "[!ERROR!] at %s:%d \"%s\" \n", file, line, func);

		exit(EXIT_FAILURE);
	}
}

static void checkErrNum(E_ReturnState errNum, const char *const file, int const line)
{
	if (errNum != E_ReturnState::SUCCESS)
	{
		fprintf(stderr, "[!ERROR!] at %s:%d\n", file, line);

		exit(EXIT_FAILURE);
	}
}

#define CheckFunc(val)					checkFuncRet((val), #val, __FILE__, __LINE__)
#define CheckErr(val)					checkErrNum((val), __FILE__, __LINE__)
typedef E_ReturnState(*PRetFunc)();
typedef void(*PVoidFunc)();
typedef E_ReturnState(*PRetFunc2)(void* param);

/************************************************************************/
/* 数据类型定义															*/
/************************************************************************/
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

typedef struct KernelArguType
{
	size_t size;
	void * ptr;
	bool isVal;
} T_KernelArgu;

struct Option
{
	std::string  _sVersion;
	std::string  _lVersion;
	std::string  _description;
	std::string  _usage;
	E_DataType  _type;
	void *       _value;
};


/************************************************************************/
/* 常量定义																*/
/************************************************************************/
#define	PI					(3.1415926535897932384626)
#define	TWO_PI				(6.28318530717958647692528676655)
#define	PI_SP				(3.1415926535897932384626F)
#define	TWO_PI_SP			(6.28318530717958647692528676655F)
#define FLT_MAX				(3.402823466e+38f)
#define	MIN_FP32_ERR		(1e-6)

/************************************************************************/
/* 硬件信息																*/
/************************************************************************/
//#define ISA_GFX800			(1)
#define	ISA_GFX900			(1)
#define	SE_NUM				(4)
#define	CU_PER_SE			(16)
#define CU_NUM				(CU_PER_SE * SE_NUM)
#define	WAVE_SIZE			(64)
#define CACHE_LINE			(16)
#define	MAX_VGPR_COUNT		(256)
#define	MAX_SGPR_COUNT		(800)




