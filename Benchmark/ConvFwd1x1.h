#pragma once 

#include "BasicClass.h" 
#include "RuntimeControl.h"
#include "ProblemControl.h"

#include "GasWriter.h"
#include "IsaWriterGfx9.h"
#include "KernelWriterConv1x1.h"

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
		//solutionCfg->KernelArgus->push_back(d_signal);

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
			solutionConfig->RepeatTime = 1;
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

			solutionConfig = new T_SolutionConfig();
			solutionConfig->ConfigName = "PreFetch_Single";
			solutionConfig->RepeatTime = 1;
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

			solutionConfig = new T_SolutionConfig();
			solutionConfig->ConfigName = "PreFetch_Mult";
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
			//solutionConfig->KernelSearchSpace.AddOneParam(searchParam);
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
			//solutionConfig->KernelSearchSpace.AddOneParam(searchParam);
			// ----------------------------------------------------------------------
			// 添加solution
			SolutionConfigList->push_back(solutionConfig);
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
			extSolution->group_size = FIXED_WORKGROUP_SIZE;
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
		else if (solutionCfg->ConfigName == "SQC")
		{
			extSolution->group_size = 64;
			extSolution->k_out_maps = 16;
			extSolution->k_out_group = (extProblem->K + extSolution->k_out_maps - 1) / extSolution->k_out_maps;
			extSolution->c_in_maps = extProblem->C;
			extSolution->c_in_group = (extProblem->C + extSolution->c_in_maps - 1) / extSolution->c_in_maps;

			extSolution->c_in_maps_once = 8;
			extSolution->out_pix_tile = 1;
			extSolution->out_tile = extSolution->out_pix_tile * extSolution->k_out_maps;
			extSolution->in_pix_maps = 64;
			extSolution->loop = extSolution->c_in_maps / extSolution->c_in_maps_once;
			align = ((extProblem->width_in * extProblem->heigh_in * extProblem->batch_size +
				extSolution->in_pix_maps - 1) / extSolution->in_pix_maps) * extSolution->in_pix_maps;
		}
		else if (solutionCfg->ConfigName == "PreFetch_Single")
		{
			extSolution->group_size = 64;
			extSolution->k_out_maps = 16;
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
		else if (solutionCfg->ConfigName == "PreFetch_Mult")
		{
			extSolution->group_size = 64;
			extSolution->k_out_maps = 16;
			extSolution->k_out_group = (extProblem->K + extSolution->k_out_maps - 1) / extSolution->k_out_maps;
			extSolution->c_in_maps = extProblem->C;
			extSolution->c_in_group = (extProblem->C + extSolution->c_in_maps - 1) / extSolution->c_in_maps;

			extSolution->c_in_maps_once = 8;
			extSolution->out_pix_tile = 1;
			extSolution->out_tile = extSolution->out_pix_tile * extSolution->k_out_maps;
			extSolution->in_pix_maps = 64;
			extSolution->loop = extSolution->c_in_maps / extSolution->c_in_maps_once;
			align = ((extProblem->width_in * extProblem->heigh_in * extProblem->batch_size +
				extSolution->in_pix_maps - 1) / extSolution->in_pix_maps) * extSolution->in_pix_maps;
		}
		else if (solutionCfg->ConfigName == "TensileConv")
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
				extSolution->k_out_maps = 16;
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
			align = ((extProblem->width_in * extProblem->heigh_in * extProblem->batch_size + 
				FIXED_WORKGROUP_SIZE - 1) / FIXED_WORKGROUP_SIZE) * FIXED_WORKGROUP_SIZE;
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
			solutionCfg->l_wk0 = extSolution->group_size;
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

			//solutionCfg->l_wk0 = 1;
			//solutionCfg->g_wk0 = 64;
		}
		else if (solutionCfg->ConfigName == "PreFetch_Single")
		{
			solutionCfg->l_wk0 = extSolution->group_size;
			solutionCfg->l_wk1 = 1;
			solutionCfg->l_wk2 = 1;
			solutionCfg->g_wk0 = align * extSolution->c_in_group * extSolution->k_out_group;
			solutionCfg->g_wk1 = 1;
			solutionCfg->g_wk2 = 1;

			//solutionCfg->l_wk0 = 64;		// 需要注释掉循环控制的exec
			//solutionCfg->g_wk0 = (64 * solutionCfg->l_wk0);

			solutionCfg->g_wk0 += (64 * solutionCfg->l_wk0);
		}
		else if (solutionCfg->ConfigName == "PreFetch_Mult")
		{
			solutionCfg->l_wk0 = extSolution->group_size;
			solutionCfg->l_wk1 = 1;
			solutionCfg->l_wk2 = 1;
			solutionCfg->g_wk0 = align * extSolution->c_in_group * extSolution->k_out_group;
			solutionCfg->g_wk1 = 1;
			solutionCfg->g_wk2 = 1;

			//solutionCfg->l_wk0 = 1;
			//solutionCfg->g_wk0 = 64;
		}
		else if (solutionCfg->ConfigName == "TensileConv")
		{
			solutionCfg->l_wk0 = extSolution->group_size;
			solutionCfg->l_wk1 = 1;
			solutionCfg->l_wk2 = 1;
			solutionCfg->g_wk0 = align * extSolution->c_in_group * extSolution->k_out_group;
			solutionCfg->g_wk1 = 1;
			solutionCfg->g_wk2 = 1;
			
			//solutionCfg->l_wk0 = 64;
			//solutionCfg->g_wk0 = 64*64;
		}

		solutionCfg->b_wk0 = solutionCfg->g_wk0 / solutionCfg->l_wk0;
		solutionCfg->b_wk1 = solutionCfg->g_wk1 / solutionCfg->l_wk1;
		solutionCfg->b_wk2 = solutionCfg->g_wk2 / solutionCfg->l_wk2;
	}

	void generateSource()
	{
		solutionCfg->KernelName = "ConvFwd1x1";
		if (solutionCfg->ConfigName == "MIOpenOcl")
		{
			solutionCfg->KernelFile = "ConvFwd1x1_Jcl.cl";
			//solutionCfg->KernelFile = "ConvFwd1x1_Jcl_NewOrg.cl";
			//solutionCfg->KernelFile = "ConvFwd1x1_Jcl_256thread.cl";
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_OCL_FILE;
		}
		else if (solutionCfg->ConfigName == "SQC")
		{
			solutionCfg->KernelFile = "ConvFwd1x1_Jasm.s";
			//solutionCfg->KernelFile = "ConvFwd1x1_Jasm_NewOrg.s";
			//solutionCfg->KernelFile = "ConvFwd1x1_Jasm_256thread.s";
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}
		else if (solutionCfg->ConfigName == "PreFetch_Single")
		{
			solutionCfg->KernelFile = "ConvFwd1x1_Jasm_PreFetch_Single.s";
			//solutionCfg->KernelFile = "ConvFwd1x1_Jasm_PreFetch_Single_NewOrg.s";
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}
		else if (solutionCfg->ConfigName == "PreFetch_Mult")
		{
			solutionCfg->KernelFile = "ConvFwd1x1_Jasm_PreFetch_Mult_Conv.s";
			solutionCfg->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;
		}
		else if (solutionCfg->ConfigName == "TensileConv")
		{
			solutionCfg->KernelFile = "ConvFwd1x1_Jasm_TensileConv.s";
			autoGenKernel();
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

		int *testGrpId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
		int *testInBlkId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
		int *testWeiBlkId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
		int *testPosId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
		int *testBatchId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));
		int *testOutId = (int*)malloc(solutionCfg->b_wk0 * sizeof(int));

		if (extSolution->group_size == 256)
		{
			uint MLO_N_OUT_GROUPS = extSolution->k_out_group;
			uint MLO_IN_CHANNEL_STRIDE = extProblem->W * extProblem->H;
			uint MLO_N_LCL_OUT_MAPS = extSolution->k_out_maps;
			uint MLO_IN_BATCH_STRIDE = extProblem->W * extProblem->H * extProblem->C;
			uint MLO_WEI_CHANNEL_STRIDE = extProblem->C;
			uint MLO_OUT_BATCH_STRIDE = extProblem->W * extProblem->H * extProblem->K;
			uint MLO_OUT_CHANNEL_STRIDE = extProblem->W * extProblem->H;

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
				testInBlkId[grp_id0] = grp_id0_faked;
				testWeiBlkId[grp_id0] = out_grp_block;
				testPosId[grp_id0] = pos;
				testBatchId[grp_id0] = batch_id;
				testOutId[grp_id0] = out_id;
			}
		}

		if (extSolution->group_size == 64)
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

			for (int grp = 0; grp < solutionCfg->b_wk0; grp++)
			{
				uint local_id0 = 0;
				uint grp_id0 = grp;

				// old organization
				uint out_grp_block = grp_id0 % MLO_N_OUT_GROUPS;
				uint grp_id0_faked = (uint)(grp_id0 / MLO_N_OUT_GROUPS) / MLO_N_IN_GROUPS;

				uint pos = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0 % FIXED_WORKGROUP_SIZE) % MLO_IN_CHANNEL_STRIDE;
				uint batch_id = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0 % FIXED_WORKGROUP_SIZE) / MLO_IN_CHANNEL_STRIDE;
				uint out_id = out_grp_block * MLO_N_LCL_OUT_MAPS;

				uint gbl_in_off = batch_id * MLO_IN_BATCH_STRIDE + pos;
				uint wei_off = out_id * MLO_WEI_CHANNEL_STRIDE;
				uint gbl_out_off = batch_id * MLO_OUT_BATCH_STRIDE + out_id * MLO_OUT_CHANNEL_STRIDE + pos;

				// new organization
				//uint inBlkId, weiBlkId;
				//uint z_round = grp_id0 / (CU_NUM * MLO_N_OUT_GROUPS);	// 第几轮Z格子
				//
				//inBlkId = z_round * CU_NUM + grp_id0 % CU_NUM;		// 即 grp_id0_faked
				//weiBlkId = grp_id0 / CU_NUM % MLO_N_OUT_GROUPS;		// 即 out_grp_block
				//
				//if (grp_id0 >= MLO_ROUND_LEFT)
				//{
				//	uint leftGrpId = 0;
				//	leftGrpId = grp_id0 - MLO_ROUND_LEFT;
				//	inBlkId = leftGrpId / 4 + MLO_INBLOCK_LEFT;
				//	weiBlkId = leftGrpId % 4;
				//}
				//
				//uint out_grp_block = weiBlkId;
				//uint grp_id0_faked = inBlkId;
				//
				//uint pos = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0 % FIXED_WORKGROUP_SIZE) % MLO_IN_CHANNEL_STRIDE;
				//uint batch_id = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0 % FIXED_WORKGROUP_SIZE) / MLO_IN_CHANNEL_STRIDE;
				//uint out_id = out_grp_block * MLO_N_LCL_OUT_MAPS;

				testGrpId[grp_id0] = grp_id0;
				testInBlkId[grp_id0] = grp_id0_faked;
				testWeiBlkId[grp_id0] = out_grp_block;
				testPosId[grp_id0] = pos;
				testBatchId[grp_id0] = batch_id;
				testOutId[grp_id0] = out_id;
			}
		}

		//printIndex(testGrpId, "group id");
		//printIndex(testInBlkId, "input block id");
		printIndex(testWeiBlkId, "weight block id");
		//printIndex(testBatchId, "batch id");
		//printIndex(testOutId, "out id");
		//printIndex(testPosId, "pos id");
		printf("input groups: %d.\n", align * N_IN_GROUPS);
		printf("output group: %d.\n", N_OUT_GROUPS);
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
		LaunchSolution();

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
			setArgusPrefetch();
			setArgusCalcu();

			printf("launch solution.\n");
			preKernel->LanchKernel2();
			runtime->LanchKernel2();
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
	void autoGenKernel()
	{
		KernelWriterBase * wr = new KernelWriterConv1x1();
		wr->KernelName = solutionCfg->KernelName;
		wr->KernelFile = solutionCfg->KernelFile;

		wr->GenKernel();
		wr->SaveKernelStr2File();
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
 