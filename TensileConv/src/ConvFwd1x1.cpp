#pragma once 

#include "ConvFwd1x1.h"

using namespace AutoGen;
using namespace AutoTune;

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
#pragma region SOLUTION

#define		MultSolution	(0)
#define		EnSimuIndex		(0)
#define		EnSaveSource	(1)
#define		EnPrintSource	(0)

E_ReturnState ConvFwd1x1Solution::generateSolutionConfigs()
{
	T_SolutionConfig * solutionConfig;
	T_ExtConvFwd1x1SolutionConfig * extSolutionConfig;
	T_SearchParam * searchParam;


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
		solutionConfigList->push_back(solutionConfig);
	}

	return E_ReturnState::SUCCESS;
}

E_ReturnState ConvFwd1x1Solution::generateKernel()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)solutionConfig->extConfig;
	
	while (true)
	{
		T_SearchParam * param;
		param = solutionConfig->KernelSearchSpace.GetOneParam();
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
		extSol->c_in_group = 4;
	}
	if (extSol->k_out_maps == 0)
	{
		extSol->k_out_maps = 8;
	}
	if (extSol->group_size == 0)
	{
		extSol->group_size = 128;
	}

	extSol->c_in_maps = extProb->C / extSol->c_in_group;
	extSol->k_out_group = divCeil(extProb->K, extSol->k_out_maps);

	extSol->c_in_maps_once = 8;
	loop = divCeil(extSol->c_in_maps, extSol->c_in_maps_once);
	extSol->pix_per_group = 64;
	extSol->pix_group = divCeil(extProb->W * extProb->H * extProb->N, extSol->group_size);
	align = extSol->pix_group * extSol->group_size;

	PRINT_SEPARATOR4();
	OUTPUT("- Kernel Param:");
	OUTPUT("- 	c_in_maps =[%d], c_in_group =[%d]", extSol->c_in_maps, extSol->c_in_group);
	OUTPUT("- 	k_out_maps=[%d], k_out_group=[%d]", extSol->k_out_maps, extSol->k_out_group);
	OUTPUT("- 	align=[%d], pix_group = [%d]", align, extSol->pix_group);
	OUTPUT("- 	group_size=[%d]", extSol->group_size);
	PRINT_SEPARATOR4();

	extProb->size_sig = extSol->pix_group * extSol->k_out_group;
	extProb->h_sig = (float*)malloc(extProb->size_sig * sizeof(float));

	solutionConfig->global_sz = dim3(align * extSol->c_in_group * extSol->k_out_group, 1, 1);
	solutionConfig->group_sz = dim3(extSol->group_size, 1, 1);
	
	solutionConfig->KernelName = "ConvFwd1x1";
	if(rtOcl->Device()->DeviceInfo()->name == "gfx900")
		kernelWriter = new KernelWriterConv1x1(problemConfig, solutionConfig);
	else if (rtOcl->Device()->DeviceInfo()->name == "gfx803")
		kernelWriter = new KernelWriterConv1x1(problemConfig, solutionConfig, E_IsaArch::Gfx800);
	kernelWriter->GenKernelString();
	kernelWriter->SaveKernelString2File();
	kernel = rtOcl->CreatKernel(
		(char *)kernelWriter->KernelFile().c_str(), kernelWriter->KernelName().c_str(), E_ProgramType::PRO_GAS_FILE);

//	kernel = rtOcl->CreatKernel("../TensileConv/kernel/VectorAdd.cl", "VectorAdd", E_ProgramType::PRO_OCL_FILE);
//	solutionConfig->global_sz = dim3(extProb->N2, 1, 1);
//	solutionConfig->group_sz = dim3(64, 1, 1);

	return E_ReturnState::SUCCESS;
}

E_ReturnState ConvFwd1x1Solution::generateKernelParam()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)solutionConfig->extConfig;
	
	d_in = rtOcl->DevMalloc(extProb->size_in * sizeof(float));
	d_wei = rtOcl->DevMalloc(extProb->size_wei * sizeof(float));
	d_bias = rtOcl->DevMalloc(extProb->size_bias * sizeof(float));
	d_out = rtOcl->DevMalloc(extProb->size_out * sizeof(float));
	d_sig = rtOcl->DevMalloc(extProb->size_sig * sizeof(float));
	negSlop = extProb->negSlop;
	
	stream->MemCopyH2D(d_in, extProb->h_in, extProb->size_in * sizeof(float));
	stream->MemCopyH2D(d_wei, extProb->h_wei, extProb->size_wei * sizeof(float));
	stream->MemCopyH2D(d_bias, extProb->h_bias, extProb->size_bias * sizeof(float));

	kernel->SetArgs(&d_in, &d_wei, &d_bias, &d_out, &d_sig, &negSlop);

