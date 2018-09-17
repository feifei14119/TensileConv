#pragma once

#include "IsaDemoConfig.h"
#include "KernelWriter.h"

class KernelWriterIsaSmem :public KernelWriterBase
{
public:
	KernelWriterIsaSmem(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg):KernelWriterBase(probCfg, solCfg)
	{
		extProbCfg = (T_ExtSmemProblemConfig *)problemConfig->extConfig;
		extSolCfg = (T_ExtSmemSolutionConfig *)solutionConfig->extConfig;

		N = extProbCfg->VectorSize;
	}

	T_ExtSmemProblemConfig * extProbCfg;	// 当前正在处理的问题扩展配置
	T_ExtSmemSolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置


protected:
	int N = 0;

	t_operator * s_privateSeg;
	t_operator * s_kernelArg;
	t_operator * s_gid_x;
	t_operator * s_gid_y;
	t_operator * s_gid_z;

	t_operator * s_ptr_a;
	t_operator * s_ptr_b;
	t_operator * s_ptr_c;

	// 这里是临时放在这里,为兼容conv1x1
	// 实际将不存在该函数,必要的寄存器alloc应该放在KernelWriter中进行
	// 因为这是每个kernel通用的部分
	void generateParam()
	{
		s_privateSeg = newSgpr("s_privateSeg", 4);
		s_kernelArg = newSgpr("s_kernelArg", 2);
		s_gid_x = newSgpr("s_gid_x");
		s_gid_y = newSgpr("s_gid_y");
		s_gid_z = newSgpr("s_gid_z");
	}
	
	void writeProgram()
	{
		s_ptr_a = newSgpr("s_ptr_a", 2, 2);
		s_ptr_b = newSgpr("s_ptr_b", 2, 2);
		s_ptr_c = newSgpr("s_ptr_c", 2, 2);

		t_operator * s_tmp1 = newSgpr("s_tmp1");
		t_operator * s_tmp2 = newSgpr("s_tmp2");

		isa->s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
		isa->s_load_dword(2, s_ptr_b, s_kernelArg, 0x08);
		isa->s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
		isa->s_waitcnt("lgkmcnt", 0);

		isa->s_load_dword(1, s_tmp1, s_ptr_a, 0x00);
		isa->s_load_dword(1, s_tmp2, s_ptr_b, 0x00);
		isa->s_waitcnt("lgkmcnt", 0);

		isa->s_store_dword(1, s_tmp1, s_ptr_c, 0);
		isa->s_store_dword(1, s_tmp2, s_ptr_c, 0x04);
		isa->s_waitcnt("lgkmcnt", 0);
		isa->inst0("s_dcache_wb","");

		clrOpter();
	}
};
