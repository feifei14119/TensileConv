#pragma once 

#include "ConvFwd1x1Config.h"
#include "ConvFwd1x1KernelWriter.h"

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
#define		MultSolution	(0)
#define		EnSimuIndex		(0)
#define		EnSaveSource	(1)
class ConvFwd1x1Solution : public SolutionCtrlBase
{
private:
	size_t align;
	T_KernelArgu d_in, d_wei, d_out;

	// -------------------------------------------------------------------
	// K划分
	int k_out_group;	// 一个像素的所有输出特征值分给几个CU计算
	// C划分
	int c_in_maps;		// 每个CU计算多少个输入通道C
	int c_in_group;		// 一个像素的所有输入通道分给几个CU计算
	// W,H划分
	int pix_per_group;	// 每个CU计算多少个输入像素
	// thread规划
	int pix_maps;	// 每个thread负责多少个输出像素
	// 程序规划
	int c_in_maps_once;	// 每轮循环计算多少个输入通道C
	int loop;			// 循环次数

	// -------------------------------------------------------------------
	// prefetch mult-kernel
	int *h_signal = nullptr;
	int sig_num_per_cu,size_sig;
	T_KernelArgu d_signal;
	RuntimeCtrl * preKernel;

public:
	ConvFwd1x1Solution() :SolutionCtrlBase()
	{
	}

	/************************************************************************/
	/* 根据problem参数成solution参数空间                                      */
	/************************************************************************/
	E_ReturnState GenerateSolutionConfigs()
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
		// solution config 4: Sigle-kernrl PreFetch
		// ======================================================================
		{
			extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();

			solutionConfig = new T_SolutionConfig("PreFetch_Single");
			solutionConfig->extConfig = extSolutionConfig;

			// ----------------------------------------------------------------------
			// 添加solution
			SolutionConfigList->push_back(solutionConfig);
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
			//searchParam->ValueArray.push_back(1024);
			solutionConfig->KernelSearchSpace.AddOneParam(searchParam);
#endif
			// ----------------------------------------------------------------------
			// 添加solution
			//SolutionConfigList->push_back(solutionConfig);
		}

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 申请显存                                                            */
	/************************************************************************/
	E_ReturnState InitDev()
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
	E_ReturnState GetBackResult()
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
	void ReleaseDev()
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
	E_ReturnState GenerateSolution()
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

	int N_LCL_IN_MAPS;
	int N_IN_GROUPS;
	int N_LCL_IN_MAPS_ONCE;
	int	N_OUT_GROUPS;
	int CLOOP0;
	int CLOOP2;
	E_ReturnState generateParameters()
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

