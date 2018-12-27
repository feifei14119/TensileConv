#pragma once 

#include "ConvFwd1x1.h"

//using namespace AutoGen;
using namespace AutoTune;

#pragma region SOLUTION
/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
#define		MultSolution	(0)
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

	d_a = rtOcl->DevMalloc(extProb->sizeN);
	d_b = rtOcl->DevMalloc(extProb->sizeN);
	d_c = rtOcl->DevMalloc(extProb->sizeN);
	 
	stream->MemCopyH2D(d_a, extProb->h_a, extProb->sizeN);
	stream->MemCopyH2D(d_b, extProb->h_b, extProb->sizeN);

	kernel->SetArgs(&d_a, &d_b, &d_c);

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 返回结果                                                            */
/************************************************************************/
E_ReturnState ConvFwd1x1Solution::GetBackResult()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

	stream->MemCopyD2H(extProb->h_c, d_c, extProb->sizeN);
}

/************************************************************************/
/* 释放显存	                                                           */
/************************************************************************/
void ConvFwd1x1Solution::ReleaseDev()
{
	rtOcl->DevFree(d_a);
	rtOcl->DevFree(d_b);
	rtOcl->DevFree(d_c);
}

/************************************************************************/
/* 根据solution参数生成source, complier和worksize                        */
/************************************************************************/
E_ReturnState ConvFwd1x1Solution::GenerateSolution()
{
	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
	T_ExtConvFwd1x1SolutionConfig * extSol = (T_ExtConvFwd1x1SolutionConfig *)SolutionConfig->extConfig;

	kernel = rtOcl->CreatKernel("../TensileConv/kernel/VectorAdd.cl", "VectorAdd", E_ProgramType::PRO_OCL_FILE);
	SolutionConfig->global_sz = dim3(extProb->N2, 1, 1);
	SolutionConfig->group_sz = dim3(64, 1, 1);

	return E_ReturnState::SUCCESS;
}


/************************************************************************/
/* 记录性能和配置															*/
/************************************************************************/
void ConvFwd1x1Solution::ReportProblemPerformence()
{
/*	T_ExtConvFwd1x1ProblemConfig * extProb = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
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
	}*/
}

/************************************************************************/
/* 测试下标计算															*/
/************************************************************************/
void ConvFwd1x1Solution::simulateIndex()
{
/*	T_ExtConvFwd1x1ProblemConfig * extProbCfg = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;
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


#pragma region PROBLEM
/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
#define		SkipHost		(1)

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
	searchParam = new T_SearchParam("UV");
	searchParam->ValueArray.push_back(0);
	searchParam->ValueArray.push_back(1);
	probCfg->ProblemParamSpace.AddOneParam(searchParam);

	ProblemConfigList->push_back(probCfg);

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
	INFO(" WHCKN=[%d * %d, %d, %d, %d] stride=[%d, %d]",
		exProbCfg->W, exProbCfg->H, exProbCfg->C, exProbCfg->K, exProbCfg->N,
		exProbCfg->U, exProbCfg->V);
	INFO(" Bias = %s, Relu = %s", exProbCfg->enBias ? "True" : "False", exProbCfg->enRelu ? "True" : "False");
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

/************************************************************************/
/* 校验                                                                 */
/************************************************************************/
E_ReturnState ConvFwd1x1Problem::Verify()
{
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;

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

/***********************************************************************/
/* 释放                                                                 */
/************************************************************************/
void ConvFwd1x1Problem::ReleaseHost()
{
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;

	free(exProbCfg->h_in);
	free(exProbCfg->h_wei);
	free(exProbCfg->h_bias);
	free(exProbCfg->h_out);
	free(exProbCfg->out_ref);

	free(exProbCfg->h_a);
	free(exProbCfg->h_b);
	free(exProbCfg->h_c);
}


void ConvFwd1x1Problem::caculPerf()
{
	T_ExtConvFwd1x1ProblemConfig * exProbCfg = (T_ExtConvFwd1x1ProblemConfig *)ProblemConfig->extConfig;

	ProblemConfig->Calculation = 1.0 * exProbCfg->W * exProbCfg->H * exProbCfg->C * exProbCfg->N * exProbCfg->K * exProbCfg->X * exProbCfg->Y; // */stride_R/stride_S
	//ProblemConfig->TheoryElapsedTime = ProblemConfig->Calculation / RuntimeCtrlBase::DeviceInfo.Fp32Flops;
	ProblemConfig->TheoryElapsedTime = ProblemConfig->Calculation / (64 * 64 * 1.5 * 1000 * 1000 * 1000);

	printf("Calculation = %.3f G\n", ProblemConfig->Calculation * 1e-9);
	printf("TheoryElapsedTime = %.3f us \n", ProblemConfig->TheoryElapsedTime * 1e6);
}

#pragma endregion
