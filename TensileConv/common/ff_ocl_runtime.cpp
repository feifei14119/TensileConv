#include <iostream> 
#include <fstream>
#include <sstream>

#include "ff_ocl_runtime.h"

using namespace feifei;

RuntimeOCL * RuntimeOCL::pInstance = nullptr;
RuntimeOCL * RuntimeOCL::GetInstance()
{
	if (pInstance == nullptr)
	{
		pInstance = new RuntimeOCL();

		if (pInstance->initPlatform() != E_ReturnState::SUCCESS)
		{
			pInstance = nullptr;
		}
	}

	return pInstance;
}
RuntimeOCL * RuntimeOCL::GetInstance(cl_platform_id platformId, cl_context context, cl_device_id deviceId)
{
	if (pInstance == nullptr)
	{
		pInstance = new RuntimeOCL();

		if (pInstance->initPlatform(platformId, context, deviceId) != E_ReturnState::SUCCESS)
		{
			pInstance = nullptr;
		}
	}

	return pInstance;
}

E_ReturnState RuntimeOCL::initPlatform()
{
	cl_int errNum;
	cl_uint pltNum;

	// platform
	errNum = clGetPlatformIDs(0, NULL, &pltNum);
	clCheckErr(errNum);
	if (pltNum == 0)
	{
		FATAL("no opencl platform support.");
	}

	errNum = clGetPlatformIDs(1, &platformId, NULL);
	clCheckErr(errNum);
	getPlatformInfo();

	// context
	cl_context_properties cps[3] =
	{
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)platformId,
		0
	};
	context = clCreateContextFromType(cps, CL_DEVICE_TYPE_GPU, NULL, NULL, &errNum);
	clCheckErr(errNum);

	// devices
	unsigned int deviceCnt;
	errNum = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, 0, NULL, &deviceCnt);
	clCheckErr(errNum);
	if (deviceCnt == 0)
	{
		FATAL("no opencl device support.");
	}

	cl_device_id * deviceIds;
	deviceIds = (cl_device_id*)alloca(sizeof(cl_device_id) * deviceCnt);
	errNum = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, deviceCnt, deviceIds, NULL);
	clCheckErr(errNum);

	for (unsigned int i = 0; i < deviceCnt; i++)
	{
		DeviceOCL *dev = new DeviceOCL(deviceIds[i]);
		devices.push_back(dev);
	}

	return E_ReturnState::SUCCESS;
}
E_ReturnState RuntimeOCL::initPlatform(cl_platform_id platformId, cl_context context, cl_device_id deviceId)
{
	cl_int errNum;

	// platform
	this->platformId = platformId;
	getPlatformInfo();

	// context
	this->context = context;

	// devices
	unsigned int deviceCnt;
	errNum = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, 0, NULL, &deviceCnt);
	clCheckErr(errNum);
	if (deviceCnt == 0)
	{
		FATAL("no opencl device support.");
	}

	cl_device_id * deviceIds;
	deviceIds = (cl_device_id*)alloca(sizeof(cl_device_id) * deviceCnt);
	errNum = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, deviceCnt, deviceIds, NULL);
	clCheckErr(errNum);

	for (unsigned int i = 0; i < deviceCnt; i++)
	{
		DeviceOCL *dev = new DeviceOCL(deviceIds[i]);
		devices.push_back(dev);
	}

	// sellect device
	int devCnt = 0;
	for (devCnt = 0; devCnt < devices.size(); devCnt++)
	{
		if (devices[devCnt]->DeviceId() == deviceId)
		{
			selDevice = devices[devCnt];
			break;
		}
	}
	if (devCnt >= devices.size())
	{
		ERR("Can't find device %d.", deviceId);
	}

	return E_ReturnState::SUCCESS;
}
void RuntimeOCL::getPlatformInfo()
{
	cl_int errNum;
	size_t size;
	char * tmpChar;

	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, size, tmpChar, NULL);
	platformInfo.version = std::string(tmpChar);

	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_NAME, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_NAME, size, tmpChar, NULL);
	platformInfo.name = std::string(tmpChar);

	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VENDOR, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VENDOR, size, tmpChar, NULL);
	platformInfo.vendor = std::string(tmpChar);
}
void DeviceOCL::getDeviceInfo()
{
	cl_int errNum;
	size_t size;
	char * tmpChar;

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &platformId, NULL);

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_TYPE, sizeof(cl_uint), &deviceInfo.type, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NAME, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NAME, size, tmpChar, NULL);
	deviceInfo.name = std::string(tmpChar);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VENDOR_ID, sizeof(cl_uint), &deviceInfo.vendorId, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VENDOR, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VENDOR, size, tmpChar, NULL);
	deviceInfo.vendor = std::string(tmpChar);
	errNum = clGetDeviceInfo(deviceId, CL_DRIVER_VERSION, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DRIVER_VERSION, size, tmpChar, NULL);
	deviceInfo.drvVersion = std::string(tmpChar);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VERSION, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VERSION, size, tmpChar, NULL);
	deviceInfo.clVersion = std::string(tmpChar);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PROFILE, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PROFILE, size, tmpChar, NULL);
	deviceInfo.profiler = std::string(tmpChar);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_EXTENSIONS, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_EXTENSIONS, size, tmpChar, NULL);
	deviceInfo.extensions = std::string(tmpChar);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(size_t), &deviceInfo.profTimerResolution, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof(cl_bool), &deviceInfo.errCorrectSupport, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_ENDIAN_LITTLE, sizeof(cl_bool), &deviceInfo.endianLittle, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_AVAILABLE, sizeof(cl_bool), &deviceInfo.devAvailable, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_COMPILER_AVAILABLE, sizeof(cl_bool), &deviceInfo.complierAvailable, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_EXECUTION_CAPABILITIES, sizeof(cl_device_exec_capabilities), &deviceInfo.execCapabilities, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &deviceInfo.queueProper, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &deviceInfo.freqMHz, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &deviceInfo.cuNum, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &deviceInfo.maxWorkItemDim, NULL);
	//errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t), &deviceInfo.maxWorkItemSize, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &deviceInfo.maxWorkGroupSize, NULL);

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint), &deviceInfo.addrBit, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &deviceInfo.maxMemAllocSize, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &deviceInfo.memBaseAddrAlign, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, sizeof(cl_uint), &deviceInfo.minDataAlignSize, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &deviceInfo.glbMemSize, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &deviceInfo.glbCacheSize, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cl_uint), &deviceInfo.glbCacheLineSize, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, sizeof(cl_device_mem_cache_type), &deviceInfo.glbMemCacheType, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(cl_ulong), &deviceInfo.constBuffSize, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_CONSTANT_ARGS, sizeof(cl_uint), &deviceInfo.maxConstArgs, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &deviceInfo.ldsMemSize, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(cl_device_local_mem_type), &deviceInfo.ldsMemType, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(cl_bool), &deviceInfo.hostUnifiedMem, NULL);

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, sizeof(cl_uint), &deviceInfo.prefVctWidthChar, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &deviceInfo.prefVctWidthShort, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof(cl_uint), &deviceInfo.prefVctWidthInt, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, sizeof(cl_uint), &deviceInfo.prefVctWidthLong, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &deviceInfo.prefVctWidthFloat, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, sizeof(cl_uint), &deviceInfo.prefVctWidthDouble, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, sizeof(cl_uint), &deviceInfo.prefVctWidthHalf, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, sizeof(cl_uint), &deviceInfo.natvVctWidthChar, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &deviceInfo.natvVctWidthShort, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, sizeof(cl_uint), &deviceInfo.natvVctWidthInt, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, sizeof(cl_uint), &deviceInfo.natvVctWidthLong, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &deviceInfo.natvVctWidthFloat, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, sizeof(cl_uint), &deviceInfo.natvVctWidthDouble, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, sizeof(cl_uint), &deviceInfo.natvVctWidthHalf, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_SINGLE_FP_CONFIG, sizeof(cl_device_fp_config), &deviceInfo.singleFpCfg, NULL);

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &deviceInfo.supportImage, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(cl_uint), &deviceInfo.maxReadImageArgs, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(cl_uint), &deviceInfo.maxWriteImageArgs, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(size_t), &deviceInfo.maxImage2DWidth, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(size_t), &deviceInfo.maxImage2DHeight, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof(size_t), &deviceInfo.maxImage3DWidth, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof(size_t), &deviceInfo.maxImage3DHeight, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof(size_t), &deviceInfo.maxImage3DDepth, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_SAMPLERS, sizeof(cl_uint), &deviceInfo.maxSamples, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_PARAMETER_SIZE, sizeof(size_t), &deviceInfo.maxParamSize, NULL);
}
void RuntimeOCL::PrintRuntimeInfo(bool isFullInfo)
{
	PrintSeperator('*');
	INFO("* OpenCL Runtime Info:");
	PrintPlatformInfo();
	PrintSeperator('*');

	for (int i = 0; i < DevicesCnt(); i++)
	{
		if (isFullInfo)
		{
			devices[i]->PrintDeviceInfoFull();
		}
		else
		{
			devices[i]->PrintDeviceInfoShort();
		}
	}
}
void RuntimeOCL::PrintPlatformInfo()
{
	INFO("* Platform Name: " + platformInfo.name);
	INFO("* Version: " + platformInfo.version);
	INFO("* Vendor Name: " + platformInfo.vendor);
}
void DeviceOCL::PrintDeviceInfoShort()
{
	PrintSeperator('+');

	int cuNum = deviceInfo.cuNum;
	double freq = deviceInfo.freqMHz * 1e6;
	double perf = SIMD_PER_CU * cuNum * freq * 2;	// 2 opts(mult & add) in one cycle
	PrintSeperator('+');
	INFO("+ Device Name: " + deviceInfo.name);
	INFO("+ Vendor Name: " + deviceInfo.vendor);
	INFO("+ Runtime Version: " + deviceInfo.clVersion);
	INFO("+ CU Number = %d", cuNum);
	INFO("+ Core Frequency = %.3f(GHz)", freq * 1e-9);
	INFO("+ Performance(fp32) = %.3f(TFlops)", perf * 1e-12);

	PrintSeperator('+');
}
void DeviceOCL::PrintDeviceInfoFull()
{
	PrintSeperator('+');

	switch (deviceInfo.type)
	{
	case CL_DEVICE_TYPE_CPU: INFO("+ CL_DEVICE_TYPE: CL_DEVICE_TYPE_CPU"); break;
	case CL_DEVICE_TYPE_GPU: INFO("+ CL_DEVICE_TYPE: CL_DEVICE_TYPE_GPU"); break;
	case CL_DEVICE_TYPE_ACCELERATOR: INFO("+ CL_DEVICE_TYPE: CL_DEVICE_TYPE_ACCELERATOR"); break;
	case CL_DEVICE_TYPE_CUSTOM: INFO("+ CL_DEVICE_TYPE: CL_DEVICE_TYPE_CUSTOM"); break;
	default: INFO("+ CL_DEVICE_TYPE: UNKNOW"); break;
	}
	INFO("+ CL_DEVICE_NAME: " + deviceInfo.name);
	INFO("+ CL_DEVICE_VENDOR_ID: %d", deviceInfo.vendorId);
	INFO("+ CL_DEVICE_VENDOR: " + deviceInfo.vendor);
	INFO("+ CL_DRIVER_VERSION: " + deviceInfo.drvVersion);
	INFO("+ CL_DEVICE_OPENCL_C_VERSION: " + deviceInfo.clVersion);
	INFO("+ CL_DEVICE_PROFILE: " + deviceInfo.profiler);
	INFO("+ CL_DEVICE_EXTENSIONS: " + deviceInfo.extensions);
	INFO("+ CL_DEVICE_PROFILING_TIMER_RESOLUTION: %d", deviceInfo.profTimerResolution);
	INFO("+ CL_DEVICE_ERROR_CORRECTION_SUPPORT: %s", deviceInfo.errCorrectSupport ? "TRUE" : "FALSE");
	INFO("+ CL_DEVICE_ENDIAN_LITTLE: %s", deviceInfo.endianLittle ? "TRUE" : "FALSE");
	INFO("+ CL_DEVICE_AVAILABLE: %s", deviceInfo.devAvailable ? "TRUE" : "FALSE");
	INFO("+ CL_DEVICE_COMPILER_AVAILABLE: %s", deviceInfo.complierAvailable ? "TRUE" : "FALSE");
	switch (deviceInfo.execCapabilities)
	{
	case CL_EXEC_KERNEL: INFO("+ CL_DEVICE_EXECUTION_CAPABILITIES: CL_EXEC_KERNEL"); break;
	case CL_EXEC_NATIVE_KERNEL: INFO("+ CL_DEVICE_EXECUTION_CAPABILITIES: CL_EXEC_NATIVE_KERNEL"); break;
	default: INFO("+ CL_DEVICE_EXECUTION_CAPABILITIES: UNKNOW"); break;
	}
	switch (deviceInfo.queueProper)
	{
	case CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE: INFO("+ CL_DEVICE_QUEUE_PROPERTIES: CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE"); break;
	case CL_QUEUE_PROFILING_ENABLE: INFO("+ CL_DEVICE_QUEUE_PROPERTIES: CL_QUEUE_PROFILING_ENABLE"); break;
	case CL_QUEUE_ON_DEVICE: INFO("+ CL_DEVICE_QUEUE_PROPERTIES: CL_QUEUE_ON_DEVICE"); break;
	case CL_QUEUE_ON_DEVICE_DEFAULT: INFO("+ CL_DEVICE_QUEUE_PROPERTIES: CL_QUEUE_ON_DEVICE_DEFAULT"); break;
	default: INFO("+ CL_DEVICE_QUEUE_PROPERTIES: UNKNOW"); break;
	}
	INFO("+ CL_DEVICE_MAX_CLOCK_FREQUENCY: %d (MHz)", deviceInfo.freqMHz);
	INFO("+ CL_DEVICE_MAX_COMPUTE_UNITS: %d", deviceInfo.cuNum);
	INFO("+ CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: %d", deviceInfo.maxWorkItemDim);
	//INFO("+ CL_DEVICE_MAX_WORK_ITEM_SIZES: %d", deviceInfo.maxWorkItemSize);
	INFO("+ CL_DEVICE_MAX_WORK_GROUP_SIZE: %ld", deviceInfo.maxWorkGroupSize);

	INFO("+ CL_DEVICE_ADDRESS_BITS: %d", deviceInfo.addrBit);
	INFO("+ CL_DEVICE_MAX_MEM_ALLOC_SIZE: %zd", deviceInfo.maxMemAllocSize);
	INFO("+ CL_DEVICE_MEM_BASE_ADDR_ALIGN: %d", deviceInfo.memBaseAddrAlign);
	INFO("+ CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE: %d", deviceInfo.minDataAlignSize);
	INFO("+ CL_DEVICE_GLOBAL_MEM_SIZE: %zd", deviceInfo.glbMemSize);
	INFO("+ CL_DEVICE_GLOBAL_MEM_CACHE_SIZE: %ld", deviceInfo.glbCacheSize);
	INFO("+ CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE: %d", deviceInfo.glbCacheLineSize);
	switch (deviceInfo.glbMemCacheType)
	{
	case CL_NONE: INFO("+ CL_DEVICE_GLOBAL_MEM_CACHE_TYPE: CL_NONE"); break;
	case CL_READ_ONLY_CACHE: INFO("+ CL_DEVICE_GLOBAL_MEM_CACHE_TYPE: CL_READ_ONLY_CACHE"); break;
	case CL_READ_WRITE_CACHE: INFO("+ CL_DEVICE_GLOBAL_MEM_CACHE_TYPE: CL_READ_WRITE_CACHE"); break;
	default: INFO("+ CL_DEVICE_GLOBAL_MEM_CACHE_TYPE: UNKNOW"); break;
	}
	INFO("+ CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE: %zd", deviceInfo.constBuffSize);
	INFO("+ CL_DEVICE_MAX_CONSTANT_ARGS: %d", deviceInfo.maxConstArgs);
	INFO("+ CL_DEVICE_LOCAL_MEM_SIZE: %ld", deviceInfo.ldsMemSize);
	switch (deviceInfo.ldsMemType)
	{
	case CL_LOCAL: INFO("+ CL_DEVICE_LOCAL_MEM_TYPE: CL_LOCAL"); break;
	case CL_GLOBAL: INFO("+ CL_DEVICE_LOCAL_MEM_TYPE: CL_GLOBAL"); break;
	default: INFO("+ CL_DEVICE_LOCAL_MEM_TYPE: UNKNOW"); break;
	}
	INFO("+ CL_DEVICE_HOST_UNIFIED_MEMORY: %s", deviceInfo.hostUnifiedMem ? "TRUE" : "FALSE");

	INFO("+ CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR: %d", deviceInfo.prefVctWidthChar);
	INFO("+ CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT: %d", deviceInfo.prefVctWidthShort);
	INFO("+ CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT: %d", deviceInfo.prefVctWidthInt);
	INFO("+ CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG: %d", deviceInfo.prefVctWidthLong);
	INFO("+ CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT: %d", deviceInfo.prefVctWidthFloat);
	INFO("+ CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE: %d", deviceInfo.prefVctWidthDouble);
	INFO("+ CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF: %d", deviceInfo.prefVctWidthHalf);
	INFO("+ CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR: %d", deviceInfo.natvVctWidthChar);
	INFO("+ CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT: %d", deviceInfo.natvVctWidthShort);
	INFO("+ CL_DEVICE_NATIVE_VECTOR_WIDTH_INT: %d", deviceInfo.natvVctWidthInt);
	INFO("+ CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG: %d", deviceInfo.natvVctWidthLong);
	INFO("+ CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT: %d", deviceInfo.natvVctWidthFloat);
	INFO("+ CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE: %d", deviceInfo.natvVctWidthDouble);
	INFO("+ CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF: %d", deviceInfo.natvVctWidthHalf);
	switch (deviceInfo.singleFpCfg)
	{
	case CL_FP_DENORM: INFO("+ CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_DENORM"); break;
	case CL_FP_INF_NAN: INFO("+ CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_INF_NAN"); break;
	case CL_FP_ROUND_TO_NEAREST: INFO("+ CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_ROUND_TO_NEAREST"); break;
	case CL_FP_ROUND_TO_ZERO: INFO("+ CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_ROUND_TO_ZERO"); break;
	case CL_FP_ROUND_TO_INF: INFO("+ CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_ROUND_TO_INF"); break;
	case CL_FP_FMA: INFO("+ CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_FMA"); break;
	case CL_FP_SOFT_FLOAT: INFO("+ CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_SOFT_FLOAT"); break;
	case CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT: INFO("+ CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT"); break;
	default: INFO("+ CL_DEVICE_SINGLE_FP_CONFIG: UNKNOW"); break;
	}

	INFO("+ CL_DEVICE_IMAGE_SUPPORT: %s", deviceInfo.supportImage ? "TRUE" : "FALSE");
	INFO("+ CL_DEVICE_MAX_READ_IMAGE_ARGS: %d", deviceInfo.maxReadImageArgs);;
	INFO("+ CL_DEVICE_MAX_WRITE_IMAGE_ARGS: %d", deviceInfo.maxWriteImageArgs);;
	INFO("+ CL_DEVICE_IMAGE2D_MAX_WIDTH: %ld", deviceInfo.maxImage2DWidth);;
	INFO("+ CL_DEVICE_IMAGE2D_MAX_HEIGHT: %ld", deviceInfo.maxImage2DHeight);;
	INFO("+ CL_DEVICE_IMAGE3D_MAX_WIDTH: %ld", deviceInfo.maxImage3DWidth);;
	INFO("+ CL_DEVICE_IMAGE3D_MAX_HEIGHT: %ld", deviceInfo.maxImage3DHeight);;
	INFO("+ CL_DEVICE_IMAGE3D_MAX_DEPTH: %ld", deviceInfo.maxImage3DDepth);;
	INFO("+ CL_DEVICE_MAX_SAMPLERS: %d", deviceInfo.maxSamples);;
	INFO("+ CL_DEVICE_MAX_PARAMETER_SIZE: %ld", deviceInfo.maxParamSize);

	PrintSeperator('+');
}