//	d_a = rtOcl->DevMalloc(extProb->sizeN);
//	d_b = rtOcl->DevMalloc(extProb->sizeN);
//	d_c = rtOcl->DevMalloc(extProb->sizeN);
//	 
//	stream->MemCopyH2D(d_a, extProb->h_a, extProb->sizeN);
//	stream->MemCopyH2D(d_b, extProb->h_b, extProb->sizeN);
//
//	kernel->SetArgs(&d_a, &d_b, &d_c);

	return E_ReturnState::SUCCESS;
}

E_ReturnState ConvFwd1x1Solution::getBackResult()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)solutionConfig->extConfig;

	stream->MemCopyD2H(extProb->h_out, d_out, extProb->size_out * sizeof(float));
	stream->MemCopyD2H(extProb->h_sig, d_sig, extProb->size_sig * sizeof(float));

//	stream->MemCopyD2H(extProb->h_c, d_c, extProb->sizeN);
}

void ConvFwd1x1Solution::releaseKernelParam()
{
	rtOcl->DevFree(d_in);
	rtOcl->DevFree(d_wei);
	rtOcl->DevFree(d_bias);
	rtOcl->DevFree(d_out);

//	rtOcl->DevFree(d_a);
//	rtOcl->DevFree(d_b);
//	rtOcl->DevFree(d_c);
}

void ConvFwd1x1Solution::reportProblemPerformence()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)solutionConfig->extConfig;
	
	INFO("shortest time: %.3f (us).\t", solutionScore.ElapsedTime * 1e6);
	INFO("best performence: %.1f%%.\n", solutionScore.Performence * 100);

	PRINT_SEPARATOR2();
	OUTPUT("ProbemConfig [WHCKN]=[%d,%d,%d,%d,%d]:\n", extProb->H, extProb->W, extProb->C, extProb->K, extProb->N);
	while (true)
	{
		T_SearchParam * param;
		param = solutionConfig->KernelSearchSpace.GetOneParam();
		if (param == NULL)
		{
			break;
		}

		OUTPUT("%s = %d\n", param->Name.c_str(), param->BestValue);
	}
	PRINT_SEPARATOR2();
}

void ConvFwd1x1Solution::simulateIndex()
{
/*	T_ExtConvFwd1x1ProblemConfig * extProbCfg = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSolCfg = (T_ExtConvFwd1x1SolutionConfig *)solutionConfig->extConfig;

	int *testId = (int*)malloc(solutionConfig->b_wk0 * sizeof(int));
	int *testPixBlkId = (int*)malloc(solutionConfig->b_wk0 * sizeof(int));
	int *testCInBlkId = (int*)malloc(solutionConfig->b_wk0 * sizeof(int));
	int *testKOutBlkId = (int*)malloc(solutionConfig->b_wk0 * sizeof(int));
	int *testPosId = (int*)malloc(solutionConfig->b_wk0 * sizeof(int));
	int *testBatchId = (int*)malloc(solutionConfig->b_wk0 * sizeof(int));
	int *testOutId = (int*)malloc(solutionConfig->b_wk0 * sizeof(int));

	uint in_chan_stride = extProbCfg->W * extProbCfg->H;
	uint in_batch_stride = extProbCfg->W * extProbCfg->H * extProbCfg->C;
	uint wei_chan_stride = extProbCfg->C;
	uint out_chan_stride = extProbCfg->W * extProbCfg->H;
	uint out_batch_stride = extProbCfg->W * extProbCfg->H * extProbCfg->K;

	uint c_in_maps = extSolCfg->c_in_maps;
	uint c_in_group = extSolCfg->c_in_group;
	uint k_out_maps = extSolCfg->k_out_maps;
	uint k_out_group = extSolCfg->k_out_group;

	uint wave_per_group = solutionConfig->l_wk0 / WAVE_SIZE;
	uint conv_loop = extProbCfg->C / extSolCfg->c_in_maps_once / 2;

	for (int grp = 0; grp < solutionConfig->b_wk0; grp++)
	{
		uint tid_x = 5 * 64 + 40;
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

		pos_id = (pixBlkId * WAVE_SIZE + tidInWave) % in_chan_stride;
		batch_id = (pixBlkId * WAVE_SIZE + tidInWave) / in_chan_stride;
		out_id = kOutBlkId * k_out_maps;

		if (batch_id >= extProbCfg->N)
			goto STORE_IDX;

		gbl_in_off = (batch_id * in_batch_stride) + (cInBlkId * c_in_maps * in_chan_stride) + pos_id;
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
	free(testOutId);*/
}
#pragma endregion

