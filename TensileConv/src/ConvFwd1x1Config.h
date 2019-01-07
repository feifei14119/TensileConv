#pragma once 

//#include "BasicClass.h" 

/************************************************************************/
/* 扩展参数                                                              */
/* 保存需要传递给 KernelWriterBase 的扩展参数								*/
/************************************************************************/
typedef struct ExtConvFwd1x1SolutionConfigTpye
{
	// thread规划
	int group_size;		// 每个workgroup有多少个thread
	
	// 对于一次循环的 input channel 的划分
	int c_in_maps_once;		 // 8:[8,16]

	// 对于一个 thread 的 input channel 划分
	int c_in_maps;
	int c_in_group;

	// 对于一个 thread 的 output channel 划分
	int k_out_maps;
	int k_out_group;
	
	int pix_group;

	// 对于一个 work group 的 pixal 划分
	int pix_per_group;
}T_ExtConvFwd1x1SolutionConfig;

