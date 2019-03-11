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

	conv = new DirConv1x1Fwd();
	perfSec = conv->TuneProblem(3, 3, 72, 6, 2, 1, 1, true, E_TCRelu::NORELU, E_TCSearch::AUTO, solution);
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

	conv = new DirConv1x1Fwd();
	perfSec = conv->TuneProblem(256, 64, 115, 690, 1, 1, 1, false, E_TCRelu::NORELU, E_TCSearch::AUTO, solution);
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

	conv = new DirConv1x1Fwd();
	perfSec = conv->TuneProblem(14, 14, 512, 64, 2, 1, 1, false, E_TCRelu::NORELU, E_TCSearch::AUTO, solution);
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

	conv = new DirConv1x1Fwd();
	perfSec = conv->TuneProblem(7, 7, 512, 64, 2, 1, 1, false, E_TCRelu::NORELU, E_TCSearch::AUTO, solution);
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

	conv = new DirConv1x1Fwd();
	perfSec = conv->TuneProblem(14, 14, 512, 64, 2, 1, 1, false, E_TCRelu::NORELU, E_TCSearch::AUTO, solution);
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
