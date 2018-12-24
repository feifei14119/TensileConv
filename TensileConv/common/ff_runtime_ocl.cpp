#include <iostream> 
#include <fstream>
#include <sstream>

#include "ff_runtime_ocl.h"
#include "ff_log.h"

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
	uint deviceCnt;
	errNum = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, 0, NULL, &deviceCnt);
	clCheckErr(errNum);
	if (deviceCnt == 0)
	{
		FATAL("no opencl device support.");
	}

	cl_device_id deviceIds[deviceCnt];
	errNum = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, deviceCnt, deviceIds, NULL);
	clCheckErr(errNum);

	for (int i = 0; i < deviceCnt; i++)
	{
		DeviceOCL *dev = new DeviceOCL(deviceIds[i]);
		devices.push_back(dev);
	}

	return E_ReturnState::SUCCESS;
}
void RuntimeOCL::getPlatformInfo()
{
	cl_int errNum;
	size_t size;

	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, 0, NULL, &size);
	platformInfo.version = (char*)alloca(size);
	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, size, platformInfo.version, NULL);

	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_NAME, 0, NULL, &size);
	platformInfo.name = (char*)alloca(size);
	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_NAME, size, platformInfo.name, NULL);

	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VENDOR, 0, NULL, &size);
	platformInfo.vendor = (char*)alloca(size);
	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VENDOR, size, platformInfo.vendor, NULL);
}
void DeviceOCL::getDeviceInfo()
{
	cl_int errNum;
	size_t size;

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_TYPE, sizeof(cl_uint), &deviceInfo.type, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VENDOR_ID, sizeof(cl_uint), &deviceInfo.vendorId, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &deviceInfo.cuNum, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &deviceInfo.freqMHz, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cl_uint), &deviceInfo.cacheLineB, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &deviceInfo.cacheSizeB, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &deviceInfo.glbMemSizeB, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &deviceInfo.ldsMemSizeB, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &platformId, NULL);

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NAME, 0, NULL, &size);
	deviceInfo.name = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NAME, size, deviceInfo.name, NULL);

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VENDOR, 0, NULL, &size);
	deviceInfo.vendor = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VENDOR, size, deviceInfo.vendor, NULL);

	errNum = clGetDeviceInfo(deviceId, CL_DRIVER_VERSION, 0, NULL, &size);
	deviceInfo.drvVersion = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DRIVER_VERSION, size, deviceInfo.drvVersion, NULL);

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VERSION, 0, NULL, &size);
	deviceInfo.clVersion = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VERSION, size, deviceInfo.clVersion, NULL);
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

KernelOCL * RuntimeOCL::CreatKernel(char * content, std::string kernelName, e_ProgramType type, int devNum)
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
	return E_ReturnState::FAIL;
}
E_ReturnState KernelOCL::creatKernelFromOclString(cl_context *ctx)
{
	cl_int errNum;

	program = clCreateProgramWithSource(*ctx, 1, (const char**)&programSrc, NULL, &errNum);
	if ((program == NULL) || (errNum != CL_SUCCESS))
	{
		ERR("Failed to create CL program from " + fileName);
	}

	return buildKernel();
}
E_ReturnState KernelOCL::creatKernelFromOclFile(cl_context *ctx)
{
	cl_int errNum;

	std::ifstream kernelFile(fileName, std::ios::in);
	if (!kernelFile.is_open())
	{
		ERR("Failed to open file for reading: " + fileName);
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

	program = clCreateProgramWithBinary(*ctx, 1, device->pDeviceId(), &programSizeB, (const unsigned char**)&programSrc, NULL, &errNum);
	if ((program == NULL) || (errNum != CL_SUCCESS))
	{
		ERR("Failed to create CL program from " + fileName);
	}

	return buildKernel();
}
E_ReturnState KernelOCL::creatKernelFromBinFile(cl_context *ctx)
{
	cl_int errNum;

	FILE * fp = fopen(fileName.c_str(), "rb");
	if (fp == NULL)
	{
		ERR("can't open bin file: " + fileName);
	}

	fseek(fp, 0, SEEK_END);
	programSizeB = ftell(fp);
	rewind(fp);

	programSrc = (char *)malloc(programSizeB);
	fread(programSrc, 1, programSizeB, fp);
	fclose(fp);

	return creatKernelFromBinString(ctx);
}


E_ReturnState KernelOCL::creatKernelFromGasFile(cl_context *ctx)
{
	std::string cmd = complier + buildAsmOption + "-o " + binFileFullName + " " + asmFileFullName;
	exec_cmd(cmd);
}
E_ReturnState KernelOCL::creatKernelFromGasString(cl_context *ctx)
{
	return E_ReturnState::SUCCESS;
}

E_ReturnState KernelOCL::buildKernel()
{
	cl_int errNum;
	size_t size;
	char * tmpLog;

	errNum = clBuildProgram(program, 1, device->pDeviceId(), BuildOption.c_str(), NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		ERR("Failed to build CL program from " + fileName);
	}

	clGetProgramBuildInfo(program, device->DeviceId(), CL_PROGRAM_BUILD_LOG, 0, NULL, &size);
	tmpLog = (char*)alloca(size);
	clGetProgramBuildInfo(program, device->DeviceId(), CL_PROGRAM_BUILD_LOG, size, tmpLog, NULL);
	buildLog = std::string(tmpLog);
	INFO("building log: " + buildLog);

	return E_ReturnState::SUCCESS;
}

