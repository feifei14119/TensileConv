#pragma once

#include <stdarg.h>

#include "BasicClass.h"

/************************************************************************/
/* 运行时头文件                                                         */
/************************************************************************/
//#include <CL/cl.h>
#include <CL/opencl.h>
#include "./utils/AMDCL2/SDKUtil/CLUtil.hpp"
using namespace appsdk;


#define		BEST_DEVICE				(-1)
#define		DEFAULT_DEVICE			(0)
#define		DEFAULT_PLATFORM		(0)

#define		GPU_CORE_FREQ_HZ		(1.5e9)
#define		GPU_SE_NUM				(4)
#define		GPU_CU_NUM_PER_SE		(16)
#define		GPU_SIMD_NUM_PER_CU		(4)
#define		GPU_ALU_NUM_PER_SIMD	(16)
#define		GPU_ALU_NUM				(GPU_ALU_NUM_PER_SIMD*GPU_SIMD_NUM_PER_CU*GPU_CU_NUM_PER_SE*GPU_SE_NUM)
#define		GPU_CALCULATION			(GPU_ALU_NUM * GPU_CORE_FREQ_HZ)

/************************************************************************/
/* 硬件信息																*/
/************************************************************************/
typedef struct DeviceInfoType
{
	unsigned int DeviceIdx;
	char *DeviceName;
	unsigned int ComputeUnitNum;
	unsigned int ProcessingElementNum;
	unsigned long GlobalMemSize;
	size_t ConstBufSize;
	size_t LocalMemSize;
	char * RuntimeVersion;
	double CoreFreq;
	double Fp32Flops;
} T_DeviceInfo;

/************************************************************************/
/* 运行时状态定义															*/
/************************************************************************/
typedef struct KernelArguType
{
	size_t size;
	void * ptr;
	bool isVal;
	float fVal;
	int iVal;
} T_KernelArgu;
#define		DevStatus						cl_int
#define		RuntimeCtrl						RuntimeCtrlOcl
#define		DevProgram						cl_program
#define		DevKernel						cl_kernel
#define		HstMalloc						malloc
#define		HstFree							free
#define		DevMalloc(D_MEM,SIZEN)			RuntimeCtrl::CreatDevMemObj(D_MEM,SIZEN)
#define		Copy2Dev(DES,SRC,SIZEN)			RuntimeCtrl::MemCopyH2D(DES,SRC,SIZEN)
#define		Copy2Hst(DES,SRC,SIZEN)			RuntimeCtrl::MemCopyD2H(DES,SRC,SIZEN)
#define		H2DMalloc(D_MEM,SIZEN,H_MEM)	RuntimeCtrl::CreatH2DMemObj(D_MEM,SIZEN,H_MEM)
#define		D2HMalloc(D_MEM,SIZEN)			RuntimeCtrl::CreatD2HMemObj(D_MEM,SIZEN)
#define		DevFree							clReleaseMemObject
//#define		LanchKernel(KERNEL,BLOCK,GRID,args...) \
//RuntimeControl::LanchNDKernel(KERNEL, GRID, BLOCK, ##args)
#define		MAX_LOG_SIZE					(16384)

#define		DevCheckFunc(val)				cl_checkFuncRet((val), #val, __FILE__, __LINE__)
#define		DevCheckErr(val)				cl_checkErrNum((val), __FILE__, __LINE__)

/************************************************************************/
/* 运行时控制基类															*/
/************************************************************************/
class RuntimeCtrlBase
{
public:
	static E_ReturnState Init() {}
	static void Destroy() {}
	static void PrintInfo() {}
	
	void SetBlockSize(dim3 size)
	{
		blockSize = size;
	}

	void SetGridSize(dim3 size)
	{
		gridSize = size;
	}

