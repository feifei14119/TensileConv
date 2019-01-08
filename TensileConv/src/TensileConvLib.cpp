#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <CL/opencl.h>

#include "ConvFwd1x1.h"
#include "../include/TensileConv.h"

using namespace TensileConv;

RuntimeOCL * pOcl;
ConvFwd1x1Problem *conv1x1;
DirConv1x1Fwd::DirConv1x1Fwd()
{
	pOcl = RuntimeOCL::GetInstance();
	pOcl->SellectDevice(0);
	conv1x1 = new ConvFwd1x1Problem("DirConv1x1Fwd");
}
DirConv1x1Fwd::~DirConv1x1Fwd()
{
	delete conv1x1;
	delete pOcl;
}

bool DirConv1x1Fwd::AutoTune(int WH, int C, int K, int N, int stride, bool enBias, bool enRelu)
{
	conv1x1->TuneProblem(WH, C, K, N, stride, enBias, enRelu);

	KernelName = conv1x1->BestSolution()->KernelName();
	KernelFile = conv1x1->BestSolution()->KernelFile();

	GroupSize[0] = conv1x1->BestSolution()->GroupSize().x;
	GroupSize[1] = conv1x1->BestSolution()->GroupSize().y;
	GroupSize[2] = conv1x1->BestSolution()->GroupSize().z;

	GlobalSize[0] = conv1x1->BestSolution()->GlobalSize().x;
	GlobalSize[1] = conv1x1->BestSolution()->GlobalSize().y;
	GlobalSize[2] = conv1x1->BestSolution()->GlobalSize().z;

	ElapsedTime = conv1x1->BestSolution()->SolutionScore().ElapsedTime;

	return true;
}
