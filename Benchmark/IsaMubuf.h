#pragma once

#include "BasicClass.h"
#include "RuntimeControl.h"
#include "ProblemControl.h"

/************************************************************************/
/* 扩展参数                                                              */
/************************************************************************/
typedef struct ExtVMBufSolutionConfigTpye
{
}T_ExtVMBufSolutionConfig;

typedef struct ExtVMBufProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_out, *c_ref;
}T_ExtVMBufProblemConfig;

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
class VMBubSolution : public SolutionCtrlBase
{
private:
	T_KernelArgu d_a, d_b, d_c;

public:
	/************************************************************************/
	/* 申请显存                                                            */
	/************************************************************************/
	E_ReturnState InitDev()
	{
		T_ExtVMBufProblemConfig * extProb = (T_ExtVMBufProblemConfig *)ProblemConfig->extConfig;

		DevMalloc((void**)&(d_a.ptr), extProb->VectorSize * sizeof(float));
		DevMalloc((void**)&(d_b.ptr), extProb->VectorSize * sizeof(float));
		DevMalloc((void**)&(d_c.ptr), extProb->VectorSize * sizeof(float));

		SolutionConfig->KernelArgus = new std::list<T_KernelArgu>;
		d_a.size = sizeof(cl_mem);	d_a.isVal = false;	SolutionConfig->KernelArgus->push_back(d_a);
		d_b.size = sizeof(cl_mem);	d_b.isVal = false;	SolutionConfig->KernelArgus->push_back(d_b);
		d_c.size = sizeof(cl_mem);	d_c.isVal = false;	SolutionConfig->KernelArgus->push_back(d_c);

		Copy2Dev((cl_mem)(d_a.ptr), extProb->h_a, extProb->VectorSize * sizeof(float));
		Copy2Dev((cl_mem)(d_b.ptr), extProb->h_b, extProb->VectorSize * sizeof(float));

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 返回结果                                                            */
	/************************************************************************/
	E_ReturnState GetBackResult()
	{
		T_ExtVMBufProblemConfig * extProb = (T_ExtVMBufProblemConfig *)ProblemConfig->extConfig;
		Copy2Hst(extProb->h_out, (cl_mem)(d_c.ptr), extProb->VectorSize * sizeof(float));
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
		T_ExtVMBufSolutionConfig * extSol;

		// ======================================================================
		// solution config 1: ASM
		// ======================================================================
		{
			extSol = new T_ExtVMBufSolutionConfig();

			solutionConfig = new T_SolutionConfig("ASM");
			solutionConfig->extConfig = extSol;

			// ----------------------------------------------------------------------
			// 添加solution
			SolutionConfigList->push_back(solutionConfig);
		}
		// ======================================================================
		// solution config 6: AutoGen
		// ======================================================================
		{
			extSol = new T_ExtVMBufSolutionConfig();

			solutionConfig = new T_SolutionConfig("AutoGen");
			solutionConfig->extConfig = extSol;

			// ----------------------------------------------------------------------
			// 添加solution
			//SolutionConfigList->push_back(solutionConfig);
		}

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 根据solution参数生成source, complier和worksize                         */
	/************************************************************************/
	E_ReturnState GenerateSolution()
	{
		T_ExtVMBufProblemConfig * extProb = (T_ExtVMBufProblemConfig *)ProblemConfig->extConfig;
		T_ExtVMBufSolutionConfig * extSol = (T_ExtVMBufSolutionConfig *)SolutionConfig->extConfig;
		
		// ======================================================================
		// 生成代码
		// ======================================================================
		if (SolutionConfig->ConfigName == "ASM")
		{
			SolutionConfig->KernelName = "IsaVMBuf";
			SolutionConfig->KernelFile = "IsaVMBuf.s";
			SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}

		// ======================================================================
		// 生成worksize
		// ======================================================================
		SolutionConfig->l_wk0 = WAVE_SIZE;
		SolutionConfig->l_wk1 = 1;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = extProb->VectorSize;
		SolutionConfig->g_wk1 = 1;
		SolutionConfig->g_wk2 = 1;

		return E_ReturnState::SUCCESS;
	}
};

/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
class VMBufProblem : public ProblemCtrlBase
{
public:
	VMBufProblem(std::string name):ProblemCtrlBase(name)
	{
		Solution = new VMBubSolution();
	}

	/************************************************************************/
	/* 生成问题空间													        */
	/************************************************************************/
	E_ReturnState GenerateProblemConfigs()
	{
		T_ProblemConfig * probCfg;
		T_ExtVMBufProblemConfig * exCfg;

		probCfg = new T_ProblemConfig();

		exCfg = new T_ExtVMBufProblemConfig();
		exCfg->VectorSize = 1024;
		probCfg->extConfig = exCfg;

		ProblemConfigList->push_back(probCfg);
	}

	/************************************************************************/
	/* 参数初始化                                                            */
	/************************************************************************/
	E_ReturnState InitHost()
	{
		T_ExtVMBufProblemConfig * exCfg = (T_ExtVMBufProblemConfig *)ProblemConfig->extConfig;

		ProblemConfig->Calculation = exCfg->VectorSize; 
		ProblemConfig->TheoryElapsedTime = ProblemConfig->Calculation / RuntimeCtrlBase::DeviceInfo.Fp32Flops;
		printf("Calculation = %.3f G\n", ProblemConfig->Calculation * 1e-9);
		printf("TheoryElapsedTime = %.3f us \n", ProblemConfig->TheoryElapsedTime * 1e6);

		exCfg->h_a = (float*)HstMalloc(exCfg->VectorSize * sizeof(float));
		exCfg->h_b = (float*)HstMalloc(exCfg->VectorSize * sizeof(float));
		exCfg->h_out = (float*)HstMalloc(exCfg->VectorSize * sizeof(float));
		exCfg->c_ref = (float*)HstMalloc(exCfg->VectorSize * sizeof(float));
		
		for (int i = 0; i < exCfg->VectorSize; i++)
		{
			exCfg->h_a[i] = i;
			exCfg->h_b[i] = 2;
			exCfg->h_out[i] = 0;
		}

		return E_ReturnState::SUCCESS; 
	} 

	/************************************************************************/
	/* HOST端                                                               */
	/************************************************************************/
	E_ReturnState Host()
	{
		T_ExtVMBufProblemConfig * exCfg = (T_ExtVMBufProblemConfig *)ProblemConfig->extConfig;

		for (int i = 0; i < exCfg->VectorSize; i++)
		{
			exCfg->c_ref[i] = exCfg->h_a[i];
		}
		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 校验                                                                 */
	/************************************************************************/
	E_ReturnState Verify()
	{
		T_ExtVMBufProblemConfig * exCfg = (T_ExtVMBufProblemConfig *)ProblemConfig->extConfig;
		
		float diff = 0;
		for (int i = 0; i < exCfg->VectorSize; i++)
		{
			diff += (exCfg->c_ref[i] - exCfg->h_out[i]) * (exCfg->c_ref[i] - exCfg->h_out[i]);
		}
		diff /= exCfg->VectorSize;
		
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
		T_ExtVMBufProblemConfig * exCfg = (T_ExtVMBufProblemConfig *)ProblemConfig->extConfig;

		HstFree(exCfg->h_a);
		HstFree(exCfg->h_b);
		HstFree(exCfg->h_out);
		HstFree(exCfg->c_ref);
	}	
};
 