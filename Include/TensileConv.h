#pragma once

#include <string>

namespace TensileConv
{
	// direct convolution 1x1 forward
	class DirConv1x1Fwd
	{
	public:
		DirConv1x1Fwd();
		~DirConv1x1Fwd();

		bool AutoTune(int WH, int C, int K, int N, int stride, bool enBias, bool enRelu);

		std::string KernelName;
		std::string KernelFile;
		size_t GroupSize[3], GlobalSize[3];

		double ElapsedTime;	//(s)
	};
}

