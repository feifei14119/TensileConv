#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <list>
#include <algorithm>
#include <math.h>
#include <stdarg.h>

#include <map>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#if RUNTIME_CUDA
#include "cuda.h"
#include "cuda_runtime.h"
#endif

#if RUNTIME_OCL
#include <CL/cl.h>
#endif

/************************************************************************/
/* 错误输出定义                                                         */
/************************************************************************/
#define WARNING(msg) std::cerr << "[!WARNING!] " << msg << std::endl;
#define FATAL(msg) \
{ \
	std::cerr << "[!FATAL!] " << msg << " @ " << __FILE__ << " : " << __LINE__ << std::endl; \
	exit(EXIT_FAILURE); \
} while(0)
#define INFO(msg) std::cout << "[INFO] " << msg << std::endl;

extern std::ofstream *performance_log_file;
extern char log_char_buffer[1024];
extern void init_log_file();
extern void write_string_to_file(std::string log_str);
extern void write_format_to_file(const char * format,...);
#define	LOG(fmt,...)	write_format_to_file(fmt,##__VA_ARGS__)
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


#if RUNTIME_OCL
typedef struct dim3
{
	unsigned int x, y, z;
	dim3(unsigned int vx = 1, unsigned int vy = 1, unsigned int vz = 1) : x(vx), y(vy), z(vz) {}
}dim3;
#endif

typedef enum KernelTypeEnum
{
	KERNEL_TYPE_OCL_FILE = 1,
	KERNEL_TYPE_HIP_FILE = 2,
	KERNEL_TYPE_GAS_FILE = 3,
	KERNEL_TYPE_BIN_FILE = 4,
	KERNEL_TYPE_OCL_STR = 5,
	KERNEL_TYPE_GAS_STR = 6
}E_KernleType;

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

/************************************************************************/
/* OS API计时器															*/
/************************************************************************/
#if CPU_TIMER
#ifdef _WIN32
class WinTimer
{
private:
	LARGE_INTEGER cpuFreqHz;
	LARGE_INTEGER startTime;
	LARGE_INTEGER stopTime;

public:
	WinTimer(){}

public:
	void Restart()
	{
		QueryPerformanceFrequency(&cpuFreqHz);
		QueryPerformanceCounter(&startTime);
	}
	
	void Stop()
	{
		double diffTime100ns;
		QueryPerformanceCounter(&stopTime);
		diffTime100ns = (stopTime.QuadPart - startTime.QuadPart) * 1000.0 / cpuFreqHz.QuadPart;
		ElapsedMilliSec = diffTime100ns/10.0;
	}

	double ElapsedMilliSec;
};
#else
class UnixTimer
{
private:
	timespec startTime;
	timespec stopTime;

public:

public:
	void Restart()
	{
		clock_gettime(CLOCK_MONOTONIC, &startTime);
	}

	void Stop()
	{
		float diffTime100ns;
		clock_gettime(CLOCK_MONOTONIC, &stopTime);
		double d_startTime = static_cast<double>(startTime.tv_sec)*1e9 + static_cast<double>(startTime.tv_nsec);
		double d_currentTime = static_cast<double>(stopTime.tv_sec)*1e9 + static_cast<double>(stopTime.tv_nsec);
		ElapsedNanoSec = d_currentTime - d_startTime;

		ElapsedMilliSec = ElapsedNanoSec / 1e6;
	}

	double ElapsedMilliSec;
	double ElapsedNanoSec = 0;
};
#endif
#endif

/************************************************************************/
/* GPU计时器																*/
/************************************************************************/
#if GPU_TIMER

#if RUNTIME_OCL
#define ffTimer OclTimer
class OclTimer
{
private:

public:
	OclTimer()
	{
	}

public:
	void Restart()
	{
	}

	void Stop()
	{
		ElapsedMilliSec = 0;
	}

	double ElapsedMilliSec;
};
#endif
#endif

class UnixTimer
{
private:
	timespec startTime;
	timespec stopTime;

public:

public:
	void Restart()
	{
		clock_gettime(CLOCK_MONOTONIC, &startTime);
	}

	void Stop()
	{
		float diffTime100ns;
		clock_gettime(CLOCK_MONOTONIC, &stopTime);
		double d_startTime = static_cast<double>(startTime.tv_sec)*1e9 + static_cast<double>(startTime.tv_nsec);
		double d_currentTime = static_cast<double>(stopTime.tv_sec)*1e9 + static_cast<double>(stopTime.tv_nsec);
		ElapsedNanoSec = d_currentTime - d_startTime;

		ElapsedMilliSec = ElapsedNanoSec / 1e6;
	}

	double ElapsedMilliSec;
	double ElapsedNanoSec = 0;
};


#include "helper_cl.h"
