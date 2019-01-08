
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
	INFO("generate solution config list.");
	generateSolutionParamSpace();

	// 遍历每个problem的solution参数空间
#define TempDo(x)	do{if(x != E_ReturnState::SUCCESS) goto CONTINUE_SEARCH;}while(0)
	while (true)
	{
		getKernelParam();
		INFO("generate program and build kernel.");	TempDo(generateKernel());
		INFO("initialize device.");					TempDo(prepareKernelArgs());
		INFO("launch kernel.");						TempDo(launchKernel());
		//INFO("copy result back to cpu.");			TempDo(getBackResult());
		INFO("release resource.");					releaseDevMem();
		INFO("search kernel parameters.");

	CONTINUE_SEARCH:
		if (solutionParamSpace->ParamNum > 0)
		{
			if (solutionParamSpace->GetNexComb() == E_ReturnState::FAIL)
			{
				INFO("search kernel parameters finished.");
				break;
			}
		}
		else
		{
			break;
		}

		sleep(0.1);
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
			stream->Finish();
			elapsedTimes.push_back(rtOcl->GetProfilingTime(&profEvt));
			usleep(0.01);
		}
	}

	INFO("collect performence.");
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
		INFO("elapsed = %.1f(us), performence = %.1f(Gflops) = %.1f%%.", 
			score.ElapsedTime * 1e6, score.Flops * 1e-9, score.Performence * 100);

		// for this problem(all solution config)
		if (solutionScore.ElapsedTime > score.ElapsedTime)
		{
			solutionScore.ElapsedTime = score.ElapsedTime;
			solutionScore.Performence = score.Performence;
			solutionParamSpace->RecordBestComb();
		}
	}

	return E_ReturnState::SUCCESS;
}

void SolutionCtrlBase::printIndex(int *index, char* name, dim3 g_wk, dim3 l_wk)
{
	/*int groupNum = g_wk.x / l_wk.x;
	int grpNumPerCUMax = (groupNum + CU_NUM - 1) / CU_NUM;
	int grpNumPerCUMin = groupNum / CU_NUM;
	int maxGrpCUNum = (groupNum - grpNumPerCUMin * CU_NUM) / SE_NUM;
	int minGrpCUNum = (CU_NUM - maxGrpCUNum * SE_NUM) / SE_NUM;

	int waveNumPerCUMax = grpNumPerCUMax * (l_wk.x / WAVE_SIZE);
	int waveNumPerCUMin = grpNumPerCUMin * (l_wk.x / WAVE_SIZE);
	int simuGrpIdx = 0;
	int grpIdxBase;

	printf("\t|---------------------------------------------------------\n");
	printf("\t| index name = %s\n", name);
	printf("\t| group size = %d\n", l_wk.x);
	printf("\t| group number = %d\n", groupNum);
	printf("\t| group number per cu = (%d * %d) + (%d * %d)\n",
		grpNumPerCUMax, maxGrpCUNum, grpNumPerCUMin, minGrpCUNum);
	printf("\t| wave number per cu = (%d * %d) + (%d * %d)\n",
		waveNumPerCUMax, maxGrpCUNum, waveNumPerCUMin, minGrpCUNum);
	printf("\t|---------------------------------------------------------\n");
	for (int se = 0; se < SE_NUM; se++)
	{
		printf("SE=%d:", se);
		grpIdxBase = se;

		for (int cu = 0; cu < CU_PER_SE; cu++)
		{
			printf("\t[%02d]: ", cu);
			simuGrpIdx = grpIdxBase;

			while (simuGrpIdx < groupNum)
			{
				printf("%03d, ", index[simuGrpIdx]);
				simuGrpIdx += CU_NUM;
			}
			printf("\n");
			grpIdxBase += 4;
		}
		printf("\n");
	}*/
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
		INFO("initialize host.");				initHostParam(); caculateTheoryPerformance();
		INFO("run host calculate.");			runHostCompute();
		INFO("solve this problem.");			solver->RunSolver();
		INFO("verify device calculation.");		verifyDevCompute();
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
	}
}

