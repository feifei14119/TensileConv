#pragma once 

#include "TensileConvBase.h"
#include "ConvFwd1x1KernelWriter.h"

namespace TensileConv {

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
class ConvFwd1x1Problem;
class ConvFwd1x1Solution : public SolutionCtrlBase
{
public:
	ConvFwd1x1Solution(ConvFwd1x1Problem * problem);
	AutoGen::T_Conv1x1KernelParam KernelParam() { return kernelParam; }

private:
	cl_mem d_in, d_wei, d_bias, d_out, d_sig;
	cl_mem d_a, d_b, d_c;
	float negSlop;
	int size_sig;

protected:
	ConvFwd1x1Problem * problem;
	AutoGen::T_Conv1x1KernelParam kernelParam;
	AutoGen::KernelWriterConv1x1 * kernelWriter;

	E_ReturnState generateSolutionParamSpace();
	E_ReturnState getKernelParam();
	E_ReturnState getBestKernelParam();
	E_ReturnState generateKernel();
	E_ReturnState prepareKernelArgs();
	void getBackResult();
	void getBestKernel();
	void releaseDevMem();

	// 测试下标计算
	void simulateIndex();
};

/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
class ConvFwd1x1Problem : public ProblemCtrlBase
{
public:
	ConvFwd1x1Problem() : ProblemCtrlBase() { solution = new ConvFwd1x1Solution(this); }
	ConvFwd1x1Problem(std::string name) :ProblemCtrlBase(name) { solution = new ConvFwd1x1Solution(this); }
	~ConvFwd1x1Problem() { delete solution; }

	E_ReturnState TurnProblem();
	E_ReturnState TurnProblem(int WH, int C, int K, int N, int UV, bool isBias, bool isRelu);

	int N() { return batch; }
	int W() { return in_width; } int H() { return in_height; }
	int C() { return in_chan; } int K() { return out_chan; }
	int X() { return wei_width; } int Y() { return wei_height; }
	int R() { return pad_x; } int S() { return pad_y; }
	int U() { return stride_x; } int V() { return stride_y; }
	int OutW() { return out_width; } int OutH() { return out_height; }
	bool EnBias() { return enBias; } bool EnRelu() { return enRelu; }
	float NegSlop() { return negSlop; }

	float* h_in, *h_wei, *h_bias, *h_out, *out_ref, *h_sig;
	int size_in, size_wei, size_bias, size_out, size_sig;

private:
	E_ReturnState initHostParam();
	E_ReturnState runHostCompute();
	E_ReturnState verifyDevCompute();
	void releaseHostParam();
	void caculateTheoryPerformance();

	int batch;					// batch size
	int in_width, in_height;	// input size
	int in_chan, out_chan;		// input channel / output feature
	int wei_width, wei_height;	// weight size
	int pad_x, pad_y;			// padding 
	int stride_x, stride_y;		// stride
	int out_width, out_height;	// output size
	bool enBias, enRelu;
	float negSlop;
};
}

