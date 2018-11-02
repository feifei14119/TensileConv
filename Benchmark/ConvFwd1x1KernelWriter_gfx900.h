#pragma once

#include "ConvFwd1x1Config.h"
#include "KernelWriter.h"

namespace AutoGen
{
	class KernelWriterConv1x1 :public AutoGen::KernelWriter
	{
	public:
		KernelWriterConv1x1(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg)
			:KernelWriter(probCfg, solCfg)
		{
			extProbCfg = (T_ExtConvFwd1x1ProblemConfig *)problemConfig->extConfig;
			extSolCfg = (T_ExtConvFwd1x1SolutionConfig *)solutionConfig->extConfig;

			in_chan_stride = extProbCfg->W * extProbCfg->H;
			in_batch_stride = extProbCfg->W * extProbCfg->H * extProbCfg->C;
			wei_chan_stride = extProbCfg->C;
			out_chan_stride = extProbCfg->W * extProbCfg->H;
			out_batch_stride = extProbCfg->W * extProbCfg->H * extProbCfg->K;

			c_in_maps_once = extSolCfg->c_in_maps_once;
			c_in_maps = extSolCfg->c_in_maps;
			c_in_group = extSolCfg->c_in_group;
			k_out_maps = extSolCfg->k_out_maps;
			k_out_group = extSolCfg->k_out_group;

			wave_per_group = solutionConfig->l_wk0 / WAVE_SIZE;
			conv_loop = c_in_maps / c_in_maps_once / 2;

			en_input_offset = ((extProbCfg->W <= 28)&&(IsaArch == E_IsaArch::Gfx900));
			offset_grp_num = c_in_maps_once * 2 / 2;
			en_wei_addr_offset = true;
		}

	protected:
		T_ExtConvFwd1x1ProblemConfig * extProbCfg;	// 当前正在处理的问题扩展配置
		T_ExtConvFwd1x1SolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置

		typedef enum
		{
			PING_FLAG = 1,
			PANG_FLAG = 2
		}E_PingPang;
		
		int c_in_maps_once;
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
		Var * s_ptr_out;

		Var * v_pixBlkId;		// grp_id0_faked
		Var * v_cInBlkId;		// in_grp_block
		Var * v_kOutBlkId;		// out_grp_block
		Var * v_posId;
		Var * v_batchId;
		Var * v_outId;

		Var * v_addr_in;
		Var * s_addr_wei;
		Var * v_addr_out;

		Var * v_in_buff_a;
		Var * v_in_buff_b;
		Var * s_wei_buff_a;
		Var * s_wei_buff_b;
		Var * v_acc_buff;
		Var * s_prefetch;

		Var * s_exec_save;

		void writeProgram()
		{
			calcuIndex();
			main_conv();

			clrVar();
		}

