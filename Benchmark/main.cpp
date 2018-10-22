#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "RuntimeControl.h"
#include "ProblemControl.h"

#include "ConvFwd1x1.h"

int main(int argc, char *argv[])
{
	RuntimeCtrl::InitRuntime(argc, argv);
	 
	ConvFwd1x1Problem *conv1x1 = new ConvFwd1x1Problem("DirConv1x1Fwd");
	conv1x1->RunProblem();						// 搜索全部尺寸范围
	conv1x1->RunProblem(28, 28, 2048, 64, 1);	// 搜索特定尺寸范围

	RuntimeCtrl::CleanRuntime();
	return 0;
}
