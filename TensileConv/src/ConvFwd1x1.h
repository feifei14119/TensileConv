#pragma once 

#include "TensileConvBase.h"
#include "ConvFwd1x1KernelWriter.h"

namespace TensileConv {

/************************************************************************/
/* solution ¿ØÖÆ										                    */
/************************************************************************/
class ConvFwd1x1Problem;
class ConvFwd1x1Solution : public SolutionCtrlBase
{
public:
	ConvFwd1x1Solution(ConvFwd1x1Problem * problem, std::string name = "", LogFile * file = nullptr);
	AutoGen::T_Conv1x1KernelParam KernelParam() { return kernelParam; }
	void GetBestKernel();

private:
	float *h_sig, *h_l2, *h_dbg;
	cl_mem d_in, d_wei, d_bias, d_out, d_sig, d_l2, d_dbg;
	float negSlop;
	size_t size_sig, size_l2, size_dbg;

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
	void releaseDevMem();
};

/************************************************************************/
/* solver ¿ØÖÆ															*/
/************************************************************************/
class ConvFwd1x1Solver : public SolverCtrlBase
{
public:
	ConvFwd1x1Solver(ConvFwd1x1Problem * problem, LogFile * file = nullptr);
	~ConvFwd1x1Solver()	{}
	
protected:
	ConvFwd1x1Problem * problem;
	void generateSolver();
};

/************************************************************************/
/* problem ¿ØÖÆ															*/
/************************************************************************/
class ConvFwd1x1Problem : public ProblemCtrlBase
{
public:
	ConvFwd1x1Problem(std::string name = "", LogFile * file = nullptr)
		:ProblemCtrlBase(name, file) 
	{
		solver = new ConvFwd1x1Solver(this, logFile);
	}
	~ConvFwd1x1Problem() { delete solver; }

	void TuneProblem();
	void TuneProblem(int WH, int C, int K, int N, int UV, bool isBias, int Relu);

	int N() { return batch; }
	int W() { return in_width; } int H() { return in_height; }
	int C() { return in_chan; } int K() { return out_chan; }
	int X() { return wei_width; } int Y() { return wei_height; }
	int R() { return pad_x; } int S() { return pad_y; }
	int U() { return stride_x; } int V() { return stride_y; }
	int OutW() { return out_width; } int OutH() { return out_height; }
	int EnBias() { return enBias; } 
	E_Relu Relu() { return relu; }
	float NegSlop() { return negSlop; }

	float* h_in, *h_wei, *h_bias, *h_out, *out_ref;
	size_t size_in, size_wei, size_bias, size_out;

private:
	void generateProblem();
	E_ReturnState initHostParam();
	void runHostCompute();
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
	bool enBias;
	E_Relu relu;
	float negSlop;
};
}

