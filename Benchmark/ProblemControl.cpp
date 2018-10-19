
#include "ProblemControl.h"

using namespace AutoTune;

/************************************************************************/
/* solution 控制 (so called generic solver)			                    */
/************************************************************************/
#pragma region SolutionRegion
SolutionCtrlBase::SolutionCtrlBase()
{
	RepeatTime = 1;
	SolutionConfigList = new std::list<T_SolutionConfig*>;
}

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
		runtime = new RuntimeCtrl(false);

		INFO("initialize device.");						InitDev();
		INFO("generate source, compiler, worksize.");	if (GenerateSolution() != E_ReturnState::SUCCESS)	goto CONTINUE_SEARCH;
		INFO("compiler kernel and program.");			SetupSolution();
		INFO("set arg and launch kernel.");				LaunchSolution(false);
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

		delete runtime;
		//sleep(1);
	}
}

E_ReturnState SolutionCtrlBase::RunSolutionOnce()
{
	runtime = new RuntimeCtrlOcl(false);
	INFO("initialize device.");						InitDev();
	INFO("generate source, compiler, worksize.");	GenerateSolution();
	INFO("compiler kernel and program.");			SetupSolution();
	INFO("set arg and launch kernel.");				LaunchSolution(false);
	INFO("collect performence.");					GetPerformence();
	INFO("copy result back to cpu.");				GetBackResult();
	INFO("release device.");						ReleaseDev();
	delete runtime;
}

E_ReturnState SolutionCtrlBase::LaunchSolution(bool isWarmup)
{
	std::list<T_KernelArgu>::iterator args;

	INFO("setup arguments.");
	int i = 0;
	for (args = SolutionConfig->KernelArgus->begin();
		args != SolutionConfig->KernelArgus->end(); args++)
	{
		if ((*args).isVal == true)
		{
			if ((*args).ptr == NULL)
			{
				DevCheckFunc(clSetKernelArg(runtime->kernel, i, sizeof(cl_mem), (void*)NULL));
			}
			else
			{
				DevCheckFunc(clSetKernelArg(runtime->kernel, i, (*args).size, (void*)((*args).ptr)));
			}
		}
		else
		{
			DevCheckFunc(clSetKernelArg(runtime->kernel, i, (*args).size, &(*args).ptr));
		}
		i++;
	}

	INFO("launch kernel.");
	if (isWarmup)
	{
		runtime->LanchKernel();
		usleep(1);
	}
	else
	{
		for (int i = 0; i < RepeatTime; i++)
		{
			runtime->LanchKernel();
			ElapsedTimes.push_back(runtime->ElapsedTime);
			usleep(1);
		}
	}

	return E_ReturnState::SUCCESS;
}

E_ReturnState SolutionCtrlBase::SetupSolution()
{
	runtime->KernelName = SolutionConfig->KernelName;
	runtime->KernelSrcType = SolutionConfig->KernelSrcType;
	runtime->extCompilerOpt = SolutionConfig->extCompilerOpt;
	SolutionConfig->b_wk0 = SolutionConfig->g_wk0 / SolutionConfig->l_wk0;
	SolutionConfig->b_wk1 = SolutionConfig->g_wk1 / SolutionConfig->l_wk1;
	SolutionConfig->b_wk2 = SolutionConfig->g_wk2 / SolutionConfig->l_wk2;
	runtime->SetBlockSize(dim3(SolutionConfig->l_wk0, SolutionConfig->l_wk1, SolutionConfig->l_wk2));
	runtime->SetGridSize(dim3(SolutionConfig->b_wk0, SolutionConfig->b_wk1, SolutionConfig->b_wk2));
		
	printf("l_wk=(%d, %d, %d)\n", SolutionConfig->l_wk0, SolutionConfig->l_wk1, SolutionConfig->l_wk2);
	printf("b_wk=(%d, %d, %d)\n", SolutionConfig->b_wk0, SolutionConfig->b_wk1, SolutionConfig->b_wk2);
	printf("g_wk=(%d, %d, %d)\n", SolutionConfig->g_wk0, SolutionConfig->g_wk1, SolutionConfig->g_wk2);
	std::cout << "compile options=" << SolutionConfig->extCompilerOpt << std::endl;

	// build source file
	runtime->GetFilesName(SolutionConfig->KernelFile);
	runtime->KernelString = SolutionConfig->KernelString;
	runtime->CreatSolution();

	// warm up
	INFO("warm up.");
	LaunchSolution(true);

	ElapsedTimes.clear();

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
#pragma endregion

/************************************************************************/
/* 问题句柄																*/
/************************************************************************/
#pragma region ProblemRegion
ProblemCtrlBase::ProblemCtrlBase()
{
	ProblemConfigList = new std::list<T_ProblemConfig*>;
}

ProblemCtrlBase::ProblemCtrlBase(std::string name)
{
	ProblemName = name;
	ProblemConfigList = new std::list<T_ProblemConfig*>;
}

void ProblemCtrlBase::RunProblem()
{
	printf("************************************************************************\n");
	printf("* Problem Name: %s.\n", ProblemName.c_str());
	printf("************************************************************************\n");

	// ======================================================================
	// 生成问题空间
	// ======================================================================
	INFO("generate problem config list.");
	ProblemConfigList->clear();
	GenerateProblemConfigs();

	// ======================================================================
	// 遍历problem参数空间,搜索参数空间
	// ======================================================================
	std::list<T_ProblemConfig*>::iterator problemCfgIt;
	for (problemCfgIt = ProblemConfigList->begin(); 
		problemCfgIt != ProblemConfigList->end(); 
		problemCfgIt++)
	{
		ProblemConfig = *problemCfgIt;

		printf("************************************************************************\n");
		printf("* Problem Name: %s.\n", ProblemName.c_str());
		printf("* Problem Config: %s.\n", ProblemConfig->ConfigName.c_str());
		printf("************************************************************************\n");

		if (ProblemConfig->ProblemParamSpace.ParamNum > 0)
		{
			RunOneProblemConfig();
		}
		else
		{
			RunProblemOnce();
		}
	}
}

E_ReturnState ProblemCtrlBase::RunOneProblemConfig()
{
	while (true)
	{
		Solution->ProblemBestTime = -1;
		INFO("initialize host.");			InitHost();
		INFO("run host calculate.");		Host();
		INFO("solve this problem.");		Solution->RunSolution(ProblemConfig);
		INFO("verify device calculation.");	Verify();
		INFO("release host.");				ReleaseHost();

		INFO("search problem parameters.");
		if (ProblemConfig->ProblemParamSpace.GetNexComb() == E_ReturnState::FAIL)
		{
			INFO("search problem parameters finished.");
			break;
		}
	}
}

E_ReturnState ProblemCtrlBase::RunProblemOnce()
{
	Solution->ProblemBestTime = -1;
	INFO("initialize host.");				InitHost();
	INFO("run host calculate.");			Host();
	INFO("solve this problem.");			Solution->RunSolution(ProblemConfig);
	INFO("verify device calculation.");		Verify();
	INFO("release host.");					ReleaseHost();

	return E_ReturnState::SUCCESS;
}
#pragma endregion