	void GetFilesName(std::string fileName)
	{
		KernelFile = fileName;

		if (KernelFile == "")
		{
			// 源文件名
			if (KernelSrcType == E_KernleType::KERNEL_TYPE_OCL_FILE)
			{
				cppFileFullName = "../../../Kernels/" + KernelName + ".cl";
			}
			else if (KernelSrcType == E_KernleType::KERNEL_TYPE_HIP_FILE)
			{
				cppFileFullName = "../../../Kernels/" + KernelName + ".cpp";
			}
			else if (KernelSrcType == E_KernleType::KERNEL_TYPE_BIN_FILE)
			{
				srcBinFileFullName = "../../../Kernels/" + KernelName + ".bin";
			}
			else if (KernelSrcType == E_KernleType::KERNEL_TYPE_GAS_FILE)
			{
				asmFileFullName = "../../../Kernels/" + KernelName + ".s";
			}

			// 生成的二进制目标文件名(亦可为源二进制文件)
#if RUNTIME_OCL
			binFileFullName = "./" + KernelName + ".bin";
#else
			binFileFullName = "./" + KernelName + ".o";
#endif

			// 保存自动生成的源文件名
			if (KernelSrcType == E_KernleType::KERNEL_TYPE_OCL_STR)
			{
				saveSrcFileFullName = "./" + KernelName + "_Gen.cl";
			}
			else if (KernelSrcType == E_KernleType::KERNEL_TYPE_GAS_STR)
			{
				saveSrcFileFullName = "./" + KernelName + "_Gen.s";
			}

			// 保存编译生成的源文件名
			saveCppFileFullName = "./" + KernelName + "_Save.cl";

			// 保存编译生成的二进制文件名
			saveBinFileFullName = "./" + KernelName + "_Save.bin";
		}
		else
		{
			int tmpIdx = KernelFile.find_last_of('.');
			std::string shortFileName = KernelFile.substr(0, tmpIdx);

			// 源文件名
			if (KernelSrcType == E_KernleType::KERNEL_TYPE_OCL_FILE)
			{
				cppFileFullName = "../../../Kernels/" + KernelFile;
			}
			else if (KernelSrcType == E_KernleType::KERNEL_TYPE_HIP_FILE)
			{
				cppFileFullName = "../../../Kernels/" + KernelName + ".cpp";
			}
			else if (KernelSrcType == E_KernleType::KERNEL_TYPE_BIN_FILE)
			{
				srcBinFileFullName = "../../../Kernels/" + shortFileName + ".bin";
			}
			else if (KernelSrcType == E_KernleType::KERNEL_TYPE_GAS_FILE)
			{
				asmFileFullName = "../../../Kernels/" + KernelFile;
			}

			// 生成的二进制目标文件名(亦可为源二进制文件)
			binFileFullName = "./" + shortFileName + ".bin";

			// 保存自动生成的源文件名
			if (KernelSrcType == E_KernleType::KERNEL_TYPE_OCL_STR)
			{
				saveSrcFileFullName = "./" + KernelName + "_Gen_" + shortFileName + ".cl";
			}
			else if (KernelSrcType == E_KernleType::KERNEL_TYPE_GAS_STR)
			{
				saveSrcFileFullName = "./" + KernelName + "_Gen_" + shortFileName + ".s";
			}

			// 保存编译生成的源文件名
			saveCppFileFullName = "./" + KernelName + "_Save_" + shortFileName + ".cl";

			// 保存编译生成的二进制文件名
			saveBinFileFullName = "./" + KernelName + "_Save_" + shortFileName + ".bin";
		}
	}

	void PrintFilesName()
	{
		printf("kernel name = %s\n", KernelName.c_str());
		printf("kernel file name = %s\n", KernelFile.c_str());
		printf("source opencl file = %s\n", cppFileFullName.c_str());
		printf("source bin file name = %s\n", srcBinFileFullName.c_str());
		printf("source asm file name = %s\n", asmFileFullName.c_str());
		printf("generate bin file name = %s\n", binFileFullName.c_str());
		printf("save generated source file name = %s\n", saveBinFileFullName.c_str());
		printf("save generated source file name = %s\n", saveBinFileFullName.c_str());
		printf("save opencl file name = %s\n", saveCppFileFullName.c_str());
		printf("save bin file name = %s\n", saveBinFileFullName.c_str());

	}

	void GetComplier()
	{
#ifdef RUNTIME_OCL
		//complier = "/opt/rocm/bin/clang-ocl ";
		//complier = "/opt/rocm/opencl/bin/x86_64/clang ";
		//complier = "llvm-mc ";
		complier = "clang ";
#else
		complier = "/opt/rocm/opencl/bin/x86_64/clang ";
#endif;
		//complier = "/opt/rocm/bin/hcc ";
	}

	void GetBuildClOptions()
	{
		//buildClOption = "-D hello=hello";
		//buildClOption = " -target amdgcn--amdhsa -mcpu=gfx900 ";
		buildClOption = extCompilerOpt + " -save-temps ";
	}

