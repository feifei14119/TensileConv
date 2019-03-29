#pragma once

#include <limits>
#include <stdarg.h>
#include <vector>

#include "ff_ocl_runtime.h"

namespace feifei
{
#define		REPEAT_COUNT	(1)

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

			rtOcl = RuntimeOCL::GetInstance();
			stream = rtOcl->CreatCmdQueue(true);

			this->problem = problem;
		}
		virtual ~SolutionCtrlBase() { delete stream; }

		void RunSolution();

		std::string KernelName() { return kernelName; }
		std::string KernelFile() { return kernelFile; }
		dim3 GroupSize() { return group_sz; }
		dim3 GlobalSize() { return global_sz; }
		T_Score Score() { return score; }

	protected:
		CmdArgs * cmdArgs;
		RuntimeOCL * rtOcl;
		CmdQueueOCL* stream;
		KernelOCL * kernel;
		cl_event profEvt;

		ProblemCtrlBase * problem;
		std::string solutionName;						// 配置名称

		std::string kernelName;
		std::string kernelFile;
		dim3 group_sz;
		dim3 global_sz;

		int repeatTime;
		T_Score score;

		virtual E_ReturnState generateKernel() { LOG("Generate program and build kernel."); return E_ReturnState::SUCCESS; }
		virtual E_ReturnState prepareKernelArgs() { LOG("Prepare kernel args."); return E_ReturnState::SUCCESS; };
		virtual E_ReturnState launchKernel();
		virtual void recordScore(double elapsedTime);
		virtual void getBackResult() { LOG("Copy result back to cpu."); };
		virtual void releaseDevMem() { LOG("Release resource."); };
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
			scores = new std::vector<T_Score>();
			solutions = new std::vector<SolutionCtrlBase*>();
			bestScore.ElapsedTime = (std::numeric_limits<double>::max)();
		}
		virtual ~SolverCtrlBase()
		{
			delete scores;
			delete solutions;
		}

		void RunSolver();
		T_Score BestScore() { return bestScore; }
		SolutionCtrlBase * BestSolution() { return bestSolution; }

	protected:
		ProblemCtrlBase * problem;

		T_Score bestScore;
		std::vector<T_Score> * scores;
		SolutionCtrlBase * bestSolution;
		std::vector<SolutionCtrlBase*> * solutions;

		virtual void generateSolver() { LOG("Generate Solver."); }
	};

	/************************************************************************/
	/* problem 控制															*/
	/************************************************************************/
	class ProblemCtrlBase
	{
	public:
		ProblemCtrlBase(std::string name = "")
		{
			problemName = name;
			rtOcl = RuntimeOCL::GetInstance();
		}
		~ProblemCtrlBase() { }

		virtual void RunProblem();

		double Calculation() { return calculation; }
		double TheoryElapsedTime() { return theoryElapsedTime; }
		SolutionCtrlBase * BestSolution() { return solver->BestSolution(); }

	protected:
		RuntimeOCL * rtOcl;
		SolverCtrlBase * solver;

		std::string problemName;
		double calculation;					// 当前正在处理的问题配置的计算量
		double theoryElapsedTime;			// 当前正在处理的问题配置的理论执行时间

		virtual void initHostParam() { LOG("Initialize Host."); }
		virtual void runHostCompute() { LOG("Run Host Calculate."); }
		virtual void verifyDevCompute() { LOG("Verify Device Calculation."); }
		virtual void releaseHostParam() { LOG("Release Host."); };
		virtual void cacuTheoryPerf() { LOG("Caculate Theory Performance."); }
	};
}