		/************************************************************************************/
		/* 卷积主体																			*/
		/************************************************************************************/
		void main_conv()
		{
			// -------------------------------------------------------------------------------
			// 数据存储区域声明:
			// -------------------------------------------------------------------------------
			v_in_buff_a = newVgpr("v_in_a", c_in_maps_once);
			v_in_buff_b = newVgpr("v_in_b", c_in_maps_once);
			s_wei_buff_a = newSgpr("s_wei_a", c_in_maps_once, 8);
			s_wei_buff_b = newSgpr("s_wei_b", c_in_maps_once, 8);
			v_acc_buff = newVgpr("v_accum", k_out_maps);
			s_prefetch = newSgpr("s_prefetch", k_out_maps);

			// -------------------------------------------------------------------------------
			// 卷积计算:
			// -------------------------------------------------------------------------------
			// 初始化:
			init_output();

			// 循环填充
			wei_offset = 0;
			prefetch_weight();
			load_input(v_in_buff_a);

			// 循环体
			if (conv_loop > 1)
			{
				Var * s_loop_cnt = newSgpr("s_loop_cnt");
				f_s_loop(s_loop_cnt, conv_loop - 1, "CONV_LOOP");
				{
					load_input(v_in_buff_b);
					conv_one_loop(v_in_buff_a, false);
					load_input(v_in_buff_a);
					conv_one_loop(v_in_buff_b, true);
				}
				f_e_loop(s_loop_cnt, "CONV_LOOP");
			}

			// 循环排空
			if (conv_loop > 0)
			{
				load_input(v_in_buff_b);
				conv_one_loop(v_in_buff_a, false);
				conv_last_loop(v_in_buff_b);
			}
			else
			{
				wei_offset -= (c_in_maps_once - wei_chan_stride * k_out_maps) * 4;
				conv_last_loop(v_in_buff_a);
			}

			// -------------------------------------------------------------------------------
			// 存储结果:
			// -------------------------------------------------------------------------------
			save_result();
			
			delVar(v_in_buff_a);
			delVar(v_in_buff_b);
			delVar(s_wei_buff_a);
			delVar(s_wei_buff_b);
			delVar(v_acc_buff);
			delVar(s_prefetch);
		}
		void conv_one_loop(Var * in_buff, bool is_pang_buff)
		{
			// 调整weight buff偏移量
			if (is_pang_buff == true)
			{
				wei_offset += (c_in_maps_once - wei_chan_stride * k_out_maps) * 4;
			}
			else
			{
				wei_offset = 0;
			}

			load_weight(s_wei_buff_a);
			s_wait_lgkmcnt(0);				// wait s_wei_buff_a
			load_weight(s_wei_buff_b);
			s_wait_vmcnt(8);				// wait in_buff
			conv_one_accum(in_buff, s_wei_buff_a, *v_acc_buff + 0);

			int loop = k_out_maps / 2 - 1;
			for (int i = 0; i < loop; i++)
			{
				s_wait_lgkmcnt(0);			// wait s_wei_buff_b
				load_weight(s_wei_buff_a);
				conv_one_accum(in_buff, s_wei_buff_b, *v_acc_buff + (i * 2 + 1));
				s_wait_lgkmcnt(0);			// wait s_wei_buff_a
				load_weight(s_wei_buff_b);
				conv_one_accum(in_buff, s_wei_buff_a, *v_acc_buff + (i * 2 + 2));
			}

			// 调整input buff地址
			if (is_pang_buff == true)
			{
				op3("s_add_u32", s_addr_wei, s_addr_wei, c_in_maps_once * 2 * 4);
				op3("s_addc_u32", *s_addr_wei + 1, *s_addr_wei + 1, 0);
			}

			s_wait_lgkmcnt(0);			// wait s_wei_buff_b
			// 为下一轮循环预读取
			if (is_pang_buff == true)
			{
				prefetch_weight();
			}
			conv_one_accum(in_buff, s_wei_buff_b, *v_acc_buff + (k_out_maps - 1));
		}
		void conv_last_loop(Var * in_buff)
		{
			// 调整weight buff偏移量
			wei_offset += (c_in_maps_once - wei_chan_stride * k_out_maps) * 4;

			load_weight(s_wei_buff_a);
			s_wait_lgkmcnt(0);				// wait s_wei_buff_a
			load_weight(s_wei_buff_b);
			s_wait_vmcnt(0);				// wait in_buff
			conv_one_accum(in_buff, s_wei_buff_a, *v_acc_buff + 0);

			int loop = k_out_maps / 2 - 1;
			for (int i = 0; i < loop; i++)
			{
				s_wait_lgkmcnt(0);			// wait s_wei_buff_b
				load_weight(s_wei_buff_a);
				conv_one_accum(in_buff, s_wei_buff_b, *v_acc_buff + (i * 2 + 1));
				s_wait_lgkmcnt(0);			// wait s_wei_buff_a
				load_weight(s_wei_buff_b);
				conv_one_accum(in_buff, s_wei_buff_a, *v_acc_buff + (i * 2 + 2));
			}

			s_wait_lgkmcnt(0);				// wait s_wei_buff_b
			conv_one_accum(in_buff, s_wei_buff_b, *v_acc_buff + (k_out_maps - 1));
		}
		void conv_one_accum(Var * in_buff, Var * wei_buff, Var * accum)
		{
			for (int i = 0; i < c_in_maps_once; i++)
			{
				// debug
				//op2("v_mov_b32", *in_buff + i, 1.23);

				op3("v_mac_f32", accum, *in_buff + i, *wei_buff + i);
			}
		}

