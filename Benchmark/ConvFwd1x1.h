#pragma once 

#include "ProblemControl.h"
#include "ConvFwd1x1Config.h"

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
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
	E_ReturnState GenerateSolutionConfigs();

	/************************************************************************/
	/* 申请显存                                                            */
	/************************************************************************/
	E_ReturnState InitDev();

	/************************************************************************/
	/* 返回结果                                                            */
	/************************************************************************/
	E_ReturnState GetBackResult();

	/************************************************************************/
	/* 释放显存	                                                           */
	/************************************************************************/
	void ReleaseDev();

	/************************************************************************/
	/* 根据solution参数生成source, complier和worksize                        */
	/************************************************************************/
	E_ReturnState GenerateSolution();

	int N_LCL_IN_MAPS;
	int N_IN_GROUPS;
	int N_LCL_IN_MAPS_ONCE;
	int	N_OUT_GROUPS;
	int CLOOP0;
	int CLOOP2;
	E_ReturnState generateParameters();

	E_ReturnState generateCompilerOption();

	E_ReturnState generateWorkLoad();

	E_ReturnState generateSource();

	/************************************************************************/
	/* 自动生成kernel								                        */
	/************************************************************************/
	void autoGenKernel();

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
	void ReportProblemPerformence();
	
	/************************************************************************/
	/* 测试下标计算															*/
	/************************************************************************/
	void simulateIndex();
	
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
	E_ReturnState GenerateProblemConfigs();
	
	/************************************************************************/
	/* 参数初始化                                                            */
	/************************************************************************/
	E_ReturnState InitHost();

	/************************************************************************/
	/* HOST端                                                               */
	/************************************************************************/
	E_ReturnState Host();

	/************************************************************************/
	/* 校验                                                                 */
	/************************************************************************/
	E_ReturnState Verify();
	 
	/************************************************************************/
	/* 释放                                                                  */
	/************************************************************************/
	void ReleaseHost();

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


