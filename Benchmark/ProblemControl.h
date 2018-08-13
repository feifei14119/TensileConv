#pragma once

#include <stdarg.h>
#include <vector>
#include "BasicClass.h"
#include "RuntimeControl.h"

#include "unistd.h"

/************************************************************************/
/* solution得分                                                         */
/************************************************************************/
typedef struct ScoreTypde
{
	double ElapsedTime;
	double Flops;
	double Performence;
}T_Score;

/************************************************************************/
/* solution 配置				                                             */
/************************************************************************/
typedef struct SolutionConfigTpye
{
	std::string ConfigName;
	std::string KernelName;			// kernel function name, will used to find source file
	std::string KernelFile;			// 可以指定文件名，不使用KernelName推导.需要后缀
	std::string KernelString;
	E_KernleType KernelSrcType;
	std::string extCompilerOpt;
	int RepeatTime;
	std::list<double> ElapsedTimes;
	T_Score BestScore;
	T_Score AverageScore;

	size_t l_wk0, l_wk1, l_wk2;
	size_t g_wk0, g_wk1, g_wk2;
	size_t b_wk0, b_wk1, b_wk2;
	
	std::list<T_KernelArgu> * KernelArgus;
	SearchSpace KernelSearchSpace;

	void * extConfig;
}T_SolutionConfig;

/************************************************************************/
/* problem 配置				                                            */
/************************************************************************/
typedef struct ProblemConfigType
{
	std::string ConfigName;
	double Calculation;
	double TheoryElapsedTime;

	void * extConfig;
	SearchSpace ProblemSearchSpace;
}T_ProblemConfig;

/************************************************************************/
/* solution 控制 (so called generic solver)			                    */
/************************************************************************/
class SolutionCtrlBase
{
public:
	SolutionCtrlBase()
	{
		SolutionConfigList = new std::list<T_SolutionConfig*>;
	}

