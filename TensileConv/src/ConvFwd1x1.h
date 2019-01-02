#pragma once 

#include "ProblemControl.h"
#include "ConvFwd1x1Config.h"
//#include "ConvFwd1x1KernelWriter.h"

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
class ConvFwd1x1Solution : public SolutionCtrlBase
{
private:
	T_KernelArgu d_in, d_wei, d_bias, d_out, d_negSlop, d_sig;
	cl_mem d_a, d_b, d_c;
	float negSlop;

	// -------------------------------------------------------------------
	size_t align;
	int loop;			// 循环次数

	// -------------------------------------------------------------------
	// prefetch mult-kernel
	int *h_signal = nullptr;
	int sig_num_per_cu,size_sig;
	T_KernelArgu d_signal;
	std::list<T_KernelArgu> * preArgus;

public:
	ConvFwd1x1Solution() :SolutionCtrlBase() {}

protected:
	E_ReturnState generateSolutionConfigs();
	E_ReturnState generateKernel();
	E_ReturnState generateKernelParam();
	E_ReturnState getBackResult();
	void releaseKernelParam();
	void reportProblemPerformence();
	
	int N_LCL_IN_MAPS;
	int N_IN_GROUPS;
	int N_LCL_IN_MAPS_ONCE;
	int	N_OUT_GROUPS;
	int CLOOP0;
	int CLOOP2;

	// 测试下标计算
	void simulateIndex();	
};

/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
class ConvFwd1x1Problem : public ProblemCtrlBase
{
public:
	ConvFwd1x1Problem() :ProblemCtrlBase() { solution = new ConvFwd1x1Solution(); }
	ConvFwd1x1Problem(std::string name) :ProblemCtrlBase(name) { solution = new ConvFwd1x1Solution(); }

	// 运行问题
	E_ReturnState TurnProblem();
	E_ReturnState TurnProblem(int WH, int C, int K, int N, int UV, bool isBias, bool isRelu);
	
protected:
	E_ReturnState initHostParam();
	E_ReturnState runHostCompute();
	E_ReturnState verifyDevCompute();
	void releaseHostParam(); 
	void caculateTheoryPerformance();
};


