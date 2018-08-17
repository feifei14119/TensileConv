#pragma once

#include "BasicClass.h"
#include "RuntimeControl.h"
#include "ProblemControl.h"

/************************************************************************/
/* 扩展参数                                                              */
/************************************************************************/
typedef struct ExtProducerConsumerSolutionConfigTpye
{
#define FIX_WORKGROUP_SIZE (1)
	std::list<T_KernelArgu> * preArgus;
}T_ExtProducerConsumerSolutionConfig;

typedef struct ExtProducerConsumerProblemConfigType
{
	size_t vectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
	int * h_sig;
}T_ExtProducerConsumerProblemConfig;

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
class ProducerConsumerSolution : public SolutionCtrlBase
{
private:
	T_KernelArgu d_a, d_b, d_c, d_sig;
	RuntimeCtrl * preKernel;
	int cu_num = 64;

public:
	/************************************************************************/
	/* 申请显存                                                            */
	/************************************************************************/
	E_ReturnState InitDev()
	{
		T_ExtProducerConsumerProblemConfig * exCfg = (T_ExtProducerConsumerProblemConfig *)problemCfg->extConfig;
		T_ExtProducerConsumerSolutionConfig * extSol = (T_ExtProducerConsumerSolutionConfig *)solutionCfg->extConfig;

		preKernel = new RuntimeCtrlOcl();

		DevMalloc((void**)&(d_a.ptr), exCfg->vectorSize * sizeof(float));
		DevMalloc((void**)&(d_b.ptr), exCfg->vectorSize * sizeof(float));
		DevMalloc((void**)&(d_c.ptr), exCfg->vectorSize * sizeof(float));
		DevMalloc((void**)&(d_sig.ptr), cu_num * sizeof(uint));

		d_a.size = sizeof(cl_mem);	d_a.isVal = false;
		d_b.size = sizeof(cl_mem);	d_b.isVal = false;
		d_c.size = sizeof(cl_mem);	d_c.isVal = false;
		d_sig.size = sizeof(cl_mem); d_sig.isVal = false;

		solutionCfg->KernelArgus = new std::list<T_KernelArgu>;
		solutionCfg->KernelArgus->push_back(d_a);
		solutionCfg->KernelArgus->push_back(d_b);
		solutionCfg->KernelArgus->push_back(d_c);
		solutionCfg->KernelArgus->push_back(d_sig);

		extSol->preArgus = new std::list<T_KernelArgu>;
		extSol->preArgus->push_back(d_a);
		extSol->preArgus->push_back(d_b);
		extSol->preArgus->push_back(d_c);
		extSol->preArgus->push_back(d_sig);

		Copy2Dev((cl_mem)(d_a.ptr), exCfg->h_a, exCfg->vectorSize * sizeof(float));
		Copy2Dev((cl_mem)(d_b.ptr), exCfg->h_b, exCfg->vectorSize * sizeof(float));

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 返回结果                                                            */
	/************************************************************************/
	E_ReturnState GetBackResult()
	{
		T_ExtProducerConsumerProblemConfig * exCfg = (T_ExtProducerConsumerProblemConfig *)problemCfg->extConfig;
		Copy2Hst(exCfg->h_c, (cl_mem)(d_c.ptr), exCfg->vectorSize * sizeof(float));
		Copy2Hst(exCfg->h_sig, (cl_mem)(d_sig.ptr), cu_num * sizeof(int));
	}

	/************************************************************************/
	/* 释放显存	                                                           */
	/************************************************************************/
	void ReleaseDev()
	{
		DevFree((cl_mem)(d_a.ptr));
		DevFree((cl_mem)(d_b.ptr));
		DevFree((cl_mem)(d_c.ptr));
		DevFree((cl_mem)(d_sig.ptr));
		delete preKernel;
	}
	
	/************************************************************************/
	/* 根据problem参数成solution参数空间                                      */
	/************************************************************************/
	E_ReturnState GenerateSolutionConfigs()
	{
		T_SolutionConfig * solutionConfig;
		T_ExtProducerConsumerSolutionConfig * extSolutionConfig;

		// ======================================================================
		// solution config 1: ASM
		// ======================================================================
		extSolutionConfig = new T_ExtProducerConsumerSolutionConfig();

		solutionConfig = new T_SolutionConfig();
		solutionConfig->ConfigName = "AsmSolution";
		solutionConfig->RepeatTime = 1;
		solutionConfig->extConfig = extSolutionConfig;

		// ----------------------------------------------------------------------
		// 添加solution
		SolutionConfigList->push_back(solutionConfig);

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 根据solution参数生成source, complier和worksize                         */
	/************************************************************************/
	E_ReturnState GenerateSolution()
	{
		T_ExtProducerConsumerProblemConfig * extProblem = (T_ExtProducerConsumerProblemConfig *)problemCfg->extConfig;
		T_ExtProducerConsumerSolutionConfig * extSolution = (T_ExtProducerConsumerSolutionConfig *)solutionCfg->extConfig;
		
		// ======================================================================
		// 生成代码
		// ======================================================================
		if (solutionCfg->ConfigName == "AsmSolution")
		{
			solutionCfg->KernelName = "Consumer";
			solutionCfg->KernelFile = "PS_Consumer.s";
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}

		// ======================================================================
		// 生成worksize
		// ======================================================================
		solutionCfg->l_wk0 = 128;
		solutionCfg->l_wk1 = 1;
		solutionCfg->l_wk2 = 1;
		solutionCfg->g_wk0 = extProblem->vectorSize;
		solutionCfg->g_wk1 = 1;
		solutionCfg->g_wk2 = 1;

		return E_ReturnState::SUCCESS;
	}
	
	/************************************************************************/
	/* 分别配置两个kernel								                        */
	/************************************************************************/
	E_ReturnState SetupSolution0()
	{
		printf("setup solution.\n");

		setupProducer();
		setupConsumer();

		// warm up
		LaunchSolution();

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState setupProducer()
	{
		printf("setup producer solution.\n");

		preKernel->KernelName = "Producer";
		preKernel->KernelFile = "PS_Producer.s";
		preKernel->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;

		dim3 l_wk = dim3(1, 1, 1);
		dim3 g_wk = dim3(64, 1, 1);
		dim3 b_wk = dim3(64, 1, 1);
		preKernel->SetBlockSize(l_wk);
		preKernel->SetGridSize(b_wk);

		// build source file
		preKernel->GetFilesName(preKernel->KernelFile);
		preKernel->KernelString = solutionCfg->KernelString;
		preKernel->CreatSolution();

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState setupConsumer()
	{
		printf("setup consumer solution.\n");

		runtime->KernelName = solutionCfg->KernelName;
		runtime->KernelSrcType = solutionCfg->KernelSrcType;
		runtime->extCompilerOpt = solutionCfg->extCompilerOpt;
		solutionCfg->b_wk0 = solutionCfg->g_wk0 / solutionCfg->l_wk0;
		solutionCfg->b_wk1 = solutionCfg->g_wk1 / solutionCfg->l_wk1;
		solutionCfg->b_wk2 = solutionCfg->g_wk2 / solutionCfg->l_wk2;
		runtime->SetBlockSize(dim3(solutionCfg->l_wk0, solutionCfg->l_wk1, solutionCfg->l_wk2));
		runtime->SetGridSize(dim3(solutionCfg->b_wk0, solutionCfg->b_wk1, solutionCfg->b_wk2));

		printf("l_wk=(%d, %d, %d)\n", solutionCfg->l_wk0, solutionCfg->l_wk1, solutionCfg->l_wk2);
		printf("b_wk=(%d, %d, %d)\n", solutionCfg->b_wk0, solutionCfg->b_wk1, solutionCfg->b_wk2);
		printf("g_wk=(%d, %d, %d)\n", solutionCfg->g_wk0, solutionCfg->g_wk1, solutionCfg->g_wk2);
		std::cout << "compile options:\n" << solutionCfg->extCompilerOpt << std::endl;

		// build source file
		runtime->GetFilesName(solutionCfg->KernelFile);
		runtime->KernelString = solutionCfg->KernelString;
		runtime->CreatSolution();

		solutionCfg->ElapsedTimes.clear();
	}

	/************************************************************************/
	/* 分别加载两个kernel								                        */
	/************************************************************************/
	E_ReturnState LaunchSolution0()
	{
		printf("set argue.\n");
		UnixTimer tmr;

		solutionCfg->RepeatTime = solutionCfg->RepeatTime == 0 ? 1 : solutionCfg->RepeatTime;
		for (int i = 0; i < solutionCfg->RepeatTime; i++)
		{
			setArgusProducer();
			setArgusConsumer();


			printf("launch solution.\n");
			preKernel->LanchKernel();
			runtime->LanchKernel();
		}
		return E_ReturnState::SUCCESS;
	}

	E_ReturnState setArgusProducer()
	{
		printf("set producer argue.\n");
		std::list<T_KernelArgu>::iterator args;
		T_ExtProducerConsumerSolutionConfig * extSolu = (T_ExtProducerConsumerSolutionConfig *)solutionCfg->extConfig;

		int i = 0;
		for (args = extSolu->preArgus->begin(); args != extSolu->preArgus->end(); args++)
		{
			DevCheckFunc(clSetKernelArg(preKernel->kernel, i, (*args).size, &(*args).ptr));
			i++;
		}

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState setArgusConsumer()
	{
		printf("set consumer argue.\n");
		std::list<T_KernelArgu>::iterator args;

		int i = 0;
		for (args = solutionCfg->KernelArgus->begin(); args != solutionCfg->KernelArgus->end(); args++)
		{
			DevCheckFunc(clSetKernelArg(runtime->kernel, i, (*args).size, &(*args).ptr));
			i++;
		}

		return E_ReturnState::SUCCESS;
	}
};
 
/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
class ProducerConsumerProblem : public ProblemCtrlBase
{
	int cu_num = 64;
public:
	ProducerConsumerProblem()
	{
		ProblemName = "Producer Consumer";
		Solution = new ProducerConsumerSolution();
		ProblemConfigList = new std::list<T_ProblemConfig*>;
	}

public:
	/************************************************************************/
	/* 生成问题空间													        */
	/************************************************************************/
	E_ReturnState GenerateProblemConfigs()
	{
		T_ProblemConfig * problemConfig;
		T_ExtProducerConsumerProblemConfig * extProblemConfig;

		// ----------------------------------------------------------------------
		// problem config 1
		extProblemConfig = new T_ExtProducerConsumerProblemConfig();
		extProblemConfig->vectorSize = 1024;

		problemConfig = new T_ProblemConfig();
		problemConfig->ConfigName = "";
		problemConfig->extConfig = extProblemConfig;

		ProblemConfigList->push_back(problemConfig);
	}

	/************************************************************************/
	/* 参数初始化                                                            */
	/************************************************************************/
	E_ReturnState InitHost()
	{
		std::cout << "ProducerConsumer init" << problemCfg->ConfigName << std::endl;
		T_ExtProducerConsumerProblemConfig * exCfg = (T_ExtProducerConsumerProblemConfig *)problemCfg->extConfig;

		printf("************************************************************************\n");
		printf("* VectorSize = %d\n", exCfg->vectorSize);
		printf("************************************************************************\n");

		exCfg->h_a = (float*)HstMalloc(exCfg->vectorSize * sizeof(float));
		exCfg->h_b = (float*)HstMalloc(exCfg->vectorSize * sizeof(float));
		exCfg->h_c = (float*)HstMalloc(exCfg->vectorSize * sizeof(float));
		exCfg->c_ref = (float*)HstMalloc(exCfg->vectorSize * sizeof(float));
		exCfg->h_sig = (int*)HstMalloc(cu_num * sizeof(int));
		
		for (int i = 0; i < exCfg->vectorSize; i++)
		{
			exCfg->h_a[i] = 1;
			exCfg->h_b[i] = 2;
			exCfg->h_c[i] = 0;
		}
		for (int i = 0; i < cu_num; i++)
		{
			exCfg->h_sig[i] = 0;
		}

		return E_ReturnState::SUCCESS; 
	} 

	/************************************************************************/
	/* HOST端                                                               */
	/************************************************************************/
	E_ReturnState Host()
	{
		printf("ProducerConsumer host.\n");
		T_ExtProducerConsumerProblemConfig * exCfg = (T_ExtProducerConsumerProblemConfig *)problemCfg->extConfig;

		for (int i = 0; i < exCfg->vectorSize; i++)
		{
			exCfg->c_ref[i] = exCfg->h_a[i] + 1;
		}
		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 校验                                                                 */
	/************************************************************************/
	E_ReturnState Verify()
	{
		printf("ProducerConsumer verify.\n");
		T_ExtProducerConsumerProblemConfig * exCfg = (T_ExtProducerConsumerProblemConfig *)problemCfg->extConfig;
		
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
		printf("ProducerConsumer destroy.\n");
		T_ExtProducerConsumerProblemConfig * exCfg = (T_ExtProducerConsumerProblemConfig *)problemCfg->extConfig;

		HstFree(exCfg->h_a);
		HstFree(exCfg->h_b);
		HstFree(exCfg->h_c);
		HstFree(exCfg->c_ref);
	}	
}; 
