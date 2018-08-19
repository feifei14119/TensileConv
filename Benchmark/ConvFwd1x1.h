#pragma once 

#include "BasicClass.h" 
#include "RuntimeControl.h"
#include "ProblemControl.h"

/************************************************************************/
/* 扩展参数                                                              */
/************************************************************************/
typedef struct ExtConvFwd1x1SolutionConfigTpye
{
	// K划分
	int k_out_maps;		// 每个CU计算多少个输出特征值K
	int k_out_group;	// 一个像素的所有输出特征值分给几个CU计算
	// C划分
	int c_in_maps;		// 每个CU计算多少个输入通道C
	int c_in_group;		// 一个像素的所有输入通道分给几个CU计算
	// W,H划分
	int in_pix_maps;	// 每个CU计算多少个输入像素

	// thread规划
	int group_size;		// 每个workgroup有多少个thread
	int out_pix_tile;	// 每个thread负责多少个输出像素
	int out_tile;		// 每个thread负责多少个输出数据(W*H*K)

	// 程序规划
	int c_in_maps_once;	// 每轮循环计算多少个输入通道C
	int loop;			// 循环次数
	int wei_pingpang_ins;	// 每轮循环中，进行weight读取时，pingpang操作的指令条数
	int wei_load_num;		// 每次weight的s_load的数据个数

	// 调整参数
	// k_out_maps:			16:[8,16,32]
	// int local_size:			64:[64,128,256]
	// c_in_maps_once:		 8:[8,16]
	// wei_pingpang_ins:	 1:[1,2,4,8]
	// en_in_pingpang:		 1:[0,1]
	// wait_cnt_in_fetch:	 4:[1,2,4,8,16]

	std::list<T_KernelArgu> * preArgus;
}T_ExtConvFwd1x1SolutionConfig;

typedef struct ExtConvFwd1x1ProblemConfigType
{
	std::string ConfigName;
	int N;
	int W, H;
	int C, K;
	int X, Y;
	int PadW, PadH;

	float* h_in, *h_wei, *h_out, *out_ref;
	int *h_signal;
	int size_in, size_wei, size_out, size_sig;
	int prefetch_loop;

	int batch_size;
	int channel_in, feature_out;
	int width_in, heigh_in;		// input size
	int width_wei, heigh_wei;	// weight size
	int width_out, heigh_out;	// output size
	int pad_w, pad_h;
	int stride_n_in;			// stride for differetn batch of input
	int stride_c_in;			// stride for differetn channel in the same batch of input
	int stride_k_wei;			// stride for differetn feature of weight
	int stride_c_wei;			// stride for differetn channel in the same feature of weight
	int stride_n_out;			// stride for differetn bathc of output
	int stride_k_out;			// stride for differetn feature in the same batch of output
}T_ExtConvFwd1x1ProblemConfig;

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
class ConvFwd1x1Solution : public SolutionCtrlBase
{
private:
	T_KernelArgu d_in, d_wei, d_out, d_lds, d_isFetch, d_relu, d_signal;
	T_KernelArgu d_in_off, d_wei_off, d_out_off;
	std::string asmKernelStr;
	int DoFetch, DoCalcu;
	cl_float relu;
	int *batch_id_buff;
	int *wei_id_buff;
	int *pos_id_buff;

	RuntimeCtrl * preKernel;
	
	size_t align;
	int N_IN_GROUPS;
	int N_LCL_IN_MAPS;
	int N_LCL_IN_MAPS_ONCE;
	int N_LCL_OUT_MAPS;
	int	N_OUT_GROUPS;
	int CLOOP0;
	int CLOOP2;
	int GroupNumber;
	int FIXED_WORKGROUP_SIZE = 64;

public:
	/************************************************************************/
	/* 申请显存                                                            */
	/************************************************************************/
	E_ReturnState InitDev()
	{
		T_ExtConvFwd1x1ProblemConfig * exCfg = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;

		preKernel = new RuntimeCtrlOcl();

		DevMalloc((void**)&(d_in.ptr), exCfg->size_in * sizeof(float));
		DevMalloc((void**)&(d_wei.ptr), exCfg->size_wei * sizeof(float));
		DevMalloc((void**)&(d_out.ptr), exCfg->size_out * sizeof(float));
		DevMalloc((void**)&(d_signal.ptr), exCfg->size_sig * sizeof(uint));

		d_in.size = sizeof(cl_mem);		d_in.isVal = false;
		d_wei.size = sizeof(cl_mem);	d_wei.isVal = false;
		d_out.size = sizeof(cl_mem);	d_out.isVal = false;
		d_signal.size = sizeof(cl_mem);	d_signal.isVal = false;

		solutionCfg->KernelArgus = new std::list<T_KernelArgu>;
		solutionCfg->KernelArgus->push_back(d_in);
		solutionCfg->KernelArgus->push_back(d_wei);
		solutionCfg->KernelArgus->push_back(d_out);
		solutionCfg->KernelArgus->push_back(d_signal);

		extSol->preArgus = new std::list<T_KernelArgu>;
		extSol->preArgus->push_back(d_in);
		extSol->preArgus->push_back(d_wei);
		extSol->preArgus->push_back(d_out);
		extSol->preArgus->push_back(d_signal);

		d_in.size = sizeof(cl_mem);		d_in.isVal = false;		
		d_wei.size = sizeof(cl_mem);	d_wei.isVal = false;	
		d_out.size = sizeof(cl_mem);	d_out.isVal = false;	
		d_signal.size = sizeof(cl_mem);	d_signal.isVal = false;			

		Copy2Dev((cl_mem)(d_in.ptr), exCfg->h_in, exCfg->size_in * sizeof(float));
		Copy2Dev((cl_mem)(d_wei.ptr), exCfg->h_wei, exCfg->size_wei * sizeof(float));

		return E_ReturnState::SUCCESS;
	}
	
	/************************************************************************/
	/* 返回结果                                                            */
	/************************************************************************/
	E_ReturnState GetBackResult()
	{
		T_ExtConvFwd1x1ProblemConfig * exCfg = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;
		Copy2Hst(exCfg->h_out, (cl_mem)(d_out.ptr), exCfg->size_out * sizeof(float));
		Copy2Hst(exCfg->h_signal, (cl_mem)(d_signal.ptr), exCfg->size_sig * sizeof(int));
	}

	/************************************************************************/
	/* 释放显存	                                                           */
	/************************************************************************/
	void ReleaseDev()
	{
		DevFree((cl_mem)(d_in.ptr));
		DevFree((cl_mem)(d_wei.ptr));
		DevFree((cl_mem)(d_out.ptr));
		DevFree((cl_mem)(d_signal.ptr));
		delete preKernel;
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

			solutionConfig = new T_SolutionConfig();
			solutionConfig->ConfigName = "MIOpenOcl";
			solutionConfig->RepeatTime = 4;
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

			solutionConfig = new T_SolutionConfig();
			solutionConfig->ConfigName = "MIOpenAsm";
			solutionConfig->RepeatTime = 4;
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

			solutionConfig = new T_SolutionConfig();
			solutionConfig->ConfigName = "SQC";
			solutionConfig->RepeatTime = 100;
			solutionConfig->extConfig = extSolutionConfig;

			// ----------------------------------------------------------------------
			// 添加solution
			//SolutionConfigList->push_back(solutionConfig);
		}

		// ======================================================================
		// solution config 4: sigle kernrl PreFetch
		// ======================================================================
		{
			extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();

			solutionConfig = new T_SolutionConfig();
			solutionConfig->ConfigName = "PreFetch1";
			solutionConfig->RepeatTime = 1;
			solutionConfig->extConfig = extSolutionConfig;

			// ----------------------------------------------------------------------
			// 添加solution
			SolutionConfigList->push_back(solutionConfig);
		}

		// ======================================================================
		// solution config 5: Mult-kernel perfetch
		// ======================================================================
		{
			extSolutionConfig = new T_ExtConvFwd1x1SolutionConfig();

			solutionConfig = new T_SolutionConfig();
			solutionConfig->ConfigName = "PreFetch2";
			solutionConfig->RepeatTime = 1;
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

			solutionConfig = new T_SolutionConfig();
			solutionConfig->ConfigName = "TensileConv";
			solutionConfig->RepeatTime = 100;
			solutionConfig->extConfig = extSolutionConfig;

			// ----------------------------------------------------------------------
			// 生成搜索空间
			searchParam = new T_SearchParam();
			searchParam->Name = "k_out_maps";
			searchParam->ValueArray.push_back(2);
			searchParam->ValueArray.push_back(4);
			searchParam->ValueArray.push_back(8);
			searchParam->ValueArray.push_back(16);
			searchParam->ValueArray.push_back(32);
			solutionConfig->KernelSearchSpace.AddOneParam(searchParam);
			//--------------------------------
			searchParam = new T_SearchParam();
			searchParam->Name = "group_size";
			//searchParam->ValueArray.push_back(64 * 1);
			//searchParam->ValueArray.push_back(64 * 2);
			//searchParam->ValueArray.push_back(64 * 3);
			//searchParam->ValueArray.push_back(64 * 4);
			//searchParam->ValueArray.push_back(64 * 5);
			//searchParam->ValueArray.push_back(64 * 6);
			//searchParam->ValueArray.push_back(64 * 7);
			//searchParam->ValueArray.push_back(64 * 8);
			//searchParam->ValueArray.push_back(64 * 9);
			//searchParam->ValueArray.push_back(64 * 10);
			//searchParam->ValueArray.push_back(64 * 11);
			//searchParam->ValueArray.push_back(64 * 12);
			//searchParam->ValueArray.push_back(64 * 13);
			//searchParam->ValueArray.push_back(64 * 14);
			//searchParam->ValueArray.push_back(64 * 15);
			//searchParam->ValueArray.push_back(64 * 16);
			searchParam->ValueArray.push_back(64);
			searchParam->ValueArray.push_back(128);
			searchParam->ValueArray.push_back(256);
			searchParam->ValueArray.push_back(512);
			//searchParam->ValueArray.push_back(1024);
			solutionConfig->KernelSearchSpace.AddOneParam(searchParam);
			// ----------------------------------------------------------------------
			// 添加solution
			//SolutionConfigList->push_back(solutionConfig);
		}

