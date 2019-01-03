#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../include/TensileConv.h"

#include "../common/ff_utils.h"
#include "ConvFwd1x1.h"

int TensileConv::TensileTest(int a, int b)
{
	return a + b;
}

bool TensileConv::SetRuntime(cl_platform_id platformId, cl_context context, cl_device_id deviceId)
{
	if (RuntimeOCL::GetInstance(platformId, context, deviceId) == nullptr)
		return false;

	return true;
}

bool TensileConv::DirConv1x1Fwd(int WH, int C, int K, int N, int stride, bool enBias, bool enRelu)
{
	ConvFwd1x1Problem *conv1x1 = new ConvFwd1x1Problem("DirConv1x1Fwd");
	conv1x1->TurnProblem(WH, C, K, N, stride, enBias, enRelu);
	return true;
}
