#pragma once

#include "BasicClass.h"
#include "RuntimeControl.h"
#include "ProblemControl.h"

/************************************************************************/
/* 扩展参数                                                              */
/************************************************************************/
typedef struct ExtGlobalSolutionConfigTpye
{
#define FIX_WORKGROUP_SIZE (64)
}T_ExtGlobalSolutionConfig;

typedef struct ExtGlobalProblemConfigType
{
#define VECTOR_SIZE (512)
	size_t vectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtGlobalProblemConfig;

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
class GlobalSolution : public SolutionCtrlBase
{
private:
	T_KernelArgu d_a, d_b, d_c;
public:
	/************************************************************************/
	/* 申请显存                                                            */
	/************************************************************************/
	E_ReturnState InitDev()
	{
		T_ExtGlobalProblemConfig * exCfg = (T_ExtGlobalProblemConfig *)ProblemConfig->extConfig;

		DevMalloc((void**)&(d_a.ptr), exCfg->vectorSize * sizeof(float));
		DevMalloc((void**)&(d_b.ptr), exCfg->vectorSize * sizeof(float));
		DevMalloc((void**)&(d_c.ptr), exCfg->vectorSize * sizeof(float));

		SolutionConfig->KernelArgus = new std::list<T_KernelArgu>;
		d_a.size = sizeof(cl_mem);	d_a.isVal = false;	SolutionConfig->KernelArgus->push_back(d_a);
		d_b.size = sizeof(cl_mem);	d_b.isVal = false;	SolutionConfig->KernelArgus->push_back(d_b);
		d_c.size = sizeof(cl_mem);	d_c.isVal = false;	SolutionConfig->KernelArgus->push_back(d_c);

		Copy2Dev((cl_mem)(d_a.ptr), exCfg->h_a, exCfg->vectorSize * sizeof(float));
		Copy2Dev((cl_mem)(d_b.ptr), exCfg->h_b, exCfg->vectorSize * sizeof(float));

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 返回结果                                                            */
	/************************************************************************/
	E_ReturnState GetBackResult()
	{
		T_ExtGlobalProblemConfig * exCfg = (T_ExtGlobalProblemConfig *)ProblemConfig->extConfig;
		Copy2Hst(exCfg->h_c, (cl_mem)(d_c.ptr), exCfg->vectorSize * sizeof(float));
	}

	/************************************************************************/
	/* 释放显存	                                                           */
	/************************************************************************/
	void ReleaseDev()
	{
		DevFree((cl_mem)(d_a.ptr));
		DevFree((cl_mem)(d_b.ptr));
		DevFree((cl_mem)(d_c.ptr));
	}

	/************************************************************************/
	/* 根据problem参数成solution参数空间                                      */
	/************************************************************************/
	E_ReturnState GenerateSolutionConfigs()
	{
		T_SolutionConfig * solutionConfig;
		T_ExtGlobalSolutionConfig * extSolutionConfig;
		
		// ======================================================================
		// solution config: ASM
		// ======================================================================
		extSolutionConfig = new T_ExtGlobalSolutionConfig();

		solutionConfig = new T_SolutionConfig();
		solutionConfig->ConfigName = "AsmSolution";
		solutionConfig->extConfig = extSolutionConfig;

		SolutionConfigList->push_back(solutionConfig);

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 根据solution参数生成source, complier和worksize                         */
	/************************************************************************/
	E_ReturnState GenerateSolution()
	{
		T_ExtGlobalProblemConfig * extProblem = (T_ExtGlobalProblemConfig *)ProblemConfig->extConfig;
		T_ExtGlobalSolutionConfig * extSolution = (T_ExtGlobalSolutionConfig *)SolutionConfig->extConfig;
		
		// ======================================================================
		// 生成代码
		// ======================================================================
		if (SolutionConfig->ConfigName == "AsmSolution")
		{
			SolutionConfig->KernelName = "IsaGlobal";
			SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}

		// ======================================================================
		// 生成worksize
		// ======================================================================
		SolutionConfig->l_wk0 = FIX_WORKGROUP_SIZE;
		SolutionConfig->l_wk1 = 1;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = extProblem->vectorSize;
		SolutionConfig->g_wk1 = 1;
		SolutionConfig->g_wk2 = 1;

		return E_ReturnState::SUCCESS;
	}
};

/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
class GlobalProblem : public ProblemCtrlBase
{
public:
	GlobalProblem()
	{
		ProblemName = "Global Instruction";
		Solution = new GlobalSolution();
		ProblemConfigList = new std::list<T_ProblemConfig*>;
	}

public:
	/************************************************************************/
	/* 生成问题空间													        */
	/************************************************************************/
	E_ReturnState GenerateProblemConfigs()
	{
		T_ProblemConfig * problemConfig;
		T_ExtGlobalProblemConfig * extProblemConfig;

		// ----------------------------------------------------------------------
		// problem config 1
		extProblemConfig = new T_ExtGlobalProblemConfig();
		extProblemConfig->vectorSize = VECTOR_SIZE;

		problemConfig = new T_ProblemConfig();
		problemConfig->ConfigName = "512";
		problemConfig->extConfig = extProblemConfig;

		ProblemConfigList->push_back(problemConfig);
	}

	/************************************************************************/
	/* 参数初始化                                                            */
	/************************************************************************/
	E_ReturnState InitHost()
	{
		std::cout << "Global instruction init" << ProblemConfig->ConfigName << std::endl;
		T_ExtGlobalProblemConfig * exCfg = (T_ExtGlobalProblemConfig *)ProblemConfig->extConfig;

		ProblemConfig->Calculation = exCfg->vectorSize; 
		ProblemConfig->TheoryElapsedTime = ProblemConfig->Calculation / RuntimeCtrlBase::DeviceInfo.Fp32Flops;
		printf("Calculation = %.3f G\n", ProblemConfig->Calculation * 1e-9);
		printf("TheoryElapsedTime = %.3f us \n", ProblemConfig->TheoryElapsedTime * 1e6);

		exCfg->h_a = (float*)HstMalloc(exCfg->vectorSize * sizeof(float));
		exCfg->h_b = (float*)HstMalloc(exCfg->vectorSize * sizeof(float));
		exCfg->h_c = (float*)HstMalloc(exCfg->vectorSize * sizeof(float));
		exCfg->c_ref = (float*)HstMalloc(exCfg->vectorSize * sizeof(float));
		
		for (int i = 0; i < exCfg->vectorSize; i++)
		{
			exCfg->h_a[i] = i;
			exCfg->h_b[i] = 2;
			exCfg->h_c[i] = 0;
		}

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* HOST端                                                               */
	/************************************************************************/
	E_ReturnState Host()
	{
		printf("Global instruction host.\n");
		T_ExtGlobalProblemConfig * exCfg = (T_ExtGlobalProblemConfig *)ProblemConfig->extConfig;

		for (int i = 0; i < exCfg->vectorSize; i++)
		{
			exCfg->c_ref[i] = exCfg->h_a[i] + exCfg->h_b[i];
		}
		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 校验                                                                 */
	/************************************************************************/
	E_ReturnState Verify()
	{
		printf("Global instruction verify.\n");
		T_ExtGlobalProblemConfig * exCfg = (T_ExtGlobalProblemConfig *)ProblemConfig->extConfig;
		
		float diff = 0;
		for (int i = 0; i < exCfg->vectorSize; i++)
		{
			diff += (exCfg->c_ref[i] - exCfg->h_c[i]) * (exCfg->c_ref[i] - exCfg->h_c[i]);
		}
		diff /= exCfg->vectorSize;

		printf("mean err = %.1f.\n", diff);
		if (diff > MIN_FP32_ERR)
		{
			printf("err = %.2f\n", diff);
			INFO("verify failed!");
			return E_ReturnState::FAIL;
		}
		INFO("verify success.");
		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 释放                                                                  */
	/************************************************************************/
	void ReleaseHost()
	{
		printf("Global instruction destroy.\n");
		T_ExtGlobalProblemConfig * exCfg = (T_ExtGlobalProblemConfig *)ProblemConfig->extConfig;

		HstFree(exCfg->h_a);
		HstFree(exCfg->h_b);
		HstFree(exCfg->h_c);
		HstFree(exCfg->c_ref);
	}	
};