	void RunSolution(T_ProblemConfig *problem)
	{
		problemCfg = problem;

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
			solutionCfg = *solutionCfgIt;

			printf("======================================================================\n");
			printf("Solution Name: %s.\n", solutionCfg->ConfigName.c_str());
			printf("======================================================================\n");
			
			if (solutionCfg->KernelSearchSpace.ParamNum > 0)
			{
				RunOneSolutionConfig();
			}
			else
			{
				RunSolutionOnce();
			}
		}
	}

	E_ReturnState RunOneSolutionConfig()
	{

		while (true)
		{
			runtime = new RuntimeCtrl();

			INFO("initialize device.");						InitDev();
			INFO("search kernel parameters.");
			if (solutionCfg->KernelSearchSpace.SearchTemp() == E_ReturnState::FAIL)
			{
				INFO("search kernel parameters finished.");
				ReportProblemPerformence();
				return E_ReturnState::SUCCESS;
			}
			INFO("generate source, compiler, worksize.");	GenerateSolution();
			INFO("compiler kernel and program.");			SetupSolution();
			INFO("set arg and launch kernel.");				LaunchSolution();
			INFO("collect performence.");					GetPerformence();
			INFO("copy result back to cpu.");				GetBackResult();
			INFO("release device.");						ReleaseDev();

			delete runtime;
		}
	
	}

	E_ReturnState RunSolutionOnce()
	{
		runtime = new RuntimeCtrl();
		INFO("initialize device.");						InitDev();
		INFO("generate source, compiler, worksize.");	GenerateSolution();
		INFO("compiler kernel and program.");			SetupSolution();
		INFO("set arg and launch kernel.");				LaunchSolution();
		INFO("collect performence.");					GetPerformence();
		INFO("copy result back to cpu.");				GetBackResult();
		INFO("release device.");						ReleaseDev();
		delete runtime;
	}

	virtual E_ReturnState LaunchSolution()
	{
		std::list<T_KernelArgu>::iterator args;

		int i = 0;
		for (args = solutionCfg->KernelArgus->begin();
			args != solutionCfg->KernelArgus->end(); args++)
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

		solutionCfg->RepeatTime = solutionCfg->RepeatTime == 0 ? 1 : solutionCfg->RepeatTime;
		for (int i = 0; i < solutionCfg->RepeatTime; i++)
		{
			runtime->LanchKernel();
			solutionCfg->ElapsedTimes.push_back(runtime->ElapsedTime);
			usleep(1);
		}

		return E_ReturnState::SUCCESS;
	}

	virtual E_ReturnState SetupSolution()
	{
		runtime->KernelName = solutionCfg->KernelName;
		runtime->KernelSrcType = solutionCfg->KernelSrcType;
		runtime->extCompilerOpt = solutionCfg->extCompilerOpt;
		solutionCfg->b_wk0 = solutionCfg->g_wk0 / solutionCfg->l_wk0;
		solutionCfg->b_wk1 = solutionCfg->g_wk1 / solutionCfg->l_wk1;
		solutionCfg->b_wk2 = solutionCfg->g_wk2 / solutionCfg->l_wk2;
		runtime->SetBlockSize(dim3(solutionCfg->l_wk0, solutionCfg->l_wk1, solutionCfg->l_wk2));
		runtime->SetGridSize(dim3(solutionCfg->b_wk0, solutionCfg->b_wk1, solutionCfg->b_wk2));
		
		printf("l_wk=(%d, %d, %d)\n", solutionCfg->l_wk0, solutionCfg->l_wk1, solutionCfg->l_wk2);
		printf("b_wk=(%d, %d, %d)\n", solutionCfg->b_wk0, solutionCfg->b_wk1, solutionCfg->b_wk2);
		printf("g_wk=(%d, %d, %d)\n", solutionCfg->g_wk0, solutionCfg->g_wk1, solutionCfg->g_wk2);
		std::cout << "compile options=" << solutionCfg->extCompilerOpt << std::endl;

		// build source file
		runtime->GetFilesName(solutionCfg->KernelFile);
		runtime->KernelString = solutionCfg->KernelString;
		runtime->CreatSolution();

		// warm up
		LaunchSolution();

		solutionCfg->ElapsedTimes.clear();

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState GetPerformence()
	{
		// 平均时间
		std::list<double>::iterator elp;
		solutionCfg->AverageScore.ElapsedTime = 0;
		for (elp = solutionCfg->ElapsedTimes.begin(); elp != solutionCfg->ElapsedTimes.end(); elp++)
		{
			//printf("elapsed time %.3f us\n",*elp * 1e6);
			solutionCfg->AverageScore.ElapsedTime += *elp;
		}
		solutionCfg->AverageScore.ElapsedTime /= solutionCfg->ElapsedTimes.size();

		// 最短时间
		solutionCfg->ElapsedTimes.sort();
		solutionCfg->BestScore.ElapsedTime = solutionCfg->ElapsedTimes.front();

		// 性能换算
		solutionCfg->AverageScore.Flops = problemCfg->Calculation / solutionCfg->AverageScore.ElapsedTime;
		solutionCfg->AverageScore.Performence = problemCfg->TheoryElapsedTime / solutionCfg->AverageScore.ElapsedTime;
		solutionCfg->BestScore.Flops = problemCfg->Calculation / solutionCfg->BestScore.ElapsedTime;
		solutionCfg->BestScore.Performence = problemCfg->TheoryElapsedTime / solutionCfg->BestScore.ElapsedTime;

		printf("best elapsed time: %.3f (us).\n", solutionCfg->BestScore.ElapsedTime * 1e6);
		printf("best performence: %.1f (Gflops) = %.1f%%.\n", solutionCfg->BestScore.Flops * 1e-9, solutionCfg->BestScore.Performence * 100);
		printf("average elapsed time: %.3f (us).\n", solutionCfg->AverageScore.ElapsedTime * 1e6);
		printf("average performence: %.1f (Gflops) = %.1f%%.\n", solutionCfg->AverageScore.Flops * 1e-9, solutionCfg->AverageScore.Performence * 100);


		if ((ProblemBestTime < 0) || (ProblemBestTime > solutionCfg->AverageScore.ElapsedTime))
		{
			ProblemBestTime = solutionCfg->AverageScore.ElapsedTime;
			ProblemBestPerformence = solutionCfg->AverageScore.Performence;
		}
	}
	
	virtual E_ReturnState InitDev() = 0;
	virtual E_ReturnState GenerateSolutionConfigs() = 0;
	virtual E_ReturnState GenerateSolution() = 0;
	virtual E_ReturnState GetBackResult() = 0;
	virtual void ReleaseDev() = 0;
	virtual void ReportProblemPerformence()
	{
		printf("please report best perfomence.\n");
	}

	RuntimeCtrl * runtime;
	T_ProblemConfig * problemCfg;
	T_SolutionConfig * solutionCfg;
	std::list<T_SolutionConfig*> *SolutionConfigList;

	public:
		double ProblemBestTime;
		double ProblemBestPerformence;
};