			k_out_group = (extProb->K + extSol->k_out_maps - 1) / extSol->k_out_maps;
		}
		else if (SolutionConfig->ConfigName == "SQC")
		{
			extSol->group_size = WAVE_SIZE;
			extSol->k_out_maps = 16;
			k_out_group = divCeil(extProb->K, extSol->k_out_maps);
			c_in_maps = extProb->C;
			c_in_group = divCeil(extProb->C, c_in_maps);

			pix_per_group = 64;
			align = divCeil(extProb->W * extProb->H * extProb->N, pix_per_group) * pix_per_group;
			c_in_maps_once = 8;
			loop = c_in_maps / c_in_maps_once;
		}
		else if (SolutionConfig->ConfigName == "PreFetch_Single")
		{
			extSol->group_size = WAVE_SIZE;
			extSol->k_out_maps = 16;
			k_out_group = divCeil(extProb->K, extSol->k_out_maps);
			c_in_maps = extProb->C;
			c_in_group = divCeil(extProb->C, c_in_maps);

			c_in_maps_once = 8;
			loop = c_in_maps / c_in_maps_once;
			pix_per_group = 64;
			align = divCeil(extProb->W * extProb->H * extProb->N, pix_per_group) * pix_per_group;
			
			// signal space
			sig_num_per_cu = c_in_maps / CACHE_LINE;
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
			k_out_group = divCeil(extProb->K, extSol->k_out_maps);
			c_in_maps = extProb->C;
			c_in_group = divCeil(extProb->C, c_in_maps);

			c_in_maps_once = 8;
			loop = c_in_maps / c_in_maps_once;
			pix_per_group = 64;
			align = divCeil(extProb->W * extProb->H * extProb->N, pix_per_group);

			// signal mem
			size_sig = loop / 2 * CU_NUM;
			DevMalloc((void**)&(d_signal.ptr), size_sig * sizeof(uint));
			d_signal.size = sizeof(cl_mem);	d_signal.isVal = false;
			SolutionConfig->KernelArgus->push_back(d_signal);
			h_signal = (int*)HstMalloc(size_sig * sizeof(int));
			// prefetch kernel
			preKernel = new RuntimeCtrlOcl();
			extSol->preArgus = new std::list<T_KernelArgu>;
			extSol->preArgus->push_back(d_in);
			extSol->preArgus->push_back(d_wei);
			extSol->preArgus->push_back(d_out);
			extSol->preArgus->push_back(d_signal);
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

				if (param->Name == "k_out_maps")
				{
					extSol->k_out_maps = param->CurrValue;
				}
				if (param->Name == "group_size")
				{
					extSol->group_size = param->CurrValue;
				}
			}

			if (extSol->k_out_maps == 0)
			{
				extSol->k_out_maps = 16;
			}
			if (extSol->group_size == 0)
			{
				extSol->group_size = 64;
			}

			printf("----------------------------------------------------------------------\n");
			printf("Kernel Param:\n");
			printf("	k_out_maps=[%d]\n", extSol->k_out_maps);
			printf("	group_size=[%d]\n", extSol->group_size);
			printf("----------------------------------------------------------------------\n");

			k_out_group = divCeil(extProb->K, extSol->k_out_maps);
			c_in_maps = extProb->C;
			c_in_group = divCeil(extProb->C, c_in_maps);

			c_in_maps_once = 8;
			loop = divCeil(c_in_maps, c_in_maps_once);
			pix_per_group = 64;
			align = divCeil(extProb->W * extProb->H * extProb->N, pix_per_group) * pix_per_group;
		}

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState generateCompilerOption()
	{
		T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

		SolutionConfig->extCompilerOpt = "";
		if (SolutionConfig->ConfigName == "MIOpenOcl")
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
				std::string(" -Wa,-defsym,GRP_NUM_SE=") + std::to_string(grpNumPerSe) +
				std::string(" -Wa,-defsym,GRP_NUM_CU_MIN=") + std::to_string(grpNumPerCUMin) +
				std::string(" -Wa,-defsym,MAX_GRP_CU_NUM=") + std::to_string(maxGrpCUNum) +
				std::string(" -Wa,-defsym,PIX_GRP_NUM=") + std::to_string(in_pix_groups) +
				std::string(" -Wa,-defsym,SIG_NUM_PER_CU=") + std::to_string(sig_num_per_cu) +
				std::string(" -Wa,-defsym,SIG_NUM_PER_CU_LOG2=") + std::to_string(log2(sig_num_per_cu));
		}
		return E_ReturnState::SUCCESS;
	}

	E_ReturnState generateWorkLoad()
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
			SolutionConfig->l_wk1 = 2;
			SolutionConfig->l_wk2 = 1;
			SolutionConfig->g_wk0 = align * c_in_group * k_out_group;
			SolutionConfig->g_wk1 = 2;
			SolutionConfig->g_wk2 = 1;

			//SolutionConfig->l_wk0 = 1;
			//SolutionConfig->g_wk0 = 64;
		}
		else if (SolutionConfig->ConfigName == "PreFetch_Single")
		{
			SolutionConfig->l_wk0 = extSol->group_size;
			SolutionConfig->l_wk1 = 1;
			SolutionConfig->l_wk2 = 1;
			SolutionConfig->g_wk0 = align * c_in_group * k_out_group;
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
			SolutionConfig->g_wk0 = align * c_in_group * k_out_group;
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
			SolutionConfig->g_wk0 = align * c_in_group * k_out_group;
			SolutionConfig->g_wk1 = 1;
			SolutionConfig->g_wk2 = 1;
			
			//SolutionConfig->l_wk0 = 64;
			//SolutionConfig->g_wk0 = 64*64;
		}

		SolutionConfig->b_wk0 = SolutionConfig->g_wk0 / SolutionConfig->l_wk0;
		SolutionConfig->b_wk1 = SolutionConfig->g_wk1 / SolutionConfig->l_wk1;
		SolutionConfig->b_wk2 = SolutionConfig->g_wk2 / SolutionConfig->l_wk2;

		if (SolutionConfig->b_wk0 == 0)
			return E_ReturnState::FAIL;

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState generateSource()
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
			//SolutionConfig->KernelFile = "ConvFwd1x1_Jasm.s";
			//SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_NewOrg.s";
			//SolutionConfig->KernelFile = "ConvFwd1x1_Jasm_256thread.s";
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
	void autoGenKernel()
	{		
		KernelWriterBase * kw = new KernelWriterConv1x1(ProblemConfig,SolutionConfig);		
		kw->GenKernelString();
#if EnSaveSource
		kw->SaveKernelStr2File();
#endif
	}

	/************************************************************************/
	/* 两个kernel										                    */
	/************************************************************************/
	E_ReturnState SetupSolution0()
	{
		printf("setup solution.\n");

		setupPrefetch();
		setupCalcu();

		// warm up
		LaunchSolution(true);

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState setupPrefetch()
	{
		printf("setup pre-fetch solution.\n");

		preKernel->KernelName = "ConvFwd1x1_Prefetch";
		preKernel->KernelFile = "ConvFwd1x1_Jasm_Prefetch_Mult_Fetch.s";
		preKernel->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;

		dim3 l_wk = dim3(1, 1, 1);
		dim3 g_wk = dim3(64, 1, 1);
		dim3 b_wk = dim3(64, 1, 1);
		preKernel->SetBlockSize(l_wk);
		preKernel->SetGridSize(b_wk);

		// build source file
		preKernel->GetFilesName(preKernel->KernelFile);
		preKernel->KernelString = SolutionConfig->KernelString;
		preKernel->CreatSolution();

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState setupCalcu()
	{
		printf("setup calculation solution.\n");

		runtime->KernelName = SolutionConfig->KernelName;
		runtime->KernelSrcType = SolutionConfig->KernelSrcType;
		runtime->extCompilerOpt = SolutionConfig->extCompilerOpt;
		SolutionConfig->b_wk0 = SolutionConfig->g_wk0 / SolutionConfig->l_wk0;
		SolutionConfig->b_wk1 = SolutionConfig->g_wk1 / SolutionConfig->l_wk1;
		SolutionConfig->b_wk2 = SolutionConfig->g_wk2 / SolutionConfig->l_wk2;
		runtime->SetBlockSize(dim3(SolutionConfig->l_wk0, SolutionConfig->l_wk1, SolutionConfig->l_wk2));
		runtime->SetGridSize(dim3(SolutionConfig->b_wk0, SolutionConfig->b_wk1, SolutionConfig->b_wk2));

		printf("l_wk=(%d, %d, %d)\n", SolutionConfig->l_wk0, SolutionConfig->l_wk1, SolutionConfig->l_wk2);
		printf("b_wk=(%d, %d, %d)\n", SolutionConfig->b_wk0, SolutionConfig->b_wk1, SolutionConfig->b_wk2);
		printf("g_wk=(%d, %d, %d)\n", SolutionConfig->g_wk0, SolutionConfig->g_wk1, SolutionConfig->g_wk2);
		std::cout << "compile options:\n" << SolutionConfig->extCompilerOpt << std::endl;

		// build source file
		runtime->GetFilesName(SolutionConfig->KernelFile);
		runtime->KernelString = SolutionConfig->KernelString;
		runtime->CreatSolution();

		ElapsedTimes.clear();
	}


	E_ReturnState LaunchSolution0(bool isWarmup)
	{
		printf("set argue.\n");
		T_ExtConvFwd1x1SolutionConfig * ext = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;
		UnixTimer tmr;

		if (isWarmup)
		{
			setArgusPrefetch();
			setArgusCalcu();
			preKernel->LanchKernel2();
			runtime->LanchKernel2();
		}
		else
		{
			for (int i = 0; i < RepeatTime; i++)
			{
				setArgusPrefetch();
				setArgusCalcu();

				printf("launch solution.\n");
				preKernel->LanchKernel2();
				runtime->LanchKernel2();
			}
		}
		return E_ReturnState::SUCCESS;
	}

	E_ReturnState setArgusPrefetch()
	{
		printf("set pre-fetch argue.\n");
		std::list<T_KernelArgu>::iterator args;
		T_ExtConvFwd1x1SolutionConfig * extSolu = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

		int i = 0;
		for (args = extSolu->preArgus->begin(); args != extSolu->preArgus->end(); args++)
		{
			if ((*args).isVal == true)
			{
				if ((*args).ptr == NULL)
				{
					DevCheckFunc(clSetKernelArg(preKernel->kernel, i, sizeof(cl_mem), (void*)NULL));
				}
				else
				{
					DevCheckFunc(clSetKernelArg(preKernel->kernel, i, (*args).size, (*args).ptr));
				}
			}
			else
			{
				DevCheckFunc(clSetKernelArg(preKernel->kernel, i, (*args).size, &(*args).ptr));
			}
			i++;
		}

		return E_ReturnState::SUCCESS;
	}

	E_ReturnState setArgusCalcu()
	{
		printf("set pooling argue.\n");
		std::list<T_KernelArgu>::iterator args;

		int i = 0;
		for (args = SolutionConfig->KernelArgus->begin(); args != SolutionConfig->KernelArgus->end(); args++)
		{
			if ((*args).isVal == true)
			{
				if ((*args).ptr == NULL)
				{
					DevCheckFunc(clSetKernelArg(runtime->kernel, i, sizeof(cl_mem), (void*)NULL));
				}
				else
				{
					DevCheckFunc(clSetKernelArg(runtime->kernel, i, (*args).size, (*args).ptr));
				}
			}
			else
			{
				DevCheckFunc(clSetKernelArg(runtime->kernel, i, (*args).size, &(*args).ptr));
			}
			i++;
		}

		for (int i = 0; i < RepeatTime; i++)
		{
			runtime->LanchKernel();
			ElapsedTimes.push_back(runtime->ElapsedTime);
			usleep(1);
		}

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 记录性能和配置															*/
	/************************************************************************/
	void ReportProblemPerformence()
	{
		T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

		LOG("------------------------------------------------\n");
		LOG("ProbemConfig [WHCKN]=[%d,%d,%d,%d,%d]:\n", extProb->H, extProb->W, extProb->C, extProb->K, extProb->N);

		LOG("shortest time: %.3f (us).\t", ProblemBestTime * 1e6);
		LOG("best performence: %.1f%%.\n", ProblemBestPerformence * 100);

		while (true)
		{
			T_SearchParam * param;
			param = SolutionConfig->KernelSearchSpace.GetOneParam();
			if (param == NULL)
			{
				break;
			}

			LOG("%s = %d\n", param->Name.c_str(), param->BestValue);
		}
	}
	
	/************************************************************************/
	/* 测试下标计算															*/
	/************************************************************************/
	void simulateIndex()
	{
		T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

		int k_blk_size = extSol->k_out_maps;
		align = ((extProb->W * extProb->H * extProb->N + WAVE_SIZE - 1) / WAVE_SIZE) * WAVE_SIZE;
		int in_pix_groups = align / extSol->group_size;
		int k_out_groups = (extProb->K + k_blk_size - 1) / k_blk_size;

		printf("in_pix_groups = %d\n", in_pix_groups);
		printf("k_out_groups = %d\n", k_out_groups);

		int *testGrpId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
		int *testId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
		int *testPixBlkId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
		int *testWeiBlkId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
		int *testPosId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
		int *testBatchId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));
		int *testOutId = (int*)malloc(SolutionConfig->b_wk0 * sizeof(int));

		uint W = extProb->W;
		uint H = extProb->H;
		uint C = extProb->C;
		uint K = extProb->K;

		uint IN_CHANNEL_STRIDE = W * H;
		uint IN_BATCH_STRIDE = W * H * C;
		uint WEI_CHANNEL_STRIDE = C;
		uint OUT_CHANNEL_STRIDE = W * H;
		uint OUT_BATCH_STRIDE = W * H * K;

		uint GROUP_SIZE = 64;
		uint PIX_MAPS = 64;
		uint K_OUT_GROUPS = k_out_groups;

		uint CHUNK_NUMBER = SolutionConfig->b_wk0 / CU_NUM;
		uint CHUNK_LEFT = CHUNK_NUMBER * CU_NUM;
		uint Z_ROUND_NUM = SolutionConfig->b_wk0 / (CU_NUM * K_OUT_GROUPS);
		uint INBLOCK_LEFT = Z_ROUND_NUM * CU_NUM;

		int pix_z_size = 8;
		int wei_z_size = 4;
		int pix_z_group = in_pix_groups / pix_z_size;
		int wei_z_group = k_out_groups / wei_z_size;

		int groupNum = in_pix_groups * k_out_groups;
		int grpNumPerCUMax = (groupNum + CU_NUM - 1) / CU_NUM;
		int grpNumPerCUMin = groupNum / CU_NUM;
		int maxGrpCUNum = (groupNum - grpNumPerCUMin * CU_NUM) / SE_NUM;
		int minGrpCUNum = (CU_NUM - maxGrpCUNum * SE_NUM) / SE_NUM;
		int grpNumPerSe = groupNum / SE_NUM;

		for (int grp = 0; grp < SolutionConfig->b_wk0; grp++)
		{
			uint local_id0 = 0;
			uint group_id0 = grp;
			uint pixBlkId, weiBlkId;

			// =====================================================================
			// old organization
			//weiBlkId = group_id0 % K_OUT_GROUPS;
			//pixBlkId = group_id0 / K_OUT_GROUPS;

			// --------------------------------------------------------------------
			// tensile fix input 
			//uint z_round = group_id0 / (CU_NUM * k_out_groups);	// 第几轮Z格子
			//
			//pixBlkId = z_round * CU_NUM + group_id0 % CU_NUM;		// 即 grp_id0_faked
			//weiBlkId = group_id0 / CU_NUM % k_out_groups;		// 即 out_grp_block
			//
			//if (group_id0 >= CHUNK_LEFT)
			//{
			//	uint leftGrpId = 0;
			//	leftGrpId = group_id0 - CHUNK_LEFT;
			//	pixBlkId = leftGrpId / 4 + INBLOCK_LEFT;
			//	weiBlkId = leftGrpId % 4;
			//}

			// --------------------------------------------------------------------
			// tensile fix weight
			uint col_id = group_id0 / CU_NUM;
			uint se_id = group_id0 % SE_NUM;				// SE编号
			uint cu_id = (group_id0 % CU_NUM) / SE_NUM;		// 一个SE内的CU编号
			uint raw_id = CU_PER_SE * se_id + cu_id;

			int cu_id2 = cu_id / 2 * 2;
			uint sr_id = grpNumPerSe * se_id;	// 前继SE上所有wave
			sr_id += grpNumPerCUMin * cu_id2;		// 前继CU上所有wave
			sr_id += maxGrpCUNum;
			if (cu_id < maxGrpCUNum)
				sr_id -= (maxGrpCUNum - cu_id2);

			sr_id = col_id * 2 + cu_id % 2 + sr_id;
			pixBlkId = sr_id % in_pix_groups;
			weiBlkId = sr_id / in_pix_groups;

			// =====================================================================
			// same
			uint pos = (pixBlkId * GROUP_SIZE + local_id0 % GROUP_SIZE) % IN_CHANNEL_STRIDE;
			uint batch_id = (pixBlkId * GROUP_SIZE + local_id0 % GROUP_SIZE) / IN_CHANNEL_STRIDE;
			uint out_id = weiBlkId * k_blk_size;

			uint gbl_in_off = batch_id * IN_BATCH_STRIDE + pos;
			uint wei_off = out_id * WEI_CHANNEL_STRIDE;
			uint gbl_out_off = batch_id * OUT_BATCH_STRIDE + out_id * OUT_CHANNEL_STRIDE + pos;

			testId[group_id0] = 0;
			testGrpId[group_id0] = group_id0;
			testPixBlkId[group_id0] = pixBlkId;
			testWeiBlkId[group_id0] = weiBlkId;
			testPosId[group_id0] = pos;
			testBatchId[group_id0] = batch_id;
			testOutId[group_id0] = out_id;
		}

		//printIndex(testGrpId, "group id");
		//printIndex(testId, "test temp id");
		printIndex(testPixBlkId, "input block id");
		printIndex(testWeiBlkId, "weight block id");
		//printIndex(testBatchId, "batch id");
		//printIndex(testOutId, "out id");
		//printIndex(testPosId, "pos id");

		free(testGrpId);
		free(testId);
		free(testPixBlkId);
		free(testWeiBlkId);
		free(testPosId);
		free(testBatchId);
		free(testOutId);
	}
	
	int next2pow(int n)
	{
		int base = 1;
		for (int i = 0; i < 32; i++)
		{
			base = 1 << i;
			if (n <= base)
			{
				break;
			}
		}
		return base;
	}
	int log2(int value)
	{
		int log2 = 0;
		while (value > 1)
		{
			value = value / 2;
			log2++;
		}
		return log2;
	}
	int divCeil(int a, int b)
	{
		return ((a + b - 1) / b);
	}
};

