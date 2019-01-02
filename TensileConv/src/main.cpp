#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../common/ff_utils.h"
#include "ConvFwd1x1.h"

void DeviceInfo()
{
	RuntimeOCL * rtOcl = RuntimeOCL::GetInstance();

	T_DeviceInfo * devInfo = rtOcl->Device()->DeviceInfo();
	int cuNum = devInfo->cuNum;
	double freq = devInfo->freqMHz * 1e6;
	double perf = SIMD_PER_CU * cuNum * freq * 2;	// 2 opts(mult & add) in one cycle
	PRINT_SEPARATOR3();
	PRINT_SEPARATOR2();
	OUTPUT("+ Vendor name: " + devInfo->vendor);
	OUTPUT("+ Device name: " + devInfo->name);
	OUTPUT("+ Runtime version: " + devInfo->clVersion);
	OUTPUT("+ CU num = %d", cuNum);
	OUTPUT("+ Freq = %.3f(GHz)", freq * 1e-9);
	OUTPUT("+ Perf(fp32) = %.3f(TFlops)", perf * 1e-12);
	PRINT_SEPARATOR2();
	PRINT_SEPARATOR3();
}

int main(int argc, char *argv[])
{
	CmdArgs * ca = new CmdArgs(argc, argv);
	
	RuntimeOCL * pOcl = RuntimeOCL::GetInstance();
	pOcl->SellectDevice(0); 
	DeviceInfo();

	
	ConvFwd1x1Problem *conv1x1 = new ConvFwd1x1Problem("DirConv1x1Fwd");
	int WH = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_WH);
	int C = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_C);
	int K = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_K);
	int N = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_N);
	int UV = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_UV);
	bool Bias = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_BIAS) == 1;
	bool Relu = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_RELU) == 1;
	conv1x1->TurnProblem(WH, C, K, N, UV, Bias, Relu);
//	conv1x1->TurnProblem();
	

	delete pOcl;

	return 0;
}
