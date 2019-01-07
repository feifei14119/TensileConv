#pragma once

#include <string>
#include <CL/opencl.h>

namespace TensileConv
{
	int TensileTest(int a, int b);
	bool SetRuntime(cl_platform_id platformId, cl_context context, cl_device_id deviceId);
	bool DirConv1x1Fwd(int WH, int C, int K, int N, int stride, bool enBias, bool enRelu);

	// direct convolution 1x1 forward
	class DirConv1x1Fwd
	{
	public:
		DirConv1x1Fwd();
		~DirConv1x1Fwd();

		bool AutoTune(int WH, int C, int K, int N, int stride, bool enBias, bool enRelu);
		std::string KernelPath();

	private:
		std::string kernelPath;
		std::string kernelFile;
	};
}