#define		SingleProblem	(1)
#define		SkipHost		(1)
/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
class ConvFwd1x1Problem : public ProblemCtrlBase
{
public:
	ConvFwd1x1Problem(std::string name) :ProblemCtrlBase(name)
	{
		Solution = new ConvFwd1x1Solution();
	}
	
	/************************************************************************/
	/* 生成问题空间													        */
	/************************************************************************/
	E_ReturnState GenerateProblemConfigs()
	{
		T_ProblemConfig * probCfg;
		T_ExtConvFwd1x1ProblemConfig * exProbCfg;

		T_SearchParam * searchParam;

#if	SingleProblem	
		probCfg = new T_ProblemConfig("convolution 1x1");

		exProbCfg = new T_ExtConvFwd1x1ProblemConfig();
		exProbCfg->W = 28;		exProbCfg->H = 28;
		exProbCfg->C = 2048;		exProbCfg->K = 128;
		exProbCfg->N = 32;
		probCfg->extConfig = exProbCfg;

		ProblemConfigList->push_back(probCfg);
#else
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
#endif
	}
	
	/************************************************************************/
	/* 参数初始化                                                            */
	/************************************************************************/
	E_ReturnState InitHost()
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
			//exProbCfg->h_in[i] = 1;
			//exProbCfg->h_in[i] = (float)(i % 7) + 1.0f;
			exProbCfg->h_in[i] = (float)(rand() % 100 - 50);
			//exProbCfg->h_in[i] = (double)rand() * (1.0 / RAND_MAX);
		}
		for (int i = 0; i < exProbCfg->size_wei; i++)
		{
			//exProbCfg->h_wei[i] = 1;
			//exProbCfg->h_wei[i] = (float)(i % 13) + 1.0f;
			exProbCfg->h_wei[i] = (float)(rand() % 100 - 50);
			//exProbCfg->h_in[i] = (double)rand() * (1.0 / RAND_MAX);
		}
		for (int i = 0; i < exProbCfg->size_out; i++)
		{
			exProbCfg->h_out[i] = 0;
		}

		//printTensor("input", h_in, exProbCfg->W, exProbCfg->H, exProbCfg->C, exProbCfg->N);
		//printTensor("weight", h_wei, exProbCfg->X, exProbCfg->Y, exProbCfg->C, exProbCfg->K);
		
		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* HOST端                                                               */
	/************************************************************************/
	E_ReturnState Host()
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
	E_ReturnState Verify()
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
	 
	/************************************************************************/
	/* 释放                                                                  */
	/************************************************************************/
	void ReleaseHost()
	{
		T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;

		HstFree(exProbCfg->h_in);
		HstFree(exProbCfg->h_wei);
		HstFree(exProbCfg->h_out);
		HstFree(exProbCfg->out_ref);
	}

	/************************************************************************/
	/* 打印矩阵			                                                    */
	/************************************************************************/
	void printTensor(char * name, float* mtx, int W, int H, int C, int N)
	{
		printf("%s tensor:\n", name);

		int nstride = W * H * C;
		int cstride = W * H;

		for (int n = 0; n < N; n++)
		{
			printf("n=%d===================\n", n);
			for (int c = 0; c < C; c++)
			{
				printf("c=%d\n", c);
				for (int h = 0; h < H; h++)
				{
					printf("| ");
					for (int w = 0; w < W; w++)
					{
						printf("%.1f", mtx[n*nstride + c * cstride + h * W + w]);
						printf(w == W - 1 ? " |" : ",\t");
					}
					printf("\n");
				}
			}
		}
	}
};


