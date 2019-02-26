#pragma once

#include <limits>
#include <stdarg.h>
#include <vector>
#include <list>

#include "../common/ff_utils.h"
#include "AutoTuning.h"

#include "unistd.h"

namespace TensileConv {

#define		MULT_SOLUTION	(1)
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

		cmdArgs = CmdArgs::GetCmdArgs();
		rtOcl = RuntimeOCL::GetInstance();
		stream = rtOcl->CreatCmdQueue(true);

		this->problem = problem;
		solutionParamSpace = new AutoTune::SearchSpace();
		geneticSearch = new AutoTune::GeneticSearch();
		repeatTime = *(int*)cmdArgs->GetOneArg(E_ArgId::CMD_ARG_LOOP);
		searchMethord = *(AutoTune::E_SearchMethord*)cmdArgs->GetOneArg(E_ArgId::CMD_ARG_SEARCH);
	}
	virtual ~SolutionCtrlBase() { delete stream; delete solutionParamSpace; delete geneticSearch; }

	void RunSolution();
	virtual void GetBestKernel() { INFO("Best solution: " + solutionName); }
	void SetSearchMethord(AutoTune::E_SearchMethord methord) { searchMethord = methord; }

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
	std::string solutionName;						// 配置名称
	AutoTune::E_SearchMethord searchMethord;
	AutoTune::SearchSpace *solutionParamSpace;		// 解决方案参数搜索空间
	AutoTune::GeneticSearch * geneticSearch;
	int SearchedKernelCnt, SearchKernelNum;
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
		switch (searchMethord)
		{
		case AutoTune::E_SearchMethord::SEARCH_BRUTE:
			INFO("Searching %.1f%%: %d / %d kernels.",
				100.0 * SearchedKernelCnt / SearchKernelNum, SearchedKernelCnt++, SearchKernelNum);
			break;
		case AutoTune::E_SearchMethord::SEARCH_GENETIC:
			INFO("Searching %.1f%%: %d / %d kernels.",
				100.0 * SearchedKernelCnt / SearchKernelNum, SearchedKernelCnt, SearchKernelNum);
			break;
		}
	}
	virtual E_ReturnState generateKernel() { INFO("Generate program and build kernel."); }
	virtual E_ReturnState prepareKernelArgs() { INFO("Prepare kernel args."); };
	virtual E_ReturnState launchKernel();
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
	LogFile * logFile;
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

