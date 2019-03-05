
#include "TensileConvBase.h"

using namespace TensileConv;
using namespace AutoTune;

/************************************************************************/
/* solution控制															*/
/************************************************************************/
void SolutionCtrlBase::RunSolution()
{
	time_t t1 = time(0);

	PRINT_SEPARATOR2();
	OUTPUT("* solution Name: %s.", solutionName.c_str());
	PRINT_SEPARATOR2();

	// 生成解决方案空间
	generateSolutionParamSpace();
	searchSpace->InitSearching();

	// 遍历每个problem的solution参数空间
#define TempDo(x)	if(x != E_ReturnState::SUCCESS) goto CONTINUE;
	while (true)
	{
		TempDo(getKernelParam());
		TempDo(generateKernel());
		TempDo(prepareKernelArgs());
		TempDo(launchKernel());
		releaseDevMem();

		CONTINUE:
		if (searchSpace->GenerateNextComb() != E_ReturnState::SUCCESS)
		{
			INFO("search solution parameters finished.");
			break;
		}

		sleep(0.5);
		PRINT_SEPARATOR('-');
	}
#undef TempDo(x)

	time_t t2 = time(0);
	searchElapsedSec = difftime(t2,t1);
}

E_ReturnState SolutionCtrlBase::launchKernel()
{
	INFO("warmup.");
	{
		stream->Launch(kernel, global_sz, group_sz, &profEvt);
		stream->Finish();
		usleep(0.1);
	}

	int loopCnt = 0;
	double t = 0, elapsedTime = 0;
	for (int i = 0; i < repeatTime; i++)
	{
		stream->Launch(kernel, global_sz, group_sz, &profEvt);
		stream->Flush();
		stream->Finish();
		usleep(0.01);

		t = rtOcl->GetProfilingTime(&profEvt);
		elapsedTime += t;

		loopCnt++;
		if (t > 5e-3)
		{
			break;
		}
	}

	INFO("launch kernel %d times.", loopCnt);
	elapsedTime /= loopCnt;
	recordScore(elapsedTime);
	
	return E_ReturnState::SUCCESS;
}

void SolutionCtrlBase::recordScore(double elapsedTime)
{
	searchSpace->SetOneCombScore(elapsedTime);
	if (solutionScore.ElapsedTime > elapsedTime)
	{
		searchSpace->RecordCurrComb();
	}

	T_Score score;
	score.ElapsedTime = elapsedTime;
	score.Flops = problem->Calculation() / score.ElapsedTime;
	score.Performence = problem->TheoryElapsedTime() / score.ElapsedTime;
	INFO("elapsed = %.1f(us), performence = %.1f(Gflops) = %.1f%%",
		score.ElapsedTime * 1e6, score.Flops * 1e-9, score.Performence * 100);

	if (solutionScore.ElapsedTime > score.ElapsedTime)
	{
		solutionScore.ElapsedTime = score.ElapsedTime;
		solutionScore.Performence = score.Performence;
		solutionScore.Flops = problem->Calculation() / solutionScore.ElapsedTime;
	}
	INFO("Best for now: elapsed = %.1f(us), performence = %.1f%%",
		solutionScore.ElapsedTime * 1e6, solutionScore.Performence * 100);
}

/************************************************************************/
/* solver 控制															*/
/************************************************************************/
void SolverCtrlBase::RunSolver()
{
	// add solutions to solver
	generateSolver();

	// 遍历solver的各个solution
	bestSolution = (*solutionList)[0];
	for(SolutionCtrlBase * solution : *solutionList)
	{
		if (EnSearch)	solution->RunSolution();
		
		T_Score score = solution->SolutionScore();
		scoreList->push_back(score);

		if (bestScore.ElapsedTime > score.ElapsedTime)
		{
			bestScore = score;
			bestSolution = solution;
		}
	}

	// best solution
	bestSolution->GetBestKernel();
}

/************************************************************************/
/* problem 控制															*/
/************************************************************************/
void ProblemCtrlBase::RunProblem()
{
	PRINT_SEPARATOR1();
	OUTPUT("* Problem Name: %s.", problemName.c_str());
	PRINT_SEPARATOR1();

	// 遍历problem参数空间,搜索参数空间
	while (true)
	{
		searchSpace->InitSearching();
		initHostParam();
		if (EnSearch)	caculateTheoryPerformance();
		if (EnSearch)	runHostCompute();
		solver->RunSolver();
		if (EnSearch)	verifyDevCompute();
		if (EnSearch)	releaseHostParam();

		if (searchSpace->GenerateNextComb() != E_ReturnState::SUCCESS)
		{
			INFO("search problem parameters finished.");
			break;
		}

		sleep(1);
	}
}

