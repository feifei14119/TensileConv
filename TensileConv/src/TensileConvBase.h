#pragma once

#include <limits>
#include <stdarg.h>
#include <vector>
#include <list>

#include "../common/ff_utils.h"
#include "AutoTuning.h"

#include "unistd.h"

namespace TensileConv {

#define		CPU_VERIFY		(1)

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
	SolutionCtrlBase(ProblemCtrlBase * problem, std::string name = "", LogFile * file = nullptr)
	{
		solutionName = name;
		logFile = file;
		solutionScore.ElapsedTime = (std::numeric_limits<double>::max)();
		solutionScore.Performence = 0;

		rtOcl = RuntimeOCL::GetInstance();
		stream = rtOcl->CreatCmdQueue(true);

		repeatTime = 10;
		this->problem = problem;
	}
	virtual ~SolutionCtrlBase() 
	{ 
		delete stream; 
		delete searchSpace;
	}

	void RunSolution();
	virtual void GetBestKernel() { INFO("Best solution: " + solutionName); }

	std::string KernelName() { return kernelName; }
	std::string KernelFile() { return kernelFile; }
	dim3 GroupSize() { return group_sz; }
	dim3 GlobalSize() { return global_sz; }
	T_Score SolutionScore() { return solutionScore; }
	double SearchElapsedSec() { return searchElapsedSec; }

protected:
	CmdArgs * cmdArgs; 
	RuntimeOCL * rtOcl;
	LogFile * logFile;
	CmdQueueOCL* stream;	// one command queue for all solution configs
	KernelOCL * kernel;
	cl_event profEvt;

	ProblemCtrlBase * problem;
	std::string solutionName;					// 配置名称
	bool enableSearch;
	AutoTune::E_SearchMethord searchMethod;		// 搜索方法选择
	AutoTune::SearchSpaceBase * searchSpace;	// 参数搜索空间
	double searchElapsedSec;

	std::string kernelName;
	std::string kernelFile;
	dim3 group_sz;
	dim3 global_sz;

	int repeatTime;
	T_Score solutionScore;				// 全部配置的平均性能

	virtual E_ReturnState generateSolutionParamSpace() { INFO("Generate solution parameters space."); }
	virtual E_ReturnState getKernelParam() 
	{ 
		switch (searchMethod)
		{
		case AutoTune::E_SearchMethord::SEARCH_BRUTE:
			INFO("Searching %.1f%%: %d / %d kernels.",
				100.0 * searchSpace->SearchedCombNum() / searchSpace->ParamCombNum(),
				searchSpace->SearchedCombNum(), searchSpace->ParamCombNum());
			break;
		case AutoTune::E_SearchMethord::SEARCH_GENETIC:
			INFO("Searching %.1f%%: %d / %d kernels.",
				100.0 * searchSpace->CheckedCombNum() / searchSpace->ParamCombNum(),
				searchSpace->CheckedCombNum(), searchSpace->ParamCombNum());
			break;
		}
	}
	virtual E_ReturnState generateKernel() { INFO("Generate program and build kernel."); }
	virtual E_ReturnState prepareKernelArgs() { INFO("Prepare kernel args."); };
	virtual E_ReturnState launchKernel();
	virtual void recordScore(double elapsedTime);
	virtual void getBackResult() { INFO("Copy result back to cpu."); };
	virtual void releaseDevMem() { INFO("Release resource."); };
};

/************************************************************************/
/* solver 控制															*/
/************************************************************************/
class SolverCtrlBase
{
public:
	SolverCtrlBase(ProblemCtrlBase * problem, LogFile * file = nullptr)
	{
		logFile = file;
		this->problem = problem;
		scoreList = new std::list<T_Score>();
		solutionList = new std::vector<SolutionCtrlBase*>();
		bestScore.ElapsedTime = (std::numeric_limits<double>::max)();
	}
	virtual ~SolverCtrlBase()
	{
		delete scoreList;
		delete solutionList;
	}

	bool EnSearch;
	void RunSolver();
	T_Score BestScore() { return bestScore; }
	SolutionCtrlBase * BestSolution() { return bestSolution; }

protected:
	LogFile * logFile;
	ProblemCtrlBase * problem;
	std::list<T_Score> * scoreList;
	std::vector<SolutionCtrlBase*> * solutionList;
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
	ProblemCtrlBase(std::string name = "", LogFile * file = nullptr)
	{
		problemName = name;
		logFile = file;

		rtOcl = RuntimeOCL::GetInstance();

		searchSpace = new AutoTune::BruteSearch();
	}
	virtual ~ProblemCtrlBase() 
	{ 
		delete searchSpace; 
	}

	bool EnSearch;
	AutoTune::E_SearchMethord SearchMethod;
	virtual void RunProblem();
	SolutionCtrlBase * BestSolution() { return solver->BestSolution(); }
	double Calculation() { return calculation; }
	double TheoryElapsedTime() { return theoryElapsedTime; }

	// TODO: dump/load input/output data

protected:
	CmdArgs * cmdArgs;
	RuntimeOCL * rtOcl;
	LogFile * logFile;
	SolverCtrlBase * solver;

	std::string problemName;
	AutoTune::BruteSearch * searchSpace;	// 问题参数搜索空间

	double calculation;					// 当前正在处理的问题配置的计算量
	double theoryElapsedTime;			// 当前正在处理的问题配置的理论执行时间
	
	virtual void initHostParam() { INFO("initialize host."); }
	virtual void runHostCompute() { INFO("run host calculate."); }
	virtual void verifyDevCompute() { INFO("verify device calculation."); }
	virtual void releaseHostParam() { INFO("release host."); };
	virtual void caculateTheoryPerformance() {}
};
}