/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
#pragma region PROBLEM

#define		SkipHost		(0)

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
	searchParam = new T_SearchParam("UV");
	searchParam->ValueArray.push_back(0);
	searchParam->ValueArray.push_back(1);
	probCfg->ProblemParamSpace.AddOneParam(searchParam);

	problemConfigList->push_back(probCfg);

	RunAllProblem();
}
E_ReturnState ConvFwd1x1Problem::TurnProblem(int WH, int C, int K, int N, int UV, bool isBias, bool isRelu)
{
	T_ProblemConfig * probCfg = new T_ProblemConfig("convolution 1x1");
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = new T_ExtConvFwd1x1ProblemConfig();

	exProbCfg->W = WH;		exProbCfg->H = WH;
	exProbCfg->C = C;		exProbCfg->K = K;
	exProbCfg->N = N;
	exProbCfg->U = UV;		exProbCfg->V = UV;		// stride
	exProbCfg->enBias = isBias;
	exProbCfg->enRelu = isRelu;
	probCfg->extConfig = exProbCfg;

	problemConfigList->push_back(probCfg);

	RunAllProblem();
}

E_ReturnState ConvFwd1x1Problem::initHostParam()
{
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;

	while (true)
	{
		T_SearchParam * param;
		param = problemConfig->ProblemParamSpace.GetOneParam();
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
		if (param->Name == "UV")
		{
			exProbCfg->U = param->CurrValue;
			exProbCfg->V = param->CurrValue;
		}
	}

	// FIX Parameters
	exProbCfg->X = 1;		exProbCfg->Y = 1;		// filter size
	exProbCfg->R = 0;		exProbCfg->S = 0;		// padding

	PRINT_SEPARATOR1();
	OUTPUT("WHCKN=[%d * %d, %d, %d, %d] stride=[%d, %d]",
		exProbCfg->W, exProbCfg->H, exProbCfg->C, exProbCfg->K, exProbCfg->N,
		exProbCfg->U, exProbCfg->V);
	OUTPUT("Bias = %s, Relu = %s", exProbCfg->enBias ? "True" : "False", exProbCfg->enRelu ? "True" : "False");
	PRINT_SEPARATOR1();

	exProbCfg->OutW = exProbCfg->W + exProbCfg->R * 2 - exProbCfg->X + 1;
	exProbCfg->OutH = exProbCfg->H + exProbCfg->S * 2 - exProbCfg->Y + 1;

	exProbCfg->size_in = exProbCfg->W * exProbCfg->H * exProbCfg->C * exProbCfg->N;
	exProbCfg->size_wei = exProbCfg->X * exProbCfg->Y * exProbCfg->C * exProbCfg->K;
	exProbCfg->size_bias = exProbCfg->K;
	exProbCfg->size_out = exProbCfg->W * exProbCfg->H * exProbCfg->K * exProbCfg->N;
	
	exProbCfg->h_in = (float*)malloc(exProbCfg->size_in * sizeof(float));
	exProbCfg->h_wei = (float*)malloc(exProbCfg->size_wei * sizeof(float));
	exProbCfg->h_bias = (float*)malloc(exProbCfg->size_bias * sizeof(float));
	exProbCfg->h_out = (float*)malloc(exProbCfg->size_out * sizeof(float));
	exProbCfg->out_ref = (float*)malloc(exProbCfg->size_out * sizeof(float));

	INFO("input  WHCN = [%d, %d, %d, %d]", exProbCfg->W, exProbCfg->H, exProbCfg->C, exProbCfg->N);
	INFO("weight WHCK = [%d, %d, %d, %d]", exProbCfg->X, exProbCfg->Y, exProbCfg->C, exProbCfg->K);
	INFO("output WHKN = [%d, %d, %d, %d]", exProbCfg->OutW, exProbCfg->OutH, exProbCfg->K, exProbCfg->N);
	INFO("init tensor input  = %d = %.3f MByte.", exProbCfg->size_in / sizeof(float), exProbCfg->size_in / 1024 / 1024.0);
	INFO("init tensor weight = %d = %.3f KByte.", exProbCfg->size_wei / sizeof(float), exProbCfg->size_wei / 1024.0);
	INFO("init tensor output = %d = %.3f MByte.", exProbCfg->size_out / sizeof(float), exProbCfg->size_out / 1024 / 1024.0);

	exProbCfg->negSlop = -1.23;
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
	for (int i = 0; i < exProbCfg->size_bias; i++)
	{
		exProbCfg->h_bias[i] = 0;
		exProbCfg->h_bias[i] = (float)(rand() % 100 - 50);
	}
	for (int i = 0; i < exProbCfg->size_out; i++)
	{
		exProbCfg->h_out[i] = 555.555;
	}

	exProbCfg->N2 = 1024; 
	exProbCfg->sizeN = exProbCfg->N2 * sizeof(float);
	exProbCfg->h_a = (float *)malloc(exProbCfg->sizeN);
	exProbCfg->h_b = (float *)malloc(exProbCfg->sizeN);
	exProbCfg->h_c = (float *)malloc(exProbCfg->sizeN);
	for (int i = 0; i < exProbCfg->N2; i++)
	{
		exProbCfg->h_a[i] = 1.234;
		exProbCfg->h_b[i] = i * 1.0f;
		exProbCfg->h_c[i] = 0;
	}
	return E_ReturnState::SUCCESS;
}

