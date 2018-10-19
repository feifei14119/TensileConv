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
	 
	// ======================================================================
	// ======================================================================
	// ----------------------------------------------------------------------
	// 测试用例
	// ----------------------------------------------------------------------
	//ProblemCtrlBase *test = new TestProblem();
	//test->RunProblem();

	// ----------------------------------------------------------------------
	// TensileConv
	// ----------------------------------------------------------------------
	ProblemCtrlBase *conv1x1 = new ConvFwd1x1Problem("DirConv1x1Fwd");
	conv1x1->RunProblem();
	
	// ======================================================================
	// ======================================================================

	RuntimeCtrl::CleanRuntime();

	//getchar();

	return 0;
}
