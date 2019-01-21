#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../include/TensileConv.h"

using namespace TensileConv;

int main(int argc, char *argv[])
{
	DirConv1x1Fwd * conv = new DirConv1x1Fwd();
	conv->AutoTune(14, 512, 64, 1, 1, true, true);

	printf("\n");
	printf("\n");
	printf("*************************************************************************\n");
	printf("*************************** TensileConv *********************************\n");
	printf("*************************************************************************\n");
	printf("kernel name: %s.\n", conv->KernelName.c_str());
	printf("kernel file: %s.\n", conv->KernelFile.c_str());
	printf("group size: [%d, %d, %d].\n", conv->GroupSize[0], conv->GroupSize[1], conv->GroupSize[2]);
	printf("global size: [%d, %d, %d].\n", conv->GlobalSize[0], conv->GlobalSize[1], conv->GlobalSize[2]);
	printf("elapsed time. %.3f (us)\n", conv->ElapsedTime * 1e6);
	printf("*************************************************************************\n");
	printf("*************************** TensileConv *********************************\n");
	printf("*************************************************************************\n");
	printf("\n");
	printf("\n");

	delete conv;
	return 0;
}
