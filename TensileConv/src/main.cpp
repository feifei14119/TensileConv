#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../common/ff_utils.h"

int main(int argc, char *argv[])
{
	CmdArgs * ca = new CmdArgs(argc, argv);
	
	RuntimeOCL * pOcl = RuntimeOCL::GetInstance();

	uint N = 1024; size_t sizeN = N * sizeof(float);
	float * h_a = (float *)malloc(sizeN);
	float * h_b = (float *)malloc(sizeN);
	float * h_c = (float *)malloc(sizeN);
	for (int i = 0; i < N; i++)
	{
		h_a[i] = 1.234;
		h_b[i] = i * 1.0f;
		h_c[i] = 0;
	}
	cl_mem d_a, d_b, d_c;
	d_a = pOcl->DevMalloc(sizeN);
	d_b = pOcl->DevMalloc(sizeN);
	d_c = pOcl->DevMalloc(sizeN);

	pOcl->SellectDevice(0);
	KernelOCL * k = pOcl->CreatKernel("../TensileConv/kernel/VectorAdd.cl", "VectorAdd", E_ProgramType::PRO_OCL_FILE);
	k->SetArgs(&d_a, &d_b, &d_c);

	CmdQueueOCL * q = pOcl->CreatCmdQueue();

	q->MemCopyH2D(d_a, h_a, sizeN);
	q->MemCopyH2D(d_b, h_b, sizeN);
	q->Launch(k, new size_t[3]{ 64,1,1 }, new size_t[3]{ N,1,1 });
	q->Finish();
	q->MemCopyD2H(h_c, d_c, sizeN);

	pOcl->DevFree(d_a);
	pOcl->DevFree(d_b);
	pOcl->DevFree(d_c);
	delete pOcl;
	free(h_a);
	free(h_b);
	free(h_c);

	return 0;
}
