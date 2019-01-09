#pragma once

#include "ConvFwd1x1KernelWriter.h"
#include "ConvFwd1x1.h"

using namespace TensileConv;
using namespace AutoGen;

KernelWriterConv1x1::KernelWriterConv1x1(ConvFwd1x1Problem * problem, ConvFwd1x1Solution * solution, E_IsaArch isaArch)
	:KernelWriter(solution, isaArch)
{
	this->problem = problem;
	this->solution = solution;

	in_chan_stride = problem->W() * problem->H();
	in_batch_stride = problem->W() * problem->H() * problem->C();
	wei_chan_stride = problem->C();
	out_chan_stride = problem->W() * problem->H();
	out_batch_stride = problem->W() * problem->H() * problem->K();

	kernelParam = solution->KernelParam();
	c_in_maps = kernelParam.c_in_maps;
	c_in_group = kernelParam.c_in_group;
	c_in_lds_group = kernelParam.c_in_lds_group;
	k_out_maps = kernelParam.k_out_maps;
	k_out_group = kernelParam.k_out_group;

	c_in_maps_once = kernelParam.c_in_maps_once;
	if (c_in_maps_once <= c_in_maps / unroll_time)
		c_in_maps_once_real = c_in_maps_once;
	else
		c_in_maps_once_real = c_in_maps / unroll_time;
	conv_loop = c_in_maps / c_in_maps_once_real / 2;
			
	wave_per_group = solution->GroupSize().x / WAVE_SIZE;

	en_input_offset = ((problem->W() <= 28) && (IsaArch == E_IsaArch::Gfx900));
	offset_grp_num = c_in_maps_once_real * 2 / 2;
	en_wei_addr_offset = true;
}

