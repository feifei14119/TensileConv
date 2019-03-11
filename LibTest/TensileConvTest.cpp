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

	while (false)
	{
		int W = rand() % 112 + 1;	int H = rand() % 112 + 1;
		int C = rand() % 2048 + 1;	int K = rand() % 64 + 1;
		int N = rand() % 4 + 1;

		if ((C % 4 != 0) || (K % 2 != 0))
			continue;

		conv = new DirConv1x1Fwd();
		perfSec = conv->TuneProblem(W, H, C, K, N, 1, 1, false, E_TCRelu::NORELU, E_TCSearch::AUTO, solution);
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
	}

	conv = new DirConv1x1Fwd();
	perfSec = conv->TuneProblem(60, 57, 1512, 14, 3, 1, 1, false, E_TCRelu::NORELU, E_TCSearch::AUTO, solution);
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
