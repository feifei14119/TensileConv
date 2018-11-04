#pragma once 

#include "ConvFwd1x1.h"

using namespace AutoGen;
using namespace AutoTune;

#pragma region SOLUTION
/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
#define		MultSolution	(1)
#define		EnSimuIndex		(0)
#define		EnSaveSource	(1)
#define		EnPrintSource	(0)

/************************************************************************/
/* 根据problem参数成solution参数空间                                      */
/************************************************************************/
E_ReturnState ConvFwd1x1Solution::GenerateSolutionConfigs()
{
	T_SolutionConfig * solutionConfig;
	T_ExtConvFwd1x1SolutionConfig * extSolutionConfig;
	T_SearchParam * searchParam;

	// ======================================================================
	// solution config 1: MIOpenOcl
	// ======================================================================
	{
		extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();

		solutionConfig = new T_SolutionConfig("MIOpenOcl");
		solutionConfig->extConfig = extSolutionConfig;

		// ----------------------------------------------------------------------
		// 添加solution
		//SolutionConfigList->push_back(solutionConfig);
	}

	// ======================================================================
	// solution config 2: MIOpenAsm
	// ======================================================================
	{
		extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();

		solutionConfig = new T_SolutionConfig("MIOpenAsm");
		solutionConfig->extConfig = extSolutionConfig;

		// ----------------------------------------------------------------------
		// 添加solution
		//SolutionConfigList->push_back(solutionConfig);
	}

	// ======================================================================
	// solution config 3: SQC: 标准汇编实现,含prefetch
	// ======================================================================
	{
		extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();

		solutionConfig = new T_SolutionConfig("SQC");
		solutionConfig->extConfig = extSolutionConfig;

		// ----------------------------------------------------------------------
		// 添加solution
		//SolutionConfigList->push_back(solutionConfig);
	}

	// ======================================================================
	// solution config 3: SQC-c_mult: 基于SQC的对C进行分解
	// ======================================================================
	{
		extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();

		solutionConfig = new T_SolutionConfig("C_MULT");
		solutionConfig->extConfig = extSolutionConfig;

		// ----------------------------------------------------------------------
		// 添加solution
		//SolutionConfigList->push_back(solutionConfig);
	}

	// ======================================================================
	// solution config 4: Sigle-kernrl PreFetch
	// ======================================================================
	{
		extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();

		solutionConfig = new T_SolutionConfig("PreFetch_Single");
		solutionConfig->extConfig = extSolutionConfig;

		// ----------------------------------------------------------------------
		// 添加solution
		//SolutionConfigList->push_back(solutionConfig);
	}

	// ======================================================================
	// solution config 5: Mult-kernel Perfetch
	// ======================================================================
	{
		extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();

		solutionConfig = new T_SolutionConfig("PreFetch_Mult");
		solutionConfig->extConfig = extSolutionConfig;

		// ----------------------------------------------------------------------
		// 添加solution
		//SolutionConfigList->push_back(solutionConfig);
	}

	// ======================================================================
	// solution config 6: AutoTuning
	// ======================================================================
	{
		extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();

		solutionConfig = new T_SolutionConfig("TensileConv");
		solutionConfig->extConfig = extSolutionConfig;
#if MultSolution
		// ----------------------------------------------------------------------
		// 生成搜索空间
		searchParam = new T_SearchParam("c_in_group");
		searchParam->ValueArray.push_back(1);
		searchParam->ValueArray.push_back(2);
		searchParam->ValueArray.push_back(4);
		searchParam->ValueArray.push_back(8);
		searchParam->ValueArray.push_back(16);
		searchParam->ValueArray.push_back(32);
		solutionConfig->KernelSearchSpace.AddOneParam(searchParam);
		//--------------------------------
		searchParam = new T_SearchParam("k_out_maps");
		searchParam->ValueArray.push_back(2);
		searchParam->ValueArray.push_back(4);
		searchParam->ValueArray.push_back(8);
		searchParam->ValueArray.push_back(16);
		searchParam->ValueArray.push_back(32);
		solutionConfig->KernelSearchSpace.AddOneParam(searchParam);
		//--------------------------------
		searchParam = new T_SearchParam("group_size");
		searchParam->ValueArray.push_back(64);
		searchParam->ValueArray.push_back(128);
		searchParam->ValueArray.push_back(256);
		searchParam->ValueArray.push_back(512);
//		searchParam->ValueArray.push_back(1024);
		solutionConfig->KernelSearchSpace.AddOneParam(searchParam);
#endif
		// ----------------------------------------------------------------------
		// 添加solution
		SolutionConfigList->push_back(solutionConfig);
	}

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 申请显存                                                            */
/************************************************************************/
E_ReturnState ConvFwd1x1Solution::InitDev()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;
				
	DevMalloc((void**)&(d_in.ptr), extProb->size_in * sizeof(float));
	DevMalloc((void**)&(d_wei.ptr), extProb->size_wei * sizeof(float));
	DevMalloc((void**)&(d_out.ptr), extProb->size_out * sizeof(float));

	d_in.size = sizeof(cl_mem);		d_in.isVal = false;
	d_wei.size = sizeof(cl_mem);	d_wei.isVal = false;
	d_out.size = sizeof(cl_mem);	d_out.isVal = false;

	SolutionConfig->KernelArgus = new std::list<T_KernelArgu>;
	SolutionConfig->KernelArgus->push_back(d_in);
	SolutionConfig->KernelArgus->push_back(d_wei);
	SolutionConfig->KernelArgus->push_back(d_out);
		
	Copy2Dev((cl_mem)(d_in.ptr), extProb->h_in, extProb->size_in * sizeof(float));
	Copy2Dev((cl_mem)(d_wei.ptr), extProb->h_wei, extProb->size_wei * sizeof(float));
		
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 返回结果                                                            */
/************************************************************************/
E_ReturnState ConvFwd1x1Solution::GetBackResult()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
		
	Copy2Hst(extProb->h_out, (cl_mem)(d_out.ptr), extProb->size_out * sizeof(float));

	if (SolutionConfig->ConfigName == "PreFetch_Mult" || SolutionConfig->ConfigName == "PreFetch_Single")
	{
		Copy2Hst(h_signal, (cl_mem)(d_signal.ptr), size_sig * sizeof(int));
	}
}

