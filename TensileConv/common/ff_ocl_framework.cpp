#include "ff_ocl_framework.h"

using namespace feifei;
/************************************************************************/
/* solution控制															*/
/************************************************************************/
void SolutionCtrlBase::RunSolution()
{
	PrintSeperator('=');
	INFO("= solution Name: %s.", solutionName.c_str());
	PrintSeperator('=');

#define TempDo(x)	do{if(x != E_ReturnState::SUCCESS) {releaseDevMem(); return;}}while(0)
	TempDo(generateKernel());
	TempDo(prepareKernelArgs());
	TempDo(launchKernel());
	getBackResult();
	releaseDevMem();
	ffSleepMS(1);
#undef TempDo(x)
}

E_ReturnState SolutionCtrlBase::launchKernel()
{
	LOG("warmup.");
	{
		stream->Launch(kernel, global_sz, group_sz, &profEvt);
		stream->Finish();
		ffSleepMS(1);
	}

	LOG("launch kernel %d times.", repeatTime);
	double elapsedTime = 0;
	{
		for (int i = 0; i < repeatTime; i++)
		{
			stream->Launch(kernel, global_sz, group_sz, &profEvt);
			stream->Flush();
			stream->Finish();
			ffSleepMS(1);
			elapsedTime += rtOcl->GetProfilingTime(&profEvt);
		}
	}

	elapsedTime /= repeatTime;
	recordScore(elapsedTime);

	return E_ReturnState::SUCCESS;
}

void SolutionCtrlBase::recordScore(double elapsedTime)
{
	score.ElapsedTime = elapsedTime;
	score.Flops = problem->Calculation() / elapsedTime;
	score.Performence = problem->TheoryElapsedTime() / elapsedTime;
}

/************************************************************************/
/* solver 控制															*/
/************************************************************************/
void SolverCtrlBase::RunSolver()
{
	// add solutions to solver
	generateSolver();

	// 遍历solver的各个solution
	bestSolution = (*solutions)[0];
	for (SolutionCtrlBase * solution : *solutions)
	{
		solution->RunSolution();

		T_Score score = solution->Score();
		scores->push_back(score);

		if (bestScore.ElapsedTime > score.ElapsedTime)
		{
			bestScore = score;
			bestSolution = solution;
		}
	}
}

/************************************************************************/
/* problem 控制															*/
/************************************************************************/
void ProblemCtrlBase::RunProblem()
{
	PrintSeperator('*');
	INFO("* Problem Name: %s.", problemName.c_str());
	PrintSeperator('*');

	initHostParam();
	cacuTheoryPerf();
	runHostCompute();
	solver->RunSolver();
	verifyDevCompute();
	releaseHostParam();

	ffSleepSec(1);
}

