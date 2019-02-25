
#include "TensileConvBase.h"

using namespace TensileConv;
using namespace AutoTune;

/************************************************************************/
/* solution控制															*/
/************************************************************************/
void SolutionCtrlBase::RunSolution()
{
	PRINT_SEPARATOR2();
	OUTPUT("* solution Name: %s.", solutionName.c_str());
	PRINT_SEPARATOR2();

	// 生成解决方案空间
#if MULT_SOLUTION
	generateSolutionParamSpace();
	if (searchMethord == SEARCH_GENETIC)
	{
		geneticSearch->InitGeneticSearch();
	}
#endif

	// 遍历每个problem的solution参数空间
#define TempDo(x)	do{if(x != E_ReturnState::SUCCESS) goto CONTINUE_SEARCH;}while(0)
	while (true)
	{
		getKernelParam();
		TempDo(generateKernel());
		TempDo(prepareKernelArgs());
		TempDo(launchKernel());
#if !MULT_SOLUTION
		getBackResult();
#endif
		releaseDevMem();

	CONTINUE_SEARCH:
		if ((searchMethord == SEARCH_BRUTE)&&(solutionParamSpace->ParamNum > 0))
		{
			if (solutionParamSpace->GetNexComb() == E_ReturnState::FAIL)
			{
				INFO("search kernel parameters finished.");
				break;
			}
		}
		else if (searchMethord == SEARCH_GENETIC)
		{
			if (SearchedKernelCnt >= POP_SIZE * MAX_GENERATION)
				break;
		}
		else
		{
			break;
		}

		sleep(0.5);
	}
#undef TempDo(x)
}

E_ReturnState SolutionCtrlBase::launchKernel()
{
	INFO("warmup.");
	{
		stream->Launch(kernel, global_sz, group_sz, &profEvt);
		stream->Finish();
		usleep(0.1);
	}

	std::vector<double> elapsedTimes;
	elapsedTimes.clear();
	INFO("launch kernel %d times.", repeatTime);
	{
		for (int i = 0; i < repeatTime; i++)
		{
			stream->Launch(kernel, global_sz, group_sz, &profEvt);
			stream->Flush();
			stream->Finish();
			elapsedTimes.push_back(rtOcl->GetProfilingTime(&profEvt));
			usleep(0.01);
		}
	}

	// collect performence
	{
		// for this solution config
		T_Score score;
		score.ElapsedTime = 0;
		for (int i = 0; i < elapsedTimes.size(); i++)
		{
			score.ElapsedTime += elapsedTimes[i];
		}
		score.ElapsedTime /= elapsedTimes.size();
		score.Flops = problem->Calculation() / score.ElapsedTime;
		score.Performence = problem->TheoryElapsedTime() / score.ElapsedTime;
		INFO("elapsed = %.1f(us), performence = %.1f(Gflops) = %.1f%%", 
			score.ElapsedTime * 1e6, score.Flops * 1e-9, score.Performence * 100);

		if (searchMethord == SEARCH_GENETIC)
		{
			geneticSearch->SetOneChromValue(score.ElapsedTime);
			SearchedKernelCnt++;
		}

		// for this problem(all solution config)
		if (solutionScore.ElapsedTime > score.ElapsedTime)
		{
			solutionScore.ElapsedTime = score.ElapsedTime;
			solutionScore.Performence = score.Performence;
			solutionParamSpace->RecordBestComb();
			geneticSearch->RecordCurrChrom();
		}
		INFO("Best for now: elapsed = %.1f(us), performence = %.1f%%",
			solutionScore.ElapsedTime * 1e6, solutionScore.Performence * 100);
	}

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* solver 控制															*/
/************************************************************************/
void SolverCtrlBase::RunSolver()
{
	// add solutions to solver
	generateSolver();

	// 遍历solver的各个solution
	std::list<SolutionCtrlBase*>::iterator solutionIt;
	for (solutionIt = solutionList->begin(); solutionIt != solutionList->end(); solutionIt++)
	{
		SolutionCtrlBase * solution = *solutionIt;
		solution->RunSolution();
		
		T_Score score = solution->SolutionScore();
		scoreList->push_back(score);

		if (bestScore.ElapsedTime > score.ElapsedTime)
		{
			bestScore = score;
			bestSolution = solution;
		}
	}

#if MULT_SOLUTION
	// best solution
	bestSolution->GetBestKernel();
#endif
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
		INFO("initialize host.");				initHostParam(); caculateTheoryPerformance();
#if CPU_VERIFY
		INFO("run host calculate.");			runHostCompute();
#endif
		INFO("solve this problem.");			solver->RunSolver();
#if CPU_VERIFY
		INFO("verify device calculation.");		verifyDevCompute();
#endif
		INFO("release host.");					releaseHostParam();

		if (problemParamSpace->ParamNum > 0)
		{
			INFO("search problem parameters.");
			if (problemParamSpace->GetNexComb() == E_ReturnState::FAIL)
			{
				INFO("search problem parameters finished.");
				break;
			}
		}
		else
		{
			break;
		}

		sleep(5);
	}
}

