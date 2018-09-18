#pragma once

#include "IsaDemoConfig.h"
#include "KernelWriter.h"

class KernelWriterIsaMubuf :public KernelWriterBase
{
public:
	KernelWriterIsaMubuf(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg):KernelWriterBase(probCfg, solCfg)
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

	t_operator * v_tid_x;

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

		v_tid_x = newVgpr("v_tid_x");
	}
	
	void writeProgram()
	{
		t_operator * s_ptr_a = newSgpr("s_ptr_a", 2, 2);
		t_operator * s_ptr_b = newSgpr("s_ptr_b", 2, 2);
		t_operator * s_ptr_c = newSgpr("s_ptr_c", 2, 2);

		t_operator * v_addr_a = newVgpr("v_addr_a", 2, 2);
		t_operator * v_addr_c = newVgpr("v_addr_c", 2, 2);
		t_operator * v_tmp1 = newVgpr("v_tmp1");
		t_operator * v_tmp2 = newVgpr("v_tmp2");

		isa->s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
		isa->s_load_dword(2, s_ptr_b, s_kernelArg, 0x08);
		isa->s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
		isa->s_waitcnt("lgkmcnt", 0);

		// -------------------------------------------------------------------------------
		// 计算输入a下标: 
		// a_addr = a_ptr + (gid_x * local_size) + tid_x
		// -------------------------------------------------------------------------------
		isa->op2("v_lshlrev_b32", v_tmp1, log2(WAVE_SIZE), s_gid_x);
		isa->op3("v_add_lshl_u32", v_tmp1, v_tmp1, v_tid_x, 2);
		isa->op1("v_mov_b32", v_tmp2, *s_ptr_a + 1);
		isa->op3("v_add_co_u32", v_addr_a, "vcc", s_ptr_a, v_tmp1);
		isa->op4("v_addc_co_u32", *v_addr_a + 1, "vcc", 0, v_tmp2, "vcc");

		// -------------------------------------------------------------------------------
		// 计算输出下标: 
		// c_addr = c_ptr + (gid_x * local_size) + tid_x
		// -------------------------------------------------------------------------------
		isa->op2("v_lshlrev_b32", v_tmp1, log2(WAVE_SIZE), s_gid_x);
		isa->op3("v_add_lshl_u32", v_tmp1, v_tmp1, v_tid_x, 2);
		isa->op1("v_mov_b32", v_tmp2, *s_ptr_c + 1);
		isa->op3("v_add_co_u32", v_addr_c, "vcc", s_ptr_c, v_tmp1);
		isa->op4("v_addc_co_u32", *v_addr_c + 1, "vcc", 0, v_tmp2, "vcc");

		// -------------------------------------------------------------------------------
		// 设置buffer描述符
		// -------------------------------------------------------------------------------
		t_operator * s_desc = newSgpr("s_ptr_c", 4, 4);
		isa->setBufferDsc(s_desc, s_ptr_a, WAVE_SIZE, N*4, 7, 4, false);
		t_operator * v_dat = newVgpr("v_dat", 2, 2);
		isa->buffer_load_dword(2, v_dat, "off", s_desc, 0, false, false, 4);
		isa->s_waitcnt("vmcnt", 0);
		isa->inst3("global_store_dword","v[4:5]", "v[8]", "off", "");

		clrOpter();
	}
};
