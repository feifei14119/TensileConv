#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../common/ff_utils.h"

int main(int argc, char *argv[])
{
	CmdArgs * ca = new CmdArgs(argc, argv);

	RuntimeOCL * pOcl = RuntimeOCL::GetInstance();
	pOcl->SellectDevice(0);
	pOcl->CreatCmdQueue();
	KernelOCL * k = pOcl->CreatKernel("../kernel/VectorAdd.cl", "VectorAdd", e_ProgramType::PRO_OCL_FILE);
	if (k == nullptr)
	{
		printf("err\n");
	}
	else
	{
		printf("yes\n");
	}


	delete pOcl;

	return 0;
}
