#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../common/ff_utils.h"
#include "ConvFwd1x1.h"

void DeviceInfo()
{
	RuntimeOCL * rtOcl = RuntimeOCL::GetInstance();
	int cuNum = rtOcl->Device()->DeviceInfo()->cuNum;
	double freq = rtOcl->Device()->DeviceInfo()->freqMHz * 1e6;
	double perf = SIMD_PER_CU * cuNum * freq * 2;	// 2 opts(mult & add) in one cycle
	PRINT_SEPARATOR3();
	PRINT_SEPARATOR2();
	OUTPUT("CU num = %d, Freq = %.3f(GHz), Perf(fp32) = %.3f(TFlops)", cuNum, freq * 1e-9, perf * 1e-12);
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
