#pragma once

#include <limits>
#include <stdarg.h>
#include <vector>
#include <list>

#include "../common/ff_utils.h"
#include "AutoTuning.h"

#include "unistd.h"

using namespace AutoTune;

/************************************************************************/
/* solution得分                                                         */
/************************************************************************/
typedef struct ScoreTypde
{
	double ElapsedTime;	//(s)
	double Flops;		//(Flops)
	double Performence;	//(%)
}T_Score;

/************************************************************************/
/* solution 配置				                                            */
/* 保存需要传递给 KernelWriterBase 的通用参数								*/
/************************************************************************/
typedef struct SolutionConfigTpye
{
	SolutionConfigTpye() {}
	SolutionConfigTpye(std::string name) { ConfigName = name; }

	std::string ConfigName;				// 配置名称
	SearchSpace KernelSearchSpace;		// 解决方案参数搜索空间
	void * extConfig;

	std::string KernelName;			// kernel function name, will used to find source file
	std::string KernelDir;
	std::string KernelFile;			// 可以指定文件名，不使用KernelName推导.需要后缀
	std::string KernelFileFullName;
	std::string kernelString;
	E_ProgramType KernelSrcType;
	std::string extCompilerOpt;

	dim3 group_sz;
	dim3 group_num;
	dim3 global_sz;
}T_SolutionConfig;

/************************************************************************/
/* problem 配置				                                            */
/************************************************************************/
typedef struct ProblemConfigType
{
	ProblemConfigType() {}
	ProblemConfigType(std::string name) { ConfigName = name; }

	std::string ConfigName;				// 问题配置名称
	SearchSpace ProblemParamSpace;		// 问题参数搜索空间
	void * extConfig;

	double Calculation;					// 计算量
	double TheoryElapsedTime;			// 理论执行时间
}T_ProblemConfig;

/************************************************************************/
/* solution 控制 (so called generic solver)			                    */
/************************************************************************/
class SolutionCtrlBase
{
public:
	SolutionCtrlBase()
	{
		repeatTime = 1;
		solutionScore.ElapsedTime = (std::numeric_limits<double>::max)();
		solutionScore.Performence = 0;

		cmdArgs = CmdArgs::GetCmdArgs();
		rtOcl = RuntimeOCL::GetInstance();
		stream = rtOcl->CreatCmdQueue(true);

		solutionConfigList = new std::list<T_SolutionConfig*>;
		solutionConfigList->clear();
	}
	~SolutionCtrlBase() { delete stream; delete solutionConfigList; }
	
	void RunAllSolution(T_ProblemConfig *problem);

protected:
	CmdArgs * cmdArgs;
	RuntimeOCL * rtOcl;
	CmdQueueOCL* stream;	// one command queue for all solution configs
	KernelOCL * kernel;
	cl_event profEvt;

	std::list<T_SolutionConfig*> *solutionConfigList;	// 所有解决方案配置
	T_ProblemConfig * problemConfig;					// 当前正在处理的问题配置
	T_SolutionConfig * solutionConfig;					// 当前正在处理的解决方案配置

	int repeatTime;
	T_Score configScore;				// 当前配置的平均性能
	T_Score solutionScore;				// 全部配置的平均性能

	virtual E_ReturnState generateSolutionConfigs() = 0;
	E_ReturnState runOneSolution();
	virtual E_ReturnState generateKernel() = 0;
	virtual E_ReturnState generateKernelParam() = 0;
	virtual E_ReturnState launchKernel();
	virtual E_ReturnState getBackResult() = 0;
	virtual void releaseKernelParam() = 0;
	virtual void reportProblemPerformence(){}
};

namespace feifei
{
	extern int next2pow(int n);
	extern int log2(int value);
	extern int divCeil(int a, int b);
	extern void printIndex(int *index, char* name, dim3 g_wk, dim3 l_wk);
}