/************************************************************************/
/* 释放显存	                                                           */
/************************************************************************/
void ConvFwd1x1Solution::ReleaseDev()
{
	DevFree((cl_mem)(d_in.ptr));
	DevFree((cl_mem)(d_wei.ptr));
	DevFree((cl_mem)(d_out.ptr));

	if (SolutionConfig->ConfigName == "PreFetch_Single")
	{
		DevFree((cl_mem)(d_signal.ptr));
		if (SolutionConfig->ConfigName == "PreFetch_Mult")
		{
			delete preKernel;
		}
	}
}

/************************************************************************/
/* 根据solution参数生成source, complier和worksize                        */
/************************************************************************/
E_ReturnState ConvFwd1x1Solution::GenerateSolution()
{
	// 提取搜索参数
	if (generateParameters() != E_ReturnState::SUCCESS)
		return E_ReturnState::FAIL;
	// 生成编译选项
	if (generateCompilerOption() != E_ReturnState::SUCCESS)
		return E_ReturnState::FAIL;
	// 生成worksize
	if (generateWorkLoad() != E_ReturnState::SUCCESS)
		return E_ReturnState::FAIL;
	// 获取/生成代码
	if (generateSource() != E_ReturnState::SUCCESS)
		return E_ReturnState::FAIL;

#if EnSimuIndex
	simulateIndex();
#endif
	return E_ReturnState::SUCCESS;
}