/************************************************************************/
/* 问题句柄																*/
/************************************************************************/
class ProblemCtrlBase
{
public:
	void RunProblem()
	{
		printf("************************************************************************\n");
		printf("* Problem Name: %s.\n", ProblemName.c_str());
		printf("************************************************************************\n");

		// ======================================================================
		// 生成问题空间
		// ======================================================================
		INFO("generate problem config list.");
		GenerateProblemConfigs();

		// ======================================================================
		// 遍历problem参数空间,搜索参数空间
		// ======================================================================
		std::list<T_ProblemConfig*>::iterator problemCfgIt;
		for (problemCfgIt = ProblemConfigList->begin(); 
			problemCfgIt != ProblemConfigList->end(); 
			problemCfgIt++)
		{
			problemCfg = *problemCfgIt;

			printf("************************************************************************\n");
			printf("* Problem Name: %s.\n", ProblemName.c_str());
			printf("* Problem Config: %s.\n", problemCfg->ConfigName.c_str());
			printf("************************************************************************\n");

			if (problemCfg->ProblemSearchSpace.ParamNum > 0)
			{
				RunOneProblemConfig();
			}
			else
			{
				RunProblemOnce();
			}
		}
	}

	E_ReturnState RunOneProblemConfig()
	{
		while (true)
		{
			INFO("search problem parameters.");
			if (problemCfg->ProblemSearchSpace.Search() == E_ReturnState::FAIL)
			{
				INFO("search problem parameters finished.");
				break;
			}
			Solution->ProblemBestTime = -1;
			INFO("initialize host.");			InitHost();
			INFO("run host calculate.");		Host();
			INFO("solve this problem.");		Solution->RunSolution(problemCfg);
			INFO("verify device calculation.");	Verify();
			INFO("release host.");				ReleaseHost();
		}
	}

	E_ReturnState RunProblemOnce()
	{
		INFO("initialize host.");				InitHost();
		INFO("run host calculate.");			Host();
		INFO("solve this problem.");			Solution->RunSolution(problemCfg);
		INFO("verify device calculation.");		Verify();
		INFO("release host.");					ReleaseHost();

		return E_ReturnState::SUCCESS;
	}

	virtual E_ReturnState GenerateProblemConfigs() = 0;
	virtual E_ReturnState InitHost() = 0;
	virtual E_ReturnState Host() = 0;
	virtual E_ReturnState Verify() = 0;
	virtual void ReleaseHost() = 0;

	SolutionCtrlBase * Solution;
	T_ProblemConfig *problemCfg;
	std::string ProblemName;
	std::list<T_ProblemConfig*> *ProblemConfigList;
};

#include "TestProblem.h"

#include "IsaFlat.h"
#include "IsaGlobal.h"
#include "IsaDs.h"
#include "IsaMubuf.h"
#include "IsaSleep.h"

#include "VectorAdd.h"
#include "ProducerConsumer.h"

#include "ConvFwd1x1.h"

