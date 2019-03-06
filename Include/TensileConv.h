#pragma once

#include <string>
#include <vector>

namespace TensileConv
{
	enum class E_TCRelu
	{
		NORELU = 0,
		RELU = 1,
		PRELU = 2
	};
	enum class E_TCSearch
	{
		NOSEARCH = 0,
		AUTO = 1,
		BRUTE = 2,
		GENETIC = 3
	};

	typedef struct TCSolutionType
	{
		std::string kernel_file;
		std::string kernel_name;
		std::vector<int> ParamSize;
		std::vector<int> GroupSize;
		std::vector<int> GlobalSize;
	} T_TCSolution;

	// direct convolution 1x1 forward
	class DirConv1x1Fwd
	{
	public:
		DirConv1x1Fwd();
		~DirConv1x1Fwd();

		double TuneProblem(int W, int H, int C, int K, int N, int U, int V,
			bool bias, E_TCRelu relu, E_TCSearch search,
			T_TCSolution & solution);

	private:
		std::string kernelFile;
		std::string kernelName;
		std::vector<int> paramSize;
		std::vector<int> groupSize;
		std::vector<int> globalSize;
		double timeSec;
	};
}