E_ReturnState ConvFwd1x1Solution::generateParameters()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

	if (SolutionConfig->ConfigName == "MIOpenOcl")
	{
		extSol->group_size = WAVE_SIZE;

		// chunk config
		// ---------------
		int n_out_pix_tiles = 16;
		extSol->k_out_maps = n_out_pix_tiles;
		extSol->k_out_maps = std::min(extSol->k_out_maps, extProb->K);
		while ((extProb->K % extSol->k_out_maps) != 0 && extSol->k_out_maps > 16)
		{
			extSol->k_out_maps /= 2;
		}
		n_out_pix_tiles = extSol->k_out_maps;

		// ---------------
		int n_in_data_tiles = 2048;
		N_LCL_IN_MAPS = n_in_data_tiles;
		N_LCL_IN_MAPS = std::min(N_LCL_IN_MAPS, extProb->C);
		if (N_LCL_IN_MAPS < extProb->C && N_LCL_IN_MAPS > 0 && (N_LCL_IN_MAPS % 8) == 0)
		{
			// Pass will do nothing
		}
		else
		{
			N_LCL_IN_MAPS = extProb->C;
		}
		n_in_data_tiles = N_LCL_IN_MAPS;

		N_IN_GROUPS = (extProb->C + N_LCL_IN_MAPS - 1) / N_LCL_IN_MAPS;
		N_LCL_IN_MAPS_ONCE = 8;

		CLOOP0 = N_LCL_IN_MAPS / N_LCL_IN_MAPS_ONCE;
		CLOOP2 = (extProb->C - N_LCL_IN_MAPS * (N_IN_GROUPS - 1)) / N_LCL_IN_MAPS_ONCE;

		align = ((extProb->W * extProb->H * extProb->N + WAVE_SIZE - 1) / WAVE_SIZE) * WAVE_SIZE;
		N_OUT_GROUPS = (extProb->K / extSol->k_out_maps);

		extSol->k_out_group = (extProb->K + extSol->k_out_maps - 1) / extSol->k_out_maps;
	}
	else if (SolutionConfig->ConfigName == "SQC")
	{
		extSol->group_size = WAVE_SIZE;
		extSol->k_out_maps = 16;
		extSol->k_out_group = divCeil(extProb->K, extSol->k_out_maps);
		extSol->c_in_maps = extProb->C;
		extSol->c_in_group = divCeil(extProb->C, extSol->c_in_maps);

		extSol->pix_per_group = 64;
		align = divCeil(extProb->W * extProb->H * extProb->N, extSol->pix_per_group) * extSol->pix_per_group;
		extSol->c_in_maps_once = 8;
		loop = extSol->c_in_maps / extSol->c_in_maps_once;
	}
	else if (SolutionConfig->ConfigName == "C_MULT")
	{
		extSol->group_size = WAVE_SIZE;
		extSol->k_out_maps = 16;
		extSol->k_out_group = divCeil(extProb->K, extSol->k_out_maps);
		extSol->c_in_maps = extProb->C / 16;
		extSol->c_in_group = divCeil(extProb->C, extSol->c_in_maps);

		extSol->pix_per_group = 64;
		align = divCeil(extProb->W * extProb->H * extProb->N, extSol->pix_per_group) * extSol->pix_per_group;
		extSol->c_in_maps_once = 8;
		loop = extSol->c_in_maps / extSol->c_in_maps_once;
	}
	else if (SolutionConfig->ConfigName == "PreFetch_Single")
	{
		extSol->group_size = WAVE_SIZE;
		extSol->k_out_maps = 16;
		extSol->k_out_group = divCeil(extProb->K, extSol->k_out_maps);
		extSol->c_in_maps = extProb->C;
		extSol->c_in_group = divCeil(extProb->C, extSol->c_in_maps);

		extSol->c_in_maps_once = 8;
		loop = extSol->c_in_maps / extSol->c_in_maps_once;
		extSol->pix_per_group = 64;
		align = divCeil(extProb->W * extProb->H * extProb->N, extSol->pix_per_group) * extSol->pix_per_group;
			
		// signal space
		sig_num_per_cu = extSol->c_in_maps / CACHE_LINE;
		size_sig = sig_num_per_cu * CU_NUM;
		h_signal = (int*)HstMalloc(size_sig * sizeof(int));
		DevMalloc((void**)&(d_signal.ptr), size_sig * sizeof(uint));
		d_signal.size = sizeof(cl_mem);	d_signal.isVal = false;
		SolutionConfig->KernelArgus->push_back(d_signal);
	}
	else if (SolutionConfig->ConfigName == "PreFetch_Mult")
	{
		extSol->group_size = WAVE_SIZE;
		extSol->k_out_maps = 16;
		extSol->k_out_group = divCeil(extProb->K, extSol->k_out_maps);
		extSol->c_in_maps = extProb->C;
		extSol->c_in_group = divCeil(extProb->C, extSol->c_in_maps);

		extSol->c_in_maps_once = 8;
		loop = extSol->c_in_maps / extSol->c_in_maps_once;
		extSol->pix_per_group = 64;
		align = divCeil(extProb->W * extProb->H * extProb->N, extSol->pix_per_group);

		// signal mem
		size_sig = loop / 2 * CU_NUM;
		DevMalloc((void**)&(d_signal.ptr), size_sig * sizeof(uint));
		d_signal.size = sizeof(cl_mem);	d_signal.isVal = false;
		SolutionConfig->KernelArgus->push_back(d_signal);
		h_signal = (int*)HstMalloc(size_sig * sizeof(int));
		// prefetch kernel
		preKernel = new RuntimeCtrlOcl();
		preArgus = new std::list<T_KernelArgu>;
		preArgus->push_back(d_in);
		preArgus->push_back(d_wei);
		preArgus->push_back(d_out);
		preArgus->push_back(d_signal);
	}
	else if (SolutionConfig->ConfigName == "TensileConv")
	{
		while (true)
		{
			T_SearchParam * param;
			param = SolutionConfig->KernelSearchSpace.GetOneParam();
			if (param == NULL)
			{
				break;
			}

			if (param->Name == "c_in_group")
			{
				extSol->c_in_group = param->CurrValue;
			}
			if (param->Name == "k_out_maps")
			{
				extSol->k_out_maps = param->CurrValue;
			}
			if (param->Name == "group_size")
			{
				extSol->group_size = param->CurrValue;
			}
		}

		if (extSol->c_in_group == 0)
		{
			extSol->c_in_group = 1;
		}
		if (extSol->k_out_maps == 0)
		{
			extSol->k_out_maps = 8;
		}
		if (extSol->group_size == 0)
		{
			extSol->group_size = 64;
		}

		extSol->c_in_maps = extProb->C / extSol->c_in_group;
		extSol->k_out_group = divCeil(extProb->K, extSol->k_out_maps);

		extSol->c_in_maps_once = 8;
		loop = divCeil(extSol->c_in_maps, extSol->c_in_maps_once);
		extSol->pix_per_group = 64;
		align = divCeil(extProb->W * extProb->H * extProb->N, extSol->group_size) * extSol->group_size;

		printf("----------------------------------------------------------------------\n");
		printf("Kernel Param:\n");
		printf("	c_in_maps =[%d], c_in_group =[%d]\n", extSol->c_in_maps, extSol->c_in_group);
		printf("	k_out_maps=[%d], k_out_group=[%d]\n", extSol->k_out_maps, extSol->k_out_group);
		printf("	group_size=[%d]\n", extSol->group_size);
		printf("	align=[%d]\n", align);
		printf("----------------------------------------------------------------------\n");
	}

	return E_ReturnState::SUCCESS;
}

