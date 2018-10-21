#pragma once 

#include "BasicClass.h" 

/************************************************************************/
/* 扩展参数                                                              */
/* 保存需要传递给 KernelWriterBase 的扩展参数								*/
/************************************************************************/
typedef struct ExtConvFwd1x1SolutionConfigTpye
{
	// K划分
	int k_out_maps;		// 每个CU计算多少个输出特征值K

	// thread规划
	int group_size;		// 每个workgroup有多少个thread
	
	// 调整参数
	int c_in_maps_once;		 // 8:[8,16]
	// wei_pingpang_ins:	 1:[1,2,4,8]
	// en_in_pingpang:		 1:[0,1]
	// wait_cnt_in_fetch:	 4:[1,2,4,8,16]

}T_ExtConvFwd1x1SolutionConfig;

typedef struct ExtConvFwd1x1ProblemConfigType
{
	int N;				// batch size
	int W, H;			// input size
	int C, K;			// input channel / output feature
	int X, Y;			// weight size
	int R, S;			// padding 
	int U, V;			// stride
	int OutW, OutH;		// output size

	float* h_in, *h_wei, *h_out;
	float *out_ref, *h_dbg;
	int size_in, size_wei, size_out;
	int size_dbg;
}T_ExtConvFwd1x1ProblemConfig;