		/************************************************************************************/
		/* 计算下标																			*/
		/************************************************************************************/
		void calcuIndex()
		{
			s_ptr_in = newSgpr("s_ptr_in", 2, 2);
			s_ptr_wei = newSgpr("s_ptr_wei", 2, 2);
			s_ptr_out = newSgpr("s_ptr_out", 2, 2);

			v_pixBlkId = newVgpr("v_pixBlkId");
			v_cInBlkId = newVgpr("v_cInBlkId");
			v_kOutBlkId = newVgpr("v_kOutBlkId");
			v_posId = newVgpr("v_posId");
			v_batchId = newVgpr("v_batchId");
			v_outId = newVgpr("v_outId");

			v_addr_in = newVgpr("v_addr_in", 2, 2);
			s_addr_wei = newSgpr("s_addr_wei", 2, 2);
			v_addr_out = newVgpr("v_addr_out", 2, 2);

			if (en_input_offset == true)
			{
				v_global_offset = newVgpr("v_global_offset", c_in_maps_once * 2 / 2, 2);
			}

			// -------------------------------------------------------------------------------
			s_load_dword(2, s_ptr_in, s_kernelArg, 0x00);
			s_load_dword(2, s_ptr_wei, s_kernelArg, 0x08);
			s_load_dword(2, s_ptr_out, s_kernelArg, 0x10);

			calcuBlkIndex();
			calcuPosIndex();
			calcuOffset();
			// -------------------------------------------------------------------------------

			if (en_input_offset == false)
			{
				delVar(s_ptr_in);
			}
			delVar(s_ptr_wei);
			delVar(s_ptr_out);

			delVar(v_pixBlkId);
			if (c_in_group == 1)
			{
				delVar(v_cInBlkId);
			}
			delVar(v_kOutBlkId);
			delVar(v_posId);
			delVar(v_batchId);
			delVar(v_outId);
		}
		void calcuBlkIndex()
		{
			Var * s_tmp1 = newSgpr("s_tmp1");
			Var * v_tmp1 = newVgpr("v_tmp1");
			Var * v_tmp2 = newVgpr("v_tmp2");

			// -------------------------------------------------------------------------------
			// pixBlkId = (gid_x * wave_per_group + tid_x / WAVE_SIZE) / k_out_group / c_in_group;
			// cInBlkId = (gid_x * wave_per_group + tid_x / WAVE_SIZE) / k_out_group % c_in_group;
			// kOutBlkId = (gid_x * wave_per_group + tid_x / WAVE_SIZE) % k_out_group;
			// -------------------------------------------------------------------------------
			op3("s_lshl_b32", s_tmp1, s_gid_x, log2(wave_per_group));					// s_tmp1 = gid_x * block_per_group
			op3("v_lshrrev_b32", v_tmp1, log2(WAVE_SIZE), v_tid_x);						// v_tmp1 = tid_x / 
			//isa->inst2("v_readfirstlane_b32", s[s_block_id], vgpr(v_tmp1), "");
			op4("v_add_co_u32", v_tmp1, "vcc", v_tmp1, s_tmp1);							// v_tmp1 = (gid * FIX_BLK_PER_GROUP + tid / FIX_BLK_SIZE)
			op3("v_lshrrev_b32", v_tmp2, log2(k_out_group), v_tmp1);					// v_tmp2 = (gid_x * wave_per_group + tid_x / WAVE_SIZE) / k_out_group
			op3("v_lshrrev_b32", v_pixBlkId, log2(c_in_group), v_tmp2);					// v_pixBlkId = pixBlkId
			op3("v_and_b32", v_cInBlkId, modMask(c_in_group), v_tmp2);					// v_cInBlkId = cInBlkId
			op3("v_and_b32", v_kOutBlkId, modMask(k_out_group), v_tmp1);				// v_kOutBlkId = kOutBlkId

			delVar(s_tmp1);
			delVar(v_tmp1);
			delVar(v_tmp2);
		}
		void calcuPosIndex()
		{
			Var * v_tmp1 = newVgpr("v_tmp1");
			Var * v_tmp2 = newVgpr("v_tmp2");

			// -------------------------------------------------------------------------------
			// pos_id   = (pixBlkId * WAVE_SIZE + tid_x % WAVE_SIZE) % in_chan_stride;
			// batch_id = (pixBlkId * WAVE_SIZE + tid_x % WAVE_SIZE) / in_chan_stride;
			// out_id   = kOutBlkId * k_out_maps;
			// -------------------------------------------------------------------------------
			op3("v_lshlrev_b32", v_tmp1, log2(WAVE_SIZE), v_pixBlkId);
			op3("v_and_b32", v_tmp2, modMask(WAVE_SIZE), v_tid_x);
			op4("v_add_co_u32", v_tmp1, "vcc", v_tmp2, v_tmp1);							// v_tmp1 = (pixBlkId * WAVE_SIZE + tid_x % WAVE_SIZE)
			op2("v_mov_b32", v_tmp2, in_chan_stride);
			fv_div_u32(v_tmp1, v_tmp2, v_batchId, v_posId);								// v_batchId = batch_id; v_posId = pos
			op3("v_lshlrev_b32", v_outId, log2(k_out_maps), v_kOutBlkId);				// v_outId = out_id

			// -------------------------------------------------------------------------------
			// if (batch_id >= BATCHSIZE)
			//		return;
			// -------------------------------------------------------------------------------
			op2("v_mov_b32", v_tmp1, extProbCfg->N);
			op3("v_cmpx_lt_u32", "exec", v_batchId, v_tmp1);

			delVar(v_tmp1);
			delVar(v_tmp2);
		}
		void calcuOffset()
		{
			Var * s_tmp1 = newSgpr("s_tmp1");
			Var * v_tmp1 = newVgpr("v_tmp1");
			Var * v_tmp2 = newVgpr("v_tmp2");
			Var * v_tmp3 = newVgpr("v_tmp3");

			// -------------------------------------------------------------------------------
			// gbl_in_off  = (batch_id * in_batch_stride) + (cInBlkId * c_in_maps * in_chan_stride) + pos_id;
			// -------------------------------------------------------------------------------
			op2("v_mov_b32", v_tmp1, in_batch_stride);
			op3("v_mul_u32_u24", v_tmp1, v_batchId, v_tmp1);						// v_tmp1 = (batch_id * in_batch_stride)
			op3("v_lshlrev_b32", v_tmp2, log2(c_in_maps), v_cInBlkId);
			op2("v_mov_b32", v_tmp3, in_chan_stride);
			op3("v_mul_u32_u24", v_tmp2, v_tmp2, v_tmp3);							// v_tmp2 = (cInBlkId * c_in_maps * in_chan_stride)
			op4("v_add3_u32", v_tmp3, v_tmp1, v_tmp2, v_posId);						// v_tmp3 = gbl_in_off
			op3("v_lshlrev_b32", v_tmp3, 2, v_tmp3);								// v_tmp3 = gbl_in_off (BYTE)

			if (en_input_offset == true)
			{
				op2("v_mov_b32", v_global_offset, v_tmp3);
				op2("v_mov_b32", v_tmp1, in_chan_stride * 2 * 4);
				for (int i = 0; i < offset_grp_num; i++)
				{
					op4("v_add_co_u32", *v_global_offset + 2 * (i + 1), "vcc", *v_global_offset + 2 * i, v_tmp1);
				}
			}
			else
			{
				s_wait_lgkmcnt(0);
				op2("v_mov_b32", *v_addr_in + 1, *s_ptr_in + 1);
				op4("v_add_co_u32", v_addr_in, "vcc", s_ptr_in, v_tmp3);
				op5("v_addc_co_u32", *v_addr_in + 1, "vcc", 0, *v_addr_in + 1, "vcc");
			}

			// -------------------------------------------------------------------------------
			// wei_off = (out_id * wei_chan_stride) + (cInBlkId * c_in_maps);
			// -------------------------------------------------------------------------------
			op2("v_mov_b32", v_tmp1, wei_chan_stride);
			op3("v_mul_u32_u24", v_tmp1, v_outId, v_tmp1);							// v_tmp1 = (out_id * wei_chan_stride)
			op3("v_lshlrev_b32", v_tmp2, log2(c_in_maps), v_cInBlkId);				// v_tmp2 = (cInBlkId * c_in_maps)
			op3("v_add_u32", v_tmp1, v_tmp1, v_tmp2);							
			op2("v_readfirstlane_b32", s_tmp1, v_tmp1);								// s_tmp1 = wei_off
			op3("s_lshl_b32", s_tmp1, s_tmp1, 2);									// s_tmp1 = wei_off (BYTE)
			s_wait_lgkmcnt(0);
			op3("s_add_u32", s_addr_wei, s_ptr_wei, s_tmp1);
			op3("s_addc_u32", *s_addr_wei + 1, 0, *s_ptr_wei + 1);

			// -------------------------------------------------------------------------------
			// gbl_out_off = (batch_id * out_batch_stride) + (out_id * out_chan_stride) + pos_id;
			// -------------------------------------------------------------------------------
			op2("v_mov_b32", v_tmp1, out_batch_stride);
			op3("v_mul_u32_u24", v_tmp1, v_batchId, v_tmp1);						// v_tmp1 = (batch_id * out_batch_stride)
			op2("v_mov_b32", v_tmp2, out_chan_stride);
			op3("v_mul_u32_u24", v_tmp2, v_outId, v_tmp2);							// v_tmp2 = (out_id * out_chan_stride)
			op4("v_add3_u32", v_tmp3, v_tmp1, v_tmp2, v_posId);						// v_tmp3 = gbl_out_off
			op3("v_lshlrev_b32", v_tmp3, 2, v_tmp3);								// v_tmp3 = gbl_out_off (BYTE)
			op2("v_mov_b32", *v_addr_out + 1, *s_ptr_out + 1);
			op4("v_add_co_u32", v_addr_out, "vcc", s_ptr_out, v_tmp3);
			op5("v_addc_co_u32", *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");

			delVar(s_tmp1);
			delVar(v_tmp1);
			delVar(v_tmp2);
			delVar(v_tmp3);
		}
		
		/************************************************************************************/
		/* 数据读取与存储																		*/
		/************************************************************************************/
		void load_input(Var * in_buff)
		{
			if (en_input_offset == true)
			{
				for (int i = 0; i < c_in_maps_once / 2; i++)
				{
					flat_load_dword(1, *(*in_buff + 2 * i) + 0, *v_global_offset + 2 * i, s_ptr_in);
					flat_load_dword(1, *(*in_buff + 2 * i) + 1, *v_global_offset + 2 * i, s_ptr_in, in_chan_stride * 4);
				}
				op3("s_add_u32", s_ptr_in, s_ptr_in, in_chan_stride * c_in_maps_once * 4);
				op3("s_addc_u32", *s_ptr_in + 1, 0, *s_ptr_in + 1);
			}
			else
			{
				for (int i = 0; i < c_in_maps_once; i++)
				{
					flat_load_dword(1, in_buff, v_addr_in, "off");
					op4("v_add_co_u32", v_addr_in, "vcc", in_chan_stride * 4, v_addr_in);
					op5("v_addc_co_u32", *v_addr_in + 1, "vcc", 0, *v_addr_in + 1, "vcc");
				}
			}
		}
		void load_weight(Var * wei_buff)
		{
			s_load_dword(8, wei_buff, s_addr_wei, wei_offset);
			wei_offset += wei_chan_stride * 4;
		}
		void prefetch_weight()
		{
			if (en_wei_addr_offset == true)
			{
				for (int i = 0; i < k_out_maps; i++)
				{
					s_load_dword(1, *s_prefetch + i, s_addr_wei, wei_chan_stride * i * 4);
				}
			}
			else
			{
				Var * s_tmp = newSgpr("s_tmp", 2, 2);

				op2("s_mov_b32", s_tmp, s_addr_wei);
				op2("s_mov_b32", *s_tmp + 1, *s_addr_wei + 1);

				for (int i = 0; i < k_out_maps; i++)
				{
					s_load_dword(1, *s_prefetch + i, s_addr_wei, 0);

					op3("s_add_u32", s_addr_wei, s_addr_wei, wei_chan_stride * 4);
					op3("s_addc_u32", *s_addr_wei + 1, *s_addr_wei + 1, 0);
				}

				op2("s_mov_b32", s_addr_wei, s_tmp);
				op2("s_mov_b32", *s_addr_wei + 1, *s_tmp + 1);

				delVar(s_tmp);
			}
		}
		void init_output()
		{
			for (int i = 0; i < k_out_maps; i++)
				op2("v_mov_b32", *v_acc_buff + i, 0);

			if (c_in_group > 1)
			{
				Var * s_cInBlkId = newSgpr("s_cInBlkId");
				Var * v_addr_save = newVgpr("v_addr_tmp", 4, 2);
				Var * l_end_init = newLaber("END_INIT");

				op2("v_readfirstlane_b32", s_cInBlkId, v_cInBlkId);
				op2("s_cmpk_eq_i32", s_cInBlkId, 0);
				op1("s_cbranch_scc0", l_end_init);
				op2("v_mov_b32", v_addr_save, v_addr_out);
				op2("v_mov_b32", *v_addr_save + 1, *v_addr_out + 1);

				for (int i = 0; i < k_out_maps; i++)
				{
					flat_store_dword(1, v_addr_out, *v_acc_buff + i, "off");
					op4("v_add_co_u32", v_addr_out, "vcc", out_chan_stride * 4, v_addr_out);
					op5("v_addc_co_u32", *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");
				}
				op2("v_mov_b32", v_addr_out, v_addr_save);
				op2("v_mov_b32", *v_addr_out + 1, *v_addr_save + 1);
								
				wrLaber(l_end_init);
				delVar(s_cInBlkId);
				delVar(v_addr_save);
			}
		}
		void save_result()
		{
			if (c_in_group == 1)
			{
				save_without_atomic();
			}
			else
			{
				s_exec_save = newSgpr("s_exec_save", 2, 2);
				op2("s_mov_b64", *s_exec_save ^ 2, "exec");

				for (int i = 0; i < k_out_maps; i++)
				{
					save_with_atomic(i, v_addr_out, v_acc_buff);
				}
				delVar(s_exec_save);
			}
		}
		void save_without_atomic()
		{
			for (int i = 0; i < k_out_maps; i++)
			{
				// debug
				//op2("v_mov_b32", *v_acc_buff + i, 1.234);
				//op2("v_cvt_f32_u32", *v_acc_buff + i, *v_acc_buff + i);

				flat_store_dword(1, v_addr_out, *v_acc_buff + i, "off");
				op4("v_add_co_u32", v_addr_out, "vcc", out_chan_stride * 4, v_addr_out);
				op5("v_addc_co_u32", *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");
			}

		}
		void save_with_atomic(int n, Var * addr_out, Var * accum)
		{
			// v_newVal = v_src_cmp
			// v_prevVal = v_src_cmp + 1
			Var * v_src_cmp = newVgpr("v_src_cmp", 2, 2);
			Var * v_rtn = newVgpr("v_rtn");

			Var * l_atomic_add = newLaber("ATOMIC_ADD_" + d2s(n));			
			wrLaber(l_atomic_add);

			flat_load_dword(1, *v_src_cmp + 1, addr_out, "off", 0, true);
			s_wait_vmcnt(0);
			op3("v_add_f32", v_src_cmp, *v_src_cmp + 1, accum);
			op4("global_atomic_cmpswap", v_rtn, *addr_out ^ 2, *v_src_cmp ^ 2, "off", true);
			s_wait_vmcnt(0);
			op3("v_cmpx_neq_f32", "vcc", *v_src_cmp + 1, v_rtn);
			op2("v_mov_b32", *v_src_cmp + 1, v_rtn);
			op1("s_cbranch_execnz", l_atomic_add);
			op0("s_barrier");
			op2("s_mov_b64", "exec", *s_exec_save ^ 2);
			op4("v_add_co_u32", v_addr_out, "vcc", out_chan_stride * 4, v_addr_out);
			op5("v_addc_co_u32", *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");

			delVar(v_src_cmp);
			delVar(v_rtn);
		}
		
	};
}