/************************************************************************************/
/* 卷积主体																			*/
/************************************************************************************/
void KernelWriterConv1x1::main_conv()
{
	// -------------------------------------------------------------------------------
	// 数据存储区域声明:
	// -------------------------------------------------------------------------------
	v_in_buff_a = newVgpr("v_in_a", c_in_maps_once_real);
	v_in_buff_b = newVgpr("v_in_b", c_in_maps_once_real);
	s_wei_buff_a = newSgpr("s_wei_a", c_in_maps_once_real, c_in_maps_once_real);
	s_wei_buff_b = newSgpr("s_wei_b", c_in_maps_once_real, c_in_maps_once_real);
	v_acc_buff = newVgpr("v_accum", k_out_maps, k_out_maps);
	s_prefetch = newSgpr("s_prefetch", k_out_maps+1);

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
		wei_offset -= (c_in_maps_once_real - wei_chan_stride * k_out_maps) * 4;
		conv_last_loop(v_in_buff_a);
	}

	// -------------------------------------------------------------------------------
	// 存储结果:
	// -------------------------------------------------------------------------------
	save_result();

	if (en_input_offset == true)
	{
		delVar(v_global_offset);
	}
	else
	{
		delVar(v_addr_in);
	}
	delVar(s_addr_wei);
	if (problem->EnBias() == true)
	{
		//delVar(s_addr_bias);
		delVar(v_addr_bias);
	}
	if (problem->EnRelu() == true)
	{
		delVar(s_slop);
	}
	delVar(v_addr_out);
	if (c_in_lds_group > 1)
	{
		delVar(v_lds_addr);
	}

	delVar(v_in_buff_a);
	delVar(v_in_buff_b);
	delVar(s_wei_buff_a);
	delVar(s_wei_buff_b);
	delVar(v_acc_buff);
	delVar(s_prefetch);
}
void KernelWriterConv1x1::conv_one_loop(Var * in_buff, bool is_pang_buff)
{
	// 调整weight buff偏移量
	if (is_pang_buff == true)
	{
		wei_offset += (c_in_maps_once_real - wei_chan_stride * k_out_maps) * 4;
	}
	else
	{
		wei_offset = 0;
	}

	load_weight(s_wei_buff_a);
	s_wait_lgkmcnt(0);				// wait s_wei_buff_a
	load_weight(s_wei_buff_b);
	s_wait_vmcnt(c_in_maps_once_real);				// wait in_buff
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

	// 调整weight地址
	if (is_pang_buff == true)
	{
		op3("s_add_u32", s_addr_wei, s_addr_wei, c_in_maps_once_real * 2 * 4);
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
void KernelWriterConv1x1::conv_last_loop(Var * in_buff)
{
	// 调整weight buff偏移量
	wei_offset += (c_in_maps_once_real - wei_chan_stride * k_out_maps) * 4;

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
void KernelWriterConv1x1::conv_one_accum(Var * in_buff, Var * wei_buff, Var * accum)
{
	for (int i = 0; i < c_in_maps_once_real; i++)
	{
		// debug
		//op2("v_mov_b32", *in_buff + i, 1.000000001);
		//op2("s_mov_b32", *wei_buff + i, 1.0000000001);

		op3("v_mac_f32", accum, *in_buff + i, *wei_buff + i);
	}
}

/************************************************************************************/
/* 计算下标																			*/
/************************************************************************************/
void KernelWriterConv1x1::calcuIndex()
{
	s_ptr_in = newSgpr("s_ptr_in", 2, 2);
	s_ptr_wei = newSgpr("s_ptr_wei", 2, 2);
	if (problem->EnBias() == true)
	{
		s_ptr_bias = newSgpr("s_ptr_bias", 2, 2);
	}
	s_ptr_out = newSgpr("s_ptr_out", 2, 2);
	if (problem->EnRelu() == true)
	{
		s_slop = newSgpr("s_slop");
		if (en_slop_zero == true)
		{
			s_ptr_sig = newSgpr("s_ptr_sig", 2, 2);
		}
	}
#if KERNEL_DEBUG
	s_ptr_dbg = newSgpr("s_ptr_dbg", 2, 2);
#endif

	// -------------------------------------------------------------------------------
	v_waveId = newVgpr("v_waveId");
	v_tidInWave = newVgpr("v_tidInWave");
	v_pixBlkId = newVgpr("v_pixBlkId");
	v_cInBlkId = newVgpr("v_cInBlkId");
	v_kOutBlkId = newVgpr("v_kOutBlkId");
	v_posId = newVgpr("v_posId");
	v_batchId = newVgpr("v_batchId");
	v_outId = newVgpr("v_outId");

	// -------------------------------------------------------------------------------
	if (en_input_offset == true)
	{
		v_global_offset = newVgpr("v_global_offset", c_in_maps_once_real * 2 / 2, 2);
	}
	else
	{
		v_addr_in = newVgpr("v_addr_in", 2, 2);
	}
	s_addr_wei = newSgpr("s_addr_wei", 2, 2);
	if (problem->EnBias() == true)
	{
		v_addr_bias = newVgpr("v_addr_bias", 2, 2);
	}
	if ((problem->EnRelu() == true)&&(en_slop_zero == true))
	{
		s_addr_sig = newSgpr("s_addr_sig", 2, 2);
	}
	v_addr_out = newVgpr("v_addr_out", 2, 2);
	if (c_in_lds_group > 1)
	{
		v_lds_addr = newVgpr("v_ds_addr");
	}
#if KERNEL_DEBUG
	v_addr_dbg = newVgpr("v_addr_dbg", 2, 2);
#endif

	// -------------------------------------------------------------------------------
	s_load_dword(2, s_ptr_in, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_wei, s_kernelArg, 0x08);
	if (problem->EnBias() == true)
	{
		s_load_dword(2, s_ptr_bias, s_kernelArg, 0x10);
	}
	s_load_dword(2, s_ptr_out, s_kernelArg, 0x18);
	if ((problem->EnRelu() == true) && (en_slop_zero == true))
	{
		s_load_dword(2, s_ptr_sig, s_kernelArg, 0x20);
	}
	if (problem->EnRelu() == true)
	{
		s_load_dword(1, s_slop, s_kernelArg, 0x28);
	}
#if KERNEL_DEBUG
	s_load_dword(2, s_ptr_dbg, s_kernelArg, 0x30);
	f_linear_addr_2d(s_ptr_dbg, v_addr_dbg);
	save_debug();
#endif

	// -------------------------------------------------------------------------------
	calcuBlkIndex();
	calcuPosIndex();
	calcuOffset();

	// -------------------------------------------------------------------------------
	if (en_input_offset == false)
	{
		delVar(s_ptr_in);
	}
	delVar(s_ptr_wei);
	if (problem->EnBias() == true)
	{
		delVar(s_ptr_bias);
	}
	delVar(s_ptr_out);
#if KERNEL_DEBUG
	delVar(s_ptr_dbg);
#endif

	delVar(v_waveId);
	delVar(v_tidInWave);
	delVar(v_pixBlkId);
	if (c_in_group == 1)
	{
		delVar(v_cInBlkId);
	}
	delVar(v_kOutBlkId);
	delVar(v_posId);
	delVar(v_batchId);
	delVar(v_outId);
	if ((problem->EnRelu() == true) && (en_slop_zero == true))
	{
		delVar(s_ptr_sig);
	}
}
void KernelWriterConv1x1::calcuBlkIndex()
{
	Var * s_tmp1 = newSgpr("s_tmp1");
	Var * v_tmp1 = newVgpr("v_tmp1");
	Var * v_tmp2 = newVgpr("v_tmp2");

	// -------------------------------------------------------------------------------
	// group_id = group_id_x
	// waveId = ((group_sz.x / WAVE_SIZE) * group_sz.y * group_id) + (tid_x / WAVE_SIZE)
	// tidInWave = tid_x % WAVE_SIZE;
	// -------------------------------------------------------------------------------
	op3("s_lshl_b32", s_tmp1, s_gid_x, log2(wave_per_group));					// s_tmp1 = gid_x * block_per_group
	op3("v_lshrrev_b32", v_tmp1, log2(WAVE_SIZE), v_tid_x);						// v_tmp1 = tid_x / WAVE_SIZE
	op4(v_addc_u32, v_waveId, "vcc", v_tmp1, s_tmp1);							// v_waveId = waveId
	op3("v_and_b32", v_tidInWave, modMask(WAVE_SIZE), v_tid_x);					// v_tidInWave = tidInWave

	// -------------------------------------------------------------------------------
	// pixBlkId = waveId / k_out_group / c_in_group;
	// cInBlkId = waveId / k_out_group % c_in_group;
	// kOutBlkId = waveId % k_out_group;
	// -------------------------------------------------------------------------------
	op3("v_lshrrev_b32", v_tmp2, log2(k_out_group), v_waveId);					// v_tmp2 = waveId / k_out_group
	op3("v_lshrrev_b32", v_pixBlkId, log2(c_in_group), v_tmp2);					// v_pixBlkId = pixBlkId
	op3("v_and_b32", v_cInBlkId, modMask(c_in_group), v_tmp2);					// v_cInBlkId = cInBlkId
	op3("v_and_b32", v_kOutBlkId, modMask(k_out_group), v_waveId);				// v_kOutBlkId = kOutBlkId

	delVar(s_tmp1);
	delVar(v_tmp1);
	delVar(v_tmp2);
}
void KernelWriterConv1x1::calcuPosIndex()
{
	Var * v_tmp1 = newVgpr("v_tmp1");
	Var * v_tmp2 = newVgpr("v_tmp2");

	// -------------------------------------------------------------------------------
	// pos_id   = (pixBlkId * WAVE_SIZE + tidInWave) % in_chan_stride;
	// batch_id = (pixBlkId * WAVE_SIZE + tidInWave) / in_chan_stride;
	// out_id   = kOutBlkId * k_out_maps;
	// -------------------------------------------------------------------------------
	op3("v_lshlrev_b32", v_tmp1, log2(WAVE_SIZE), v_pixBlkId);
	op4(v_addc_u32, v_tmp1, "vcc", v_tidInWave, v_tmp1);						// v_tmp1 = (pixBlkId * WAVE_SIZE + tidInWave)
	op2("v_mov_b32", v_tmp2, in_chan_stride);
	fv_div_u32(v_tmp1, v_tmp2, v_batchId, v_posId);								// v_batchId = batch_id; v_posId = pos
	op3("v_lshlrev_b32", v_outId, log2(k_out_maps), v_kOutBlkId);				// v_outId = out_id

	// -------------------------------------------------------------------------------
	// if (batch_id >= extProbCfg->N)
	//		return;
	// -------------------------------------------------------------------------------
	op2("v_mov_b32", v_tmp1, problem->N());
	op3("v_cmpx_lt_u32", "vcc", v_batchId, v_tmp1); 
	//op1("s_nop", 5);
	op1("s_cbranch_execz", l_end_prg);


	delVar(v_tmp1);
	delVar(v_tmp2);
}
void KernelWriterConv1x1::calcuOffset()
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

	if (IsaArch == E_IsaArch::Gfx800)
	{
		op4(v_addc_u32, v_tmp3, "vcc", v_tmp1, v_tmp2);
		//op1("s_nop", 1);
		op5(v_addc_co_u32, v_tmp3, "vcc", v_tmp3, v_posId, "vcc");
		//op1("s_nop", 1);
	}
	else if (IsaArch == E_IsaArch::Gfx900)
	{
		op4("v_add3_u32", v_tmp3, v_tmp1, v_tmp2, v_posId);						// v_tmp3 = gbl_in_off
	}
	op3("v_lshlrev_b32", v_tmp3, 2, v_tmp3);								// v_tmp3 = gbl_in_off (BYTE)

	if (en_input_offset == true)
	{
		op2("v_mov_b32", v_global_offset, v_tmp3);
		op2("v_mov_b32", v_tmp1, in_chan_stride * 2 * 4);
		for (int i = 0; i < offset_grp_num; i++)
		{
			op4(v_addc_u32, *v_global_offset + 2 * (i + 1), "vcc", *v_global_offset + 2 * i, v_tmp1);
		}
	}
	else
	{
		s_wait_lgkmcnt(0);
		op2("v_mov_b32", *v_addr_in + 1, *s_ptr_in + 1);
		op4(v_addc_u32, v_addr_in, "vcc", s_ptr_in, v_tmp3);
		//op1("s_nop", 5);
		op5(v_addc_co_u32, *v_addr_in + 1, "vcc", 0, *v_addr_in + 1, "vcc");
		//op1("s_nop", 5);
	}

	// -------------------------------------------------------------------------------
	// wei_off = (out_id * wei_chan_stride) + (cInBlkId * c_in_maps);
	// -------------------------------------------------------------------------------
	op2("v_mov_b32", v_tmp1, wei_chan_stride);
	op3("v_mul_u32_u24", v_tmp1, v_outId, v_tmp1);							// v_tmp1 = (out_id * wei_chan_stride)
	op3("v_lshlrev_b32", v_tmp2, log2(c_in_maps), v_cInBlkId);				// v_tmp2 = (cInBlkId * c_in_maps)
	op4(v_addc_u32, v_tmp1, "vcc", v_tmp1, v_tmp2);
	op2("v_readfirstlane_b32", s_tmp1, v_tmp1);								// s_tmp1 = wei_off
	//op1("s_nop", 5);			
	op3("s_lshl_b32", s_tmp1, s_tmp1, 2);									// s_tmp1 = wei_off (BYTE)
	s_wait_lgkmcnt(0);
	op3("s_add_u32", s_addr_wei, s_ptr_wei, s_tmp1);
	op3("s_addc_u32", *s_addr_wei + 1, 0, *s_ptr_wei + 1);

	// -------------------------------------------------------------------------------
	// bias_off = out_id
	// -------------------------------------------------------------------------------
	if (problem->EnBias() == true)
	{
		//op3("s_add_u32", s_addr_bias, s_ptr_bias, s_tmp1);
		//op3("s_addc_u32", *s_addr_bias + 1, 0, *s_ptr_bias + 1);
		op3("v_lshlrev_b32", v_tmp1, 2, v_outId);
		op2("v_mov_b32", *v_addr_bias + 1, *s_ptr_bias + 1);
		op4(v_addc_u32, v_addr_bias, "vcc", s_ptr_bias, v_tmp1);
		op5(v_addc_co_u32, *v_addr_bias + 1, "vcc", 0, *v_addr_bias + 1, "vcc");
	}

	// -------------------------------------------------------------------------------
	// sig_off = pixBlkId * k_out_group + kOutBlkId
	// -------------------------------------------------------------------------------
	if ((problem->EnRelu() == true)&&(en_slop_zero == true))
	{
		op3("v_mul_u32_u24", v_tmp1, v_pixBlkId, k_out_group);				// v_tmp1 = pixBlkId * k_out_group
		op4(v_addc_u32, v_tmp1, "vcc", v_tmp1, v_kOutBlkId);
		op2("v_readfirstlane_b32", s_tmp1, v_tmp1);							// s_tmp1 = sig_off
		op3("s_lshl_b32", s_tmp1, s_tmp1, 2);								// s_tmp1 = sig_off (BYTE)
		s_wait_lgkmcnt(0);
		op3("s_add_u32", s_addr_sig, s_ptr_sig, s_tmp1);
		op3("s_addc_u32", *s_addr_sig + 1, 0, *s_ptr_sig + 1);
	}

	// -------------------------------------------------------------------------------
	// not use lds fp32 atomic add:
	// lds_addr = (tid_x / WAVE_SIZE) * WAVE_SIZE * k_out_maps + (tid_x % WAVE_SIZE)
	// using lds fp32 atomic add:
	// lds_addr = tid_x (cus lds has fp32 atomic add)
	// -------------------------------------------------------------------------------
	if (c_in_lds_group > 1)
	{
		//ldsByteCount += c_in_lds_group * WAVE_SIZE * k_out_maps * 4;	// 申请lds字节数
		ldsByteCount += WAVE_SIZE * k_out_maps * 4;	// 申请lds字节数
		op3("v_lshlrev_b32", v_lds_addr, 2, v_tid_x);
	}

	// -------------------------------------------------------------------------------
	// gbl_out_off = (batch_id * out_batch_stride) + (out_id * out_chan_stride) + pos_id;
	// -------------------------------------------------------------------------------
	op2("v_mov_b32", v_tmp1, out_batch_stride);
	op3("v_mul_u32_u24", v_tmp1, v_batchId, v_tmp1);						// v_tmp1 = (batch_id * out_batch_stride)
	op2("v_mov_b32", v_tmp2, out_chan_stride);
	op3("v_mul_u32_u24", v_tmp2, v_outId, v_tmp2);							// v_tmp2 = (out_id * out_chan_stride)

	if (IsaArch == E_IsaArch::Gfx800)
	{
		op4(v_addc_u32, v_tmp3, "vcc", v_tmp1, v_tmp2);
		//op1("s_nop", 1);
		op5(v_addc_co_u32, v_tmp3, "vcc", v_tmp3, v_posId, "vcc");
	}
	else if (IsaArch == E_IsaArch::Gfx900)
	{
		op4("v_add3_u32", v_tmp3, v_tmp1, v_tmp2, v_posId);					// v_tmp3 = gbl_out_off
	}
	op3("v_lshlrev_b32", v_tmp3, 2, v_tmp3);								// v_tmp3 = gbl_out_off (BYTE)
	op2("v_mov_b32", *v_addr_out + 1, *s_ptr_out + 1);
	op4(v_addc_u32, v_addr_out, "vcc", s_ptr_out, v_tmp3);
	//op1("s_nop", 1);
	op5(v_addc_co_u32, *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");

	delVar(s_tmp1);
	delVar(v_tmp1);
	delVar(v_tmp2);
	delVar(v_tmp3);
}

/************************************************************************************/
/* 数据读取与存储																		*/
/************************************************************************************/
void KernelWriterConv1x1::load_input(Var * in_buff)
{
	if (en_input_offset == true)
	{
		if (c_in_maps_once_real >= 2)
		{
			for (int i = 0; i < c_in_maps_once_real / 2; i++)
			{
				flat_load_dword(1, *(*in_buff + 2 * i) + 0, *v_global_offset + 2 * i, s_ptr_in);
				flat_load_dword(1, *(*in_buff + 2 * i) + 1, *v_global_offset + 2 * i, s_ptr_in, in_chan_stride * 4);
			}
		}
		else
		{
			flat_load_dword(1, in_buff, v_global_offset, s_ptr_in);
		}
		op3("s_add_u32", s_ptr_in, s_ptr_in, in_chan_stride * c_in_maps_once_real * 4);
		op3("s_addc_u32", *s_ptr_in + 1, 0, *s_ptr_in + 1);
	}
	else
	{
		if (c_in_maps_once_real >= 2)
		{
			for (int i = 0; i < c_in_maps_once_real; i++)
			{
				//op2("v_mov_b32", *in_buff + i, 2.01);
				flat_load_dword(1, *in_buff + i, v_addr_in, "off");
				//op1("s_nop", 5);
				op4(v_addc_u32, v_addr_in, "vcc", in_chan_stride * 4, v_addr_in);
				//op1("s_nop", 5);
				op5(v_addc_co_u32, *v_addr_in + 1, "vcc", 0, *v_addr_in + 1, "vcc");
				//op1("s_nop", 5);
			}
		}
		else
		{
			flat_load_dword(1, in_buff, v_addr_in, "off");
			op4(v_addc_u32, v_addr_in, "vcc", in_chan_stride * 4, v_addr_in);
			op5(v_addc_co_u32, *v_addr_in + 1, "vcc", 0, *v_addr_in + 1, "vcc");
		}
	}
}
void KernelWriterConv1x1::load_weight(Var * wei_buff)
{
	s_load_dword(c_in_maps_once_real, wei_buff, s_addr_wei, wei_offset);
	wei_offset += wei_chan_stride * 4;
}
void KernelWriterConv1x1::prefetch_weight()
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
void KernelWriterConv1x1::init_output()
{
	for (int i = 0; i < k_out_maps; i++)
		op2("v_mov_b32", *v_acc_buff + i, 0);

	if (c_in_group == 1)
	{
		if (problem->EnBias() == true)
		{
			//s_load_dword(k_out_maps, s_wei_buff_a, s_addr_bias, "off");
			for (int i = 0; i < k_out_maps; i++)
			{
				flat_load_dword(1, *v_acc_buff + i, v_addr_bias, "off");
				op4(v_addc_u32, v_addr_bias, "vcc", 4, v_addr_bias);
				op5(v_addc_co_u32, *v_addr_bias + 1, "vcc", 0, *v_addr_bias + 1, "vcc");
			}
		}
	}

	if (c_in_group > 1)
	{
		Var * s_cInBlkId = newSgpr("s_cInBlkId");
		Var * v_addr_save = newVgpr("v_addr_tmp", 4, 2);
		//Var * l_end_init = newLaber("END_INIT");
		Var * l_end_init = newLaber("SEG_2");
		Var * v_init = newVgpr("v_init");

		op2("v_readfirstlane_b32", s_cInBlkId, v_cInBlkId);
		//op1("s_nop", 5);
		op2("s_cmpk_eq_i32", s_cInBlkId, 0);
		op1("s_cbranch_scc0", l_end_init); 
		{
			if ((problem->EnRelu() == true) && (en_slop_zero == true))
			{
				Var * s_tmp = newSgpr("s_tmp");
				op2("s_mov_b32", s_tmp, 100);
				s_store_dword(1, s_tmp, s_addr_sig, 0, true);
				delVar(s_tmp);
			}

			op2("v_mov_b32", v_addr_save, v_addr_out);
			op2("v_mov_b32", *v_addr_save + 1, *v_addr_out + 1);

			op2("v_mov_b32", v_init, 0);
			for (int i = 0; i < k_out_maps; i++)
			{
				if (problem->EnBias() == true)
				{
					flat_store_dword(1, v_addr_out, v_init, "off");
				}
				else
				{
					flat_store_dword(1, v_addr_out, *v_acc_buff + i, "off");
				}

				op4(v_addc_u32, v_addr_out, "vcc", out_chan_stride * 4, v_addr_out);
				//op1("s_nop", 1);
				op5(v_addc_co_u32, *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");
			}

			if (problem->EnBias() == true)
			{
				for (int i = 0; i < k_out_maps; i++)
				{
					flat_load_dword(1, *v_acc_buff + i, v_addr_bias, "off");
					op4(v_addc_u32, v_addr_bias, "vcc", 4, v_addr_bias);
					op5(v_addc_co_u32, *v_addr_bias + 1, "vcc", 0, *v_addr_bias + 1, "vcc");
				}
			}
			op2("v_mov_b32", v_addr_out, v_addr_save);
			op2("v_mov_b32", *v_addr_out + 1, *v_addr_save + 1);


			if ((problem->EnRelu() == true) && (en_slop_zero == true))
			{
				s_wait_lgkmcnt(0);
			}
		}
		wrLaber(l_end_init);
				
		delVar(s_cInBlkId);
		delVar(v_addr_save);
		delVar(v_init);			// you yinhuan
	}
}
void KernelWriterConv1x1::save_result()
{
	if (c_in_group == 1)
	{
		save_without_atomic();
	}
	else
	{
		s_exec_save = newSgpr("s_exec_save", 2, 2);
		op2("s_mov_b64", *s_exec_save ^ 2, "exec");
		v_addr_out_back = newVgpr("v_addr_out_back", 2, 2);
		op2("v_mov_b32", v_addr_out_back, v_addr_out);
		op2("v_mov_b32", *v_addr_out_back + 1, *v_addr_out + 1);

		if (c_in_lds_group > 1)
		{
//			lds_split_save();
//			inner_group_sync();
//			save_from_lds_split();
			save_with_lds_atomic();
		}

		/*for (int i = 0; i < k_out_maps; i++)
		{
			save_with_atomic(i, v_addr_out, *v_acc_buff + i);
		}

		if ((problem->EnRelu() == true) && (en_slop_zero == true))
		{
			save_with_slop_zero();
		}*/

		delVar(s_exec_save);
		delVar(v_addr_out_back);
	}
}
void KernelWriterConv1x1::save_with_lds_atomic()
{
	Var * l_end_lds = newLaber("END_LDS_ATOMIC");
	Var * v_lds_offset = newVgpr("v_tmp1");

	// -------------------------------------------------------------------------------
	// 将当前wave的结果存到lds
	// -------------------------------------------------------------------------------
	for (int i = 0; i < k_out_maps; i++)
	{
		// debug
		//op2("v_mov_b32", *v_acc_buff +i, 1.000001);

		op2("ds_add_f32", v_lds_addr, *v_acc_buff + i, WAVE_SIZE * i * 4);
	}
	s_wait_lgkmcnt(0);

	// -------------------------------------------------------------------------------
	// tid_y = 0 读取LDS结果
	// -------------------------------------------------------------------------------
	op0("s_barrier");
	op3("v_cmpx_eq_u32", "vcc", v_tid_y, 0);
	op1("s_cbranch_execz", l_end_lds);
	op2("v_mov_b32", v_lds_offset, WAVE_SIZE * k_out_maps * 4);

	for (int i = 0; i < k_out_maps; i++)
	{
		ds_read_dword(1, *v_acc_buff + i, v_lds_addr, WAVE_SIZE * i * 4);
	}
	s_wait_lgkmcnt(0);

	// -------------------------------------------------------------------------------
	// 存储lds累加结果
	// -------------------------------------------------------------------------------
	for (int i = 0; i < k_out_maps; i++)
	{
		/*if ((problem->EnRelu()) && (extSolCfg->l2_rdc_group <= 1))
		{
			op3("v_cmpx_lt_f32", "vcc", *v_acc_buff + i, 0);
			{
				op3("v_mul_f32", *v_acc_buff + i, *v_acc_buff + i, s_slop);
				flat_store_dword(1, v_addr_out, *v_acc_buff + i, "off");
			}
			op2("s_mov_b64", "exec", *s_exec_save ^ 2);

			// debug
			//op2("v_mov_b32", v_test, 1234);
			//op2("v_mov_b32", *v_acc_buff + i, v_test);
			//op2("v_cvt_f32_u32", *v_acc_buff + i, *v_acc_buff + i);
			//flat_store_dword(1, v_addr_out, *v_acc_buff + i, "off");

			op4(v_addc_u32, v_addr_out, "vcc", out_chan_stride * 4, v_addr_out);
			op5(v_addc_co_u32, *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");
		}*/

		// debug
		op2("v_mov_b32", v_debug, 1234);
		op2("v_mov_b32", *v_acc_buff + i, v_debug);
		op2("v_cvt_f32_u32", *v_acc_buff + i, *v_acc_buff + i);

		flat_store_dword(1, v_addr_out, *v_acc_buff + i, "off");

		op4(v_addc_u32, v_addr_out, "vcc", out_chan_stride * 4, v_addr_out);
		op5(v_addc_co_u32, *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");
	}
	wrLaber(l_end_lds);

	delVar(v_lds_offset);
}
void KernelWriterConv1x1::lds_split_save()
{
	for (int i = 0; i < k_out_maps; i++)
	{
		// debug
		//op2("v_mov_b32", *v_acc_buff +i, 1.000001);

		op2("ds_add_f32", v_lds_addr, *v_acc_buff + i, WAVE_SIZE * i * 4);
	}
	s_wait_lgkmcnt(0);

}
void KernelWriterConv1x1::inner_group_sync()
{
	op0("s_barrier");
}
void KernelWriterConv1x1::save_from_lds_split()
{

}

void KernelWriterConv1x1::save_without_atomic()
{
	for (int i = 0; i < k_out_maps; i++)
	{
		// debug
		//op2("v_mov_b32", *v_acc_buff + i, v_in_buff_b);
		//op2("v_cvt_f32_u32", *v_acc_buff + i, *v_acc_buff + i);
		if (problem->EnRelu())
		{
			s_exec_save = newSgpr("s_exec_save", 2, 2);
			op2("s_mov_b64", *s_exec_save ^ 2, "exec");
			op3("v_cmpx_lt_f32", "vcc", *v_acc_buff + i, 0);
			op3("v_mul_f32", *v_acc_buff + i, *v_acc_buff + i, s_slop);
			op2("s_mov_b64", "exec", *s_exec_save ^ 2);
			delVar(s_exec_save);
		}

		flat_store_dword(1, v_addr_out, *v_acc_buff + i, "off");
		op4(v_addc_u32, v_addr_out, "vcc", out_chan_stride * 4, v_addr_out);
		op5(v_addc_co_u32, *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");
	}
}
void KernelWriterConv1x1::save_with_atomic(int n, Var * addr_out, Var * accum)
{
	// v_newVal = v_src_cmp
	// v_prevVal = v_src_cmp + 1
	Var * v_src_cmp = newVgpr("v_src_cmp", 2, 2);
	Var * v_rtn = newVgpr("v_rtn");
	//Var * l_atomic_add = newLaber("ATOMIC_ADD_" + d2s(n));
	Var * l_atomic_add = newLaber("SEG_3_" + d2s(n));
	Var * v_tmp = newVgpr("v_tmp");
	Var * s_exec2 = newSgpr("s_exec2", 2, 2);

	// debug
	//op2("v_mov_b32", accum, 1.0001);
	//op2("v_mov_b32", accum, v_in_buff_a);

	flat_load_dword(1, *v_src_cmp + 1, addr_out, "off", 0, true);
	s_wait_vmcnt(0);
	wrLaber(l_atomic_add);


	if ((problem->EnRelu() == true) && (en_slop_zero == false))
	{
		//op4("v_fma_f32", v_src_cmp, accum, s_slop, *v_src_cmp + 1);
		/*
		if (old < 0)
		{
			tmp = 1 / slop;
			old = old * tmp;
		}
		xin = old + acc;
		if (xin < 0)
			xin *= slop;
		*/
		op2("s_mov_b64", *s_exec2 ^ 2, "exec");
		op2("v_mov_b32", v_tmp, *v_src_cmp + 1);
		op3("v_cmpx_lt_f32", "vcc", *v_src_cmp + 1, 0);
		op2("v_rcp_f32", v_tmp, s_slop);				// v_tmp = 1 / s_slop
		op3("v_mul_f32", v_tmp, *v_src_cmp + 1, v_tmp);	// v_tmp = old / s_slop
		op2("s_mov_b64", "exec", *s_exec2 ^ 2);
		//op1("s_nop", 5);
		op3("v_add_f32", v_src_cmp, v_tmp, accum);
		op3("v_cmpx_lt_f32", "vcc", v_src_cmp, 0);
		op3("v_mul_f32", v_src_cmp, v_src_cmp, s_slop);
		op2("s_mov_b64", "exec", *s_exec2 ^ 2);
		//op1("s_nop", 5);
	}
	else
	{
		op3("v_add_f32", v_src_cmp, *v_src_cmp + 1, accum);
	}
	if (IsaArch == E_IsaArch::Gfx800)
	{
		op3("flat_atomic_cmpswap", v_rtn, *addr_out ^ 2, *v_src_cmp ^ 2, true);
	}
	else if (IsaArch == E_IsaArch::Gfx900)
	{
		op4("global_atomic_cmpswap", v_rtn, *addr_out ^ 2, *v_src_cmp ^ 2, "off", true);
	}
	s_wait_vmcnt(0);
	op3("v_cmpx_neq_f32", "vcc", *v_src_cmp + 1, v_rtn);
	op2("v_mov_b32", *v_src_cmp + 1, v_rtn);
	//op1("s_nop", 5);
	op1("s_cbranch_execnz", l_atomic_add);
	op0("s_barrier");
	op2("s_mov_b64", "exec", *s_exec_save ^ 2);
	//op1("s_nop", 5);
	op4(v_addc_u32, v_addr_out, "vcc", out_chan_stride * 4, v_addr_out);
	//op1("s_nop", 1);
	op5(v_addc_co_u32, *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");

	delVar(v_src_cmp);
	delVar(v_rtn);
	delVar(s_exec2);
	delVar(v_tmp);
}
void KernelWriterConv1x1::save_with_slop_zero()
{
	// 写信号
	Var * v_addr_sig = newVgpr("addr_sig", 2, 2);
	op2("v_mov_b32", v_addr_sig, s_addr_sig);
	op2("v_mov_b32", *v_addr_sig + 1, *s_addr_sig + 1);
	op2("s_mov_b64", "exec", 1);
	Var*v_sig = newVgpr("v_sig");
	op2("v_mov_b32", v_sig, 1);
	op3("flat_atomic_add", v_sig, *v_addr_sig ^ 2, v_sig);
			
	// 读取信号
	Var * s_cInBlkId = newSgpr("s_cInBlkId");
	op2("v_readfirstlane_b32", s_cInBlkId, v_cInBlkId);
	op2("s_cmpk_eq_i32", s_cInBlkId, 0);
	op1("s_cbranch_scc0", l_end_prg);

	Var * l_atomic_slop_zero = newLaber("SEG_4");
	Var*s_sig = newSgpr("s_sig");
	wrLaber(l_atomic_slop_zero);
	{
		s_load_dword(1, s_sig, s_addr_sig, 0, true);
		s_wait_lgkmcnt(0);
		op2("s_cmpk_eq_i32", s_sig, 104);
		op1("s_cbranch_scc0", l_atomic_slop_zero);
	}
			
	// 读取累加值，乘以slop
	op2("s_mov_b64", "exec", *s_exec_save ^ 2);
	op2("v_mov_b32", v_addr_out, v_addr_out_back);
	op2("v_mov_b32", *v_addr_out + 1, *v_addr_out_back + 1);

	Var * v_tmp = newVgpr("v_slop");
	op2("v_mov_b32", v_tmp, s_slop);					// v_tmp = 1 / s_slop
	for (int i = 0; i < k_out_maps; i++)
	{
		flat_load_dword(1, *v_acc_buff + i, v_addr_out, "off");
		s_wait_vmcnt(0);

		op3("v_cmpx_lt_f32", "vcc", *v_acc_buff + i, 0);
		op3("v_mul_f32", *v_acc_buff + i, *v_acc_buff + i, v_tmp);	// v_tmp = old / s_slop
		flat_store_dword(1, v_addr_out, *v_acc_buff + i, "off");

		op2("s_mov_b64", "exec", *s_exec_save ^ 2);
		op4(v_addc_u32, v_addr_out, "vcc", out_chan_stride * 4, v_addr_out);
		op5(v_addc_co_u32, *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");

	}

	delVar(v_tmp);
	delVar(s_cInBlkId);
	delVar(v_sig);
	delVar(v_addr_sig);
}

/************************************************************************************/
/* 测试下标																			*/
/************************************************************************************/
void KernelWriterConv1x1::simulate_index()
{
	int *testId = (int*)malloc(group_num.x * sizeof(int));
	int *testPixBlkId = (int*)malloc(group_num.x * sizeof(int));
	int *testCInBlkId = (int*)malloc(group_num.x * sizeof(int));
	int *testKOutBlkId = (int*)malloc(group_num.x * sizeof(int));
	int *testPosId = (int*)malloc(group_num.x * sizeof(int));
	int *testBatchId = (int*)malloc(group_num.x * sizeof(int));
	int *testOutId = (int*)malloc(group_num.x * sizeof(int));

	uint in_chan_stride = problem->W() * problem->H();
	uint in_batch_stride = problem->W() * problem->H() * problem->C();
	uint wei_chan_stride = problem->C();
	uint out_chan_stride = problem->W() * problem->H();
	uint out_batch_stride = problem->W() * problem->H() * problem->K();

	uint c_in_maps = kernelParam.c_in_maps;
	uint c_in_group = kernelParam.c_in_group;
	uint k_out_maps = kernelParam.k_out_maps;
	uint k_out_group = kernelParam.k_out_group;

	uint wave_per_group = group_sz.x / WAVE_SIZE;
	uint conv_loop = problem->C() / kernelParam.c_in_maps_once / 2;

	for (int grp = 0; grp < group_num.x; grp++)
	{
		uint tid_x = 5 * 64 + 40;
		uint gid_x = grp;
		int waveId = -1, tidInWave = -1;
		int pixBlkId = -1, kOutBlkId = -1, cInBlkId = -1;
		int pos_id = -1, batch_id = -1, out_id = -1;
		int gbl_in_off = -1, wei_off = -1, gbl_out_off = -1;

		waveId = gid_x * wave_per_group + tid_x / WAVE_SIZE;
		tidInWave = tid_x % WAVE_SIZE;

		cInBlkId = waveId / k_out_group % c_in_group;
		pixBlkId = waveId / k_out_group / c_in_group;
		kOutBlkId = waveId % k_out_group;

		pos_id = (pixBlkId * WAVE_SIZE + tidInWave) % in_chan_stride;
		batch_id = (pixBlkId * WAVE_SIZE + tidInWave) / in_chan_stride;
		out_id = kOutBlkId * k_out_maps;

		if (batch_id >= problem->N())
			goto STORE_IDX;

		gbl_in_off = (batch_id * in_batch_stride) + (cInBlkId * c_in_maps * in_chan_stride) + pos_id;
		wei_off = (out_id * wei_chan_stride) + (cInBlkId * c_in_maps);
		gbl_out_off = (batch_id * out_batch_stride) + (out_id * out_chan_stride) + pos_id;

	STORE_IDX:
		testId[grp] = wei_off;
		testPixBlkId[grp] = pixBlkId;
		testCInBlkId[grp] = cInBlkId;
		testKOutBlkId[grp] = kOutBlkId;
		testPosId[grp] = pos_id;
		testBatchId[grp] = batch_id;
		testOutId[grp] = out_id;
	}

	//print_index(testId, "test temp id");
	//print_index(testPixBlkId, "pix block id");
	print_index(testCInBlkId, "c_in block id");
	//print_index(testKOutBlkId, "k_out block id");
	//print_index(testBatchId, "batch id");
	//print_index(testOutId, "out id");
	//print_index(testPosId, "pos id");

	free(testId);
	free(testPixBlkId);
	free(testCInBlkId);
	free(testKOutBlkId);
	free(testPosId);
	free(testBatchId);
	free(testOutId);
}

void KernelWriterConv1x1::save_debug()
{
	op2("v_mov_b32", v_debug, v_tid_x);
	op2("v_cvt_f32_u32", v_debug, v_debug);

	flat_store_dword(1, v_addr_dbg, v_debug, "off");
	op1("s_branch", l_end_prg);
}