E_ReturnState ConvFwd1x1Solution::generateCompilerOption()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

	if (SolutionConfig->ConfigName == "TensileConv")
	{
		SolutionConfig->extCompilerOpt = "";
	}
	else if (SolutionConfig->ConfigName == "MIOpenOcl")
	{
		SolutionConfig->extCompilerOpt =
			std::string(" -DMLO_FILTER_STRIDE0=1") +
			std::string(" -DMLO_FILTER_STRIDE1=1") +
			std::string(" -DMLO_N_LCL_IN_MAPS_ONCE=") + std::to_string(N_LCL_IN_MAPS_ONCE) +
			std::string(" -DBATCHSIZE=") + std::to_string(extProb->N) +
			std::string(" -DH=") + std::to_string(extProb->H) +
			std::string(" -DW=") + std::to_string(extProb->W) +
			std::string(" -DC=") + std::to_string(extProb->C) +
			std::string(" -DK=") + std::to_string(extProb->K) +
			std::string(" -DMLO_N_LCL_IN_MAPS=") + std::to_string(N_LCL_IN_MAPS) +
			std::string(" -DMLO_N_INPUTS=") + std::to_string(extProb->C) +
			std::string(" -DMLO_N_OUTPUTS=") + std::to_string(extProb->K) +
			std::string(" -DH_out=") + std::to_string(extProb->OutH) +
			std::string(" -DW_out=") + std::to_string(extProb->OutW) +
			std::string(" -DMLO_N_IN_GROUPS=") + std::to_string(N_IN_GROUPS) +
			std::string(" -DMLO_CLOOP0=") + std::to_string(CLOOP0) +
			std::string(" -DMLO_CLOOP2=") + std::to_string(CLOOP2) +
			std::string(" -DMLO_N_LCL_OUT_MAPS=") + std::to_string(extSol->k_out_maps) +
			std::string(" -DMLO_CHEAT_SHADER_COMPILER=1") +
			std::string(" -DMLopen_RUNNING=1") +
			std::string(" -DMIOPEN_USE_FP32=1");
	}
	else
	{
		int in_pix_groups = divCeil(align, extSol->group_size);
		int k_out_groups = divCeil(extProb->K, extSol->k_out_maps);
		int groupNum = in_pix_groups * k_out_groups;

		int grpNumPerCUMax = (groupNum + CU_NUM - 1) / CU_NUM;
		int grpNumPerCUMin = groupNum / CU_NUM;
		int maxGrpCUNum = (groupNum - grpNumPerCUMin * CU_NUM) / SE_NUM;
		int minGrpCUNum = (CU_NUM - maxGrpCUNum * SE_NUM) / SE_NUM;
		int grpNumPerSe = groupNum / SE_NUM;

		SolutionConfig->extCompilerOpt =
			std::string(" -Wa,-defsym,W=") + std::to_string(extProb->W) +
			std::string(" -Wa,-defsym,H=") + std::to_string(extProb->H) +
			std::string(" -Wa,-defsym,C=") + std::to_string(extProb->C) +
			std::string(" -Wa,-defsym,K=") + std::to_string(extProb->K) +
			std::string(" -Wa,-defsym,N=") + std::to_string(extProb->N) +
			std::string(" -Wa,-defsym,MLO_N_OUT_GROUPS=") + std::to_string(extSol->k_out_group) +
			std::string(" -Wa,-defsym,MLO_N_OUT_GROUPS_LOG2=") + std::to_string(log2(extSol->k_out_group)) +
			std::string(" -Wa,-defsym,MLO_N_OUT_GROUPS_DIV_MASK=") + std::to_string(extSol->k_out_group-1) +
			std::string(" -Wa,-defsym,MLO_N_LCL_IN_MAPS=") + std::to_string(extSol->c_in_maps) +
			std::string(" -Wa,-defsym,MLO_N_IN_GROUPS=") + std::to_string(extSol->c_in_group) +
			std::string(" -Wa,-defsym,MLO_N_IN_GROUPS_LOG2=") + std::to_string(log2(extSol->c_in_group)) +
			std::string(" -Wa,-defsym,MLO_N_IN_GROUPS_DIV_MASK=") + std::to_string(extSol->c_in_group-1) +

			std::string(" -Wa,-defsym,GRP_NUM_SE=") + std::to_string(grpNumPerSe) +
			std::string(" -Wa,-defsym,GRP_NUM_CU_MIN=") + std::to_string(grpNumPerCUMin) +
			std::string(" -Wa,-defsym,MAX_GRP_CU_NUM=") + std::to_string(maxGrpCUNum) +
			std::string(" -Wa,-defsym,PIX_GRP_NUM=") + std::to_string(in_pix_groups) +
			std::string(" -Wa,-defsym,SIG_NUM_PER_CU=") + std::to_string(sig_num_per_cu) +
			std::string(" -Wa,-defsym,SIG_NUM_PER_CU_LOG2=") + std::to_string(log2(sig_num_per_cu));
	}
	return E_ReturnState::SUCCESS;
}

