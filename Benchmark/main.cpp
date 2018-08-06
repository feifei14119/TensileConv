#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "RuntimeControl.h"
#include "ProblemControl.h"

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
	// Sample
	// ----------------------------------------------------------------------
	//ProblemCtrlBase *vAdd = new VectorAddProblem();
	//vAdd->RunProblem();
	
	//ProblemCtrlBase *flatInstr = new FlatInstructionProblem();
	//flatInstr->RunProblem();
	//ProblemCtrlBase *glbInsr = new GlobalInstructionProblem();
	//glbInsr->RunProblem();
	//ProblemCtrlBase *lds = new DsInstructionProblem();
	//lds->RunProblem();

	// ----------------------------------------------------------------------
	// TensileConv
	// ----------------------------------------------------------------------
	ProblemCtrlBase *conv1x1 = new ConvFwd1x1Problem();
	conv1x1->RunProblem();
	
	// ======================================================================
	// ======================================================================

	RuntimeCtrl::CleanRuntime();

	//getchar();
	return 0;
}
