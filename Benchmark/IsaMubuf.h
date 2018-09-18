#pragma once

#include "IsaDemoConfig.h"
#include "IsaMubufKernelWriter.h"

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
		T_ExtMubufProblemConfig * extProb = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;

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
		T_ExtMubufProblemConfig * extProb = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;
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
		T_ExtMubufSolutionConfig * extSol;

		// ======================================================================
		// solution config 1: ASM
		// ======================================================================
		{
			extSol = new T_ExtMubufSolutionConfig();

			solutionConfig = new T_SolutionConfig("ASM");
			solutionConfig->extConfig = extSol;

			//SolutionConfigList->push_back(solutionConfig);
		}
		// ======================================================================
		// solution config 2: AutoGen
		// ======================================================================
		{
			extSol = new T_ExtMubufSolutionConfig();

			solutionConfig = new T_SolutionConfig("AutoGen");
			solutionConfig->extConfig = extSol;

			SolutionConfigList->push_back(solutionConfig);
		}

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 根据solution参数生成source, complier和worksize                         */
	/************************************************************************/
	E_ReturnState GenerateSolution()
	{
		T_ExtMubufProblemConfig * extProb = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;
		T_ExtMubufSolutionConfig * extSol = (T_ExtMubufSolutionConfig *)SolutionConfig->extConfig;

		// ======================================================================
		// 生成worksize
		// ======================================================================
		SolutionConfig->l_wk0 = WAVE_SIZE;
		SolutionConfig->l_wk1 = 1;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = extProb->VectorSize;
		SolutionConfig->g_wk1 = 1;
		SolutionConfig->g_wk2 = 1;
		
		// ======================================================================
		// 生成代码
		// ======================================================================
		if (SolutionConfig->ConfigName == "ASM")
		{
			SolutionConfig->KernelName = "IsaMubuf";
			SolutionConfig->KernelFile = "IsaMubuf.s";
			SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}
		else if (SolutionConfig->ConfigName == "AutoGen")
		{
			SolutionConfig->KernelName = "IsaMubuf";
			SolutionConfig->KernelFile = "IsaMubufAutoGen.s";
			SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;

			KernelWriterBase * kw = new KernelWriterIsaMubuf(ProblemConfig, SolutionConfig);
			kw->GenKernelString();
			kw->SaveKernelStr2File();
		}

		return E_ReturnState::SUCCESS;
	}
};

/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
class MubufProblem : public ProblemCtrlBase
{
public:
	MubufProblem(std::string name):ProblemCtrlBase(name)
	{
		Solution = new VMBubSolution();
	}

	/************************************************************************/
	/* 生成问题空间													        */
	/************************************************************************/
	E_ReturnState GenerateProblemConfigs()
	{
		T_ProblemConfig * probCfg;
		T_ExtMubufProblemConfig * extProb;

		probCfg = new T_ProblemConfig();

		extProb = new T_ExtMubufProblemConfig();
		extProb->VectorSize = 1024;
		probCfg->extConfig = extProb;

		ProblemConfigList->push_back(probCfg);
	}

	/************************************************************************/
	/* 参数初始化                                                            */
	/************************************************************************/
	E_ReturnState InitHost()
	{
		T_ExtMubufProblemConfig * extProb = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;

		extProb->h_a = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
		extProb->h_b = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
		extProb->h_out = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
		extProb->c_ref = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
		ProblemConfig->Calculation = extProb->VectorSize;
		
		for (int i = 0; i < extProb->VectorSize; i++)
		{
			extProb->h_a[i] = i;
			extProb->h_b[i] = 2;
			extProb->h_out[i] = 0;
		}

		return E_ReturnState::SUCCESS; 
	} 

	/************************************************************************/
	/* HOST端                                                               */
	/************************************************************************/
	E_ReturnState Host()
	{
		T_ExtMubufProblemConfig * extProb = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;

		for (int i = 0; i < extProb->VectorSize; i++)
		{
			extProb->c_ref[i] = extProb->h_a[i] + extProb->h_b[i];
		}
		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 校验                                                                 */
	/************************************************************************/
	E_ReturnState Verify()
	{
		T_ExtMubufProblemConfig * extProb = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;
		
		float diff = 0;
		for (int i = 0; i < extProb->VectorSize; i++)
		{
			diff += (extProb->c_ref[i] - extProb->h_out[i]) * (extProb->c_ref[i] - extProb->h_out[i]);
		}
		diff /= extProb->VectorSize;
		
		printf("mean err = %.1f.\n", diff);
		if (!(diff >= 0 && diff < MIN_FP32_ERR))
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
		T_ExtMubufProblemConfig * extProb = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;

		HstFree(extProb->h_a);
		HstFree(extProb->h_b);
		HstFree(extProb->h_out);
		HstFree(extProb->c_ref);
	}	
};
 