E_ReturnState ConvFwd1x1Solution::generateWorkLoad()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

	if (SolutionConfig->ConfigName == "MIOpenOcl")
	{
		SolutionConfig->l_wk0 = extSol->group_size;
		SolutionConfig->l_wk1 = 1;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = align * N_IN_GROUPS * N_OUT_GROUPS;
		SolutionConfig->g_wk1 = 1;
		SolutionConfig->g_wk2 = 1;

		//SolutionConfig->l_wk0 = WAVE_SIZE*4;
	}
	else if (SolutionConfig->ConfigName == "SQC")
	{
		SolutionConfig->l_wk0 = extSol->group_size;
		SolutionConfig->l_wk1 = 1;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = align * extSol->c_in_group * extSol->k_out_group;
		SolutionConfig->g_wk1 = 1;
		SolutionConfig->g_wk2 = 1;

		//SolutionConfig->l_wk0 = 1;
		//SolutionConfig->g_wk0 = 64;
	}
	else if (SolutionConfig->ConfigName == "C_MULT")
	{
		SolutionConfig->l_wk0 = extSol->group_size;
		SolutionConfig->l_wk1 = 1;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = align * extSol->c_in_group * extSol->k_out_group;
		SolutionConfig->g_wk1 = 1;
		SolutionConfig->g_wk2 = 1;
	}
	else if (SolutionConfig->ConfigName == "PreFetch_Single")
	{
		SolutionConfig->l_wk0 = extSol->group_size;
		SolutionConfig->l_wk1 = 1;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = align * extSol->c_in_group * extSol->k_out_group;
		SolutionConfig->g_wk1 = 1;
		SolutionConfig->g_wk2 = 1;

		//SolutionConfig->l_wk0 = 64;		// 需要注释掉循环控制的exec
		//SolutionConfig->g_wk0 = (CU_NUM * SolutionConfig->l_wk0);

		SolutionConfig->g_wk0 += (CU_NUM * SolutionConfig->l_wk0);
	}
	else if (SolutionConfig->ConfigName == "PreFetch_Mult")
	{
		SolutionConfig->l_wk0 = extSol->group_size;
		SolutionConfig->l_wk1 = 1;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = align * extSol->c_in_group * extSol->k_out_group;
		SolutionConfig->g_wk1 = 1;
		SolutionConfig->g_wk2 = 1;

		//SolutionConfig->l_wk0 = 1;
		//SolutionConfig->g_wk0 = 64;
	}
	else if (SolutionConfig->ConfigName == "TensileConv")
	{
		SolutionConfig->l_wk0 = extSol->group_size;
		SolutionConfig->l_wk1 = 1;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = align * extSol->c_in_group * extSol->k_out_group;
		SolutionConfig->g_wk1 = 1;
		SolutionConfig->g_wk2 = 1;
			
		//SolutionConfig->l_wk0 = 64;
		//SolutionConfig->g_wk0 = 64*64;
	}

	SolutionConfig->b_wk0 = (SolutionConfig->g_wk0 + SolutionConfig->l_wk0 - 1) / SolutionConfig->l_wk0;
	SolutionConfig->b_wk1 = (SolutionConfig->g_wk1 + SolutionConfig->l_wk1 - 1) / SolutionConfig->l_wk1;
	SolutionConfig->b_wk2 = (SolutionConfig->g_wk2 + SolutionConfig->l_wk2 - 1) / SolutionConfig->l_wk2;

	if (SolutionConfig->b_wk0 == 0)
		return E_ReturnState::FAIL;

	return E_ReturnState::SUCCESS;
}

E_ReturnState ConvFwd1x1Solution::generateSource()
{
	SolutionConfig->KernelName = "ConvFwd1x1";
	if (SolutionConfig->ConfigName == "MIOpenOcl")
	{
		SolutionConfig->KernelFile = "ConvFwd1x1_Jcl.cl";
		//SolutionConfig->KernelFile = "ConvFwd1x1_Jcl_NewOrg.cl";
		//SolutionConfig->KernelFile = "ConvFwd1x1_Jcl_256thread.cl";
		SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_OCL_FILE;
	}
	else if (SolutionConfig->ConfigName == "SQC")
	{
		SolutionConfig->KernelFile = "ConvFwd1x1_Jasm.s";
		//SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_NewOrg.s";
		//SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_256thread.s";
		SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
	}
	else if (SolutionConfig->ConfigName == "C_MULT")
	{
		SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_Cin_Mult.s";
		SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
	}
	else if (SolutionConfig->ConfigName == "PreFetch_Single")
	{
		//SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_PreFetch_Single.s";
		//SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_PreFetch_Single_NewOrg.s";
		//SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_PreFetch_Single_FixWei.s";
		SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_PreFetch_Single_FixWei_2.s";
		SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
	}
	else if (SolutionConfig->ConfigName == "PreFetch_Mult")
	{
		SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_PreFetch_Mult_Conv.s";
		SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
	}
	else if (SolutionConfig->ConfigName == "TensileConv")
	{
		SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_TensileConv.s";
		autoGenKernel();
		SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
	}

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 自动生成kernel								                        */
/************************************************************************/
void ConvFwd1x1Solution::autoGenKernel()
{		
	KernelWriterConv1x1 * kw = new KernelWriterConv1x1(ProblemConfig,SolutionConfig);
	kw->GenKernelString();
#if EnPrintSource
	kw->PrintKernelString();
#endif
#if EnSaveSource
	kw->SaveKernelString2File();
#endif
}

/************************************************************************/
/* 记录性能和配置															*/
/************************************************************************/
void ConvFwd1x1Solution::ReportProblemPerformence()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

	printf("------------------------------------------------\n");
	printf("ProbemConfig [WHCKN]=[%d,%d,%d,%d,%d]:\n", extProb->H, extProb->W, extProb->C, extProb->K, extProb->N);

	printf("shortest time: %.3f (us).\t", ProblemBestTime * 1e6);
	printf("best performence: %.1f%%.\n", ProblemBestPerformence * 100);

	while (true)
	{
		T_SearchParam * param;
		param = SolutionConfig->KernelSearchSpace.GetOneParam();
		if (param == NULL)
		{
			break;
		}

		printf("%s = %d\n", param->Name.c_str(), param->BestValue);
	}
}
	
