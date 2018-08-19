#pragma once

#include "BasicClass.h"
#include "RuntimeControl.h"
#include "ProblemControl.h"

/************************************************************************/
/* ��չ����                                                              */
/************************************************************************/
typedef struct ExtDsSolutionConfigTpye
{
#define FIX_WORKGROUP_SIZE (64)
}T_ExtDsSolutionConfig;

typedef struct ExtDsProblemConfigType
{
#define VECTOR_SIZE (512)
	size_t vectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtDsProblemConfig;

/************************************************************************/
/* solution����                                                          */
/************************************************************************/
class DsSolution : public SolutionCtrlBase
{
private:
	T_KernelArgu d_a, d_b, d_c, d_lds;
public:
	/************************************************************************/
	/* �����Դ�                                                            */
	/************************************************************************/
	E_ReturnState InitDev()
	{
		T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)problemCfg->extConfig;

		DevMalloc((void**)&(d_a.ptr), exCfg->vectorSize * sizeof(float));
		DevMalloc((void**)&(d_b.ptr), exCfg->vectorSize * sizeof(float));
		DevMalloc((void**)&(d_c.ptr), exCfg->vectorSize * sizeof(float));
		d_lds.ptr = NULL;

		solutionCfg->KernelArgus = new std::list<T_KernelArgu>;
		d_a.size = sizeof(cl_mem);	d_a.isVal = false;	solutionCfg->KernelArgus->push_back(d_a);
		d_b.size = sizeof(cl_mem);	d_b.isVal = false;	solutionCfg->KernelArgus->push_back(d_b);
		d_c.size = sizeof(cl_mem);	d_c.isVal = false;	solutionCfg->KernelArgus->push_back(d_c);
		d_lds.size = sizeof(cl_uint);	d_lds.isVal = true;	solutionCfg->KernelArgus->push_back(d_lds);

		Copy2Dev((cl_mem)(d_a.ptr), exCfg->h_a, exCfg->vectorSize * sizeof(float));
		Copy2Dev((cl_mem)(d_b.ptr), exCfg->h_b, exCfg->vectorSize * sizeof(float));

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* ���ؽ��                                                            */
	/************************************************************************/
	E_ReturnState GetBackResult()
	{
		T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)problemCfg->extConfig;
		Copy2Hst(exCfg->h_c, (cl_mem)(d_c.ptr), exCfg->vectorSize * sizeof(float));
	}

	/************************************************************************/
	/* �ͷ��Դ�	                                                           */
	/************************************************************************/
	void ReleaseDev()
	{
		DevFree((cl_mem)(d_a.ptr));
		DevFree((cl_mem)(d_b.ptr));
		DevFree((cl_mem)(d_c.ptr));
	}

	/************************************************************************/
	/* ����problem������solution�����ռ�                                      */
	/************************************************************************/
	E_ReturnState GenerateSolutionConfigs()
	{
		T_SolutionConfig * solutionConfig;
		T_ExtDsSolutionConfig * extSolutionConfig;

		SolutionConfigList = new std::list<T_SolutionConfig*>;

		// ======================================================================
		// solution config 2: ASM
		// ======================================================================
		extSolutionConfig = new T_ExtDsSolutionConfig();

		solutionConfig = new T_SolutionConfig();
		solutionConfig->ConfigName = "AsmSolution";
		solutionConfig->RepeatTime = 5;
		solutionConfig->extConfig = extSolutionConfig;

		// ----------------------------------------------------------------------
		SolutionConfigList->push_back(solutionConfig);

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* ����solution��������source, complier��worksize                         */
	/************************************************************************/
	E_ReturnState GenerateSolution()
	{
		T_ExtDsProblemConfig * extProblem = (T_ExtDsProblemConfig *)problemCfg->extConfig;
		T_ExtDsSolutionConfig * extSolution = (T_ExtDsSolutionConfig *)solutionCfg->extConfig;
		
		// ======================================================================
		// ���ɴ���
		// ======================================================================
		if (solutionCfg->ConfigName == "AsmSolution")
		{
			solutionCfg->KernelName = "IsaDs";
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}

		// ======================================================================
		// ����worksize
		// ======================================================================
		solutionCfg->l_wk0 = FIX_WORKGROUP_SIZE;
		solutionCfg->l_wk1 = 1;
		solutionCfg->l_wk2 = 1;
		solutionCfg->g_wk0 = extProblem->vectorSize;
		solutionCfg->g_wk1 = 1;
		solutionCfg->g_wk2 = 1;

		return E_ReturnState::SUCCESS;
	}
};

/************************************************************************/
/* �������                                                             */
/************************************************************************/
class DsProblem : public ProblemCtrlBase
{
public:
	DsProblem()
	{
		ProblemName = "DsInstruction";
		Solution = new DsSolution();
		ProblemConfigList = new std::list<T_ProblemConfig*>;
	}

public:
	/************************************************************************/
	/* ��������ռ�													        */
	/************************************************************************/
	E_ReturnState GenerateProblemConfigs()
	{
		T_ProblemConfig * problemConfig;
		T_ExtDsProblemConfig * extProblemConfig;

		// ----------------------------------------------------------------------
		// problem config 1
		extProblemConfig = new T_ExtDsProblemConfig();
		extProblemConfig->vectorSize = VECTOR_SIZE;

		problemConfig = new T_ProblemConfig();
		problemConfig->ConfigName = "512";
		problemConfig->extConfig = extProblemConfig;

		ProblemConfigList->push_back(problemConfig);
	}

	/************************************************************************/
	/* ������ʼ��                                                            */
	/************************************************************************/
	E_ReturnState InitHost()
	{
		std::cout << "Lds instruction init" << problemCfg->ConfigName << std::endl;
		T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)problemCfg->extConfig;

		problemCfg->Calculation = exCfg->vectorSize;
		problemCfg->TheoryElapsedTime = problemCfg->Calculation / RuntimeCtrlBase::DeviceInfo.Fp32Flops;
		printf("Calculation = %.3f G\n", problemCfg->Calculation * 1e-9);
		printf("TheoryElapsedTime = %.3f us \n", problemCfg->TheoryElapsedTime * 1e6);

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
	/* HOST��                                                               */
	/************************************************************************/
	E_ReturnState Host()
	{
		printf("Lds instruction host.\n");
		T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)problemCfg->extConfig;

		for (int i = 0; i < exCfg->vectorSize; i++)
		{
			exCfg->c_ref[i] = exCfg->h_a[i] + exCfg->h_b[i];
		}
		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* У��                                                                 */
	/************************************************************************/
	E_ReturnState Verify()
	{
		printf("Lds instruction verify.\n");
		T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)problemCfg->extConfig;
		
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
	/* �ͷ�                                                                  */
	/************************************************************************/
	void ReleaseHost()
	{
		printf("Lds instruction destroy.\n");
		T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)problemCfg->extConfig;

		HstFree(exCfg->h_a);
		HstFree(exCfg->h_b);
		HstFree(exCfg->h_c);
		HstFree(exCfg->c_ref);
	}	
};