	void GetBuildGasOptions()
	{
		//buildAsmOption = extCompilerOpt + " -target amdgcn--amdhsa -mcpu=gfx900 -I/home/feifei/projects/Benchmark/Kernels/ ";
		//buildAsmOption = " -x assembler -mcpu=gfx900 ";
		//buildAsmOption = " clrxasm -b amdcl2 -g GFX900 -A GFX9 -6 -o gfx800_va.o gfx800_va.s";
		buildAsmOption = extCompilerOpt + " -x assembler -target amdgcn--amdhsa -mcpu=gfx900 -I/home/feifei/projects/Benchmark/Kernels/ ";
	}
	
public:
	static T_DeviceInfo DeviceInfo;
	std::string KernelName;
	std::string KernelFile;
	std::string KernelString;
	std::string extCompilerOpt;
	E_KernleType KernelSrcType;

	static int DeviceNum;
	static int SpecifyDeviceIdx;
	static int DeviceIdx;
	 
	static size_t Compute;
	static size_t Bandwidth;

	dim3 blockSize;
	dim3 gridSize;

protected:
	// cuda/opencl源文件
	std::string cppFileName;
	std::string cppFilePath;
	std::string cppFileFullName;
	// 汇编源文件
	std::string asmFileName;
	std::string asmFilePath;
	std::string asmFileFullName;
	// 二进制源文件
	std::string srcBinFileName;
	std::string srcBinFilePath;
	std::string srcBinFileFullName;
	// 二进制文件
	std::string binFileName;
	std::string binFilePath;
	std::string binFileFullName;
	// 保存编译生成的opencl源文件
	std::string saveCppFileName;
	std::string saveCppFilePath;
	std::string saveCppFileFullName;
	// 保存编译生成的二进制文件
	std::string saveBinFileName;
	std::string saveBinFilePath;
	std::string saveBinFileFullName;
	// 保存自动生成的源文件(opencl或汇编)
	std::string saveSrcFileName;
	std::string saveSrcFilePath;
	std::string saveSrcFileFullName;

	std::string buildClOption;
	std::string buildAsmOption;
	std::string complier;
}; 

/************************************************************************/
/* 运行时控制                                                            */
/************************************************************************/
class RuntimeCtrlOcl :public RuntimeCtrlBase
{
public:

	static void GetDeviceInfo()
	{
		DeviceInfo.DeviceIdx = clArgs->deviceId;
		DeviceInfo.DeviceName = deviceInfo.name;
		DeviceInfo.ComputeUnitNum = deviceInfo.maxComputeUnits;
		DeviceInfo.CoreFreq = deviceInfo.maxClockFrequency * 1e6;
		DeviceInfo.GlobalMemSize = deviceInfo.globalMemSize;

		DeviceInfo.ProcessingElementNum = DeviceInfo.ComputeUnitNum * GPU_SIMD_NUM_PER_CU * GPU_ALU_NUM_PER_SIMD;
		DeviceInfo.Fp32Flops = DeviceInfo.ProcessingElementNum * DeviceInfo.CoreFreq * 2;

		printf("################################################################################\n");
		printf("# Device Info:\n");
		printf("# Device Id: %d\n", DeviceInfo.DeviceIdx);
		printf("# Device Name: %s\n", DeviceInfo.DeviceName);
		printf("# CU num: %d\n", DeviceInfo.ComputeUnitNum);
		printf("# PE num: %d\n", DeviceInfo.ProcessingElementNum);
		printf("# Clock Frequency: %.1f GHz\n", DeviceInfo.CoreFreq * 1e-9);
		printf("# Performence %.1f Tflops\n", DeviceInfo.Fp32Flops * 1e-12);
		printf("# Global Memory %.1fG Byte\n", DeviceInfo.GlobalMemSize * 1e-9);
		printf("################################################################################\n");		
	}


	// -----------------------------------------------------------------------------
	