/************************************************************************/
/* 测试下标计算															*/
/************************************************************************/
void ConvFwd1x1Solution::simulateIndex()
{
	T_ExtConvFwd1x1ProblemConfig * extProbCfg = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSolCfg = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

	int *testId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
	int *testPixBlkId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
	int *testCInBlkId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
	int *testKOutBlkId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
	int *testPosId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
	int *testBatchId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
	int *testOutId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));

	uint in_chan_stride = extProbCfg->W * extProbCfg->H;
	uint in_batch_stride = extProbCfg->W * extProbCfg->H * extProbCfg->C;
	uint wei_chan_stride = extProbCfg->C;
	uint out_chan_stride = extProbCfg->W * extProbCfg->H;
	uint out_batch_stride = extProbCfg->W * extProbCfg->H * extProbCfg->K;

	uint c_in_maps = extSolCfg->c_in_maps;
	uint c_in_group = extSolCfg->c_in_group;
	uint k_out_maps = extSolCfg->k_out_maps;
	uint k_out_group = extSolCfg->k_out_group;

	uint wave_per_group = SolutionConfig->l_wk0 / WAVE_SIZE;
	uint conv_loop = extProbCfg->C / extSolCfg->c_in_maps_once / 2;
	
	for (int grp = 0; grp < SolutionConfig->b_wk0; grp++)
	{
		uint tid_x = 5*64+40;
		uint gid_x = grp;
		int waveId = -1, tidInWave = -1;
		int pixBlkId = -1, kOutBlkId = -1, cInBlkId = -1;
		int pos_id = -1, batch_id = -1, out_id = -1;
		int gbl_in_off = -1, wei_off = -1, gbl_out_off = -1;

		waveId = gid_x * wave_per_group + tid_x / WAVE_SIZE;
		tidInWave = tid_x % WAVE_SIZE;

		cInBlkId = waveId / k_out_group % c_in_group;
		pixBlkId = waveId / k_out_group / c_in_group;
		kOutBlkId = waveId % k_out_group;

		pos_id   = (pixBlkId * WAVE_SIZE + tidInWave) % in_chan_stride;
		batch_id = (pixBlkId * WAVE_SIZE + tidInWave) / in_chan_stride;
		out_id   = kOutBlkId * k_out_maps;

		if (batch_id >= extProbCfg->N)
			goto STORE_IDX;

		gbl_in_off  = (batch_id * in_batch_stride) + (cInBlkId * c_in_maps * in_chan_stride) + pos_id;
		wei_off = (out_id * wei_chan_stride) + (cInBlkId * c_in_maps);
		gbl_out_off = (batch_id * out_batch_stride) + (out_id * out_chan_stride) + pos_id;

	STORE_IDX:
		testId[grp] = wei_off;
		testPixBlkId[grp] = pixBlkId;
		testCInBlkId[grp] = cInBlkId;
		testKOutBlkId[grp] = kOutBlkId;
		testPosId[grp] = pos_id;
		testBatchId[grp] = batch_id;
		testOutId[grp] = out_id;
	}

	//printIndex(testId, "test temp id");
	//printIndex(testPixBlkId, "pix block id");
	//printIndex(testCInBlkId, "c_in block id");
	//printIndex(testKOutBlkId, "k_out block id");
	printIndex(testBatchId, "batch id");
	//printIndex(testOutId, "out id");
	//printIndex(testPosId, "pos id");

	free(testId);
	free(testPixBlkId);
	free(testCInBlkId);
	free(testKOutBlkId);
	free(testPosId);
	free(testBatchId);
	free(testOutId);
}
#pragma endregion

#pragma region PROBLEM
/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
#define		SkipHost		(0)

/************************************************************************/
/* 生成问题空间													        */
/************************************************************************/
E_ReturnState ConvFwd1x1Problem::TurnProblem()
{
	T_ProblemConfig * probCfg;
	T_ExtConvFwd1x1ProblemConfig * exProbCfg;

	T_SearchParam * searchParam;

	probCfg = new T_ProblemConfig("convolution 1x1");
	exProbCfg = new T_ExtConvFwd1x1ProblemConfig();
	probCfg->extConfig = exProbCfg;

	searchParam = new T_SearchParam("C");
	searchParam->ValueArray.push_back(64);
	searchParam->ValueArray.push_back(128);
	searchParam->ValueArray.push_back(256);
	searchParam->ValueArray.push_back(512);
	searchParam->ValueArray.push_back(1024);
	searchParam->ValueArray.push_back(2048);
	probCfg->ProblemParamSpace.AddOneParam(searchParam);
	searchParam = new T_SearchParam("K");
	searchParam->ValueArray.push_back(64);
	searchParam->ValueArray.push_back(128);
	searchParam->ValueArray.push_back(256);
	searchParam->ValueArray.push_back(512);
	searchParam->ValueArray.push_back(1024);
	searchParam->ValueArray.push_back(2048);
	probCfg->ProblemParamSpace.AddOneParam(searchParam);
	searchParam = new T_SearchParam("N");
	searchParam->ValueArray.push_back(1);
	searchParam->ValueArray.push_back(2);
	searchParam->ValueArray.push_back(4);
	searchParam->ValueArray.push_back(8);
	searchParam->ValueArray.push_back(16);
	searchParam->ValueArray.push_back(32);
	probCfg->ProblemParamSpace.AddOneParam(searchParam);
	searchParam = new T_SearchParam("WH");
	searchParam->ValueArray.push_back(7);
	searchParam->ValueArray.push_back(14);
	searchParam->ValueArray.push_back(28);
	searchParam->ValueArray.push_back(56);
	searchParam->ValueArray.push_back(112);
	probCfg->ProblemParamSpace.AddOneParam(searchParam);

	ProblemConfigList->push_back(probCfg);

	RunAllProblem();
}
E_ReturnState ConvFwd1x1Problem::TurnProblem(int W, int H, int C, int K, int N)
{
	T_ProblemConfig * probCfg = new T_ProblemConfig("convolution 1x1");
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = new T_ExtConvFwd1x1ProblemConfig();

	exProbCfg->W = W;		exProbCfg->H = H;
	exProbCfg->C = C;		exProbCfg->K = K;
	exProbCfg->N = N;
	probCfg->extConfig = exProbCfg;

	ProblemConfigList->push_back(probCfg);

	RunAllProblem();
}
	
