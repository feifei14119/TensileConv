#pragma once

#include <limits>
#include <stdarg.h>
#include <vector>
#include <list>

#include "../common/ff_utils.h"
#include "AutoTuning.h"

#include "unistd.h"

namespace TensileConv {

#define		MULT_SOLUTION	(0)
#define		SIMU_INDEX		(0)
#define		CPU_VERIFY		(1)
#define		REPEAT_COUNT	(1)
#define		KERNEL_DEBUG	(1)

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
/* solution 控制										                    */
/************************************************************************/
class ProblemCtrlBase;
class SolutionCtrlBase
{
public:
	SolutionCtrlBase(ProblemCtrlBase * problem)
	{
		repeatTime = REPEAT_COUNT;
		solutionScore.ElapsedTime = (std::numeric_limits<double>::max)();
		solutionScore.Performence = 0;

		cmdArgs = CmdArgs::GetCmdArgs();
		rtOcl = RuntimeOCL::GetInstance();
		stream = rtOcl->CreatCmdQueue(true);

		this->problem = problem;
		solutionParamSpace = new AutoTune::SearchSpace();
	}
	virtual ~SolutionCtrlBase() { delete stream; delete solutionParamSpace; }

	void RunSolution();
	virtual void GetBestKernel() { INFO("Best solution: " + solutionName); }

	std::string KernelName() { return kernelName; }
	std::string KernelFile() { return kernelFile; }
	dim3 GroupSize() { return group_sz; }
	dim3 GlobalSize() { return global_sz; }
	T_Score SolutionScore() { return solutionScore; }

protected:
	CmdArgs * cmdArgs;
	RuntimeOCL * rtOcl;
	CmdQueueOCL* stream;	// one command queue for all solution configs
	KernelOCL * kernel;
	cl_event profEvt;

	ProblemCtrlBase * problem;
	std::string solutionName;						// 配置名称
	AutoTune::SearchSpace *solutionParamSpace;		// 解决方案参数搜索空间

	std::string kernelName;
	std::string kernelFile;
	dim3 group_sz;
	dim3 global_sz;

	int repeatTime;
	T_Score solutionScore;				// 全部配置的平均性能

	virtual E_ReturnState generateSolutionParamSpace() = 0;
	virtual E_ReturnState getKernelParam() {}
	virtual E_ReturnState generateKernel() = 0;
	virtual E_ReturnState prepareKernelArgs() = 0;
	virtual E_ReturnState launchKernel();
	virtual void getBackResult() = 0;
	virtual void releaseDevMem() = 0;
};

/************************************************************************/
/* solver 控制															*/
/************************************************************************/
class SolverCtrlBase
{
public:
	SolverCtrlBase(ProblemCtrlBase * problem)
	{
		this->problem = problem;
		scoreList = new std::list<T_Score>();
		solutionList = new std::list<SolutionCtrlBase*>();
		bestScore.ElapsedTime = (std::numeric_limits<double>::max)();
	}
	virtual ~SolverCtrlBase()
	{
		delete scoreList;
		delete solutionList;
	}

	void RunSolver();
	T_Score BestScore() { return bestScore; }
	SolutionCtrlBase * BestSolution() { return bestSolution; }

protected:
	ProblemCtrlBase * problem;
	std::list<T_Score> * scoreList;
	std::list<SolutionCtrlBase*> * solutionList;
	T_Score bestScore;
	SolutionCtrlBase * bestSolution;

	virtual void generateSolver() = 0;
};

/************************************************************************/
/* problem 控制															*/
/************************************************************************/
class ProblemCtrlBase
{
public:
	ProblemCtrlBase()
	{
		cmdArgs = CmdArgs::GetCmdArgs();
		rtOcl = RuntimeOCL::GetInstance();

		problemParamSpace = new AutoTune::SearchSpace();
	}
	ProblemCtrlBase(std::string name)
	{
		problemName = name;

		cmdArgs = CmdArgs::GetCmdArgs();
		rtOcl = RuntimeOCL::GetInstance();

		problemParamSpace = new AutoTune::SearchSpace();
	}
	virtual ~ProblemCtrlBase() { delete problemParamSpace; }

	virtual void RunProblem();
	SolutionCtrlBase * BestSolution() { return solver->BestSolution(); }
	double Calculation() { return calculation; }
	double TheoryElapsedTime() { return theoryElapsedTime; }

	// TODO: dump/load input/output data

protected:
	CmdArgs * cmdArgs;
	RuntimeOCL * rtOcl;
	SolverCtrlBase * solver;

	std::string problemName;
	AutoTune::SearchSpace *problemParamSpace;		// 问题参数搜索空间

	double calculation;					// 当前正在处理的问题配置的计算量
	double theoryElapsedTime;			// 当前正在处理的问题配置的理论执行时间

	virtual E_ReturnState initHostParam() = 0;
	virtual void runHostCompute() = 0;
	virtual E_ReturnState verifyDevCompute() = 0;
	virtual void releaseHostParam() = 0;
	virtual void caculateTheoryPerformance() {}
};
}

