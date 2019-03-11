#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "../common/ff_utils.h"
#include "ConvFwd1x1.h"

using namespace TensileConv;

int main(int argc, char *argv[])
{
	CmdArgs * ca = new CmdArgs(argc, argv);
	RuntimeOCL * pOcl = RuntimeOCL::GetInstance();
	LogFile * logFile = new LogFile("TensileConv", false);

	bool evinfo = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_EVINFO) == 1;
	if (evinfo == true)
	{
		pOcl->PrintRuntimeInfo(true);
		delete pOcl;
		return 0;
	}

	pOcl->PrintRuntimeInfo();
	pOcl->SellectDevice(0);

	int W = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_W);
	int H = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_H);
	int C = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_C);
	int K = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_K);
	int N = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_N);
	int UV = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_UV);
	bool Bias = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_BIAS) == 1;
	int Relu = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_RELU);
	int TuneMethod = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_SEARCH);
	W = 6; H = 6; N = 1; C = 48; K = 6; UV = 1; Bias = true; Relu = E_Relu::PRELU; TuneMethod = 1;

	ConvFwd1x1Problem * conv = new ConvFwd1x1Problem("DirConv1x1Fwd", logFile);
	conv->TuneProblem(W, H, C, K, N, UV, Bias, Relu, TuneMethod);
	ConvFwd1x1Solution * slt = (ConvFwd1x1Solution*)conv->BestSolution();
	OUTPUT("kernel file: " + slt->KernelFile());
	OUTPUT("kernel name: " + slt->KernelName());
	OUTPUT("signal size: %d", slt->SignalSize());
	OUTPUT("l2 split size: %d", slt->L2SplitSize());
	OUTPUT("debug size: %d", slt->DebugSize());
	OUTPUT("group size: [%d, %d, %d]", slt->GroupSize().x, slt->GroupSize().y, slt->GroupSize().z);
	OUTPUT("global size: [%d, %d, %d]", slt->GlobalSize().x, slt->GlobalSize().y, slt->GlobalSize().z);
	OUTPUT("elapsed time: %.1f(us)", slt->SolutionScore().ElapsedTime * 1e6);

	delete conv;
	delete pOcl;
	return 0;
}