CmdQueueOCL * RuntimeOCL::CreatCmdQueue(bool enProf, int devNum)
{	
	DeviceOCL * dev;
	if (devNum < 0)
	{
		dev = selDevice;
	}
	else
	{
		if (devNum >= DevicesCnt())
			return nullptr;

		dev = devices[devNum];
	}
	
	CmdQueueOCL * q = new CmdQueueOCL(dev);
	if (q->CreatQueue(&context, enProf) == E_ReturnState::SUCCESS)
	{
		dev->AddCmdQueue(q);
		return q;
	}
	else
	{
		return nullptr;
	}
}
E_ReturnState CmdQueueOCL::CreatQueue(cl_context *ctx, bool enProf)
{
	cl_int errNum;

	if (enProf == true)
	{
		prop |= CL_QUEUE_PROFILING_ENABLE;
	}
	cmdQueue = clCreateCommandQueue(*ctx, device->DeviceId(), prop, &errNum);
	clCheckErr(errNum);

	return E_ReturnState::SUCCESS;
}

KernelOCL * RuntimeOCL::CreatKernel(char * content, std::string kernelName, E_ProgramType type, int devNum)
{
	DeviceOCL * dev;
	if (devNum < 0)
	{
		dev = selDevice;
	}
	else
	{
		if (devNum >= DevicesCnt())
			return nullptr;

		dev = devices[devNum];
	}

	KernelOCL * k = new KernelOCL(content, kernelName, type, dev);

	if (k->CreatKernel(&context) == E_ReturnState::SUCCESS)
	{
		dev->AddKernel(k);
		return k;
	}
	else
	{
		return nullptr;
	}
}
E_ReturnState KernelOCL::CreatKernel(cl_context *ctx)
{
	switch (programType)
	{
	case PRO_OCL_FILE:		return creatKernelFromOclFile(ctx);
	case PRO_OCL_STRING:	return creatKernelFromOclString(ctx);
	case PRO_GAS_FILE:		return creatKernelFromGasFile(ctx);
	case PRO_GAS_STRING:	return creatKernelFromGasString(ctx);
	case PRO_BIN_FILE:		return creatKernelFromBinFile(ctx);
	case PRO_BIN_STRING:	return creatKernelFromBinString(ctx);
	default: ERR("not support program type");
	}
	return E_ReturnState::RTN_ERR;
}
E_ReturnState KernelOCL::creatKernelFromOclString(cl_context *ctx)
{
	cl_int errNum;

	program = clCreateProgramWithSource(*ctx, 1, (const char**)&programSrc, NULL, &errNum);
	if ((program == NULL) || (errNum != CL_SUCCESS))
	{
		ERR("Failed to create CL program from " + programFile);
	}

	if (buildKernel() != E_ReturnState::SUCCESS)
		return E_ReturnState::RTN_ERR;
	if (dumpKernel() != E_ReturnState::SUCCESS)
		return E_ReturnState::RTN_ERR;

	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelOCL::creatKernelFromOclFile(cl_context *ctx)
{
	std::ifstream kernelFile(programFile, std::ios::in);
	if (!kernelFile.is_open())
	{
		ERR("Failed to open file for reading: " + programFile);
	}
	std::ostringstream oss;
	oss << kernelFile.rdbuf();
	std::string str = oss.str();
	programSrc = (char *)(str.c_str());

	return creatKernelFromOclString(ctx);
}
E_ReturnState KernelOCL::creatKernelFromBinString(cl_context *ctx)
{
	cl_int errNum;

	program = clCreateProgramWithBinary(*ctx, 1, device->pDeviceId(), &programSize, (const unsigned char**)&programSrc, NULL, &errNum);
	if ((program == NULL) || (errNum != CL_SUCCESS))
	{
		ERR("Failed to create CL program from " + programFile);
	}

	return buildKernel();
}
E_ReturnState KernelOCL::creatKernelFromBinFile(cl_context *ctx)
{
	std::ifstream fin(programFile.c_str(), std::ios::in | std::ios::binary);
	if (!fin.is_open())
	{
		ERR("can't open bin file: " + programFile);
	}

	fin.seekg(0, std::ios::end);
	programSize = (size_t)fin.tellg();
	fin.seekg(0, std::ios::beg);

	programSrc = (char *)malloc(programSize);
	fin.read(programSrc, programSize);
	fin.close();

	return creatKernelFromBinString(ctx);
}
E_ReturnState KernelOCL::creatKernelFromGasFile(cl_context *ctx)
{
	std::string compiler = RuntimeOCL::GetInstance()->Compiler();
	if (kernelFile == "")
		kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + get_file_name(programFile) + ".bin";

	if (device->DeviceInfo()->name == "gfx900")
		BuildOption = "-x assembler -target amdgcn--amdhsa -mcpu=gfx900 ";
	else if (device->DeviceInfo()->name == "gfx803")
		BuildOption = "-x assembler -target amdgcn--amdhsa -mcpu=gfx803 ";
	else
		ERR("not support hardware.");

	std::string cmd = compiler + " " + BuildOption + "-o " + kernelFile + " " + programFile;
	exec_cmd(cmd);
	//LOG(cmd);

	// creatKernelFromBinString()
	std::ifstream fin(kernelFile.c_str(), std::ios::in | std::ios::binary);
	if (!fin.is_open())
	{
		ERR("can't open bin file: " + kernelFile);
	}

	fin.seekg(0, std::ios::end);
	programSize = (size_t)fin.tellg();
	fin.seekg(0, std::ios::beg);

	programSrc = (char *)malloc(programSize);
	fin.read(programSrc, programSize);
	fin.close();

	// creatKernelFromBinString()
	cl_int errNum;
	program = clCreateProgramWithBinary(*ctx, 1, device->pDeviceId(), &programSize, (const unsigned char**)&programSrc, NULL, &errNum);
	if ((program == NULL) || (errNum != CL_SUCCESS))
	{
		ERR("Failed to create CL program from " + programFile);
	}

	return buildKernel();
}
E_ReturnState KernelOCL::creatKernelFromGasString(cl_context *ctx)
{
	programFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + kernelName + ".s";
	kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + kernelName + ".bin";

	std::ofstream fout(programFile.c_str(), std::ios::out);
	if (!fout.is_open())
	{
		ERR("can't open save file: " + programFile);
	}
	fout.write(kernelFile.c_str(), sizeof(*programSrc)/sizeof(char));
	fout.close();

	return creatKernelFromGasFile(ctx);
}
E_ReturnState KernelOCL::buildKernel()
{
	cl_int errNum;
	size_t size;
	char * tmpLog;

	errNum = clBuildProgram(program, 1, device->pDeviceId(), "", NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		ERR("Failed to build CL program from " + programFile);
	}

	clGetProgramBuildInfo(program, device->DeviceId(), CL_PROGRAM_BUILD_LOG, 0, NULL, &size);
	tmpLog = (char*)alloca(size);
	clGetProgramBuildInfo(program, device->DeviceId(), CL_PROGRAM_BUILD_LOG, size, tmpLog, NULL);
	//LOG("building log: " + std::string(tmpLog));

	kernel = clCreateKernel(program, kernelName.c_str(), &errNum);
	if ((errNum != CL_SUCCESS)||(kernel == NULL))
	{
		ERR("Failed to create kernel " + kernelName);
	}

	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelOCL::dumpKernel()
{
	if (programFile == "")
	{
		kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + kernelName + ".bin";
	}
	else
	{
		kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + get_file_name(programFile) + ".bin";
	}
	size_t binary_size;
	clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, nullptr);
	std::vector<char> binary(binary_size);
	char* src[1] = { binary.data() };
	clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(src), &src, nullptr);

	std::ofstream fout(kernelFile.c_str(), std::ios::out | std::ios::binary);
	if (!fout.is_open())
	{
		ERR("can't open save file: " + kernelFile);
	}
	fout.write(binary.data(), binary.size());
	fout.close();

	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelOCL::dumpProgram()
{
	if (programFile == "")
	{
		kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + kernelName + ".cl";
	}
	else
	{
		kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + get_file_name(programFile) + ".cl";
	}

	size_t src_size;
	clGetProgramInfo(program, CL_PROGRAM_SOURCE, sizeof(size_t), &src_size, nullptr);

	char * clProgram;
	clProgram = (char*)alloca(src_size);
	clGetProgramInfo(program, CL_PROGRAM_SOURCE, sizeof(clProgram), &clProgram, nullptr);

	std::ofstream fout(kernelFile.c_str(), std::ios::out);
	if (!fout.is_open())
	{
		ERR("can't open save file: " + kernelFile);
	}
	fout.write(clProgram, src_size);
	fout.close();

	return E_ReturnState::SUCCESS;
}
E_ReturnState CmdQueueOCL::Launch(KernelOCL *k, dim3 global_sz, dim3 group_sz, cl_event * evt_creat)
{
	cl_int errNum;

	if (k->DeviceId() != this->DeviceId())
	{
		WARN("kernel device not match command queue device.");
	}

	errNum = clEnqueueNDRangeKernel(cmdQueue, k->Kernel(), 3, NULL, global_sz.arr(), group_sz.arr(), 0, NULL, evt_creat);
	if(errNum != CL_SUCCESS)
	{
		clErrInfo(errNum);
		ERR("Failed launch kernel: " + std::string(clGetErrorInfo(errNum)));
	}

	return E_ReturnState::SUCCESS;
}
double RuntimeOCL::GetProfilingTime(cl_event * evt)
{
	if (evt == NULL)
	{
		WARN("no profiling event.");
		return 0.0;
	}

	cl_ulong start_time, end_time;
	clGetEventProfilingInfo(*evt, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start_time, NULL);
	clGetEventProfilingInfo(*evt, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end_time, NULL);

	return (end_time - start_time) * 1e-9;
}

cl_mem RuntimeOCL::DevMalloc(size_t byteNum)
{
	cl_int errNum;
	cl_mem d_mem;

	d_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, byteNum, NULL, &errNum);
	if ((errNum != CL_SUCCESS) || (d_mem == nullptr))
	{
		clErrInfo(errNum);
		d_mem = nullptr;
	}
	return d_mem;
}
E_ReturnState CmdQueueOCL::MemCopyH2D(cl_mem d_mem, void * h_mem, size_t byteNum)
{
	cl_int errNum;

	errNum = clEnqueueWriteBuffer(cmdQueue, d_mem, CL_TRUE, 0, byteNum, h_mem, 0, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		clErrInfo(errNum);
		ERR("Failed to copy memory to device %d Byte", byteNum);
	}

	return E_ReturnState::SUCCESS;
}
E_ReturnState CmdQueueOCL::MemCopyD2H(void * h_mem, cl_mem d_mem, size_t byteNum)
{
	cl_int errNum;

	errNum = clEnqueueReadBuffer(cmdQueue, d_mem, CL_TRUE, 0, byteNum, h_mem, 0, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		clErrInfo(errNum);
		ERR("Failed to copy memory to device %d Byte", byteNum);
	}

	return E_ReturnState::SUCCESS;
}
