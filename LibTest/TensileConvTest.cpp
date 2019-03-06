#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../Include/TensileConv.h"

using namespace TensileConv;

int main(int argc, char *argv[])
{
	printf("0");
	double perfSec;
	TCSolutionType solution;
	printf("1");
	DirConv1x1Fwd * conv = new DirConv1x1Fwd();
	printf("2");
	perfSec = conv->TuneProblem(14, 14, 512, 64, 2, 1, 1, false, E_TCRelu::NORELU, E_TCSearch::AUTO, solution);
	printf("3");

	printf("\n");
	printf("\n");
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
	printf("\n");
	printf("\n");

	delete conv;
	return 0;
}