void ConvFwd1x1Problem::caculateTheoryPerformance()
{
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;

	int cuNum = rtOcl->Device()->DeviceInfo()->cuNum;
	double freq = rtOcl->Device()->DeviceInfo()->freqMHz * 1e6;

	problemConfig->Calculation = 1.0 * exProbCfg->W * exProbCfg->H * exProbCfg->C * exProbCfg->N * exProbCfg->K * exProbCfg->X * exProbCfg->Y / exProbCfg->U / exProbCfg->V * 2.0;
	problemConfig->TheoryElapsedTime = problemConfig->Calculation / (SIMD_PER_CU * cuNum * freq * 2.0);

	INFO("Calculation = %.3f(G), TheoryElapsedTime = %.3f(us)",
		problemConfig->Calculation * 1e-9, problemConfig->TheoryElapsedTime * 1e6);
}

E_ReturnState ConvFwd1x1Problem::runHostCompute()
{
#if SkipHost
	return E_ReturnState::SUCCESS;
#endif
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;

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
				for (int j = 0; j < exProbCfg->OutW; j++)	// for output width
				{
					float acc = exProbCfg->enBias ? exProbCfg->h_bias[w] : 0.0;

					for (int k = 0; k < exProbCfg->C; k++)		// sum input channels
					{
						for (int x = 0; x < exProbCfg->Y; x++)		// for filter heigh
						{
							for (int y = 0; y < exProbCfg->X; y++)	// for filter width
							{
								int in_off_w = j * exProbCfg->U;
								int in_off_h = i * exProbCfg->V;
								int in_x = in_off_w - exProbCfg->R + y * dilation_w;
								int in_y = in_off_h - exProbCfg->S + x * dilation_h;

								if ((in_y >= 0 && in_y < exProbCfg->H) && (in_x >= 0 && in_x < exProbCfg->W))
								{
									int idx_in = o * stride_n_in + k * stride_c_in + in_y * exProbCfg->W + in_x;
									int idx_wei = w * stride_k_wei + k * stride_c_wei + x * exProbCfg->X + y;

									acc += exProbCfg->h_in[idx_in] * exProbCfg->h_wei[idx_wei];
								}
							}
						}
					}
					if (exProbCfg->enRelu == true)
					{
						if (acc < 0)
						{
							exProbCfg->out_ref[o * stride_n_out + w * stride_k_out + i * exProbCfg->OutW + j] = acc * exProbCfg->negSlop;
						}
						else
						{
							exProbCfg->out_ref[o * stride_n_out + w * stride_k_out + i * exProbCfg->OutW + j] = acc;
						}
					}
					else
					{
						exProbCfg->out_ref[o * stride_n_out + w * stride_k_out + i * exProbCfg->OutW + j] = acc;
					}
				}
			}
		}
	}

	return E_ReturnState::SUCCESS;
}

E_ReturnState ConvFwd1x1Problem::verifyDevCompute()
{
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;

	float diff = 0;
	for (int i = 0; i < exProbCfg->size_out; i++)
	{
		diff += (exProbCfg->out_ref[i] - exProbCfg->h_out[i]) * (exProbCfg->out_ref[i] - exProbCfg->h_out[i]);
	}
	diff /= exProbCfg->size_out;

	if (!(diff >= 0 && diff < MIN_FP32_ERR))
	{
		ERR("verify failed! mean err = %.2f.", diff);
	}

	INFO("verify success. mean err = %.1f.", diff);
	return E_ReturnState::SUCCESS;
}

void ConvFwd1x1Problem::releaseHostParam()
{
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;

	free(exProbCfg->h_in);
	free(exProbCfg->h_wei);
	free(exProbCfg->h_bias);
	free(exProbCfg->h_out);
	free(exProbCfg->out_ref);

	free(exProbCfg->h_a);
	free(exProbCfg->h_b);
	free(exProbCfg->h_c);
}

#pragma endregion
