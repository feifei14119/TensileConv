#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <CL/opencl.h>

#include "ConvFwd1x1.h"
#include "../../Include/TensileConv.h"

using namespace TensileConv;

char * arg = "";
CmdArgs * pCmdArg = new CmdArgs(0, &arg);
RuntimeOCL * pOcl;
ConvFwd1x1Problem *conv1x1;
LogFile * logFile = new LogFile("TensileConv", false);
DirConv1x1Fwd::DirConv1x1Fwd()
{
	pOcl = RuntimeOCL::GetInstance();
	pOcl->SellectDevice(0);
	conv1x1 = new ConvFwd1x1Problem("DirConv1x1Fwd", logFile);
}
DirConv1x1Fwd::~DirConv1x1Fwd()
{
	delete conv1x1;
	delete pOcl;
}

double DirConv1x1Fwd::TuneProblem(int W, int H, int C, int K, int N, int U, int V,
	bool bias, E_TCRelu relu, E_TCSearch search,
	T_TCSolution & solution)
{
	conv1x1->TuneProblem(W, C, K, N, U, bias, (int)relu, (int)search);

	kernelName = conv1x1->BestSolution()->KernelName();
	kernelFile = conv1x1->BestSolution()->KernelFile();

	groupSize.clear();
	groupSize.push_back(conv1x1->BestSolution()->GroupSize().x);
	groupSize.push_back(conv1x1->BestSolution()->GroupSize().y);
	groupSize.push_back(conv1x1->BestSolution()->GroupSize().z);

	globalSize.clear();
	globalSize.push_back(conv1x1->BestSolution()->GlobalSize().x);
	globalSize.push_back(conv1x1->BestSolution()->GlobalSize().y);
	globalSize.push_back(conv1x1->BestSolution()->GlobalSize().z);

	paramSize.clear();
	ConvFwd1x1Solution * slt = (ConvFwd1x1Solution*)conv1x1->BestSolution();
	paramSize.push_back(slt->SignalSize());
	paramSize.push_back(slt->L2SplitSize());
	paramSize.push_back(slt->DebugSize());

	timeSec = conv1x1->BestSolution()->SolutionScore().ElapsedTime;

	if (timeSec < 0)
		return -1;

	solution.kernel_file = kernelFile;
	solution.kernel_name = kernelName;
	solution.ParamSize = paramSize;
	solution.GroupSize = groupSize;
	solution.GlobalSize = globalSize;

	return timeSec;
}
