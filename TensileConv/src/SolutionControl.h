#pragma once

#include <stdarg.h>
#include <vector>
#include <list>

#include "../common/ff_utils.h"
#include "AutoTuning.h"

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
	std::string KernelDir;
	std::string KernelFile;			// 可以指定文件名，不使用KernelName推导.需要后缀
	std::string KernelFileFullName;
	std::string KernelString;
	E_ProgramType KernelSrcType;
	std::string extCompilerOpt;

	size_t l_wk0, l_wk1, l_wk2;
	size_t g_wk0, g_wk1, g_wk2;
	size_t b_wk0, b_wk1, b_wk2;

	SolutionConfigTpye(std::string name)
	{
		ConfigName = name;
	}
	SolutionConfigTpye()
	{
	}
}T_SolutionConfig;

/************************************************************************/
/* solution 控制 (so called generic solver)			                    */
/************************************************************************/
class SolutionCtrlBase
{
public:
	SolutionCtrlBase()
	{
		RepeatTime = 1;
		cmdArgs = CmdArgs::GetCmdArgs();
		pOclRt = RuntimeOCL::GetInstance();

		SolutionConfigList = new std::list<T_SolutionConfig*>;
	}

public:
	void RunSolution(T_ProblemConfig *problem);

	E_ReturnState RunOneSolutionConfig();

	E_ReturnState RunSolutionOnce();

	void CleanSolution();

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
	//RuntimeCtrl * runtime;			
	std::list<double> ElapsedTimes;		// 每次运行的耗时
	T_Score BestScore;					// 当前解决方案配置的最佳性能
	T_Score AverageScore;				// 当前解决方案配置的平均性能
	double ProblemBestTime;				// 当前问题配置的最佳运行时间
	double ProblemBestPerformence;		// 当前问题配置的最佳性能

protected:
	CmdArgs * cmdArgs;
	int devId;

	RuntimeOCL * pOclRt;
	CmdQueueOCL* stream;	// command queue
};