/************************************************************************/
/* 参数初始化                                                            */
/************************************************************************/
E_ReturnState ConvFwd1x1Problem::InitHost()
{
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;

	while (true)
	{
		T_SearchParam * param;
		param = ProblemConfig->ProblemParamSpace.GetOneParam();
		if (param == NULL)
		{
			break;
		}

		if (param->Name == "N")
		{
			exProbCfg->N = param->CurrValue;
		}
		if (param->Name == "C")
		{
			exProbCfg->C = param->CurrValue;
		}
		if (param->Name == "K")
		{
			exProbCfg->K = param->CurrValue;
		}
		if (param->Name == "WH")
		{
			exProbCfg->W = param->CurrValue;
			exProbCfg->H = param->CurrValue;
		}
	}

	exProbCfg->X = 1;		exProbCfg->Y = 1;
	exProbCfg->R = 0;		exProbCfg->S = 0;
	exProbCfg->U = 1;		exProbCfg->V = 1;

	printf("************************************************************************\n");
	printf("* WHCKN=[%d * %d, %d, %d, %d]\n", exProbCfg->W, exProbCfg->H, exProbCfg->C, exProbCfg->K, exProbCfg->N);
	printf("************************************************************************\n");

	exProbCfg->OutW = exProbCfg->W + exProbCfg->R * 2 - exProbCfg->X + 1;
	exProbCfg->OutH = exProbCfg->H + exProbCfg->S * 2 - exProbCfg->Y + 1;
		
	exProbCfg->size_in = exProbCfg->W * exProbCfg->H * exProbCfg->C * exProbCfg->N;
	exProbCfg->size_wei = exProbCfg->X * exProbCfg->Y * exProbCfg->C * exProbCfg->K;
	exProbCfg->size_out = exProbCfg->W * exProbCfg->H * exProbCfg->K * exProbCfg->N;
	
	ProblemConfig->Calculation = 1.0 * exProbCfg->W * exProbCfg->H * exProbCfg->C * exProbCfg->N * exProbCfg->K * exProbCfg->X * exProbCfg->Y; // */stride_R/stride_S
	//ProblemConfig->TheoryElapsedTime = ProblemConfig->Calculation / RuntimeCtrlBase::DeviceInfo.Fp32Flops;
	ProblemConfig->TheoryElapsedTime = ProblemConfig->Calculation / (64*64*1.5*1000*1000*1000);

	printf("Calculation = %.3f G\n", ProblemConfig->Calculation * 1e-9);
	printf("TheoryElapsedTime = %.3f us \n", ProblemConfig->TheoryElapsedTime * 1e6);

	exProbCfg->h_in = (float*)HstMalloc(exProbCfg->size_in * sizeof(float));
	exProbCfg->h_wei = (float*)HstMalloc(exProbCfg->size_wei * sizeof(float));
	exProbCfg->h_out = (float*)HstMalloc(exProbCfg->size_out * sizeof(float));
	exProbCfg->out_ref = (float*)HstMalloc(exProbCfg->size_out * sizeof(float));

	printf("input  WHCN = [%d, %d, %d, %d]\n", exProbCfg->W, exProbCfg->H, exProbCfg->C, exProbCfg->N);
	printf("weight WHCK = [%d, %d, %d, %d]\n", exProbCfg->X, exProbCfg->Y, exProbCfg->C, exProbCfg->K);
	printf("output WHKN = [%d, %d, %d, %d]\n", exProbCfg->OutW, exProbCfg->OutH, exProbCfg->K, exProbCfg->N);
	printf("init tensor input  = %d = %.3f MByte.\n", exProbCfg->size_in / sizeof(float), exProbCfg->size_in / 1024 / 1024.0);
	printf("init tensor weight = %d = %.3f KByte.\n", exProbCfg->size_wei / sizeof(float), exProbCfg->size_wei / 1024.0);
	printf("init tensor output = %d = %.3f MByte.\n", exProbCfg->size_out / sizeof(float), exProbCfg->size_out / 1024 / 1024.0);

	for (int i = 0; i < exProbCfg->size_in; i++)
	{
		exProbCfg->h_in[i] = 1;
		//exProbCfg->h_in[i] = (float)(i % 7) + 1.0f;
		exProbCfg->h_in[i] = (float)(rand() % 100 - 50);
		//exProbCfg->h_in[i] = (double)rand() * (1.0 / RAND_MAX);
	}
	for (int i = 0; i < exProbCfg->size_wei; i++)
	{
		exProbCfg->h_wei[i] = 1;
		//exProbCfg->h_wei[i] = (float)(i % 3);
		exProbCfg->h_wei[i] = (float)(rand() % 100 - 50);
		//exProbCfg->h_in[i] = (double)rand() * (1.0 / RAND_MAX);
	}
	for (int i = 0; i < exProbCfg->size_out; i++)
	{
		exProbCfg->h_out[i] = 555.555;
	}

	//printTensor("input", h_in, exProbCfg->W, exProbCfg->H, exProbCfg->C, exProbCfg->N);
	//printTensor("weight", h_wei, exProbCfg->X, exProbCfg->Y, exProbCfg->C, exProbCfg->K);
	
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* HOST端                                                               */
/************************************************************************/
E_ReturnState ConvFwd1x1Problem::Host()
{
#if SkipHost
	return E_ReturnState::SUCCESS;
#endif
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;

	int stride_n_in;			// stride for differetn batch of input
	int stride_c_in;			// stride for differetn channel in the same batch of input
	int stride_k_wei;			// stride for differetn feature of weight
	int stride_c_wei;			// stride for differetn channel in the same feature of weight
	int stride_n_out;			// stride for differetn bathc of output
	int stride_k_out;			// stride for differetn feature in the same batch of output

	int dilation_h = 1;
	int dilation_w = 1;

	stride_c_in = exProbCfg->W * exProbCfg->H;
	stride_n_in = exProbCfg->W * exProbCfg->H * exProbCfg->C;
	stride_c_wei = exProbCfg->X * exProbCfg->Y;
	stride_k_wei = exProbCfg->X * exProbCfg->Y * exProbCfg->C;
	stride_k_out = exProbCfg->OutW * exProbCfg->OutH;
	stride_n_out = exProbCfg->OutW * exProbCfg->OutH * exProbCfg->K;

	for (int o = 0; o < exProbCfg->N; o++)			// for batch size
	{
		for (int w = 0; w < exProbCfg->K; w++)		// for output features 
		{
			for (int i = 0; i < exProbCfg->OutH; i++)		// for output heigh
			{
				int in_off_h = i * exProbCfg->U;
				for (int j = 0; j < exProbCfg->OutW; j++)	// for output width
				{
					float acc = 0;
					int in_off_w = j * exProbCfg->V;
					for (int k = 0; k < exProbCfg->C; k++)		// sum input channels
					{
						for (int x = 0; x < exProbCfg->Y; x++)		// for filter heigh
						{
							int in_x = in_off_h - exProbCfg->S + x * dilation_h;

							if (in_x >= 0 && in_x < exProbCfg->H)
							{
								for (int y = 0; y < exProbCfg->X; y++)	// for filter width
								{
									int in_y = in_off_w - exProbCfg->R + y * dilation_w;// start idx of input a line for conv

									if (in_y >= 0 && in_y < exProbCfg->W)
									{
										int idx_in = o * stride_n_in + k * stride_c_in + in_x * exProbCfg->W + in_y;
										int idx_wei = w * stride_k_wei + k * stride_c_wei + x * exProbCfg->X + y;
										acc += exProbCfg->h_in[idx_in] * exProbCfg->h_wei[idx_wei];
									}
								}
							}
						}
					}
					exProbCfg->out_ref[o * stride_n_out + w * stride_k_out + i * exProbCfg->OutW + j] = acc;
				}
			}
		}
	}

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 校验                                                                 */
/************************************************************************/
E_ReturnState ConvFwd1x1Problem::Verify()
{
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;

	//printTensor("output", out_ref, OutW, OutH, K, N);

	float diff = 0;
	for (int i = 0; i < exProbCfg->size_out; i++)
	{
		diff += (exProbCfg->out_ref[i] - exProbCfg->h_out[i]) * (exProbCfg->out_ref[i] - exProbCfg->h_out[i]);
	}
	diff /= exProbCfg->size_out;

	printf("mean err = %.1f.\n", diff);
	if (!(diff >= 0 && diff < MIN_FP32_ERR))
	{
		printf("err = %.2f\n", diff);
		printf("verify failed!\n");
		return E_ReturnState::FAIL;
	}
	printf("verify success.\n");
	return E_ReturnState::SUCCESS;
}
	 
/***********************************************************************/
/* 释放                                                                 */
/************************************************************************/
void ConvFwd1x1Problem::ReleaseHost()
{
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;

	HstFree(exProbCfg->h_in);
	HstFree(exProbCfg->h_wei);
	HstFree(exProbCfg->h_out);
	HstFree(exProbCfg->out_ref);
}



T_SolutionConfig * ConvFwd1x1Problem::NewSolutionConfig(std::string name)
{
	T_SolutionConfig * solutionConfig = new T_SolutionConfig(name);
	T_ExtConvFwd1x1SolutionConfig * extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();
	solutionConfig->extConfig = extSolutionConfig;

	Solution->SolutionConfigList->push_back(solutionConfig);

	return solutionConfig;
}

E_ReturnState ConvFwd1x1Problem::NewTurnParam(T_SolutionConfig * solCfg, E_TurnParam param, std::vector<int> turnVal)
{
	T_SearchParam * searchParam;
	if (param == E_TurnParam::TURN_PARAM_C_IN_GROUP)
	{
		searchParam = new T_SearchParam("c_in_group");
	}
	else if (param == E_TurnParam::TURN_PARAM_K_OUT_MAPS)
	{
		searchParam = new T_SearchParam("k_out_maps");
	}
	else if (param == E_TurnParam::TURN_PARAM_GROUP_SIZE)
	{
		searchParam = new T_SearchParam("group_size");
	}
	else
	{
		return E_ReturnState::FAIL;
	}

	for (int i = 0; i < turnVal.size(); i++)
	{
		int val = turnVal[i];
		searchParam->ValueArray.push_back(val);
	}

	solCfg->KernelSearchSpace.AddOneParam(searchParam);

	return E_ReturnState::SUCCESS;
}



#pragma endregion
