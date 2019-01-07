
#include "TensileConvBase.h"

using namespace feifei;
using namespace AutoTune;

/************************************************************************/
/* solution控制															*/
/************************************************************************/
#pragma region SOLUTION
void SolutionCtrlBase::RunAllSolution(T_ProblemConfig *problem)
{
	problemConfig = problem;

	// ======================================================================
	// 生成解决方案空间
	// ======================================================================
	INFO("generate solution config list.");
	generateSolutionConfigs();

	// ======================================================================
	// 遍历每个problem的solution参数空间
	// ======================================================================
	std::list<T_SolutionConfig*>::iterator solutionCfgIt;
	for (solutionCfgIt = solutionConfigList->begin();
		solutionCfgIt != solutionConfigList->end();
		solutionCfgIt++)
	{
		solutionConfig = *solutionCfgIt;

		PRINT_SEPARATOR1();
		OUTPUT("solution Name: %s.", solutionConfig->ConfigName.c_str());
		PRINT_SEPARATOR1();

		runOneSolution();
	}
}

#define TempDo(x)	do{if(x != E_ReturnState::SUCCESS) goto CONTINUE_SEARCH;}while(0)
E_ReturnState SolutionCtrlBase::runOneSolution()
{
	while (true)
	{
		INFO("generate program and build kernel.");	TempDo(generateKernel());
		INFO("initialize device.");					TempDo(generateKernelParam());
		INFO("launch kernel.");						TempDo(launchKernel());
		INFO("copy result back to cpu.");			TempDo(getBackResult());
		INFO("release resource.");					releaseKernelParam();
		INFO("search kernel parameters.");

	CONTINUE_SEARCH:
		if (solutionConfig->KernelSearchSpace.ParamNum > 0)
		{
			if (solutionConfig->KernelSearchSpace.GetNexComb() == E_ReturnState::FAIL)
			{
				INFO("search kernel parameters finished.");
				reportProblemPerformence();
				return E_ReturnState::SUCCESS;
			}
		}
		else
		{
			return E_ReturnState::SUCCESS;
		}

		sleep(0.1);
	}
}
#undef TempDo(x)

E_ReturnState SolutionCtrlBase::launchKernel()
{
	INFO("warmup.");
	{
		stream->Launch(kernel, solutionConfig->global_sz, solutionConfig->group_sz, &profEvt);
		stream->Finish();
		usleep(0.1);
	}

	std::vector<double> elapsedTimes;
	elapsedTimes.clear();
	INFO("launch kernel %d times.", repeatTime);
	{
		for (int i = 0; i < repeatTime; i++)
		{
			stream->Launch(kernel, solutionConfig->global_sz, solutionConfig->group_sz, &profEvt);
			stream->Finish();
			elapsedTimes.push_back(rtOcl->GetProfilingTime(&profEvt));
			usleep(0.01);
		}
	}

	INFO("collect performence.");
	{
		// for this solution config
		configScore.ElapsedTime = 0;
		for (int i = 0; i < elapsedTimes.size(); i++)
		{
			configScore.ElapsedTime += elapsedTimes[i];
		}
		configScore.ElapsedTime /= elapsedTimes.size();		
		configScore.Flops = problem->Calculation() / configScore.ElapsedTime;
		configScore.Performence = problem->TheoryElapsedTime() / configScore.ElapsedTime;
		INFO("elapsed = %.1f(us), performence = %.1f(Gflops) = %.1f%%.", 
			configScore.ElapsedTime * 1e6, configScore.Flops * 1e-9, configScore.Performence * 100);

		// for this problem(all solution config)
		if (solutionScore.ElapsedTime > configScore.ElapsedTime)
		{
			solutionScore.ElapsedTime = configScore.ElapsedTime;
			solutionScore.Performence = configScore.Performence;
			solutionConfig->KernelSearchSpace.RecordBestComb();
		}
	}

	return E_ReturnState::SUCCESS;
}
#pragma endregion

/************************************************************************/
/* 问题控制																*/
/************************************************************************/
#pragma region PROBLEM
void ProblemCtrlBase::RunAllProblem()
{
	PRINT_SEPARATOR1();
	INFO(" Problem Name: %s.", problemName.c_str());
	PRINT_SEPARATOR1();

	// ======================================================================
	// 遍历problem参数空间,搜索参数空间
	// ======================================================================
	std::list<T_ProblemConfig*>::iterator problemCfgIt;
	for (problemCfgIt = problemConfigList->begin(); problemCfgIt != problemConfigList->end(); problemCfgIt++)
	{
		problemConfig = *problemCfgIt;
		runOneProblem();
	}
}

E_ReturnState ProblemCtrlBase::runOneProblem()
{
	while (true)
	{
		INFO("initialize host.");				initHostParam(); caculateTheoryPerformance();
		INFO("run host calculate.");			runHostCompute();
		INFO("solve this problem.");			solution->RunAllSolution(problemConfig);
		INFO("verify device calculation.");		verifyDevCompute();
		INFO("release host.");					releaseHostParam();

		if (problemConfig->ProblemParamSpace.ParamNum > 0)
		{
			INFO("search problem parameters.");
			if (problemConfig->ProblemParamSpace.GetNexComb() == E_ReturnState::FAIL)
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

	return E_ReturnState::SUCCESS;
}
#pragma endregion

namespace feifei
{
	int next2pow(int n)
	{
		int base = 1;
		for (int i = 0; i < 32; i++)
		{
			base = 1 << i;
			if (n <= base)
			{
				break;
			}
		}
		return base;
	}

	int log2(int value)
	{
		int log2 = 0;
		while (value > 1)
		{
			value = value / 2;
			log2++;
		}
		return log2;
	}

	int divCeil(int a, int b)
	{
		return ((a + b - 1) / b);
	}

	void printIndex(int *index, char* name, dim3 g_wk, dim3 l_wk)
	{
		int groupNum = g_wk.x / l_wk.x;
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
		}
	}
}
