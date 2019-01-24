#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../common/ff_utils.h"
#include "ConvFwd1x1.h"

using namespace TensileConv;

void DeviceInfo()
{
	RuntimeOCL * rtOcl = RuntimeOCL::GetInstance();

	T_DeviceInfo * devInfo = rtOcl->Device()->DeviceInfo();
	int cuNum = devInfo->cuNum;
	double freq = devInfo->freqMHz * 1e6;
	double perf = SIMD_PER_CU * cuNum * freq * 2;	// 2 opts(mult & add) in one cycle
	PRINT_SEPARATOR('+');
	OUTPUT("+ Vendor Name: " + devInfo->vendor);
	OUTPUT("+ Device Name: " + devInfo->name);
	OUTPUT("+ Runtime Version: " + devInfo->clVersion);
	OUTPUT("+ CU Number = %d", cuNum);
	OUTPUT("+ Core Frequency = %.3f(GHz)", freq * 1e-9);
	OUTPUT("+ Performance(fp32) = %.3f(TFlops)", perf * 1e-12);
	PRINT_SEPARATOR('+');
}

void EnvironmentInfo()
{
	RuntimeOCL * pOcl = RuntimeOCL::GetInstance();

	T_PlatformInfo * pltInfo = pOcl->PlatformInfo();
	PRINT_SEPARATOR('*');
	OUTPUT("* Platform Name: " + pltInfo->name);
	OUTPUT("* Version: " + pltInfo->version);
	OUTPUT("* Vendor Name: " + pltInfo->vendor);
	PRINT_SEPARATOR('*');

	for (int dev = 0; dev < pOcl->DevicesCnt(); dev++)
	{
		pOcl->SellectDevice(dev);
		DeviceInfo();
	}

	delete pOcl;
}

int main(int argc, char *argv[])
{
	CmdArgs * ca = new CmdArgs(argc, argv);	

	bool evinfo = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_EVINFO) == 1;
	if (evinfo == true)
	{
		EnvironmentInfo();
		return 0;
	}

	RuntimeOCL * pOcl = RuntimeOCL::GetInstance();
	pOcl->SellectDevice(0);
	DeviceInfo();
	
	int WH = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_WH);
	int C = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_C);
	int K = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_K);
	int N = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_N);
	int UV = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_UV);
	bool Bias = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_BIAS) == 1;
	int Relu = *(int*)ca->GetOneArg(E_ArgId::CMD_ARG_RELU) == 1;

	ConvFwd1x1Problem *conv1x1 = new ConvFwd1x1Problem("DirConv1x1Fwd");
	//conv1x1->TuneProblem();
	conv1x1->TuneProblem(WH, C, K, N, UV, Bias, Relu);

	//WH = 7; N = 8; C = 2048; K = 512; UV = 1; Bias = false; Relu = NORELU;
	//conv1x1->TuneProblem(WH, C, K, N, UV, Bias, Relu);
	
	delete conv1x1;
	delete pOcl;

	return 0;
}