	E_ReturnState CreatSolution()
	{
		switch (KernelSrcType)
		{
		case E_KernleType::KERNEL_TYPE_OCL_FILE:
			CheckFunc(CreatProgramFromOclFileRuntime());
			CheckFunc(CreatKernel());
			CheckFunc(SaveProgramBinary());
			return E_ReturnState::SUCCESS;

		case E_KernleType::KERNEL_TYPE_GAS_FILE:
			CheckFunc(CreatProgramFromGasFile());
			return(CreatKernel());

		case E_KernleType::KERNEL_TYPE_BIN_FILE:
			CheckFunc(CopyBinSourceFile());
			CheckFunc(CreatProgramFromBinaryFile());
			return(CreatKernel());
			
		case E_KernleType::KERNEL_TYPE_OCL_STR:
			CheckFunc(CreatProgramFromOclString());
			CheckFunc(SaveGenSource());
			return(CreatKernel());

		case E_KernleType::KERNEL_TYPE_GAS_STR:
			CheckFunc(CreatProgramFromGasString());
			CheckFunc(SaveGenSource());
			return(CreatKernel());

		default:
			FATAL("not supported kernel type.");
		}
	}
	
	E_ReturnState CreatProgramFromOclFileRuntime()
	{
		cl_int errNum;
		GetBuildClOptions();

		std::ifstream kernelFile(cppFileFullName.c_str(), std::ios::in);
		if (!kernelFile.is_open())
		{
			FATAL("Failed to open file for reading: " + cppFileFullName);
		}
		std::ostringstream oss;
		oss << kernelFile.rdbuf();
		KernelString = oss.str();
		const char *srcStr = KernelString.c_str();

		program = clCreateProgramWithSource(context, 1, (const char**)&srcStr, NULL, &errNum);
		DevCheckErr(errNum);
		if (program == NULL)
		{
			FATAL("Failed to create CL program from source.");
		}

		errNum = clBuildProgram(program, 1, &device, buildClOption.c_str(), NULL, NULL);

		checkBuildLog();
		DevCheckErr(errNum);
		
		return E_ReturnState::SUCCESS;
	}

