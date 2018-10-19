#pragma once

#include <stdarg.h>
#include <vector>
#include "BasicClass.h"
#include "RuntimeControl.h"

#include "unistd.h"

using namespace AutoTune;

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
/* solution 配置				                                            */
/* 保存需要传递给 KernelWriterBase 的通用参数								*/
/************************************************************************/
typedef struct SolutionConfigTpye
{
	std::string ConfigName;				// 解决方案名称
	SearchSpace KernelSearchSpace;		// 解决方案参数搜索空间
	void * extConfig;

	std::string KernelName;			// kernel function name, will used to find source file
	std::string KernelFile;			// 可以指定文件名，不使用KernelName推导.需要后缀
	std::string KernelString;
	E_KernleType KernelSrcType;
	std::string extCompilerOpt;

	size_t l_wk0, l_wk1, l_wk2;
	size_t g_wk0, g_wk1, g_wk2;
	size_t b_wk0, b_wk1, b_wk2;

	std::list<T_KernelArgu> * KernelArgus;

	SolutionConfigTpye(std::string name)
	{
		ConfigName = name;
	}
	SolutionConfigTpye()
	{
	}
}T_SolutionConfig;

/************************************************************************/
/* problem 配置				                                            */
/************************************************************************/
typedef struct ProblemConfigType
{
	std::string ConfigName;				// 问题配置名称
	SearchSpace ProblemParamSpace;		// 问题参数搜索空间
	void * extConfig;

	double Calculation;					// 计算量
	double TheoryElapsedTime;			// 理论执行时间

	ProblemConfigType(std::string name)
	{
		ConfigName = name;
	}
	ProblemConfigType()
	{
	}
}T_ProblemConfig;

/************************************************************************/
/* solution 控制 (so called generic solver)			                    */
/************************************************************************/
class SolutionCtrlBase
{
public:
	SolutionCtrlBase();

public:
	void RunSolution(T_ProblemConfig *problem);

	E_ReturnState RunOneSolutionConfig();

	E_ReturnState RunSolutionOnce();

	virtual E_ReturnState LaunchSolution(bool isWarmup);

	virtual E_ReturnState SetupSolution();

	E_ReturnState GetPerformence();

	void printIndex(int *index, char* name);
	
	virtual E_ReturnState InitDev() = 0;
	virtual E_ReturnState GenerateSolutionConfigs() = 0;
	virtual E_ReturnState GenerateSolution() = 0;
	virtual E_ReturnState GetBackResult() = 0;
	virtual void ReleaseDev() = 0;
	virtual void ReportProblemPerformence()
	{
		printf("please report best perfomence.\n");
	}

	T_ProblemConfig * ProblemConfig;					// 当前正在处理的问题配置
	T_SolutionConfig * SolutionConfig;					// 当前正在处理的解决方案配置
	std::list<T_SolutionConfig*> *SolutionConfigList;	// 所有解决方案配置

	int RepeatTime;
	RuntimeCtrl * runtime;			
	std::list<double> ElapsedTimes;		// 每次运行的耗时
	T_Score BestScore;					// 当前解决方案配置的最佳性能
	T_Score AverageScore;				// 当前解决方案配置的平均性能
	double ProblemBestTime;				// 当前问题配置的最佳运行时间
	double ProblemBestPerformence;		// 当前问题配置的最佳性能
};

/************************************************************************/
/* 问题句柄																*/
/************************************************************************/
class ProblemCtrlBase
{
public:
	ProblemCtrlBase();
	ProblemCtrlBase(std::string name);

public:
	void RunProblem();

	E_ReturnState RunOneProblemConfig();

	E_ReturnState RunProblemOnce();

	virtual E_ReturnState GenerateProblemConfigs() = 0;
	virtual E_ReturnState InitHost() = 0;
	virtual E_ReturnState Host() = 0;
	virtual E_ReturnState Verify() = 0;
	virtual void ReleaseHost() = 0;

	std::string ProblemName;
	std::list<T_ProblemConfig*> *ProblemConfigList;	// 所有问题配置
	T_ProblemConfig *ProblemConfig;					// 当前正在处理的问题配置
	SolutionCtrlBase * Solution;
};

