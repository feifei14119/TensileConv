
#include "SolutionControl.h"

using namespace feifei;
using namespace AutoTune;


void SolutionCtrlBase::RunSolution(T_ProblemConfig *problem)
{
	ProblemConfig = problem;

	// ======================================================================
	// 生成解决方案空间
	// ======================================================================
	INFO("generate solution config list.");
	SolutionConfigList->clear();
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

		printf("======================================================================\n");
		printf("Solution Name: %s.\n", SolutionConfig->ConfigName.c_str());
		printf("======================================================================\n");
		
		if (SolutionConfig->KernelSearchSpace.ParamNum > 0)
		{
			RunOneSolutionConfig();
		}
		else
		{
			RunSolutionOnce();
		}
	}
}

E_ReturnState SolutionCtrlBase::RunOneSolutionConfig()
{
	while (true)
	{
		//runtime = new RuntimeCtrlOcl();

		INFO("generate source, compiler, worksize.");	if (GenerateSolution() != E_ReturnState::SUCCESS)	goto CONTINUE_SEARCH;
		INFO("compiler kernel and program.");			SetupSolution();
		INFO("initialize device.");						InitDev();
		INFO("warmup.");								LaunchSolution(true);
		INFO("launch kernel.");							LaunchSolution(false);
		INFO("collect performence.");					GetPerformence();
		INFO("copy result back to cpu.");				GetBackResult();
		INFO("release device.");						ReleaseDev();
		INFO("search kernel parameters.");
	CONTINUE_SEARCH:
		if (SolutionConfig->KernelSearchSpace.GetNexComb() == E_ReturnState::FAIL)
		{
			INFO("search kernel parameters finished.");
			ReportProblemPerformence();
			return E_ReturnState::SUCCESS;
		}

		CleanSolution();
	}
}

E_ReturnState SolutionCtrlBase::RunSolutionOnce()
{
	INFO("generate source, compiler, worksize.");	GenerateSolution();
	INFO("compiler kernel and program.");			SetupSolution();
	INFO("initialize device.");						InitDev();
	INFO("warmup.");								LaunchSolution(true);
	INFO("launch kernel.");							LaunchSolution(false);
	INFO("collect performence.");					GetPerformence();
	INFO("copy result back to cpu.");				GetBackResult();
	INFO("release device.");						ReleaseDev();

	CleanSolution();
}

void SolutionCtrlBase::CleanSolution()
{
	delete stream;
}

E_ReturnState SolutionCtrlBase::LaunchSolution(bool isWarmup)
{
	if (isWarmup)
	{
		stream->SetupDispatch(&dispatch_param);
		stream->Launch(1);
		stream->Wait();
		usleep(1);
	}
	else
	{
		ElapsedTimes.clear();
		stream->SetupDispatch(&dispatch_param);
		stream->Launch(RepeatTime);
		stream->Wait();
		usleep(1);
	}

	return E_ReturnState::SUCCESS;
}

E_ReturnState SolutionCtrlBase::SetupSolution()
{
	stream = (OCLStream*)gpuDevice->CreateStream();

	//code_obj = (*compiler)(SolutionConfig->KernelName, SolutionConfig->KernelFileFullName, gpuDevice);
	code_obj = (*bcompiler)(SolutionConfig->KernelName, SolutionConfig->KernelFileFullName, gpuDevice);
	kernel_obj = code_obj->CreateKernelObject(SolutionConfig->KernelName.c_str());	
	dispatch_param.kernel_obj = kernel_obj;

	return E_ReturnState::SUCCESS;
}

E_ReturnState SolutionCtrlBase::GetPerformence()
{
	// 平均时间
	std::list<double>::iterator elp;
	AverageScore.ElapsedTime = 0;
	for (elp = ElapsedTimes.begin(); elp != ElapsedTimes.end(); elp++)
	{
		//printf("elapsed time %.3f us\n",*elp * 1e6);
		AverageScore.ElapsedTime += *elp;
	}
	AverageScore.ElapsedTime /= ElapsedTimes.size();

	// 最短时间
	ElapsedTimes.sort();
	BestScore.ElapsedTime = ElapsedTimes.front();

	// 性能换算
	AverageScore.Flops = ProblemConfig->Calculation / AverageScore.ElapsedTime;
	AverageScore.Performence = ProblemConfig->TheoryElapsedTime / AverageScore.ElapsedTime;
	BestScore.Flops = ProblemConfig->Calculation / BestScore.ElapsedTime;
	BestScore.Performence = ProblemConfig->TheoryElapsedTime / BestScore.ElapsedTime;

	printf("best elapsed time: %.3f (us).\t", BestScore.ElapsedTime * 1e6);
	printf("best performence: %.1f (Gflops) = %.1f%%.\n", BestScore.Flops * 1e-9, BestScore.Performence * 100);
	printf("average elapsed time: %.3f (us).\t", AverageScore.ElapsedTime * 1e6);
	printf("average performence: %.1f (Gflops) = %.1f%%.\n", AverageScore.Flops * 1e-9, AverageScore.Performence * 100);
		
	if ((ProblemBestTime < 0) || (ProblemBestTime > AverageScore.ElapsedTime))
	{
		ProblemBestTime = AverageScore.ElapsedTime;
		ProblemBestPerformence = AverageScore.Performence;
		SolutionConfig->KernelSearchSpace.RecordBestComb();
	}
}

void SolutionCtrlBase::printIndex(int *index, char* name)
{
	int groupNum = SolutionConfig->g_wk0 / SolutionConfig->l_wk0;
	int grpNumPerCUMax = (groupNum + CU_NUM - 1) / CU_NUM;
	int grpNumPerCUMin = groupNum / CU_NUM;
	int maxGrpCUNum = (groupNum - grpNumPerCUMin * CU_NUM) / SE_NUM;
	int minGrpCUNum = (CU_NUM - maxGrpCUNum * SE_NUM) / SE_NUM;

	int waveNumPerCUMax = grpNumPerCUMax * (SolutionConfig->l_wk0 / WAVE_SIZE);
	int waveNumPerCUMin = grpNumPerCUMin * (SolutionConfig->l_wk0 / WAVE_SIZE);
	int simuGrpIdx = 0;
	int grpIdxBase;

	printf("\t|---------------------------------------------------------\n");
	printf("\t| index name = %s\n", name);
	printf("\t| group size = %d\n", SolutionConfig->l_wk0);
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