	E_ReturnState CreatProgramFromOclFileRuntime(const char* fileName)
	{
		cl_int errNum;
		GetBuildClOptions();

		std::ifstream kernelFile(fileName, std::ios::in);
		if (!kernelFile.is_open())
		{
			FATAL("Failed to open file for reading: " + std::string(fileName));
		}
		std::ostringstream oss;
		oss << kernelFile.rdbuf();
		KernelString = oss.str();
		const char *srcStr = KernelString.c_str();

		program = clCreateProgramWithSource(context, 1, (const char**)&srcStr, NULL, &errNum);
		DevCheckErr(errNum);
		if (program == NULL)
		{
			FATAL("Failed to create CL program from source.");
		}

		errNum = clBuildProgram(program, 1, &device, buildClOption.c_str(), NULL, NULL);
		checkBuildLog();
		DevCheckErr(errNum);

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState CreatProgramFromOclFileCompiler()
	{
		GetComplier();
		GetBuildClOptions();

		std::string cmd = complier + buildClOption + "-o " + binFileFullName + " " + cppFileFullName;
		std::cout << "compiler command = " << cmd << std::endl;

		FILE * pfp = popen(cmd.c_str(), "r");
		auto status = pclose(pfp); 
		WEXITSTATUS(status);

		CheckFunc(CreatProgramFromBinaryFile());
	}

	E_ReturnState CreatProgramFromGasFile()
	{
		GetComplier();
		GetBuildGasOptions();

		/*std::string cmd0 = complier + "-x assembler -target amdgcn--amdhsa -mcpu=gfx900 -c -o./temp.o " + asmFileFullName;
		FILE * pfp0 = popen(cmd0.c_str(), "r");
		auto status0 = pclose(pfp0);
		WEXITSTATUS(status0);

		std::string cmd1 = complier + "-target amdgcn--amdhsa ./temp.o -o " + binFileFullName;
		FILE * pfp1 = popen(cmd1.c_str(), "r");
		auto status1 = pclose(pfp1);
		WEXITSTATUS(status1);*/

		std::string cmd = complier + buildAsmOption +"-o " + binFileFullName + " " + asmFileFullName;
		std::cout << "compiler command = " << cmd << std::endl;
		FILE * pfp = popen(cmd.c_str(), "r");
		auto status = pclose(pfp);
		WEXITSTATUS(status);

		CheckFunc(CreatProgramFromBinaryFile());

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState CreatProgramFromGasFile(const char* fileName)
	{
		GetComplier();
		GetBuildGasOptions();

		/*std::string cmd0 = complier + "-x assembler -target amdgcn--amdhsa -mcpu=gfx900 -c -o./temp.o " + asmFileFullName;
		FILE * pfp0 = popen(cmd0.c_str(), "r");
		auto status0 = pclose(pfp0);
		WEXITSTATUS(status0);

		std::string cmd1 = complier + "-target amdgcn--amdhsa ./temp.o -o " + binFileFullName;
		FILE * pfp1 = popen(cmd1.c_str(), "r");
		auto status1 = pclose(pfp1);
		WEXITSTATUS(status1);*/

		std::string cmd = complier + buildAsmOption + "-o " + binFileFullName + " " + std::string(fileName);
		std::cout << "compiler command = " << cmd << std::endl;
		std::cout << cmd << std::endl;
		FILE * pfp = popen(cmd.c_str(), "r");
		auto status = pclose(pfp);
		WEXITSTATUS(status);

		CheckFunc(CreatProgramFromBinaryFile());

		return E_ReturnState::SUCCESS;
	}
	
	E_ReturnState CreatProgramFromBinaryFile()
	{
		cl_int errNum;

		FILE * fp = fopen(binFileFullName.c_str(), "rb");
		if (fp == NULL)
		{
			FATAL("can't open bin file: " + binFileFullName);
		}

		size_t binSize;
		fseek(fp, 0, SEEK_END);
		binSize = ftell(fp);
		rewind(fp);
		printf("bin size = %d\n", binSize);

		unsigned char * binProgram = new unsigned char[binSize];
		fread(binProgram, 1, binSize, fp);
		fclose(fp);

		cl_int *binStatus = new cl_int[DeviceNum];
		cl_int binStatusSum = 0;
		program = clCreateProgramWithBinary(context, 1, &device, &binSize, (const unsigned char**)&binProgram, binStatus, &errNum);
		for (int i = 0; i < DeviceNum; i++)	binStatusSum += binStatus[i];
		DevCheckErr(binStatusSum);
		DevCheckErr(errNum);

		errNum = clBuildProgram(program, 1, &device, buildClOption.c_str(), NULL, NULL);
		checkBuildLog();
		DevCheckErr(errNum);

		delete[] binProgram;
		return E_ReturnState::SUCCESS;
	}

	E_ReturnState CreatProgramFromBinaryFile(const char * binFile)
	{
		cl_int errNum;
		FILE * fp = fopen(binFile, "rb");
		if (fp == NULL)
		{
			FATAL("can't open bin file: " + std::string(binFile));
		}

		size_t binSize;
		fseek(fp, 0, SEEK_END);
		binSize = ftell(fp);
		rewind(fp);

		unsigned char * binProgram = new unsigned char[binSize];
		fread(binProgram, 1, binSize, fp);
		fclose(fp);

		cl_int *binStatus = new cl_int[DeviceNum];
		cl_int binStatusSum = 0;
		program = clCreateProgramWithBinary(context, 1, &device, &binSize, (const unsigned char**)&binProgram, NULL, &errNum);
		for (int i = 0; i < DeviceNum; i++)	binStatusSum += binStatus[i];
		DevCheckErr(binStatusSum);
		DevCheckErr(errNum);

		errNum = clBuildProgram(program, 1, &device, buildClOption.c_str(), NULL, NULL);
		checkBuildLog();
		DevCheckErr(errNum);

		delete[] binProgram;
		return E_ReturnState::SUCCESS;
	}
	
	E_ReturnState CreatProgramFromOclString()
	{
		cl_int errNum;
		GetBuildClOptions();
		const char *srcStr = KernelString.c_str();

		program = clCreateProgramWithSource(context, 1, (const char**)&srcStr, NULL, &errNum);
		DevCheckErr(errNum);
		if (program == NULL)
		{
			FATAL("Failed to create CL program from source.");
		}

		errNum = clBuildProgram(program, 1, &device, buildClOption.c_str(), NULL, NULL);

		checkBuildLog();
		DevCheckErr(errNum);

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState CreatProgramFromGasString()
	{
		GetComplier();
		GetBuildGasOptions();

		std::ofstream fout(asmFileFullName.c_str(), std::ios::out);
		if (!fout.is_open())
		{
			FATAL("can't open save file: " + asmFileFullName);
		}
		fout.write(KernelString.c_str(), KernelString.length());
		fout.close();
		
		std::string cmd = complier + buildAsmOption + "-o " + binFileFullName + " " + asmFileFullName;
		std::cout << "compiler command = " << cmd << std::endl;
		FILE * pfp = popen(cmd.c_str(), "r");
		auto status = pclose(pfp);
		WEXITSTATUS(status);

		CheckFunc(CreatProgramFromBinaryFile());

		return E_ReturnState::SUCCESS;
	}

	// -----------------------------------------------------------------------------

	E_ReturnState SaveGenSource()
	{
		std::ofstream fout(saveSrcFileFullName.c_str(), std::ios::out);
		if (!fout.is_open())
		{
			FATAL("can't open save file: " + saveSrcFileFullName);
		}
		fout.write(KernelString.c_str(), KernelString.length());
		fout.close();

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState CopyBinSourceFile()
	{
		std::string cmd = "cp -f " + srcBinFileFullName + " " + binFileFullName;
		std::cout << cmd << std::endl;
		FILE * pfp = popen(cmd.c_str(), "r");
		auto status = pclose(pfp);
		WEXITSTATUS(status);

		return E_ReturnState::SUCCESS;
	}
	
	E_ReturnState SaveProgramOcl()
	{
		char clProgram[MAX_LOG_SIZE];

		DevCheckFunc(clGetProgramInfo(program, CL_PROGRAM_SOURCE, sizeof(clProgram), &clProgram, nullptr));

		std::ofstream fout(saveCppFileFullName.c_str(), std::ios::out);
		if (!fout.is_open())
		{
			FATAL("can't open save file: " + saveCppFileFullName);
		}
		//fout.write((clProgram));
		fout.close();

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState SaveProgramBinary()
	{
		size_t binary_size;
		
		DevCheckFunc(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, nullptr));
		std::vector<char> binary(binary_size);
		char* src[1] = { binary.data() };
		DevCheckFunc(clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(src), &src, nullptr));

		std::ofstream fout(saveBinFileFullName.c_str(), std::ios::out | std::ios::binary);
		if (!fout.is_open())
		{
			FATAL("can't open save file: " + saveBinFileFullName);
		}
		fout.write(binary.data(), binary.size());
		fout.close();

		return E_ReturnState::SUCCESS;
	}

	// -----------------------------------------------------------------------------

	E_ReturnState CreatKernel()
	{
		cl_int errNum;
		
		kernel = clCreateKernel(program, KernelName.c_str(), &errNum);
		DevCheckErr(errNum);
		if (kernel == NULL)
		{
			FATAL("Failed to create kernel");
		}
		return E_ReturnState::SUCCESS;
	}

	E_ReturnState LanchKernel()
	{
		// TODO: set parameters
		// ....
		cl_int errNum;
		size_t cl_gridSize[3] = { gridSize.x,gridSize.y,gridSize.z };
		size_t cl_blockSize[3] = { blockSize.x,blockSize.y,blockSize.z };

		// queue the kernel
#if GPU_TIMER
		DevCheckFunc(clEnqueueNDRangeKernel(stream, kernel, 3, NULL, cl_gridSize, cl_blockSize, 0, NULL, &exeEvent));
		clFinish(stream);

		//clGetEventProfilingInfo(exeEvent, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &startTime, NULL);
		clGetEventProfilingInfo(exeEvent, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startTime, NULL);
		clGetEventProfilingInfo(exeEvent, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endTime, NULL);
		ElapsedTime = (endTime - startTime) * 1e-9;
#else
		DevCheckFunc(clEnqueueNDRangeKernel(stream, kernel, 3, NULL, cl_gridSize, cl_blockSize, 0, NULL, NULL));
#endif

		return E_ReturnState::SUCCESS;
	}


	E_ReturnState LanchKernel(bool enTime)
	{
		cl_int errNum;
		size_t cl_gridSize[3] = { gridSize.x,gridSize.y,gridSize.z };
		size_t cl_blockSize[3] = { blockSize.x,blockSize.y,blockSize.z };

		if (enTime)
		{
			DevCheckFunc(clEnqueueNDRangeKernel(stream, kernel, 3, NULL, cl_gridSize, cl_blockSize, 0, NULL, NULL));
		}
		else
		{
			DevCheckFunc(clEnqueueNDRangeKernel(stream, kernel, 3, NULL, cl_gridSize, cl_blockSize, 0, NULL, NULL));
		}
	}

	E_ReturnState LanchKernel(dim3 blockSize, dim3 gridSize)
	{
		// TODO: set parameters
		// ....
		cl_int errNum;
		size_t cl_gridSize[3] = { gridSize.x,gridSize.y,gridSize.z };
		size_t cl_blockSize[3] = { blockSize.x,blockSize.y,blockSize.z };

		// queue the kernel
		DevCheckFunc(clEnqueueNDRangeKernel(stream, kernel, 3, NULL, cl_gridSize, cl_blockSize, 0, NULL, NULL));

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState LanchKernel(cl_kernel kernel, dim3 blockSize, dim3 gridSize,...)
	{
		// set kernel argument
		va_list va;
		va_start(va, gridSize);
		for (int i = 0; ; i++)
		{
			cl_mem d_mem = va_arg(va, cl_mem);
			if (d_mem == NULL) break;
			DevCheckFunc(clSetKernelArg(kernel, i, sizeof(cl_mem), &d_mem));
		}
		va_end(va);

		cl_int errNum;
		size_t cl_gridSize[3] = { gridSize.x,gridSize.y,gridSize.z };
		size_t cl_blockSize[3] = { blockSize.x,blockSize.y,blockSize.z };

		// queue the kernel
		DevCheckFunc(clEnqueueNDRangeKernel(stream, kernel, 3, NULL, cl_gridSize, cl_blockSize, 0, NULL, NULL));
		
		return E_ReturnState::SUCCESS;
	}



	E_ReturnState LanchKernel2()
	{
		cl_int errNum;
		size_t cl_gridSize[3] = { gridSize.x,gridSize.y,gridSize.z };
		size_t cl_blockSize[3] = { blockSize.x,blockSize.y,blockSize.z };

		DevCheckFunc(clEnqueueNDRangeKernel(stream, kernel, 3, NULL, cl_gridSize, cl_blockSize, 0, NULL, NULL));
	}

	// -----------------------------------------------------------------------------
	
	static void CreatDevMemObj(void **d_mem, size_t byteNum)
	{
		cl_int errNum;
		*(cl_mem*)(d_mem) = clCreateBuffer(context, CL_MEM_READ_WRITE, byteNum, NULL, &errNum);
		DevCheckErr(errNum);
	}
	
	static void MemCopyH2D(cl_mem d_mem, void * h_mem, size_t byteNum)
	{
		DevCheckFunc(clEnqueueWriteBuffer((*streams)[0], d_mem, CL_TRUE, 0, byteNum, h_mem, 0, NULL, NULL));
	}

	static void MemCopyD2H(void * h_mem, cl_mem d_mem, size_t byteNum)
	{
		DevCheckFunc(clEnqueueReadBuffer((*streams)[0], d_mem, CL_TRUE, 0, byteNum, h_mem, 0, NULL, NULL));
	}

	static void CreatH2DMemObj(cl_mem &d_mem, size_t byteNum, void * h_mem)
	{
		d_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, byteNum, h_mem, NULL);
	}

	static void CreatD2HMemObj(cl_mem &d_mem, size_t byteNum)
	{
		d_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, byteNum, NULL, NULL);
	}

	// -----------------------------------------------------------------------------

	void StartTimer()
	{
		clEnqueueMarkerWithWaitList(stream, 0, NULL, &startEvent);
	}

	void EndTimer()
	{
		clEnqueueMarkerWithWaitList(stream, 0, NULL, &endEvent);
		clWaitForEvents(1, &endEvent);
		cl_ulong start;
		clGetEventProfilingInfo(startEvent, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &start, NULL);
		cl_ulong end;
		clGetEventProfilingInfo(endEvent, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
		ElapsedTime = 1e-9 * (end - start);
	}

	
	void SetGridSize(dim3 size)
	{
		gridSize.x = size.x * blockSize.x;
		gridSize.y = size.y * blockSize.y;
		gridSize.z = size.z * blockSize.z;
	}
	
private:
	void checkBuildLog()
	{
		char buildLog[MAX_LOG_SIZE];
		char buildOpt[MAX_LOG_SIZE];
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, MAX_LOG_SIZE, buildLog, NULL);
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_OPTIONS, MAX_LOG_SIZE, buildOpt, NULL);
		printf("build kernel options = %s\n", buildOpt);
		printf("build kernel log = %s\n", buildLog);
	}

public:
	cl_program program;
	cl_kernel kernel;
	double ElapsedTime = 0;

private:
	cl_event exeEvent;
	cl_event startEvent;
	cl_event endEvent;
	cl_ulong startTime = 0, endTime = 0;


private:
	static CLCommandArgs *clArgs;
	static cl_platform_id platform;
	static cl_context context;
	static cl_device_id device;
	static SDKDeviceInfo deviceInfo;
	static std::vector<cl_command_queue> * streams;
	
	cl_command_queue stream;
	int streamIdx;

public:
	static int InitRuntime(int argc, char ** argv)
	{
		clArgs = new CLCommandArgs();
		clArgs->initialize();
		clArgs->parseCommandLine(argc, argv);

		cl_int status = 0;
		cl_device_type dType;

		if (clArgs->deviceType.compare("cpu") == 0)
		{
			dType = CL_DEVICE_TYPE_CPU;
		}
		else //deviceType = "gpu"
		{
			dType = CL_DEVICE_TYPE_GPU;
			if (clArgs->isThereGPU() == false)
			{
				std::cout << "GPU not found. Falling back to CPU device" << std::endl;
				dType = CL_DEVICE_TYPE_CPU;
			}
		}

		/*
		* Have a look at the available platforms and pick either
		* the AMD one if available or a reasonable default.
		*/
		platform = NULL;
		int retValue = getPlatformL(platform, clArgs->platformId, clArgs->isPlatformEnabled(), dType);
		CHECK_ERROR(retValue, SDK_SUCCESS, "getPlatform() failed");

		// Display available devices.
		retValue = displayDevices(platform, dType);
		CHECK_ERROR(retValue, SDK_SUCCESS, "displayDevices() failed");

		// creating context
		cl_context_properties cps[3] =
		{
			CL_CONTEXT_PLATFORM,
			(cl_context_properties)platform,
			0
		};

		context = clCreateContextFromType(cps, dType, NULL, NULL, &status);
		CHECK_OPENCL_ERROR(status, "clCreateContextFromType failed.");

		// getting device on which to run the sample
		cl_device_id *devices;
		status = getDevices(context, &devices, clArgs->deviceId, clArgs->isDeviceIdEnabled());
		CHECK_ERROR(status, SDK_SUCCESS, "getDevices() failed");
		device = devices[clArgs->deviceId];

		//Set device info of given cl_device_id
		retValue = deviceInfo.setDeviceInfo(devices[clArgs->deviceId]);
		CHECK_ERROR(retValue, SDK_SUCCESS, "SDKDeviceInfo::setDeviceInfo() failed");
		GetDeviceInfo();

		// 创建一个commandQueue用作default
		bool eAppGFLOPS = false;

		// The block is to move the declaration of prop closer to its use
		cl_command_queue_properties prop = 0;
#ifdef GPU_TIMER
		prop |= CL_QUEUE_PROFILING_ENABLE;
#endif
		streams = new std::vector<cl_command_queue>;
		cl_command_queue *st = new cl_command_queue;
		*st = clCreateCommandQueue(context, devices[clArgs->deviceId], prop, &status);
		streams->push_back(*st);
		CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed.");
		
		return SDK_SUCCESS;
	}

	static int CleanRuntime()
	{
		cl_int status = 0;
		
		std::vector<cl_command_queue>::iterator si;
		for (si = streams->begin(); si != streams->end(); si++)
		{
			status = clReleaseCommandQueue(*si);
			CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");
		}

		status = clReleaseDevice(device);
		CHECK_OPENCL_ERROR(status, "clReleaseDevice failed.(device)");

		status = clReleaseContext(context);
		CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");

		return SDK_SUCCESS;
	}

	RuntimeCtrlOcl(bool createNewStream = true)
	{
		if (createNewStream == true)
		{
			cl_int status = 0;
			bool eAppGFLOPS = false;

			// The block is to move the declaration of prop closer to its use
			cl_command_queue_properties prop = 0;
#ifdef GPU_TIMER
			prop |= CL_QUEUE_PROFILING_ENABLE;
#endif
			streamIdx = streams->size();
			stream = clCreateCommandQueue(context, device, prop, &status);
			streams->push_back(stream);
		}
		else
		{
			stream = (*streams)[0];
			streamIdx = 0;
		}

		kernel = NULL;
		program = NULL;
	}

	~RuntimeCtrlOcl()
	{
		if (kernel != NULL)		clReleaseKernel(kernel);
		if (program != NULL)	clReleaseProgram(program);
		if (streamIdx != 0)
		{
			streams->erase(streams->begin() + streamIdx);
			clReleaseCommandQueue(stream);
		}
	}

};

