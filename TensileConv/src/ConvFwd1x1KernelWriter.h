#pragma once

#include "ConvFwd1x1Config.h"
#include "KernelWriter.h"

class ConvFwd1x1Problem;
class ConvFwd1x1Solution;

namespace AutoGen
{
	class KernelWriterConv1x1 :public AutoGen::KernelWriter
	{
	public:
		KernelWriterConv1x1(ConvFwd1x1Problem * problem, T_SolutionConfig * solCfg, E_IsaArch isaArch = E_IsaArch::Gfx900);
		
		ConvFwd1x1Problem * problem;
		ConvFwd1x1Solution * solution;
	protected:
		T_ExtConvFwd1x1SolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置

		typedef enum
		{
			PING_FLAG = 1,
			PANG_FLAG = 2
		}E_PingPang;

		int c_in_maps_once;
		int unroll_time = 2;
		int c_in_maps_once_real;

		int c_in_maps;
		int c_in_group;
		int k_out_maps;
		int k_out_group;

		int wave_per_group;		// PIX_BLK_PER_GROUP
		int in_chan_stride;		// IN_CHANNEL_STRIDE
		int in_batch_stride;	// IN_BATCH_STRIDE
		int wei_chan_stride;	// WEI_CHANNEL_STRIDE
		int out_chan_stride;	// OUT_CHANNEL_STRIDE
		int out_batch_stride;	// OUT_BATCH_STRIDE
		int conv_loop;			// LOOP

		bool en_input_offset;
		bool en_wei_addr_offset;
		int offset_grp_num;
		Var * v_global_offset;
		int wei_offset;

		Var * s_ptr_in;
		Var * s_ptr_wei;
		Var * s_ptr_bias;
		Var * s_ptr_out;
		Var * s_ptr_sig;
		Var * s_slop;
		
		Var * v_waveId;
		Var * v_tidInWave;
		Var * v_pixBlkId;		// grp_id0_faked
		Var * v_cInBlkId;		// in_grp_block
		Var * v_kOutBlkId;		// out_grp_block
		Var * v_posId;
		Var * v_batchId;
		Var * v_outId;

		Var * v_addr_in;
		Var * s_addr_wei;
		Var * s_addr_bias;
		Var * v_addr_bias;
		Var * v_addr_out;
		Var * s_addr_sig;
		Var * v_addr_out_back;

		Var * v_in_buff_a;
		Var * v_in_buff_b;
		Var * s_wei_buff_a;
		Var * s_wei_buff_b;
		Var * v_acc_buff;
		Var * s_prefetch;

		Var * s_exec_save;

		bool en_slop_zero = true;
		
		void writeProgram()
		{
			calcuIndex();
			main_conv();

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
		void calcuIndex();
		void calcuBlkIndex();
		void calcuPosIndex();
		void calcuOffset();

		/************************************************************************************/
		/* 数据读取与存储																		*/
		/************************************************************************************/
		void load_input(Var * in_buff);
		void load_weight(Var * wei_buff);
		void prefetch_weight();
		void init_output();
		void save_result();
		void save_without_atomic();
		void save_with_atomic(int n, Var * addr_out, Var * accum);
		void save_with_slop_zero();
	};
}
