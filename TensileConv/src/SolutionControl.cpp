#include <limits>
#include "SolutionControl.h"

using namespace feifei;
using namespace AutoTune;


void SolutionCtrlBase::RunAllSolution(T_ProblemConfig *problem)
{
	ProblemConfig = problem;

	// ======================================================================
	// 生成解决方案空间
	// ======================================================================
	INFO("generate solution config list.");
	GenerateSolutionConfigs();

	// ======================================================================
	// 遍历每个problem的solution参数空间
	// ======================================================================
	std::list<T_SolutionConfig*>::iterator solutionCfgIt;
	for (solutionCfgIt = SolutionConfigList->begin();
		solutionCfgIt != SolutionConfigList->end();
		solutionCfgIt++)
	{
		SolutionConfig = *solutionCfgIt;

		PRINT_SEPARATOR1();
		INFO("Solution Name: %s.", SolutionConfig->ConfigName.c_str());
		PRINT_SEPARATOR1();

		RunOneSolution();
	}
}

#define TempDo(x)	do{if(x != E_ReturnState::SUCCESS) goto CONTINUE_SEARCH;}while(0)
E_ReturnState SolutionCtrlBase::RunOneSolution()
{
	while (true)
	{
		INFO("generate program and build kernel.");	TempDo(GenerateSolution());
		INFO("initialize device.");					TempDo(InitDev());	// alloc dev mem and NOT create cmd queue
		INFO("warmup.");							TempDo(LaunchSolution(true));
		INFO("launch kernel.");						TempDo(LaunchSolution(false));
		INFO("collect performence.");				TempDo(GetPerformence());
		INFO("copy result back to cpu.");			TempDo(GetBackResult());
		INFO("release resource.");					ReleaseDev();	// release dev mem and cmd queue
		INFO("search kernel parameters.");

	CONTINUE_SEARCH:
		if (SolutionConfig->KernelSearchSpace.ParamNum > 0)
		{
			if (SolutionConfig->KernelSearchSpace.GetNexComb() == E_ReturnState::FAIL)
			{
				INFO("search kernel parameters finished.");
				ReportProblemPerformence();
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

E_ReturnState SolutionCtrlBase::LaunchSolution(bool isWarmup)
{
	if (isWarmup)
	{
		stream->Launch(kernel, SolutionConfig->global_sz, SolutionConfig->group_sz, &profEvt);
		stream->Finish();
		elapsedTimes.clear();
		usleep(0.1);
	}
	else
	{
		for (int i = 0; i < RepeatTime; i++)
		{
			stream->Launch(kernel, SolutionConfig->global_sz, SolutionConfig->group_sz, &profEvt);
			stream->Finish();
			elapsedTimes.push_back(rtOcl->GetProfilingTime(&profEvt));
			usleep(0.01);
		}
	}

	return E_ReturnState::SUCCESS;
}

E_ReturnState SolutionCtrlBase::GetPerformence()
{
	// 统计时间
	AverageScore.ElapsedTime = 0;
	BestScore.ElapsedTime = (std::numeric_limits<double>::max)();
	for (int i = 0; i < elapsedTimes.size(); i++)
	{
		AverageScore.ElapsedTime += elapsedTimes[i];
		if (elapsedTimes[i] < BestScore.ElapsedTime)
			BestScore.ElapsedTime = elapsedTimes[i];
	}
	AverageScore.ElapsedTime /= elapsedTimes.size();
	
	INFO("average elapsed time: %.3f (us).", AverageScore.ElapsedTime * 1e9);
	INFO("best elapsed time: %.3f (us).", BestScore.ElapsedTime * 1e6);

	// 性能换算
/*	AverageScore.Flops = ProblemConfig->Calculation / AverageScore.ElapsedTime;
	AverageScore.Performence = ProblemConfig->TheoryElapsedTime / AverageScore.ElapsedTime;
	BestScore.Flops = ProblemConfig->Calculation / BestScore.ElapsedTime;
	BestScore.Performence = ProblemConfig->TheoryElapsedTime / BestScore.ElapsedTime;

	printf("best performence: %.1f (Gflops) = %.1f%%.\n", BestScore.Flops * 1e-9, BestScore.Performence * 100);
	printf("average performence: %.1f (Gflops) = %.1f%%.\n", AverageScore.Flops * 1e-9, AverageScore.Performence * 100);
		
	if ((ProblemBestTime < 0) || (ProblemBestTime > AverageScore.ElapsedTime))
	{
		ProblemBestTime = AverageScore.ElapsedTime;
		ProblemBestPerformence = AverageScore.Performence;
		SolutionConfig->KernelSearchSpace.RecordBestComb();
	}*/

	return E_ReturnState::SUCCESS;
}


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