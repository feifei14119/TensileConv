#pragma once

#include<vector>
#include <CL/opencl.h>

#include "ff_basic.h"
#include "ff_ocl_helper.h"
#include "ff_file_opt.h"

namespace feifei 
{
	class CmdQueueOCL;
	class KernelOCL;
	class DeviceOCL
	{
	public:
		DeviceOCL(cl_device_id id)
		{
			deviceId = id;
			getDeviceInfo();
		}
		cl_device_id DeviceId()
		{
			return deviceId;
		}
		cl_device_id * pDeviceId()
		{
			return &deviceId;
		}

		void AddCmdQueue(CmdQueueOCL * q)
		{
			queues.push_back(q);
		}
		void AddKernel(KernelOCL * k)
		{
			kernels.push_back(k);
		}

	protected:
		cl_platform_id platformId;
		cl_device_id deviceId;
		t_DeviceInfo deviceInfo;
		std::vector<CmdQueueOCL*> queues;
		std::vector<KernelOCL*> kernels;

		void getDeviceInfo();
	};

	class CmdQueueOCL
	{
	public:
		CmdQueueOCL(DeviceOCL * dev)
		{
			device = dev;
		}
		E_ReturnState CreatQueue(cl_context *ctx, bool enProf);

	private:
		cl_command_queue_properties prop = 0;
		DeviceOCL * device;
		cl_command_queue cmdQueue;
	};

	class KernelOCL
	{
	public:
		KernelOCL(char * content, std::string kernelName, e_ProgramType type, DeviceOCL * dev)
		{
			switch (type)
			{
			case PRO_OCL_FILE:		fileName = content;		break;
			case PRO_OCL_STRING:	programSrc = content;	break;
			case PRO_GAS_FILE:		fileName = content;		break;
			case PRO_GAS_STRING:	programSrc = content;	break;
			case PRO_BIN_FILE:		fileName = content;		break;
			case PRO_BIN_STRING:	programSrc = content;	break;
			}
			programType = type;
			device = dev;
		}
		E_ReturnState CreatKernel(cl_context *ctx);
		std::string BuildOption = "";

	protected:
		cl_program program;
		std::string kernelName;
		e_ProgramType programType;
		std::string fileName;
		char * programSrc;
		char * programBin;
		size_t programSizeB;
		std::string buildLog;
		DeviceOCL * device;

		E_ReturnState creatKernelFromOclFile(cl_context *ctx);
		E_ReturnState creatKernelFromOclString(cl_context *ctx);
		E_ReturnState creatKernelFromGasFile(cl_context *ctx);
		E_ReturnState creatKernelFromGasString(cl_context *ctx);
		E_ReturnState creatKernelFromBinFile(cl_context *ctx);
		E_ReturnState creatKernelFromBinString(cl_context *ctx);
		E_ReturnState buildKernel();
	};

	class RuntimeOCL
	{
	private:
		static RuntimeOCL * pInstance;
		RuntimeOCL()
		{
			kernelTempDir = "./kernel";
			ensure_dir(kernelTempDir.c_str());
		}
			
	protected:
		cl_platform_id platformId;
		t_PlatformInfo platformInfo;
		cl_context context;
		std::vector<DeviceOCL*> devices;
		DeviceOCL * selDevice;

		E_ReturnState initPlatform();
		void getPlatformInfo();

		std::string kernelTempDir = "./kernel";

	public:
		static RuntimeOCL * GetInstance(); 
		~RuntimeOCL()
		{
			for (int i = 0; i < DevicesCnt(); i++)
			{
				clReleaseDevice(devices[i]->DeviceId());
			}

			clReleaseContext(context);

			if (RuntimeOCL::pInstance)
				pInstance = nullptr;
		}

		uint DevicesCnt()
		{
			return devices.size();
		}
		E_ReturnState SellectDevice(uint devNum)
		{
			selDevice = devices[devNum];
		}

		CmdQueueOCL * CreatCmdQueue(bool enProf = false, int devNum = -1);
		KernelOCL * CreatKernel(char * content, std::string kernelName, e_ProgramType type, int devNum = -1);

		void SetKernelTempDir(std::string dir)
		{
			kernelTempDir = dir;
			ensure_dir(kernelTempDir.c_str());
		}
	};
}
