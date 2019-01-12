#pragma once

#include "KernelWriter.h"


namespace TensileConv {
namespace AutoGen {

typedef enum LdsReduceEnum
{
	LDS_SPLIT = 1,
	LDS_ATOMIC = 2
}E_LdsReduce;
typedef enum L2ReduceEnum
{
	ATOMIC_SPLIT = 1,
	SPLIT_ATOMIC = 2
}E_L2Reduce;
typedef struct Conv1x1KernelParamType
{
	int group_size_x;
	
	E_LdsReduce lds_method;
	E_L2Reduce l2_method;
	int PCK_order;
	int c_in_lds_group;
	int c_in_l2_split_group;
	int c_in_l2_atomic_group;
	int k_out_maps;

	int N, C, H, W, K;
	bool EnBias, EnRelu;
}T_Conv1x1KernelParam;

class KernelWriterConv1x1 :public AutoGen::KernelWriter
{
public:
	KernelWriterConv1x1(T_Conv1x1KernelParam kernelParam, E_IsaArch isaArch = E_IsaArch::Gfx900);
	int SlotSize() { return size_sig; }
	int DebugSize() { return size_dbg; }

protected:
	T_Conv1x1KernelParam kernelParam;

	typedef enum
	{
		PING_FLAG = 1,
		PANG_FLAG = 2
	}E_PingPang;

	// -------------------------------------------------------------------------------
	int N, C, H, W, K;
	bool enBias, enRelu;

	int PCK_order;
	E_LdsReduce lds_method;
	E_L2Reduce l2_method;
	int c_in_maps;
	int c_in_group;				// c_in_l2_group * c_in_lds_group
	int c_in_lds_group;			// c_lds_split_group * c_lds_atomic_group = group_sz.y
	int c_in_l2_group;			// c_l2_split_group * c_l2_atomic_group
	int c_in_l2_split_group;
	int c_in_l2_atomic_group;
	int	c_in_maps_once = 8;		// 对于一次循环的 input channel 的划分 [8,16]
	int c_in_maps_once_real;
	int unroll_time = 2;

	int k_out_maps;
	int k_out_group;

	size_t align;
	int pix_group;				// 所有像素被分到几个group
	int pix_wave;				// 所有像素被分到几个wave
	int pix_per_group = 64;

	int size_sig, size_dbg, size_l2_split;

	// -------------------------------------------------------------------------------
	int in_chan_stride;		// IN_CHANNEL_STRIDE
	int in_batch_stride;	// IN_BATCH_STRIDE
	int wei_chan_stride;	// WEI_CHANNEL_STRIDE
	int out_chan_stride;	// OUT_CHANNEL_STRIDE
	int out_batch_stride;	// OUT_BATCH_STRIDE
	int group_wave_num_x;	// PIX_BLK_PER_GROUP
	int conv_loop;			// LOOP
	
	bool en_input_offset;
	bool en_wei_addr_offset = true;
	int offset_grp_num;
	Var * v_global_offset;
	int wei_offset;

	Var * s_ptr_in;
	Var * s_ptr_wei;
	Var * s_ptr_bias;
	Var * s_ptr_out;
	Var * s_ptr_sig;
	Var * s_slop;

	Var * v_wave_id_x;		// 全局x方向的wave id, 不关心LDS方向的wave id
	Var * v_wave_tid_x;		// 一个wave内的thread id
	Var * v_pix_blk_id;		// grp_id0_faked
	Var * v_c_l2_blk_id;	// in_grp_block
	Var * v_c_blk_id;
	Var * v_k_blk_id;		// out_grp_block
	Var * v_lds_split_id;
	Var * v_l2_split_id;
	Var * v_pos_id;
	Var * v_batch_id;
	Var * v_out_id;

	Var * v_addr_in;
	Var * s_addr_wei;
	Var * s_addr_bias;
	Var * v_addr_bias;
	Var * v_addr_out;
	Var * s_addr_sig;
	Var * v_addr_out_back;
	Var * v_lds_addr;

	Var * v_in_buff_a;
	Var * v_in_buff_b;
	Var * s_wei_buff_a;
	Var * s_wei_buff_b;
	Var * v_acc_buff;
	Var * s_prefetch;

	Var * s_exec_save;

	bool en_slop_zero = true;

#if KERNEL_DEBUG
	Var * v_debug;
	Var * s_ptr_dbg;
	Var * v_addr_dbg;
#endif

	E_ReturnState checkKernelParam();
	E_ReturnState writeProgram()
	{
		CheckFunc(checkKernelParam());
		CheckFunc(simulate_index());
		CheckFunc(calcuIndex());
		main_conv();

#if KERNEL_DEBUG
		v_debug = newVgpr("v_debug");
		save_debug();
#endif

		clrVar();
	}

	/************************************************************************************/
	/* 卷积主体																			*/
	/************************************************************************************/
	void main_conv();
	void conv_one_loop(Var * in_buff, bool is_pang_buff);
	void conv_last_loop(Var * in_buff);
	void conv_one_accum(Var * in_buff, Var * wei_buff, Var * accum);

	/************************************************************************************/
	/* 计算下标																			*/
	/************************************************************************************/
	E_ReturnState calcuIndex();
	E_ReturnState loadKernelArgs();
	E_ReturnState calcuWaveIndex();
	E_ReturnState calcuBlkIndex();
	E_ReturnState calcuPosIndex();
	E_ReturnState calcuOffset();

	/************************************************************************************/
	/* 数据读取与存储																		*/
	/************************************************************************************/
	void load_input(Var * in_buff);
	void load_weight(Var * wei_buff);
	void prefetch_weight();
	void init_output();

	void save_result();
	void save_to_lds_split();
	void save_to_lds_atomic();
	void save_to_l2_split();
	void save_to_l2_atomic();
	void save_to_l2_direct();
	
	void save_with_atomic(int n, Var * addr_out, Var * accum);
	void save_with_slop_zero();

	void inner_group_sync();

	E_ReturnState simulate_index();
	void save_debug();
	void print_kernel_param();
};
}
}