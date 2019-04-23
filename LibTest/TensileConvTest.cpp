#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../Include/TensileConv.h"

using namespace TensileConv;

int main(int argc, char *argv[])
{
	double perfSec;
	TCSolutionType solution;
	DirConv1x1Fwd * conv;

	//DirConv1x1Fwd::SetWorkPath("/home/feifei/projects/out/db");
	//printf("database file path: %s\n", DirConv1x1Fwd::GetWorkPath().c_str());

	int W = 7, H = 7, C = 2048, K = 512, N=8;
	while (0)
	{
		W = rand() % 112 + 1;	H = rand() % 112 + 1;
		//C = rand() % 2048 + 1;	
		K = rand() % 64 + 1;
		N = rand() % 4 + 1;
		if (K % 2 != 0) continue;

		C++;

		conv = new DirConv1x1Fwd();
		perfSec = conv->TuneProblem(W, H, C, K, N, 1, 1, false, E_TCRelu::NORELU, E_TCSearch::GENETIC, solution);
		printf("*************************************************************************\n");
		printf("*************************** TensileConv *********************************\n");
		printf("*************************************************************************\n");
		printf("kernel name: %s.\n", solution.kernel_name.c_str());
		printf("kernel file: %s.\n", solution.kernel_file.c_str());
		printf("group size: [%d, %d, %d].\n", solution.GroupSize[0], solution.GroupSize[1], solution.GroupSize[2]);
		printf("global size: [%d, %d, %d].\n", solution.GlobalSize[0], solution.GlobalSize[1], solution.GlobalSize[2]);
		printf("elapsed time. %.3f (us)\n", perfSec * 1e6);
		printf("*************************************************************************\n");
		printf("*************************** TensileConv *********************************\n");
		printf("*************************************************************************\n");
		delete conv;
		usleep(1000000);
	}

	conv = new DirConv1x1Fwd();
	perfSec = conv->TuneProblem(W, H, C, K, N, 1, 1, true, E_TCRelu::NORELU, E_TCSearch::NOSEARCH, solution);
	printf("*************************************************************************\n");
	printf("*************************** TensileConv *********************************\n");
	printf("*************************************************************************\n");
	printf("kernel name: %s.\n", solution.kernel_name.c_str());
	printf("kernel file: %s.\n", solution.kernel_file.c_str());
	printf("group size: [%d, %d, %d].\n", solution.GroupSize[0], solution.GroupSize[1], solution.GroupSize[2]);
	printf("global size: [%d, %d, %d].\n", solution.GlobalSize[0], solution.GlobalSize[1], solution.GlobalSize[2]);
	printf("elapsed time. %.3f (us)\n", perfSec * 1e6);
	printf("*************************************************************************\n");
	printf("*************************** TensileConv *********************************\n");
	printf("*************************************************************************\n");
	delete conv;
	return 0;
}