		return E_ReturnState::SUCCESS; 
	}

	/************************************************************************/
	/* 根据solution参数生成source, complier和worksize                        */
	/************************************************************************/
	E_ReturnState GenerateSolution()
	{		
		generateParameters();// 提取搜索参数		
		generateCompilerOption();// 生成编译选项		
		generateWorkLoad();// 生成worksize		
		generateSource();// 获取/生成代码

		simulateIndex();

		return E_ReturnState::SUCCESS;
	}

	void generateParameters()
	{
		T_ExtConvFwd1x1ProblemConfig * extProblem = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSolution = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;

		if (solutionCfg->ConfigName == "MIOpenOcl")
		{
			// chunk config
			// ---------------
			int n_out_pix_tiles = 16;
			N_LCL_OUT_MAPS = n_out_pix_tiles;
			N_LCL_OUT_MAPS = std::min(N_LCL_OUT_MAPS, extProblem->feature_out);
			while ((extProblem->feature_out % N_LCL_OUT_MAPS) != 0 && N_LCL_OUT_MAPS > 16)
			{
				N_LCL_OUT_MAPS /= 2;
			}
			n_out_pix_tiles = N_LCL_OUT_MAPS;

			// ---------------
			int n_in_data_tiles = 2048;
			N_LCL_IN_MAPS = n_in_data_tiles;
			N_LCL_IN_MAPS = std::min(N_LCL_IN_MAPS, extProblem->channel_in);
			if (N_LCL_IN_MAPS < extProblem->channel_in && N_LCL_IN_MAPS > 0 && (N_LCL_IN_MAPS % 8) == 0)
			{
				// Pass will do nothing
			}
			else
			{
				N_LCL_IN_MAPS = extProblem->channel_in;
			}
			n_in_data_tiles = N_LCL_IN_MAPS;

			N_IN_GROUPS = (extProblem->channel_in + N_LCL_IN_MAPS - 1) / N_LCL_IN_MAPS;
			N_LCL_IN_MAPS_ONCE = 8;

			CLOOP0 = N_LCL_IN_MAPS / N_LCL_IN_MAPS_ONCE;
			CLOOP2 = (extProblem->channel_in - N_LCL_IN_MAPS * (N_IN_GROUPS - 1)) / N_LCL_IN_MAPS_ONCE;

			align = ((extProblem->width_in * extProblem->heigh_in * extProblem->batch_size + FIXED_WORKGROUP_SIZE - 1) / FIXED_WORKGROUP_SIZE) * FIXED_WORKGROUP_SIZE;
			N_OUT_GROUPS = (extProblem->feature_out / N_LCL_OUT_MAPS);
			
			extSolution->k_out_maps = N_LCL_OUT_MAPS;
			extSolution->k_out_group = (extProblem->K + extSolution->k_out_maps - 1) / extSolution->k_out_maps;
		}
		else if ((solutionCfg->ConfigName == "SQC") || (solutionCfg->ConfigName == "TensileConv"))
		{
			solutionCfg->KernelSearchSpace.StartGetParam();
			while (true)
			{
				T_SearchParam * param;
				param = solutionCfg->KernelSearchSpace.GetOneParam();
				if (param == NULL)
				{
					break;
				}

				if (param->Name == "k_out_maps")
				{
					extSolution->k_out_maps = param->CurrValue;
				}
				if (param->Name == "group_size")
				{
					extSolution->group_size = param->CurrValue;
				}
			}

			if (extSolution->k_out_maps == 0)
			{
				extSolution->k_out_maps = 8;
			}
			if (extSolution->group_size == 0)
			{
				extSolution->group_size = 64;
			}

			printf("----------------------------------------------------------------------\n");
			printf("Kernel Param:\n");
			printf("	k_out_maps=[%d]\n", extSolution->k_out_maps);
			printf("	group_size=[%d]\n", extSolution->group_size);
			printf("----------------------------------------------------------------------\n");

			extSolution->k_out_group = (extProblem->K + extSolution->k_out_maps - 1) / extSolution->k_out_maps;
			extSolution->c_in_maps = extProblem->C;
			extSolution->c_in_group = (extProblem->C + extSolution->c_in_maps - 1) / extSolution->c_in_maps;

			extSolution->c_in_maps_once = 8;
			extSolution->out_pix_tile = 1;
			extSolution->out_tile = extSolution->out_pix_tile * extSolution->k_out_maps;
			extSolution->in_pix_maps = 64;
			extSolution->loop = extSolution->c_in_maps / extSolution->c_in_maps_once;
			align = ((extProblem->width_in * extProblem->heigh_in * extProblem->batch_size + FIXED_WORKGROUP_SIZE - 1) / FIXED_WORKGROUP_SIZE) * FIXED_WORKGROUP_SIZE;
		}		
		else if (solutionCfg->ConfigName == "PreFetch1")
		{
			extSolution->k_out_maps = 16;
			extSolution->group_size = 64;
			extSolution->k_out_group = (extProblem->K + extSolution->k_out_maps - 1) / extSolution->k_out_maps;
			extSolution->c_in_maps = extProblem->C;
			extSolution->c_in_group = (extProblem->C + extSolution->c_in_maps - 1) / extSolution->c_in_maps;

			extSolution->c_in_maps_once = 8;
			extSolution->out_pix_tile = 1;
			extSolution->out_tile = extSolution->out_pix_tile * extSolution->k_out_maps;
			extSolution->in_pix_maps = 64;
			extSolution->loop = extSolution->c_in_maps / extSolution->c_in_maps_once;
			align = ((extProblem->width_in * extProblem->heigh_in * extProblem->batch_size + FIXED_WORKGROUP_SIZE - 1) / FIXED_WORKGROUP_SIZE) * FIXED_WORKGROUP_SIZE;
		}
		else if (solutionCfg->ConfigName == "PreFetch2")
		{
			extSolution->k_out_maps = 16;
			extSolution->group_size = 256;
			extSolution->k_out_group = (extProblem->K + extSolution->k_out_maps - 1) / extSolution->k_out_maps;
			extSolution->c_in_maps = extProblem->C;
			extSolution->c_in_group = (extProblem->C + extSolution->c_in_maps - 1) / extSolution->c_in_maps;

			extSolution->c_in_maps_once = 8;
			extSolution->out_pix_tile = 1;
			extSolution->out_tile = extSolution->out_pix_tile * extSolution->k_out_maps;
			extSolution->in_pix_maps = 64;
			extSolution->loop = extSolution->c_in_maps / extSolution->c_in_maps_once;
			align = ((extProblem->width_in * extProblem->heigh_in * extProblem->batch_size + FIXED_WORKGROUP_SIZE - 1) / FIXED_WORKGROUP_SIZE) * FIXED_WORKGROUP_SIZE;
		}
	}

	void generateCompilerOption()
	{
		T_ExtConvFwd1x1ProblemConfig * extProblem = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSolution = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;

		solutionCfg->extCompilerOpt = "";
		if (solutionCfg->ConfigName == "MIOpenOcl")
		{
			solutionCfg->extCompilerOpt =
				std::string(" -DMLO_FILTER_STRIDE0=1") +
				std::string(" -DMLO_FILTER_STRIDE1=1") +
				std::string(" -DMLO_N_LCL_IN_MAPS_ONCE=") + std::to_string(N_LCL_IN_MAPS_ONCE) +
				std::string(" -DBATCHSIZE=") + std::to_string(extProblem->batch_size) +
				std::string(" -DH=") + std::to_string(extProblem->heigh_in) +
				std::string(" -DW=") + std::to_string(extProblem->width_in) +
				std::string(" -DC=") + std::to_string(extProblem->channel_in) +
				std::string(" -DK=") + std::to_string(extProblem->feature_out) +
				std::string(" -DMLO_N_LCL_IN_MAPS=") + std::to_string(N_LCL_IN_MAPS) +
				std::string(" -DMLO_N_INPUTS=") + std::to_string(extProblem->channel_in) +
				std::string(" -DMLO_N_OUTPUTS=") + std::to_string(extProblem->feature_out) +
				std::string(" -DH_out=") + std::to_string(extProblem->heigh_out) +
				std::string(" -DW_out=") + std::to_string(extProblem->width_out) +
				std::string(" -DMLO_N_IN_GROUPS=") + std::to_string(N_IN_GROUPS) +
				std::string(" -DMLO_CLOOP0=") + std::to_string(CLOOP0) +
				std::string(" -DMLO_CLOOP2=") + std::to_string(CLOOP2) +
				std::string(" -DMLO_N_LCL_OUT_MAPS=") + std::to_string(N_LCL_OUT_MAPS) +
				std::string(" -DMLO_CHEAT_SHADER_COMPILER=1") +
				std::string(" -DMLopen_RUNNING=1") +
				std::string(" -DMIOPEN_USE_FP32=1");
		}
		else
		{
			//solutionCfg->extCompilerOpt = 
			//	std::string(" -Wa, -defsym, $name=$val ");
		}
	}

	void generateWorkLoad()
	{
		T_ExtConvFwd1x1ProblemConfig * extProblem = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSolution = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;

		if (solutionCfg->ConfigName == "MIOpenOcl")
		{
			solutionCfg->l_wk0 = FIXED_WORKGROUP_SIZE;
			solutionCfg->l_wk1 = 1;
			solutionCfg->l_wk2 = 1;
			solutionCfg->g_wk0 = align * N_IN_GROUPS * N_OUT_GROUPS;
			solutionCfg->g_wk1 = 1;
			solutionCfg->g_wk2 = 1;

			//solutionCfg->l_wk0 = FIXED_WORKGROUP_SIZE*4;
		}
		else if (solutionCfg->ConfigName == "SQC")
		{
			solutionCfg->l_wk0 = extSolution->group_size;
			solutionCfg->l_wk1 = 1;
			solutionCfg->l_wk2 = 1;
			solutionCfg->g_wk0 = align * extSolution->c_in_group * extSolution->k_out_group;
			solutionCfg->g_wk1 = 1;
			solutionCfg->g_wk2 = 1;
		}
		else if ((solutionCfg->ConfigName == "PreFetch1") || (solutionCfg->ConfigName == "PreFetch2"))
		{
			solutionCfg->l_wk0 = extSolution->group_size;
			solutionCfg->l_wk1 = 1;
			solutionCfg->l_wk2 = 1;
			solutionCfg->g_wk0 = align * extSolution->c_in_group * extSolution->k_out_group;
			solutionCfg->g_wk1 = 1;
			solutionCfg->g_wk2 = 1;

			//solutionCfg->l_wk0 = 1;
			solutionCfg->g_wk0 = 64 * 3 * solutionCfg->l_wk0;
			solutionCfg->g_wk0 += (64 * solutionCfg->l_wk0);
		}
		else if (solutionCfg->ConfigName == "TensileConv")
		{
			solutionCfg->l_wk0 = extSolution->group_size;
			solutionCfg->l_wk1 = 1;
			solutionCfg->l_wk2 = 1;
			solutionCfg->g_wk0 = align * extSolution->c_in_group * extSolution->k_out_group;
			solutionCfg->g_wk1 = 1;
			solutionCfg->g_wk2 = 1;
		}

		solutionCfg->b_wk0 = solutionCfg->g_wk0 / solutionCfg->l_wk0;
		solutionCfg->b_wk1 = solutionCfg->g_wk0 / solutionCfg->l_wk1;
		solutionCfg->b_wk2 = solutionCfg->g_wk0 / solutionCfg->l_wk2;
	}

	void generateSource()
	{
		solutionCfg->KernelName = "ConvFwd1x1";
		if (solutionCfg->ConfigName == "MIOpenOcl")
		{
			solutionCfg->KernelFile = "ConvFwd1x1_Jcl.cl";
			solutionCfg->KernelFile = "ConvFwd1x1_Jcl_NewOrg.cl";
			solutionCfg->KernelFile = "ConvFwd1x1_Jcl_256thread.cl";
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_OCL_FILE;
		}
		else if (solutionCfg->ConfigName == "SQC")
		{
			solutionCfg->KernelFile = "ConvFwd1x1_Jasm.s";
			solutionCfg->KernelFile = "ConvFwd1x1_Jasm_NewOrg.s";
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}
		else if (solutionCfg->ConfigName == "PreFetch1")
		{
			solutionCfg->KernelFile = "ConvFwd1x1_Jasm_Prefetch1.s";
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}
		else if (solutionCfg->ConfigName == "PreFetch2")
		{
			solutionCfg->KernelFile = "ConvFwd1x1_Jasm_Prefetch2.s";
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}
		else if (solutionCfg->ConfigName == "TensileConv")
		{
			solutionCfg->KernelFile = "ConvFwd1x1_Jasm_TensileConv.s";
			writeAsmKernel();
			saveKernelStr2File();
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}
	}

	/************************************************************************/
	/************************************************************************/
	void ReportProblemPerformence()
	{
		T_ExtConvFwd1x1ProblemConfig * extProblem = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSolution = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;

		printf("ProbemConfig [WHCKN]=[%d,%d,%d,%d,%d]:", extProblem->H, extProblem->W, extProblem->C, extProblem->K, extProblem->N);

		printf("shortest time: %.3f (us).\t", ProblemBestTime * 1e6);
		printf("best performence: %.1f%%.\n", ProblemBestPerformence * 100);
	}
	
	/************************************************************************/
	/* 测试下标计算															*/
	/************************************************************************/
	void simulateIndex()
	{
		T_ExtConvFwd1x1ProblemConfig * extProblem = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSolution = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;

		//if (solutionCfg->ConfigName == "MIOpenOcl")
		{
			// group size = 256
			/*{
				uint MLO_N_OUT_GROUPS = extSolution->k_out_group;
				uint MLO_IN_CHANNEL_STRIDE = extProblem->W * extProblem->H;
				uint MLO_N_LCL_OUT_MAPS = extSolution->k_out_maps;
				uint MLO_IN_BATCH_STRIDE = extProblem->W * extProblem->H * extProblem->C;
				uint MLO_WEI_CHANNEL_STRIDE = extProblem->C;
				uint MLO_OUT_BATCH_STRIDE = extProblem->W * extProblem->H * extProblem->K;
				uint MLO_OUT_CHANNEL_STRIDE = extProblem->W * extProblem->H;

				int *testGrpId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
				int *testPosId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
				int *testBatchId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
				int *testOutId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));

				for (int grp = 0; grp < solutionCfg->b_wk0; grp++)
				{
					uint local_id0 = 0;
					uint grp_id0 = grp;

					uint out_grp_block = (grp_id0 * 4 + local_id0 / FIXED_WORKGROUP_SIZE) % MLO_N_OUT_GROUPS;
					uint grp_id0_faked = (uint)((grp_id0 * 4 + local_id0 / FIXED_WORKGROUP_SIZE) / MLO_N_OUT_GROUPS);

					uint pos = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0 % FIXED_WORKGROUP_SIZE) % MLO_IN_CHANNEL_STRIDE;
					uint batch_id = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0 % FIXED_WORKGROUP_SIZE) / MLO_IN_CHANNEL_STRIDE;
					uint out_id = out_grp_block * MLO_N_LCL_OUT_MAPS;

					uint gbl_in_off = batch_id * MLO_IN_BATCH_STRIDE + pos;
					uint wei_off = out_id * MLO_WEI_CHANNEL_STRIDE;
					uint gbl_out_off = batch_id * MLO_OUT_BATCH_STRIDE + out_id * MLO_OUT_CHANNEL_STRIDE + pos;

					testGrpId[grp_id0] = grp_id0;

					testPosId[grp_id0] = pos;
					testBatchId[grp_id0] = batch_id;
					testOutId[grp_id0] = out_id;
				}

				printIndex(testGrpId, "group id");
				printIndex(testBatchId, "batch id");
				printIndex(testOutId, "out id");
				printIndex(testPosId, "pos id");

			}*/

			// group size = 64
			{
				uint MLO_IN_HEIGHT = extProblem->H;
				uint MLO_IN_WIDTH = extProblem->W;
				uint MLO_N_INPUTS = extProblem->C;
				uint MLO_N_LCL_IN_MAPS = extProblem->C;
				uint MLO_N_IN_GROUPS = ((MLO_N_INPUTS + MLO_N_LCL_IN_MAPS - 1) / MLO_N_LCL_IN_MAPS);
				uint MLO_N_OUT_GROUPS = extSolution->k_out_group;
				uint MLO_IN_CHANNEL_STRIDE = extProblem->W * extProblem->H;
				uint MLO_N_LCL_OUT_MAPS = extSolution->k_out_maps;
				uint MLO_IN_BATCH_STRIDE = extProblem->W * extProblem->H * extProblem->C;
				uint MLO_WEI_CHANNEL_STRIDE = extProblem->C;
				uint MLO_OUT_BATCH_STRIDE = extProblem->W * extProblem->H * extProblem->K;
				uint MLO_OUT_CHANNEL_STRIDE = extProblem->W * extProblem->H;

				uint MLO_ROUND_NUMBER = solutionCfg->b_wk0 / CU_NUM;
				uint MLO_ROUND_LEFT = MLO_ROUND_NUMBER * CU_NUM;
				uint MLO_Z_ROUND_NUM = solutionCfg->b_wk0 / (CU_NUM * MLO_N_OUT_GROUPS);
				uint MLO_INBLOCK_LEFT = MLO_Z_ROUND_NUM * CU_NUM;

				int *testGrpId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));

				int *testInBlkId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
				int *testWeiBlkId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));

				int *testPosId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
				int *testBatchId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
				int *testOutId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));

				for (int grp = 0; grp < solutionCfg->b_wk0; grp++)
				{
					uint local_id0 = 0;
					uint grp_id0 = grp;

					//// old organization
					//uint out_grp_block = grp_id0 % MLO_N_OUT_GROUPS;
					//uint grp_id0_faked = (uint)(grp_id0 / MLO_N_OUT_GROUPS) / MLO_N_IN_GROUPS;
					//
					//uint pos = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0 % FIXED_WORKGROUP_SIZE) % MLO_IN_CHANNEL_STRIDE;
					//uint batch_id = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0 % FIXED_WORKGROUP_SIZE) / MLO_IN_CHANNEL_STRIDE;
					//uint out_id = out_grp_block * MLO_N_LCL_OUT_MAPS;
					//
					//uint gbl_in_off = batch_id * MLO_IN_BATCH_STRIDE + pos;
					//uint wei_off = out_id * MLO_WEI_CHANNEL_STRIDE;
					//uint gbl_out_off = batch_id * MLO_OUT_BATCH_STRIDE + out_id * MLO_OUT_CHANNEL_STRIDE + pos;
					
					// new organization
					uint inBlkId, weiBlkId;
					uint z_round = grp_id0 / (CU_NUM * MLO_N_OUT_GROUPS);	// 第几轮Z格子
					
					inBlkId = z_round * CU_NUM + grp_id0 % CU_NUM;		// 即 grp_id0_faked
					weiBlkId = grp_id0 / CU_NUM % MLO_N_OUT_GROUPS;		// 即 out_grp_block
					
					if (grp_id0 >= MLO_ROUND_LEFT)
					{
						uint leftGrpId = 0;
						leftGrpId = grp_id0 - MLO_ROUND_LEFT;
						inBlkId = leftGrpId / 4 + MLO_INBLOCK_LEFT;
						weiBlkId = leftGrpId % 4;
					}
					
					uint out_grp_block = weiBlkId;
					uint grp_id0_faked = inBlkId;
					
					uint pos = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0 % FIXED_WORKGROUP_SIZE) % MLO_IN_CHANNEL_STRIDE;
					uint batch_id = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0 % FIXED_WORKGROUP_SIZE) / MLO_IN_CHANNEL_STRIDE;
					uint out_id = out_grp_block * MLO_N_LCL_OUT_MAPS;

					testGrpId[grp_id0] = grp_id0;

					testInBlkId[grp_id0] = grp_id0_faked;
					testWeiBlkId[grp_id0] = out_grp_block;
					
					testPosId[grp_id0] = pos;
					testBatchId[grp_id0] = batch_id;
					testOutId[grp_id0] = out_id;
				}

				printIndex(testGrpId, "group id");

				printIndex(testInBlkId, "input block id");
				printIndex(testWeiBlkId, "weight block id");

				//printIndex(testBatchId, "batch id");
				//printIndex(testOutId, "out id");
				//printIndex(testPosId, "pos id");
			}
		}
	} 


	/************************************************************************/
	/* 两个kernel										                    */
	/************************************************************************/
	E_ReturnState SetupSolution0()
	{
		printf("setup solution.\n");
	
//		setupPrefetch();
//		setupCalcu();
//
//		// warm up
//		LaunchSolution();
	
		return E_ReturnState::SUCCESS;
	}

	E_ReturnState setupPrefetch()
	{
		printf("setup pre-fetch solution.\n");

		preKernel->KernelName = "ConvFwd1x1_Jasm_Prefetch";
		preKernel->KernelFile = "ConvFwd1x1_Jasm_Prefetch0.s";
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

	E_ReturnState setupCalcu()
	{
		printf("setup calculation solution.\n");

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
	

	E_ReturnState LaunchSolution0()
	{
		printf("set argue.\n");
		T_ExtConvFwd1x1SolutionConfig * ext = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;
		UnixTimer tmr;
	
		solutionCfg->RepeatTime = solutionCfg->RepeatTime == 0 ? 1 : solutionCfg->RepeatTime;
		for (int i = 0; i < solutionCfg->RepeatTime; i++)
		{	
//			setArgusPrefetch();
//			setArgusCalcu();
//	
//	
//			printf("launch solution.\n");
//			preKernel->LanchKernel2();
//			runtime->LanchKernel2();	
		}
		return E_ReturnState::SUCCESS;
	}

	E_ReturnState setArgusPrefetch()
	{
		printf("set pre-fetch argue.\n");
		std::list<T_KernelArgu>::iterator args;
		T_ExtConvFwd1x1SolutionConfig * extSolu = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;

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
		for (args = solutionCfg->KernelArgus->begin(); args != solutionCfg->KernelArgus->end(); args++)
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

		solutionCfg->RepeatTime = solutionCfg->RepeatTime == 0 ? 1 : solutionCfg->RepeatTime;
		for (int i = 0; i < solutionCfg->RepeatTime; i++)
		{
			runtime->LanchKernel();
			solutionCfg->ElapsedTimes.push_back(runtime->ElapsedTime);
			usleep(1);
		}

		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* 自动生成kernel								                        */
	/************************************************************************/
	void wirteCommom1(std::string common)
	{
		asmKernelStr.append("/************************************************************************************/\n");
		asmKernelStr.append("/* " + common + " */\n");
		asmKernelStr.append("/************************************************************************************/\n");
	}

	void wirteCommom2(std::string common)
	{
		asmKernelStr.append("// ==================================================================================\n");
		asmKernelStr.append("// " + common + " \n");
		asmKernelStr.append("// ==================================================================================\n");
	}

	void wirteCommom3(std::string common)
	{
		asmKernelStr.append("// ----------------------------------------------------------------------------------\n");
		asmKernelStr.append("// " + common + " \n");
		asmKernelStr.append("// ----------------------------------------------------------------------------------\n");
	}

	void defineConst(std::string name, const char * fmt, int value)
	{
		char tmp[10];
		sprintf(tmp, fmt, value);
		asmKernelStr.append(".set " + name + ", " + std::string(tmp) + "\n");
	}

	int getLog2(int value)
	{
		int log2 = 0;
		while (value > 1)
		{
			value = value / 2;
			log2++;
		}
		return log2;
	}

	int getModMask(int value)
	{
		return value - 1;
	}

	void saveKernelStr2File()
	{
		std::string SrcFileName = "../../../Kernels/" + solutionCfg->KernelFile;
		std::ofstream fout(SrcFileName, std::ios::out);
		if (!fout.is_open())
		{
			FATAL("can't open save file");
		}
		fout.write(asmKernelStr.c_str(), asmKernelStr.length());
		fout.close();
	}
	

	

	void writeAsmKernel()
	{
		asmKernelStr.clear();
		writeSignature();
		writeConstantDefine();
		writeRegisterLayout();
		writeProgram();
		writeMetadate();
	}

	void writeSignature()
	{
		asmKernelStr.append(".hsa_code_object_version 2, 1\n");
		asmKernelStr.append(".hsa_code_object_isa 9, 0, 0, \"AMD\", \"AMDGPU\"\n");
		asmKernelStr.append("\n");
		asmKernelStr.append(".text\n");
		asmKernelStr.append(".globl " + solutionCfg->KernelName + "\n");
		asmKernelStr.append(".p2align 8\n");
		asmKernelStr.append(".type " + solutionCfg->KernelName + ",@function\n");
		asmKernelStr.append(".amdgpu_hsa_kernel " + solutionCfg->KernelName + "\n");
		asmKernelStr.append("\n");
		asmKernelStr.append(".include \"gpr_alloc.inc\"\n");
		asmKernelStr.append(".include \"common.inc\"\n");
		asmKernelStr.append(".include \"inst_wrappers.inc\"\n");
		asmKernelStr.append("\n");
	}
	
	void writeConstantDefine()
	{
		T_ExtConvFwd1x1ProblemConfig * extProblem = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSolution = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;

		wirteCommom1("Constant Define");
		defineConst("N", "%d", extProblem->N);
		defineConst("C", "%d", extProblem->C);
		defineConst("H", "%d", extProblem->H);
		defineConst("W", "%d", extProblem->W);
		defineConst("K", "%d", extProblem->K);
		asmKernelStr.append("\n");
		defineConst("MLO_IN_CHANNEL_STRIDE", "%d", extProblem->stride_c_in);
		defineConst("MLO_IN_BATCH_STRIDE", "%d", extProblem->stride_n_in);
		defineConst("MLO_WEI_CHANNEL_STRIDE", "%d", extProblem->stride_k_wei);
		defineConst("MLO_OUT_CHANNEL_STRIDE", "%d", extProblem->stride_k_out);
		defineConst("MLO_OUT_BATCH_STRIDE", "%d", extProblem->stride_n_out);
		asmKernelStr.append("\n");
		defineConst("MLO_N_LCL_OUT_MAPS", "%d", extSolution->k_out_maps);
		defineConst("MLO_N_LCL_OUT_MAPS_LOG2", "%d", getLog2(extSolution->k_out_maps));
		defineConst("MLO_N_LCL_OUT_MAPS_DIV_MASK", "0x%X", getModMask(extSolution->k_out_maps));
		defineConst("MLO_N_OUT_GROUPS", "%d", extSolution->k_out_group);
		defineConst("MLO_N_OUT_GROUPS_LOG2", "%d", getLog2(extSolution->k_out_group));
		defineConst("MLO_N_OUT_GROUPS_DIV_MASK", "0x%X", getModMask(extSolution->k_out_group));

		defineConst("MLO_N_LCL_IN_MAPS", "%d", extSolution->c_in_maps);
		defineConst("MLO_N_IN_GROUPS", "%d", extSolution->c_in_group);
		defineConst("MLO_N_IN_GROUPS_LOG2", "%d", getLog2(extSolution->c_in_group));
		defineConst("MLO_N_IN_GROUPS_DIV_MASK", "0x%X", getModMask(extSolution->c_in_group));
		defineConst("MLO_N_LCL_IN_MAPS_ONCE", "%d", extSolution->c_in_maps_once);
		asmKernelStr.append("\n");
		defineConst("IN_PIXEL_PER_GROUP", "%d", 64);
		defineConst("IN_PIXEL_PER_GROUP_LOG2", "%d", getLog2(64));
		defineConst("IN_PIXEL_PER_GROUP_MOD_MASK", "%d", getModMask(64));
		defineConst("WORKGROUP_SIZE", "%d", extSolution->group_size);
		defineConst("WORKGROUP_SIZE_LOG2", "%d", getLog2(extSolution->group_size));
		defineConst("OUT_BLOCKS_PER_GROUP", "%d", extSolution->group_size / 64);
		defineConst("OUT_BLOCKS_PER_GROUP_LOG2", "%d", getLog2(extSolution->group_size / 64));

		defineConst("CLOOP0", "%d", extSolution->loop / 2);
		asmKernelStr.append("\n");
	}

	void writeRegisterLayout()
	{
		wirteCommom1("Register Layout"); 
		writeSgprInit();
		writeArgument();
		writeRegUsage();
	}

	void writeSgprInit()
	{
		wirteCommom2("SGPR Init");
		asmKernelStr.append("privateSeg = 0\n");
		asmKernelStr.append("kernarg = 4\n");
		asmKernelStr.append("gid_x0 = 6\n");
		asmKernelStr.append("gid_y0 = 7\n");
		asmKernelStr.append("gid_z0 = 8\n");
		asmKernelStr.append("\n");
	}

	void writeArgument()
	{
		int offset = 0;
		wirteCommom2("Input Argument");
		defineConst("in_ptr_off", "0x%X", offset); offset += 8;
		defineConst("wei_ptr_off", "0x%X", offset); offset += 8;
		defineConst("out_ptr_off", "0x%X", offset); offset += 8;
		asmKernelStr.append("\n");
	}

	void writeRegUsage()
	{
		T_ExtConvFwd1x1SolutionConfig * extSolution = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;

		wirteCommom2("Register Usage");
		asmKernelStr.append(".GPR_ALLOC_BEGIN\n");

		wirteCommom3("SGPR");
		asmKernelStr.append("\t.SGPR_ALLOC_FROM 9\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_tmp0\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_ptr_in, 2\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_ptr_wei, 2\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_ptr_out, 2\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_weia, " + 
			std::to_string(extSolution->c_in_maps_once) + ", " + 
			std::to_string(extSolution->c_in_maps_once) + "\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_weib, " +
			std::to_string(extSolution->c_in_maps_once) + ", " +
			std::to_string(extSolution->c_in_maps_once) + "\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_wei_prefetch, " +
			std::to_string(extSolution->k_out_maps) + ", " +
			std::to_string(extSolution->k_out_maps) + "\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_loop_cnt\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_prefatch\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_ptr_wei_save, 2\n");
		//asmKernelStr.append("\t.SGPR_ALLOC s_wei_desc, 4\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_tmp1, 2\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_tmp2, 2\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_tmp3\n");
		asmKernelStr.append("\t.SGPR_ALLOC s_tmp4\n");

		wirteCommom3("VGPR");
		asmKernelStr.append("\t.VGPR_ALLOC_FROM 0\n");
		asmKernelStr.append("\t.VGPR_ALLOC tid\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_addr_in, 2\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_addr_out, 2\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_addr_dbg, 2\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_tmp1\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_tmp2\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_tmp3\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_tmp4\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_tmp5\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_tmp6\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_data, " + std::to_string(extSolution->c_in_maps_once) + "\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_datb, " + std::to_string(extSolution->c_in_maps_once) + "\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_acc, " + std::to_string(extSolution->k_out_maps) + "\n");
		//asmKernelStr.append("\t.VGPR_ALLOC v_acc, 16\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_io_offset0, 2\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_io_offset1, 2\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_io_offset2, 2\n");
		asmKernelStr.append("\t.VGPR_ALLOC v_io_offset3, 2\n");
		//asmKernelStr.append("\t.VGPR_ALLOC v_io_offset4, 2\n");
		//asmKernelStr.append("\t.VGPR_ALLOC v_io_offset5, 2\n");
		//asmKernelStr.append("\t.VGPR_ALLOC v_io_offset6, 2\n");
		//asmKernelStr.append("\t.VGPR_ALLOC v_io_offset7, 2\n");
		//asmKernelStr.append("\t.VGPR_ALLOC v_test\n");

		wirteCommom3("LDS");
		asmKernelStr.append("\t.LDS_ALLOC_FROM 0\n");

		asmKernelStr.append(".GPR_ALLOC_END\n");
		asmKernelStr.append("\n");
	}

	void writeProgram()
	{
		wirteCommom1(solutionCfg->KernelName + "Program");
		asmKernelStr.append(solutionCfg->KernelName + ":\n");
		writeCodeObj();
		asmKernelStr.append("START_PROG:\n");
		//writeGetArgu();
		writeFunctions();
		writeCalcuIndex();
		asmKernelStr.append("CONV_PROG:\n"); 
		writeConvProgram();
		asmKernelStr.append("END_PROG:\n");
		asmKernelStr.append("\ts_endpgm\n");
		asmKernelStr.append("\n");
	}

	void writeCodeObj()
	{
		wirteCommom2("code object");
		asmKernelStr.append(".amd_kernel_code_t\n");
		asmKernelStr.append("\tenable_sgpr_private_segment_buffer = 1\n");
		asmKernelStr.append("\tenable_sgpr_kernarg_segment_ptr = 1\n");
		asmKernelStr.append("\tenable_sgpr_workgroup_id_x = 1\n");
		asmKernelStr.append("\tenable_sgpr_workgroup_id_y = 1\n");
		asmKernelStr.append("\tenable_sgpr_workgroup_id_z = 1\n");
		asmKernelStr.append("\tenable_vgpr_workitem_id = 0\n");
		asmKernelStr.append("\tis_ptr64 = 1\n");
		asmKernelStr.append("\tfloat_mode = 240\n");
		asmKernelStr.append("\tuser_sgpr_count = 6\n");
		asmKernelStr.append("\twavefront_sgpr_count = .AUTO_SGPR_COUNT\n");
		asmKernelStr.append("\tworkitem_vgpr_count = .AUTO_VGPR_COUNT\n");
		asmKernelStr.append("\tgranulated_workitem_vgpr_count = .AUTO_VGPR_GRANULATED_COUNT\n");
		asmKernelStr.append("\tgranulated_wavefront_sgpr_count = .AUTO_SGPR_GRANULATED_COUNT\n");
		asmKernelStr.append("\tkernarg_segment_byte_size = 56\n");
		asmKernelStr.append("\tworkgroup_group_segment_byte_size = 0\n");
		asmKernelStr.append(".end_amd_kernel_code_t\n");
		asmKernelStr.append("\n");
	}

	void writeMetadate()
	{
		wirteCommom1("Metadate");
		asmKernelStr.append(".amd_amdgpu_hsa_metadata\n");
		asmKernelStr.append("{ Version: [1, 0],\n");
		asmKernelStr.append("  Kernels :\n");
		asmKernelStr.append("    - { Name: " + solutionCfg->KernelName + ",\n");
		asmKernelStr.append("        SymbolName: " + solutionCfg->KernelName + ",\n");
		asmKernelStr.append("        Language: OpenCL C, LanguageVersion: [ 1, 2 ],\n");
		asmKernelStr.append("        Attrs: { ReqdWorkGroupSize: [ " + std::to_string(solutionCfg->l_wk0) + ", 1, 1 ] }\n");
		asmKernelStr.append("        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize : 0, PrivateSegmentFixedSize : 0, KernargSegmentAlign : 8, WavefrontSize : 64, MaxFlatWorkGroupSize : 1024 }\n");
		asmKernelStr.append("        Args:\n");
		asmKernelStr.append("        - { Name: d_in  , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global, IsConst : true }\n");
		asmKernelStr.append("        - { Name: d_wei , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global, IsConst : true }\n");
		asmKernelStr.append("        - { Name: d_out , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global  }\n");
		asmKernelStr.append("      }\n");
		asmKernelStr.append("}\n");
		asmKernelStr.append(".end_amd_amdgpu_hsa_metadata\n");
		asmKernelStr.append("\n");
	}

	void writeGetArgu()
	{
		wirteCommom2("Get Argument");
		asmKernelStr.append("\ts_load_dwordx2 		s[s_ptr_in :s_ptr_in +1], s[kernarg:kernarg+1], 0x0 + in_ptr_off\n");
		asmKernelStr.append("\ts_load_dwordx2 		s[s_ptr_wei:s_ptr_wei+1], s[kernarg:kernarg+1], 0x0 + wei_ptr_off\n");
		asmKernelStr.append("\ts_load_dwordx2 		s[s_ptr_out:s_ptr_out+1], s[kernarg:kernarg+1], 0x0 + out_ptr_off\n");
		//asmKernelStr.append("\ts_waitcnt 			lgkmcnt(0)\n");
		asmKernelStr.append("\n");
	}

	void writeFunctions()
	{
		T_ExtConvFwd1x1ProblemConfig * extProblem = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;
		T_ExtConvFwd1x1SolutionConfig * extSolution = (T_ExtConvFwd1x1SolutionConfig *)solutionCfg->extConfig;

		wirteCommom2("Sub Functions");
		writeWeightPrefetchFunc();

		if(extProblem->W > 28)		//if (extProblem->W*extProblem->W > pow(2,12)-1)
		{
			writeReadInputOnceFunc1(); 
		}
		else
		{
			writeReadInputOnceFunc2();
		}
		writeReadWeightOnceFunc();
		writeConvOneOutFeatureOnceFunc();
		//writeCalcuAllOutFeatureOnceFunc();
		writeCalcuAllOutFeaturePingFunc();
		writeCalcuAllOutFeaturePangFunc();
		writeCalcuAllOutFeatureLastFunc();
		writeSaveOutputFunc();
	}

	void writeWeightPrefetchFunc()
	{
		wirteCommom3("weight pre-fetch function");
		asmKernelStr.append(".macro m_weight_pre_fatch\n");
		asmKernelStr.append("    imm_offset = 0\n");
		asmKernelStr.append("    tmp = s_wei_prefetch\n");
		asmKernelStr.append("    .rept MLO_N_LCL_OUT_MAPS\n");
		asmKernelStr.append("        s_load_dword	s[tmp], s[s_ptr_wei:s_ptr_wei + 1], 0x0 + imm_offset\n");
		asmKernelStr.append("        imm_offset = imm_offset + MLO_WEI_CHANNEL_STRIDE * 4\n");
		asmKernelStr.append("        tmp = tmp + 1\n");
		asmKernelStr.append("    .endr\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}

	void writeReadInputOnceFunc1()
	{
		wirteCommom3("read input data once function");
		asmKernelStr.append(".macro m_load_input dest_base\n");
		asmKernelStr.append("    v_dat = \\dest_base\n");
		asmKernelStr.append("    .rept(MLO_N_LCL_IN_MAPS_ONCE)\n");
		asmKernelStr.append("        global_load_dword 	v[v_dat], v[v_addr_in:v_addr_in + 1], off\n");
		asmKernelStr.append("        v_add_co_u32 		v[v_addr_in], vcc, 0x0 + MLO_IN_CHANNEL_STRIDE * 4, v[v_addr_in]\n");
		asmKernelStr.append("        v_addc_co_u32 		v[v_addr_in + 1], vcc, 0, v[v_addr_in + 1], vcc\n");
		asmKernelStr.append("        v_dat = v_dat + 1\n");
		asmKernelStr.append("    .endr\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}

	void writeReadInputOnceFunc2()
	{
		wirteCommom3("read input data once function");
		asmKernelStr.append(".macro m_load_input	dest_base\n");
		asmKernelStr.append("    v_dat = \\dest_base\n");
		asmKernelStr.append("    voffset = v_io_offset0\n");
		asmKernelStr.append("    .rept(MLO_N_LCL_IN_MAPS_ONCE / 2)\n");
		asmKernelStr.append("        global_load_dword 	v[v_dat], v[voffset:voffset + 1], s[s_ptr_in:s_ptr_in + 1]	offset:0\n");
		asmKernelStr.append("        v_dat = v_dat + 1\n");
		asmKernelStr.append("        global_load_dword 	v[v_dat], v[voffset:voffset + 1], s[s_ptr_in:s_ptr_in + 1]	offset : 0x0 + MLO_IN_CHANNEL_STRIDE * 4\n");
		asmKernelStr.append("        v_dat = v_dat + 1\n");
		asmKernelStr.append("        voffset = voffset + 2\n");
		asmKernelStr.append("    .endr\n");
		asmKernelStr.append("    s_add_u32    s[s_ptr_in], 0x0 + MLO_IN_CHANNEL_STRIDE * 4 * 8, s[s_ptr_in]\n");
		asmKernelStr.append("    s_addc_u32   s[s_ptr_in + 1], 0, s[s_ptr_in + 1]\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}

	void writeReadWeightOnceFunc()
	{
		wirteCommom3("read weight once function");
		asmKernelStr.append(".macro m_load_weight  wei_base, imm_offset\n");
		asmKernelStr.append("    s_load_dwordx8    s[\\wei_base:\\wei_base + 7], s[s_ptr_wei:s_ptr_wei + 1], 0x0 + \\imm_offset\n");
		asmKernelStr.append("    \\imm_offset = \\imm_offset + MLO_WEI_CHANNEL_STRIDE * 4\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}
	
	void writeConvOneOutFeatureOnceFunc()
	{		
		wirteCommom3("conv one out feature once function");
		asmKernelStr.append(".macro m_conv_once input, weight, output\n");
		asmKernelStr.append("    v_dat = \\input\n");
		asmKernelStr.append("    s_wei = \\weight\n");
		asmKernelStr.append("    .rept MLO_N_LCL_IN_MAPS_ONCE\n");
		asmKernelStr.append("        v_fma_f32     v[\\output], v[v_dat], s[s_wei], v[\\output]\n");
		asmKernelStr.append("        v_dat = v_dat + 1\n");
		asmKernelStr.append("        s_wei = s_wei + 1\n");
		asmKernelStr.append("    .endr\n");
		asmKernelStr.append("    \\output = \\output + 1\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}

	void writeCalcuAllOutFeatureOnceFunc()
	{
		wirteCommom3("conv all out feature once function");
		asmKernelStr.append(".macro m_cacul_all_feature_once input\n");
		asmKernelStr.append("    v_sum = v_acc\n");
		asmKernelStr.append("    weight_offset = 0\n");

		asmKernelStr.append("    m_load_weight    s_weia, weight_offset\n");
		asmKernelStr.append("    s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("    m_load_weight    s_weib, weight_offset\n");
		asmKernelStr.append("    s_waitcnt        vmcnt(8)\n");
		asmKernelStr.append("    m_conv_once      \\input, s_weia, v_sum\n");

		asmKernelStr.append("    .rept MLO_N_LCL_OUT_MAPS / 2 - 1\n"); 
		asmKernelStr.append("        s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("        m_load_weight    s_weia, weight_offset\n");
		asmKernelStr.append("        m_conv_once      \\input, s_weib, v_sum\n");

		asmKernelStr.append("        s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("        m_load_weight    s_weib, weight_offset\n");
		asmKernelStr.append("        m_conv_once      \\input, s_weia, v_sum\n");
		asmKernelStr.append("    .endr\n");

		asmKernelStr.append("    s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("    m_conv_once      \\input, s_weib, v_sum\n");

		asmKernelStr.append("    s_add_u32        s[s_ptr_wei], s[s_ptr_wei], 0x0 + MLO_N_LCL_IN_MAPS_ONCE * 4\n");
		asmKernelStr.append("    s_addc_u32       s[s_ptr_wei + 1], s[s_ptr_wei + 1], 0x0\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}
	
	void writeCalcuAllOutFeaturePingFunc()
	{
		wirteCommom3("conv all out feature once function");
		asmKernelStr.append(".macro m_cacul_all_feature_ping input wei_offset\n");
		asmKernelStr.append("    v_sum = v_acc\n");
		asmKernelStr.append("    \\wei_offset = 0\n");

		asmKernelStr.append("    m_load_weight    s_weia, \\wei_offset\n");
		asmKernelStr.append("    s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("    m_load_weight    s_weib, \\wei_offset\n");
		asmKernelStr.append("    s_waitcnt        vmcnt(8)\n");
		asmKernelStr.append("    m_conv_once      \\input, s_weia, v_sum\n");

		asmKernelStr.append("    .rept MLO_N_LCL_OUT_MAPS / 2 - 1\n");
		asmKernelStr.append("        s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("        m_load_weight    s_weia, \\wei_offset\n");
		asmKernelStr.append("        m_conv_once      \\input, s_weib, v_sum\n");

		asmKernelStr.append("        s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("        m_load_weight    s_weib, \\wei_offset\n");
		asmKernelStr.append("        m_conv_once      \\input, s_weia, v_sum\n");
		asmKernelStr.append("    .endr\n");

		asmKernelStr.append("    s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("    m_conv_once      \\input, s_weib, v_sum\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}

	void writeCalcuAllOutFeaturePangFunc()
	{
		wirteCommom3("conv all out feature once function");
		asmKernelStr.append(".macro m_cacul_all_feature_pang input wei_offset\n");
		asmKernelStr.append("    v_sum = v_acc\n");
		asmKernelStr.append("    \\wei_offset = \\wei_offset + (MLO_N_LCL_IN_MAPS_ONCE - MLO_WEI_CHANNEL_STRIDE * MLO_N_LCL_OUT_MAPS) * 4\n");

		asmKernelStr.append("    m_load_weight    s_weia, \\wei_offset\n");
		asmKernelStr.append("    s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("    m_load_weight    s_weib, \\wei_offset\n");
		asmKernelStr.append("    s_waitcnt        vmcnt(8)\n");
		asmKernelStr.append("    m_conv_once      \\input, s_weia, v_sum\n");

		asmKernelStr.append("    .rept MLO_N_LCL_OUT_MAPS / 2 - 1\n");
		asmKernelStr.append("        s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("        m_load_weight    s_weia, \\wei_offset\n");
		asmKernelStr.append("        m_conv_once      \\input, s_weib, v_sum\n");

		asmKernelStr.append("        s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("        m_load_weight    s_weib, \\wei_offset\n");
		asmKernelStr.append("        m_conv_once      \\input, s_weia, v_sum\n");
		asmKernelStr.append("    .endr\n");

		asmKernelStr.append("    s_add_u32 			s[s_ptr_wei], s[s_ptr_wei], 0x0 + MLO_N_LCL_IN_MAPS_ONCE * 4 * 2\n");
		asmKernelStr.append("    s_addc_u32 		s[s_ptr_wei + 1], s[s_ptr_wei + 1], 0x0	\n");
		asmKernelStr.append("    s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("    m_weight_pre_fatch\n");
		asmKernelStr.append("    m_conv_once      \\input, s_weib, v_sum\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}

	void writeCalcuAllOutFeatureLastFunc()
	{
		wirteCommom3("conv all out feature once function");
		asmKernelStr.append(".macro m_cacul_all_feature_last input wei_offset\n");
		asmKernelStr.append("    v_sum = v_acc\n");
		asmKernelStr.append("    \\wei_offset = \\wei_offset + (MLO_N_LCL_IN_MAPS_ONCE - MLO_WEI_CHANNEL_STRIDE * MLO_N_LCL_OUT_MAPS) * 4\n");
		asmKernelStr.append("    vout_offset = v_io_offset0\n");

		asmKernelStr.append("    m_load_weight    s_weia, \\wei_offset\n");
		asmKernelStr.append("    s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("    m_load_weight    s_weib, \\wei_offset\n");
		asmKernelStr.append("    s_waitcnt        vmcnt(0)\n");
		asmKernelStr.append("    m_conv_once      \\input, s_weia, v_sum\n");

		asmKernelStr.append("    .rept MLO_N_LCL_OUT_MAPS / 2 - 1\n");
		asmKernelStr.append("        s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("        m_load_weight    s_weia, \\wei_offset\n");
		asmKernelStr.append("        m_conv_once      \\input, s_weib, v_sum\n");
		asmKernelStr.append("        vout_offset = vout_offset + 2\n");

		asmKernelStr.append("        s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("        m_load_weight    s_weib, \\wei_offset\n");
		asmKernelStr.append("        m_conv_once      \\input, s_weia, v_sum\n");
		asmKernelStr.append("    .endr\n");

		asmKernelStr.append("    s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("    m_conv_once      \\input, s_weib, v_sum\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}

	void writeCalcuAllOutFeatureLastnSaveFunc()
	{
		wirteCommom3("conv all out feature once and save output function");
		asmKernelStr.append(".macro m_cacul_all_feature_last_save input wei_offset\n");
		asmKernelStr.append("    v_sum = v_acc\n");
		asmKernelStr.append("    \\wei_offset = \\wei_offset + (MLO_N_LCL_IN_MAPS_ONCE - MLO_WEI_CHANNEL_STRIDE * MLO_N_LCL_OUT_MAPS) * 4\n");
		asmKernelStr.append("    vout_offset = v_io_offset0\n");

		asmKernelStr.append("    m_load_weight    s_weia, \\wei_offset\n");
		asmKernelStr.append("    s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("    m_load_weight    s_weib, \\wei_offset\n");
		asmKernelStr.append("    s_waitcnt        vmcnt(0)\n");
		asmKernelStr.append("    m_conv_once      \\input, s_weia, v_sum\n");
		asmKernelStr.append("    global_store_dword    	v[vout_offset:vout_offset + 1], v[v_sum - 1], s[s_ptr_out:s_ptr_out + 1]	offset:0x0\n");

		asmKernelStr.append("    .rept MLO_N_LCL_OUT_MAPS / 2 - 1\n");
		asmKernelStr.append("        s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("        m_load_weight    s_weia, \\wei_offset\n");
		asmKernelStr.append("        m_conv_once      \\input, s_weib, v_sum\n");
		asmKernelStr.append("		 global_store_dword    	v[vout_offset:vout_offset + 1], v[v_sum - 1], s[s_ptr_out:s_ptr_out + 1]	offset:0x0 + MLO_OUT_CHANNEL_STRIDE * 4\n");
		asmKernelStr.append("        vout_offset = vout_offset + 2\n");

		asmKernelStr.append("        s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("        m_load_weight    s_weib, \\wei_offset\n");
		asmKernelStr.append("        m_conv_once      \\input, s_weia, v_sum\n");
		asmKernelStr.append("		global_store_dword    	v[vout_offset:vout_offset + 1], v[v_sum - 1], s[s_ptr_out:s_ptr_out + 1]	offset:0x0\n");
		asmKernelStr.append("    .endr\n");

		asmKernelStr.append("    s_waitcnt        lgkmcnt(0)\n");
		asmKernelStr.append("    m_conv_once      \\input, s_weib, v_sum\n");
		asmKernelStr.append("	 global_store_dword    	v[vout_offset:vout_offset + 1], v[v_sum - 1], s[s_ptr_out:s_ptr_out + 1]	offset:0x0 + MLO_OUT_CHANNEL_STRIDE * 4\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}

	void writeSaveOutputFunc()
	{
		wirteCommom3("save output function");
		asmKernelStr.append(".macro m_save_output\n");
		asmKernelStr.append("    v_sum = v_acc\n");
		asmKernelStr.append("    .rept MLO_N_LCL_OUT_MAPS\n");
		asmKernelStr.append("        global_store_dword    v[v_addr_out:v_addr_out + 1], v[v_sum], off\n");
		asmKernelStr.append("        v_add_co_u32          v[v_addr_out], vcc, 0x0 + MLO_OUT_CHANNEL_STRIDE * 4, v[v_addr_out]\n");
		asmKernelStr.append("        v_addc_co_u32         v[v_addr_out + 1], vcc, 0, v[v_addr_out + 1], vcc\n");
		asmKernelStr.append("        v_sum = v_sum + 1\n");
		asmKernelStr.append("    .endr\n");
		asmKernelStr.append(".endm\n");
		asmKernelStr.append("\n");
	}

	void writeCalcuIndex()
	{
		wirteCommom2("Calculate Index");
		//writeCalcuInputIdx();
		//writeCalcuWeightIdx();
		//writeCalcuOutputIdx();
		asmKernelStr.append("    s_load_dwordx2 			s[s_ptr_in :s_ptr_in +1], s[kernarg:kernarg+1], 0x0 + in_ptr_off\n");
		asmKernelStr.append("    s_load_dwordx2 			s[s_ptr_wei:s_ptr_wei + 1], s[kernarg:kernarg + 1], 0x0 + wei_ptr_off\n");
		asmKernelStr.append("    s_load_dwordx2 			s[s_ptr_out:s_ptr_out + 1], s[kernarg:kernarg + 1], 0x0 + out_ptr_off\n");
		//asmKernelStr.append("    s_waitcnt 					lgkmcnt(0)\n");

		asmKernelStr.append("v_acc1 = v_data + 1\n");
		asmKernelStr.append("v_acc2 = v_data + 2\n");
		asmKernelStr.append("v_acc3 = v_data + 3\n");
		asmKernelStr.append("v_acc4 = v_data + 4\n");
		asmKernelStr.append("v_acc5 = v_data + 5\n");
		asmKernelStr.append("v_acc6 = v_data + 6\n");
		asmKernelStr.append("v_acc7 = v_data + 7\n");
		asmKernelStr.append("v_acc8 = v_data + 8\n");
		asmKernelStr.append("v_acc9 = v_data + 9\n");
		asmKernelStr.append("v_acc10 = v_data + 10\n");
		asmKernelStr.append("v_acc11 = v_data + 11\n");
		asmKernelStr.append("v_acc12 = v_data + 12\n");
		asmKernelStr.append("v_acc13 = v_data + 13\n");

		asmKernelStr.append("    s_lshl_b32			s[s_tmp1], s[gid_x0], 0x0 + OUT_BLOCKS_PER_GROUP_LOG2\n");
		asmKernelStr.append("    v_lshrrev_b32		v[v_acc13], 0x0 + IN_PIXEL_PER_GROUP_LOG2, v[tid]\n");
		asmKernelStr.append("    v_add_co_u32		v[v_acc13], vcc, v[v_acc13], s[s_tmp1]\n");
		asmKernelStr.append("    v_and_b32 			v[v_acc12], 0x0 + MLO_N_OUT_GROUPS_DIV_MASK, v[v_acc13]\n");	// acc12 = out_grp_block
		asmKernelStr.append("    v_lshrrev_b32 		v[v_acc13], 0x0 + MLO_N_OUT_GROUPS_LOG2, v[v_acc13]\n");		// acc13 = grp_id0_faked

		asmKernelStr.append("    v_lshlrev_b32		v[v_acc13], 0x0 + IN_PIXEL_PER_GROUP_LOG2, v[v_acc13]\n");
		asmKernelStr.append("    v_and_b32			v[v_acc11], 0x0 + IN_PIXEL_PER_GROUP_MOD_MASK, v[tid]\n");
		asmKernelStr.append("    v_add_co_u32		v[v_acc13], vcc, v[v_acc13], v[v_acc11]\n");
		asmKernelStr.append("    v_mov_b32			v[v_acc6], 0x0 + MLO_IN_CHANNEL_STRIDE\n");
		asmKernelStr.append("    mv_div_u32			v[v_acc13], v[v_acc6], v[v_acc7], v[v_acc8]\n");			// acc7 = batch_id; acc8 = pos
		asmKernelStr.append("    v_lshlrev_b32		v[v_acc6], 0x0 + MLO_N_LCL_OUT_MAPS_LOG2, v[v_acc12]\n");	// acc6 = out_id

		asmKernelStr.append("    v_mov_b32 			v[v_acc9], 0x0 + MLO_IN_BATCH_STRIDE\n");
		asmKernelStr.append("    v_mul_u32_u24		v[v_acc10], v[v_acc7], v[v_acc9]\n");
		//asmKernelStr.append("    v_mov_b32 			v[v_acc11], 0x0 + MLO_N_LCL_IN_MAPS * MLO_IN_CHANNEL_STRIDE\n");
		//asmKernelStr.append("    v_mul_u32_u24		v[v_acc12], v[v_acc4], s[v_acc11]\n");
		//asmKernelStr.append("    v_add3_u32			v[v_acc13], v[v_acc10], v[v_acc12], v[v_acc8]\n");
		asmKernelStr.append("    v_add_co_u32		v[v_acc13], vcc, v[v_acc10], v[v_acc8]\n");	// v_acc13 = gbl_in_off
		asmKernelStr.append("    v_lshlrev_b32 		v[v_io_offset0], 2, v[v_acc13]\n");			// v_io_offset0 = gbl_in_off(DWORD)
		asmKernelStr.append("    v_mov_b32			v[v_tmp1], 0x0 + MLO_IN_CHANNEL_STRIDE * 2 * 4\n");
		asmKernelStr.append("    v_add_co_u32		v[v_io_offset1], vcc, v[v_io_offset0], v[v_tmp1]\n");
		asmKernelStr.append("    v_add_co_u32		v[v_io_offset2], vcc, v[v_io_offset1], v[v_tmp1]\n");
		asmKernelStr.append("    v_add_co_u32		v[v_io_offset3], vcc, v[v_io_offset2], v[v_tmp1]\n");

		asmKernelStr.append("    s_waitcnt 			lgkmcnt(0)\n");//!!!!!!
		asmKernelStr.append("    v_lshlrev_b32 		v[v_acc13], 2, v[v_acc13]\n");
		asmKernelStr.append("    v_mov_b32			v[v_addr_in], s[s_ptr_in]\n");
		asmKernelStr.append("    v_mov_b32			v[v_addr_in + 1], s[s_ptr_in + 1]\n");
		asmKernelStr.append("    v_add_co_u32		v[v_addr_in], vcc, v[v_acc13], v[v_addr_in]\n");
		asmKernelStr.append("    v_addc_co_u32		v[v_addr_in + 1], vcc, 0, v[v_addr_in + 1], vcc\n");

		asmKernelStr.append("    v_mov_b32 			v[v_acc9], 0x0 + MLO_WEI_CHANNEL_STRIDE\n");
		asmKernelStr.append("    v_mul_u32_u24		v[v_acc9], v[v_acc6], v[v_acc9]\n");
		//asmKernelStr.append("    v_mov_b32 			v[v_acc10], 0x0 + MLO_N_LCL_IN_MAPS\n");
		//asmKernelStr.append("    v_mul_u32_u24		v[v_acc10], v[v_acc4], v[v_acc10]\n");
		//asmKernelStr.append("    v_add_u32			v[v_acc9], v[v_acc9], v[v_acc10]\n");
		asmKernelStr.append("    v_readfirstlane_b32	s[s_tmp0], v[v_acc9]\n");	// s_tmp0 = v_acc9 = wei_off
		asmKernelStr.append("    s_lshl_b32			s[s_tmp1], s[s_tmp0], 2\n");	// s_tmp1 = wei_off(DWORD)
		asmKernelStr.append("    s_waitcnt 			lgkmcnt(0)\n");
		asmKernelStr.append("    s_add_u32			s[s_ptr_wei], s[s_ptr_wei], s[s_tmp1]\n");
		asmKernelStr.append("    s_addc_u32			s[s_ptr_wei + 1], 0x0, s[s_ptr_wei + 1]\n");

		asmKernelStr.append("    v_mov_b32 			v[v_acc9], 0x0 + MLO_OUT_BATCH_STRIDE\n");
		asmKernelStr.append("    v_mul_u32_u24		v[v_acc9], v[v_acc7], v[v_acc9]\n");
		asmKernelStr.append("    v_mov_b32 			v[v_acc10], 0x0 + MLO_OUT_CHANNEL_STRIDE\n");
		asmKernelStr.append("    v_mul_u32_u24		v[v_acc11], v[v_acc6], v[v_acc10]\n");
		asmKernelStr.append("    v_add3_u32			v[v_acc12], v[v_acc9], v[v_acc11], v[v_acc8]\n");
		//asmKernelStr.append("    v_mov_b32			v[v_test], v[v_acc12]\n");
		// direct write
		asmKernelStr.append("    v_lshlrev_b32 		v[v_acc12], 2, v[v_acc12]\n");
		asmKernelStr.append("    v_mov_b32			v[v_addr_out], s[s_ptr_out]\n");
		asmKernelStr.append("    v_mov_b32			v[v_addr_out + 1], s[s_ptr_out + 1]\n");
		asmKernelStr.append("    v_add_co_u32		v[v_addr_out], vcc, v[v_acc12], v[v_addr_out]\n");
		asmKernelStr.append("    v_addc_co_u32		v[v_addr_out + 1], vcc, 0, v[v_addr_out + 1], vcc\n");
	}
	
	void writeCalcuInputIdx()
	{
		wirteCommom3("Input Index");
		asmKernelStr.append("    s_lshr_b32       s[s_tmp1], s[gid_x0], 0x0 + MLO_N_OUT_GROUPS_LOG2\n");
		asmKernelStr.append("    s_and_b32        s[s_tmp2], s[s_tmp1], 0x0 + MLO_N_IN_GROUPS_DIV_MASK\n");
		asmKernelStr.append("    s_lshr_b32       s[s_tmp1], s[s_tmp1], 0x0 + MLO_N_IN_GROUPS_LOG2\n");
		asmKernelStr.append("    v_lshl_add_u32   v[v_tmp3], s[s_tmp1], 0x0 + IN_PIXEL_PER_GROUP_LOG2, v[tid]\n");
		asmKernelStr.append("    v_mov_b32        v[v_tmp4], 0x0 + MLO_IN_CHANNEL_STRIDE\n");
		asmKernelStr.append("    mv_div_u32       v[v_tmp3], v[v_tmp4], v[v_tmp5], v[v_tmp6]\n");
		asmKernelStr.append("    v_mov_b32        v[v_tmp3], 0x0 + MLO_IN_BATCH_STRIDE\n");
		asmKernelStr.append("    v_mul_u32_u24    v[v_tmp3], v[v_tmp3], v[v_tmp5]\n");
		asmKernelStr.append("    v_mov_b32        v[v_tmp4], 0x0 + MLO_N_LCL_IN_MAPS * MLO_IN_CHANNEL_STRIDE\n");
		asmKernelStr.append("    v_mul_u32_u24    v[v_tmp4], v[v_tmp4], s[s_tmp2]\n");
		asmKernelStr.append("    v_add3_u32       v[v_tmp1], v[v_tmp3], v[v_tmp4], v[v_tmp6]\n");
		asmKernelStr.append("    v_lshlrev_b32    v[v_tmp1], 2, v[v_tmp1]\n");
		asmKernelStr.append("    v_mov_b32        v[v_addr_in], s[s_ptr_in]\n");
		asmKernelStr.append("    v_mov_b32        v[v_addr_in + 1], s[s_ptr_in + 1]\n");
		asmKernelStr.append("    v_add_co_u32     v[v_addr_in], vcc, v[v_tmp1], v[v_addr_in]\n");
		asmKernelStr.append("    v_addc_co_u32    v[v_addr_in + 1], vcc, 0, v[v_addr_in + 1], vcc\n");
		asmKernelStr.append("\n");
	}

	void writeCalcuWeightIdx()
	{
		wirteCommom3("Weight Index");
		asmKernelStr.append("    s_and_b32        s[s_tmp1], s[gid_x0], 0x0 + MLO_N_OUT_GROUPS_DIV_MASK\n");
		asmKernelStr.append("    s_lshr_b32       s[s_tmp1 + 1], s[gid_x0], 0x0 + MLO_N_OUT_GROUPS_LOG2\n");
		asmKernelStr.append("    s_and_b32        s[s_tmp2], s[s_tmp1 + 1], 0x0 + MLO_N_IN_GROUPS_DIV_MASK\n");
		asmKernelStr.append("    s_lshr_b32       s[s_tmp2 + 1], s[s_tmp1 + 1], 0x0 + MLO_N_IN_GROUPS_LOG2\n");
		asmKernelStr.append("    s_mul_i32        s[s_tmp1], s[s_tmp1], 0x0 + MLO_N_LCL_OUT_MAPS\n");
		asmKernelStr.append("    s_mul_i32        s[s_tmp1], s[s_tmp1], 0x0 + MLO_WEI_CHANNEL_STRIDE\n");
		asmKernelStr.append("    s_mul_i32        s[s_tmp2], s[s_tmp2], 0x0 + MLO_N_LCL_IN_MAPS\n");
		asmKernelStr.append("    s_add_u32        s[s_tmp1], s[s_tmp1], s[s_tmp2]\n");
		asmKernelStr.append("    s_lshl_b32       s[s_tmp1], s[s_tmp1], 2\n");
		asmKernelStr.append("    s_add_u32        s[s_ptr_wei], s[s_ptr_wei], s[s_tmp1]\n");
		asmKernelStr.append("    s_addc_u32       s[s_ptr_wei + 1], 0x0, s[s_ptr_wei + 1]\n");
		asmKernelStr.append("\n");
	}

	void writeCalcuOutputIdx()
	{
		wirteCommom3("Output Index");
		asmKernelStr.append("    s_and_b32        s[s_tmp1], s[gid_x0], 0x0 + MLO_N_OUT_GROUPS_DIV_MASK\n");
		asmKernelStr.append("    s_lshr_b32       s[s_tmp2], s[gid_x0], 0x0 + MLO_N_OUT_GROUPS_LOG2\n");
		asmKernelStr.append("    s_mov_b32        s[s_tmp3], 0x0 + MLO_N_IN_GROUPS\n");
		asmKernelStr.append("    mv_div_u32       s[s_tmp2], s[s_tmp3], v[v_tmp1], v[v_tmp2]\n");
		asmKernelStr.append("    v_lshl_add_u32   v[v_tmp3], v[v_tmp1], 0x0 + IN_PIXEL_PER_GROUP_LOG2, v[tid]\n");
		asmKernelStr.append("    v_mov_b32        v[v_tmp4], 0x0 + MLO_IN_CHANNEL_STRIDE\n");
		asmKernelStr.append("    mv_div_u32       v[v_tmp3], v[v_tmp4], v[v_tmp5], v[v_tmp6]\n");
		asmKernelStr.append("    v_mov_b32        v[v_tmp3], 0x0 + MLO_N_LCL_OUT_MAPS\n");
		asmKernelStr.append("    v_mul_u32_u24    v[v_tmp3], s[s_tmp1], v[v_tmp3]\n");
		asmKernelStr.append("    v_mov_b32        v[v_tmp1], 0x0 + MLO_OUT_BATCH_STRIDE\n");
		asmKernelStr.append("    v_mul_u32_u24    v[v_tmp1], v[v_tmp5], v[v_tmp1]\n");
		asmKernelStr.append("    v_mov_b32        v[v_tmp2], 0x0 + MLO_OUT_CHANNEL_STRIDE\n");
		asmKernelStr.append("    v_mul_u32_u24    v[v_tmp2], v[v_tmp3], v[v_tmp2]\n");
		asmKernelStr.append("    v_add3_u32       v[v_tmp1], v[v_tmp1], v[v_tmp2], v[v_tmp6]\n");
		asmKernelStr.append("    v_lshlrev_b32    v[v_tmp1], 2, v[v_tmp1]\n");
		asmKernelStr.append("    v_mov_b32        v[v_addr_out], s[s_ptr_out]\n");
		asmKernelStr.append("    v_mov_b32        v[v_addr_out + 1], s[s_ptr_out + 1]\n");
		asmKernelStr.append("    v_add_co_u32     v[v_addr_out], vcc, v[v_tmp1], v[v_addr_out]\n");
		asmKernelStr.append("    v_addc_co_u32    v[v_addr_out + 1], vcc, 0, v[v_addr_out + 1], vcc\n");
		asmKernelStr.append("\n");
	}
	
	void writeCalcuLineIdx()
	{
		wirteCommom3("Debug Liner Index");
		asmKernelStr.append("    s_lshl_b32		s[s_tmp1], s[gid_x0], .WAVE_SIZE_LOG2\n");
		asmKernelStr.append("    s_mov_b32		s[s_tmp1 + 1], 0\n");
		asmKernelStr.append("    s_lshl_b64		s[s_tmp1:s_tmp1 + 1], s[s_tmp1:s_tmp1 + 1], 2\n");
		asmKernelStr.append("    s_add_u32		s[s_ptr_out], s[s_ptr_out], s[s_tmp1]\n");
		asmKernelStr.append("    s_addc_u32 	s[s_ptr_out + 1], s[s_ptr_out + 1], s[s_tmp1 + 1]\n");
		asmKernelStr.append("    v_lshlrev_b32 	v[v_tmp1], 2, v[tid]\n");
		asmKernelStr.append("    v_mov_b32 		v[v_tmp2], s[s_ptr_out + 1]\n");
		asmKernelStr.append("    v_add_co_u32 	v[v_addr_dbg], vcc, s[s_ptr_out], v[v_tmp1]\n");
		asmKernelStr.append("    v_addc_co_u32 	v[v_addr_dbg + 1], vcc, 0, v[v_tmp2], vcc\n");
		asmKernelStr.append("\n");
	}

	void writeConvProgram()
	{
		wirteCommom2("Conv Program");
		writeLoopInit();
		writeMainLoop();
	}

	void writeLoopInit()
	{
		wirteCommom3("Loop Init");
		asmKernelStr.append("    v_sum = v_acc\n");
		asmKernelStr.append("    .rept MLO_N_LCL_OUT_MAPS\n");
		asmKernelStr.append("        v_mov_b32    v[v_sum], 0\n");
		asmKernelStr.append("        v_sum = v_sum + 1\n");
		asmKernelStr.append("    .endr\n");
		asmKernelStr.append("    s_mov_b32 					s[s_loop_cnt], CLOOP0 - 1\n");
		asmKernelStr.append("\n");
	}

	void writeMainLoop()
	{
		wirteCommom3("Main Loop");
		asmKernelStr.append("    m_weight_pre_fatch\n");
		asmKernelStr.append("    m_load_input               v_data\n");
		asmKernelStr.append("    weight_offset = 0\n");
		
		asmKernelStr.append("LOOP_CONV:\n");
		asmKernelStr.append("    m_load_input               v_datb\n");
		asmKernelStr.append("    m_cacul_all_feature_ping   v_data, weight_offset\n");
		asmKernelStr.append("    m_load_input               v_data\n");
		asmKernelStr.append("    m_cacul_all_feature_pang   v_datb, weight_offset\n");

		asmKernelStr.append("END_LOOP_CONV:\n");
		asmKernelStr.append("    s_sub_u32                  s[s_loop_cnt], s[s_loop_cnt], 0x1\n");
		asmKernelStr.append("    s_cmpk_eq_i32              s[s_loop_cnt], 0x0\n");
		asmKernelStr.append("    s_cbranch_scc0             LOOP_CONV\n");

		asmKernelStr.append("LAST_CYCLE:\n");
		asmKernelStr.append("    m_load_input               v_datb\n");

		//asmKernelStr.append("    v_lshlrev_b32 		v[v_io_offset0], 2, v[v_test]\n");
		//asmKernelStr.append("    v_mov_b32			v[v_tmp1], 0x0+MLO_OUT_CHANNEL_STRIDE*2*4\n");
		//
		//asmKernelStr.append("    v_add_co_u32		v[v_io_offset1], vcc, v[v_io_offset0], v[v_tmp1]\n");
		//asmKernelStr.append("    v_add_co_u32		v[v_io_offset2], vcc, v[v_io_offset1], v[v_tmp1]\n");
		//asmKernelStr.append("    v_add_co_u32		v[v_io_offset3], vcc, v[v_io_offset2], v[v_tmp1]\n");
		//asmKernelStr.append("    v_add_co_u32		v[v_io_offset4], vcc, v[v_io_offset3], v[v_tmp1]\n");
		//asmKernelStr.append("    v_add_co_u32		v[v_io_offset5], vcc, v[v_io_offset4], v[v_tmp1]\n");
		//asmKernelStr.append("    v_add_co_u32		v[v_io_offset6], vcc, v[v_io_offset5], v[v_tmp1]\n");
		//asmKernelStr.append("    v_add_co_u32		v[v_io_offset7], vcc, v[v_io_offset6], v[v_tmp1]\n");

		asmKernelStr.append("    m_cacul_all_feature_ping   v_data, weight_offset\n");
		asmKernelStr.append("    m_cacul_all_feature_last   v_datb, weight_offset\n");
		asmKernelStr.append("\n");
		
		asmKernelStr.append("    m_save_output\n");
		asmKernelStr.append("\n");
	}
	
};
 
/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
class ConvFwd1x1Problem : public ProblemCtrlBase
{
public:
	ConvFwd1x1Problem()
	{
		ProblemName = "DirConv1x1Fwd";

		Solution = new ConvFwd1x1Solution();
		ProblemConfigList = new std::list<T_ProblemConfig*>;
	}
	
	/************************************************************************/
	/* 生成问题空间													        */
	/************************************************************************/
	E_ReturnState GenerateProblemConfigs()
	{
		T_ProblemConfig * problemConfig;
		T_ExtConvFwd1x1ProblemConfig * extProblemConfig;

		T_SearchParam * searchParam;

		searchParam = new T_SearchParam();
		searchParam->Name = "N";
		//searchParam->ValueArray.push_back(1);
		//searchParam->ValueArray.push_back(2);
		//searchParam->ValueArray.push_back(4);
		//searchParam->ValueArray.push_back(8);
		searchParam->ValueArray.push_back(16);
		//searchParam->ValueArray.push_back(32);

		// 单独测试
		{
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			//extProblemConfig->W = 7;		extProblemConfig->H = 7;
			//extProblemConfig->C = 1024;		extProblemConfig->K = 1024;

			extProblemConfig->W = 28;		extProblemConfig->H = 28;
			extProblemConfig->C = 192;		extProblemConfig->K = 64;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[28*192*64]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);
		}
/*
		// 7*7
		{
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 7;		extProblemConfig->H = 7;
			extProblemConfig->C = 832;		extProblemConfig->K = 256;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[7*832*256]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 7;		extProblemConfig->H = 7;
			extProblemConfig->C = 512;		extProblemConfig->K = 512;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[7*512*512]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 7;		extProblemConfig->H = 7;
			extProblemConfig->C = 512;		extProblemConfig->K = 2048;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[7*512*2048]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 7;		extProblemConfig->H = 7;
			extProblemConfig->C = 2048;		extProblemConfig->K = 512;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[7*2048*512]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);
		} // 7*7 
		
		// 14*14
		{
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 14;		extProblemConfig->H = 14;
			extProblemConfig->C = 512;		extProblemConfig->K = 192;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[14*512*192]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			//ProblemConfigList->push_back(problemConfig);
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 14;		extProblemConfig->H = 14;
			extProblemConfig->C = 256;		extProblemConfig->K = 256;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[14*256*256]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 14;		extProblemConfig->H = 14;
			extProblemConfig->C = 256;		extProblemConfig->K = 1024;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[14*256*1024]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 14;		extProblemConfig->H = 14;
			extProblemConfig->C = 1024;		extProblemConfig->K = 256;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[14*1024*256]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);
		} // 14*14

		// 28*28
		{
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 28;		extProblemConfig->H = 28;
			extProblemConfig->C = 192;		extProblemConfig->K = 64;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[28*192*64]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);

			// ----------------------------------------------------------------------
			// problem config 2：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 28;		extProblemConfig->H = 28;
			extProblemConfig->C = 128;		extProblemConfig->K = 512;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[28*128*512]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);

			// ----------------------------------------------------------------------
			// problem config 3：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 28;		extProblemConfig->H = 28;
			extProblemConfig->C = 512;		extProblemConfig->K = 128;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[28*512*128]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);

			// ----------------------------------------------------------------------
			// problem config 4：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 28;		extProblemConfig->H = 28;
			extProblemConfig->C = 512;		extProblemConfig->K = 256;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[28*512*256]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);

			// ----------------------------------------------------------------------
			// problem config 5：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 28;		extProblemConfig->H = 28;
			extProblemConfig->C = 512;		extProblemConfig->K = 1024;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[28*512*1024]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig); 
		}		

		// 56*56
		{
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 56;		extProblemConfig->H = 56;
			extProblemConfig->C = 64;		extProblemConfig->K = 256;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[56*64*256]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);

			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 56;		extProblemConfig->H = 56;
			extProblemConfig->C = 256;		extProblemConfig->K = 64;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[56*256*64]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);
		}

		// 112*112
		{
			// ----------------------------------------------------------------------
			// problem config 1：
			extProblemConfig = new T_ExtConvFwd1x1ProblemConfig();
			extProblemConfig->W = 112;		extProblemConfig->H = 112;
			extProblemConfig->C = 64;		extProblemConfig->K = 64;

			problemConfig = new T_ProblemConfig();
			problemConfig->ConfigName = "Conv1x1 WHCK=[112*64*64]";
			problemConfig->extConfig = extProblemConfig;
			problemConfig->ProblemSearchSpace.AddOneParam(searchParam);

			ProblemConfigList->push_back(problemConfig);
		}
*/
	} 
	
	/************************************************************************/
	/* 参数初始化                                                            */
	/************************************************************************/
	E_ReturnState InitHost()
	{
		T_ExtConvFwd1x1ProblemConfig * exCfg = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;

		problemCfg->ProblemSearchSpace.StartGetParam();
		while (true)
		{
			T_SearchParam * param;
			param = problemCfg->ProblemSearchSpace.GetOneParam();
			if (param == NULL)
			{
				break;
			}

			if (param->Name == "N")
			{
				exCfg->N = param->CurrValue;
			}
		}

		exCfg->X = 1;
		exCfg->Y = 1;
		exCfg->PadW = 0;
		exCfg->PadH = 0;

		printf("************************************************************************\n");
		printf("* WHCK=[%d,%d,%d,%d] N=[%d]\n", exCfg->W, exCfg->H, exCfg->C, exCfg->K, exCfg->N);
		printf("************************************************************************\n");

		exCfg->batch_size = exCfg->N;
		exCfg->channel_in = exCfg->C;
		exCfg->feature_out = exCfg->K;
		exCfg->width_in = exCfg->W;
		exCfg->heigh_in = exCfg->H;
		exCfg->width_wei = exCfg->X;
		exCfg->heigh_wei = exCfg->Y;
		exCfg->pad_w = exCfg->PadW;
		exCfg->pad_h = exCfg->PadH;
		exCfg->width_out = exCfg->W + exCfg->PadW * 2 - exCfg->X + 1;
		exCfg->heigh_out = exCfg->H + exCfg->PadH * 2 - exCfg->Y + 1;
		exCfg->stride_c_in = exCfg->W * exCfg->H;
		exCfg->stride_n_in = exCfg->W * exCfg->H * exCfg->C;
		exCfg->stride_c_wei = exCfg->X * exCfg->Y;
		exCfg->stride_k_wei = exCfg->X * exCfg->Y * exCfg->C;
		exCfg->stride_k_out = exCfg->width_out * exCfg->heigh_out;
		exCfg->stride_n_out = exCfg->width_out * exCfg->heigh_out * exCfg->K;

		exCfg->prefetch_loop = 32; // exCfg->C / CACHE_LINE;

		exCfg->size_in = exCfg->W * exCfg->H * exCfg->C * exCfg->N;
		exCfg->size_wei = exCfg->X * exCfg->Y * exCfg->C * exCfg->K;
		exCfg->size_out = exCfg->W * exCfg->H * exCfg->K * exCfg->N;
		exCfg->size_sig = exCfg->prefetch_loop * CU_NUM;

		problemCfg->Calculation = 1.0 * exCfg->W * exCfg->H * exCfg->C * exCfg->N * exCfg->K * exCfg->X * exCfg->Y; // */stride_R/stride_S
		//problemCfg->TheoryElapsedTime = problemCfg->Calculation / RuntimeCtrlBase::DeviceInfo.Fp32Flops;
		problemCfg->TheoryElapsedTime = problemCfg->Calculation / (64*64*1.5*1000*1000*1000);

		printf("Calculation = %.3f G\n", problemCfg->Calculation * 1e-9);
		printf("TheoryElapsedTime = %.3f us \n", problemCfg->TheoryElapsedTime * 1e6);

		exCfg->h_in = (float*)HstMalloc(exCfg->size_in * sizeof(float));
		exCfg->h_wei = (float*)HstMalloc(exCfg->size_wei * sizeof(float));
		exCfg->h_out = (float*)HstMalloc(exCfg->size_out * sizeof(float));
		exCfg->out_ref = (float*)HstMalloc(exCfg->size_out * sizeof(float));
		exCfg->h_signal = (int*)HstMalloc(exCfg->size_sig * sizeof(int));

		printf("input  WHCN = [%d, %d, %d, %d]\n", exCfg->width_in, exCfg->heigh_in, exCfg->C, exCfg->N);
		printf("weight WHCK = [%d, %d, %d, %d]\n", exCfg->width_wei, exCfg->heigh_wei, exCfg->C, exCfg->K);
		printf("output WHKN = [%d, %d, %d, %d]\n", exCfg->width_out, exCfg->heigh_out, exCfg->K, exCfg->N);
		printf("init tensor input  = %d = %.3f MByte.\n", exCfg->size_in / sizeof(float), exCfg->size_in / 1024 / 1024.0);
		printf("init tensor weight = %d = %.3f KByte.\n", exCfg->size_wei / sizeof(float), exCfg->size_wei / 1024.0);
		printf("init tensor output = %d = %.3f MByte.\n", exCfg->size_out / sizeof(float), exCfg->size_out / 1024 / 1024.0);

		for (int i = 0; i < exCfg->size_in; i++)
		{
			//exCfg->h_in[i] = 1;
			//exCfg->h_in[i] = (float)(i % 7) + 1.0f;
			exCfg->h_in[i] = (float)(rand() % 100 - 50);
			//exCfg->h_in[i] = (double)rand() * (1.0 / RAND_MAX);
		}
		for (int i = 0; i < exCfg->size_wei; i++)
		{
			//exCfg->h_wei[i] = 1;
			//exCfg->h_wei[i] = (float)(i % 13) + 1.0f;
			exCfg->h_wei[i] = (float)(rand() % 100 - 50);
			//exCfg->h_in[i] = (double)rand() * (1.0 / RAND_MAX);
		}
		for (int i = 0; i < exCfg->size_out; i++)
		{
			exCfg->h_out[i] = 0;
		}
		for (int i = 0; i < exCfg->size_sig; i++)
		{
			exCfg->h_signal[i] = 567;
		}

		//printTensor("input", h_in, exCfg->W, exCfg->H, exCfg->C, exCfg->N);
		//printTensor("weight", h_wei, exCfg->width_wei, exCfg->width_wei, exCfg->C, exCfg->K);
		
		return E_ReturnState::SUCCESS;
	}

	/************************************************************************/
	/* HOST端                                                               */
	/************************************************************************/
	E_ReturnState Host()
	{
		//return E_ReturnState::SUCCESS;
		T_ExtConvFwd1x1ProblemConfig * exCfg = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;

		int u = 1; // stride height
		int v = 1; // stride width
		int dilation_h = 1;
		int dilation_w = 1;

		for (int o = 0; o < exCfg->batch_size; o++)			// for batch size
		{
			for (int w = 0; w < exCfg->feature_out; w++)		// for output features 
			{
				for (int i = 0; i < exCfg->heigh_out; i++)		// for output heigh
				{
					int in_off_h = i * u;
					for (int j = 0; j < exCfg->width_out; j++)	// for output width
					{
						float acc = 0;
						int in_off_w = j * v;
						for (int k = 0; k < exCfg->channel_in; k++)		// sum input channels
						{
							for (int x = 0; x < exCfg->heigh_wei; x++)		// for filter heigh
							{
								int in_x = in_off_h - exCfg->pad_h + x * dilation_h;

								if (in_x >= 0 && in_x < exCfg->heigh_in)
								{
									for (int y = 0; y < exCfg->width_wei; y++)	// for filter width
									{
										int in_y = in_off_w - exCfg->pad_w + y * dilation_w;// start idx of input a line for conv

										if (in_y >= 0 && in_y < exCfg->width_in)
										{
											int idx_in = o * exCfg->stride_n_in + k * exCfg->stride_c_in + in_x * exCfg->width_in + in_y;
											int idx_wei = w * exCfg->stride_k_wei + k * exCfg->stride_c_wei + x * exCfg->width_wei + y;
											acc += exCfg->h_in[idx_in] * exCfg->h_wei[idx_wei];
										}
									}
								}
							}
						}
						exCfg->out_ref[o * exCfg->stride_n_out + w * exCfg->stride_k_out + i * exCfg->width_out + j] = acc;
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
		T_ExtConvFwd1x1ProblemConfig * exCfg = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;

		//printTensor("output", out_ref, width_out, heigh_out, feature_out, batch_size);

		float diff = 0;
		for (int i = 0; i < exCfg->size_out; i++)
		{
			diff += (exCfg->out_ref[i] - exCfg->h_out[i]) * (exCfg->out_ref[i] - exCfg->h_out[i]);
			//diff += (exCfg->h_out[i]) * (exCfg->h_out[i]);
		}
		diff /= exCfg->size_out;

		printf("mean err = %.1f.\n", diff);
		if (diff > MIN_FP32_ERR)
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
		T_ExtConvFwd1x1ProblemConfig * exCfg = (T_ExtConvFwd1x1ProblemConfig *)problemCfg->extConfig;

		HstFree(exCfg->h_in);
		HstFree(exCfg->h_wei);
		HstFree(exCfg->h_out);
		HstFree(exCfg->out_ref);
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
 