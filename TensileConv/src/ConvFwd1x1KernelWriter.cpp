#pragma once

#include "ConvFwd1x1KernelWriter.h"

using namespace TensileConv;
using namespace AutoGen;

KernelWriterConv1x1::KernelWriterConv1x1(T_Conv1x1KernelParam kernelParam, E_IsaArch isaArch)
	:KernelWriter(isaArch)
{
	kernelName = "ConvFwd1x1";

	// -------------------------------------------------------------------------------
	// 复制参数, 方便使用
	// -------------------------------------------------------------------------------
	this->kernelParam = kernelParam;
	N = kernelParam.N;
	C = kernelParam.C; K = kernelParam.K;
	W = kernelParam.W; H = kernelParam.H;
	EnBias = kernelParam.EnBias; 
	Relu = kernelParam.Relu;

	c_in_lds_split_group = kernelParam.c_in_lds_split_group;
	c_in_lds_atomic_group = kernelParam.c_in_lds_atomic_group;
	c_in_l2_split_group = kernelParam.c_in_l2_split_group;
	c_in_l2_atomic_group = kernelParam.c_in_l2_atomic_group;
	k_out_maps = kernelParam.k_out_maps;
	group_sz.x = kernelParam.group_size_x;
	PCK_order = kernelParam.PCK_order;

	// -------------------------------------------------------------------------------
	// 中间变量计算
	// -------------------------------------------------------------------------------
	in_chan_stride = W * H;
	in_batch_stride = W * H * C;
	wei_chan_stride = C;
	out_chan_stride = W * H;
	out_batch_stride = W * H * K;
	out_size = W * H * K * N;

	c_in_lds_group = c_in_lds_split_group * c_in_lds_atomic_group;
	c_in_l2_group = c_in_l2_split_group * c_in_l2_atomic_group;
	c_in_group = c_in_l2_group * c_in_lds_group;
	c_in_maps = C / c_in_group;
	if (c_in_maps_once <= c_in_maps / unroll_time)
	{
		c_in_maps_once_real = c_in_maps_once;
	}
	else
	{
		c_in_maps_once_real = c_in_maps / unroll_time;
	}
	conv_loop = c_in_maps / c_in_maps_once_real / 2;

	k_out_group = _divCeil(K, k_out_maps);
	pix_group = _divCeil(W * H * N, group_sz.x);
	pix_wave = group_sz.x / WAVE_SIZE * pix_group;
	align = pix_group * group_sz.x;

	en_l2_sync = ((Relu == RELU && c_in_l2_atomic_group > 1) || (c_in_l2_split_group > 1));
	en_input_offset = ((IsaArch == E_IsaArch::Gfx900) && (W*H <= 4095));
	en_wei_addr_offset = true;
		
	size_sig = size_l2 = size_dbg = 0;
	if (en_l2_sync)
		size_sig = pix_group * k_out_group;
	if (c_in_l2_split_group > 1)
		size_l2 = out_size * c_in_l2_split_group;
#ifdef KERNEL_DEBUG
	size_dbg = align * c_in_group * k_out_group;	// total thread number, 2D
#endif

	// -------------------------------------------------------------------------------
	// work load
	// -------------------------------------------------------------------------------
	global_sz.x = align * c_in_l2_group * k_out_group;
	global_sz.y = c_in_lds_group;
	global_sz.z = 1;
	group_sz.y = c_in_lds_group;
	group_sz.z = 1;
	group_num = global_sz / group_sz;
	group_wave_num_x = group_sz.x / WAVE_SIZE;

	print_kernel_param();
}
E_ReturnState KernelWriterConv1x1::checkKernelParam()
{
	return E_ReturnState::SUCCESS;
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
	s_prefetch = newSgpr("s_prefetch", k_out_maps);

	// -------------------------------------------------------------------------------
	// 卷积计算:
	// -------------------------------------------------------------------------------
	// 初始化:
	init_output();

//	// 循环填充
//	wei_offset = 0;
//	prefetch_weight();
//	load_input(v_in_buff_a);
//
//	// 循环体
//	if (conv_loop > 1)
//	{
//		Var * s_loop_cnt = newSgpr("s_loop_cnt");
//		f_s_loop(s_loop_cnt, conv_loop - 1, "CONV_LOOP");
//		{
//			load_input(v_in_buff_b);
//			conv_one_loop(v_in_buff_a, false);
//			load_input(v_in_buff_a);
//			conv_one_loop(v_in_buff_b, true);
//		}
//		f_e_loop(s_loop_cnt, "CONV_LOOP");
//	}
//
//	// 循环排空
//	if (conv_loop > 0)
//	{
//		load_input(v_in_buff_b);
//		conv_one_loop(v_in_buff_a, false);
//		conv_last_loop(v_in_buff_b);
//	}
//	else
//	{
//		wei_offset -= (c_in_maps_once_real - wei_chan_stride * k_out_maps) * 4;
//		conv_last_loop(v_in_buff_a);
//	}
//
	// -------------------------------------------------------------------------------
	// 存储结果:
	// -------------------------------------------------------------------------------
	save_result();

	// -------------------------------------------------------------------------------
	// 销毁变量
	// -------------------------------------------------------------------------------
	// 销毁地址
	if (en_input_offset == true)
	{
		delVar(v_global_offset);
	}
	else
	{
		delVar(v_addr_in);
	}
	delVar(s_addr_wei);
	delVar(v_addr_out);
	if (EnBias == true)				delVar(v_addr_bias);
	if (Relu == PRELU)				delVar(s_slop);
	if (en_l2_sync == true)			delVar(s_addr_sig);
	if (c_in_lds_group > 1)			delVar(v_addr_lds);
	if (c_in_l2_split_group > 1)	delVar(v_addr_l2);

	// 销毁结果buffer
	delVar(v_acc_buff);
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
E_ReturnState KernelWriterConv1x1::calcuIndex()
{
	// -------------------------------------------------------------------------------
	// kenrel 参数
	// -------------------------------------------------------------------------------
	{
		s_ptr_in = newSgpr("s_ptr_in", 2, 2);
		s_ptr_wei = newSgpr("s_ptr_wei", 2, 2);
		s_ptr_out = newSgpr("s_ptr_out", 2, 2);
		if (EnBias == true)				s_ptr_bias = newSgpr("s_ptr_bias", 2, 2);
		if (en_l2_sync)					s_ptr_sig = newSgpr("s_ptr_sig", 2, 2);
		if (c_in_l2_split_group > 1)	s_ptr_l2 = newSgpr("s_ptr_l2", 2, 2);
		if (Relu == PRELU)				s_slop = newSgpr("s_slop");
#if KERNEL_DEBUG
		s_ptr_dbg = newSgpr("s_ptr_dbg", 2, 2);
#endif
	}

	// -------------------------------------------------------------------------------
	// 中间变量
	// -------------------------------------------------------------------------------
	{
		v_wave_id_x = newVgpr("v_wave_id_x");
		v_wave_tid_x = newVgpr("v_wave_tid_x");
		v_pix_blk_id = newVgpr("v_pix_blk_id");
		v_c_blk_id = newVgpr("v_c_blk_id");
		v_c_l2_blk_id = newVgpr("v_c_l2_blk_id");
		v_k_blk_id = newVgpr("v_k_blk_id");
		v_pos_id = newVgpr("v_pos_id");
		v_batch_id = newVgpr("v_batch_id");
		v_out_id = newVgpr("v_out_id");
		v_l2_pos_id = newVgpr("v_l2_pos_id");
		v_lds_pos_id = newVgpr("v_lds_id");
		s_c_blk_id = newSgpr("s_c_blk_id");
		s_c_l2_blk_id = newSgpr("s_c_blk_id");
		s_c_lds_blk_id = newSgpr("s_c_blk_id");
	}

	// -------------------------------------------------------------------------------
	// 实际地址
	// -------------------------------------------------------------------------------
	{
		if (en_input_offset == true)	v_global_offset = newVgpr("v_global_offset", c_in_maps_once_real * 2 / 2, 2);
		else							v_addr_in = newVgpr("v_addr_in", 2, 2);
		s_addr_wei = newSgpr("s_addr_wei", 2, 2);
		v_addr_out = newVgpr("v_addr_out", 2, 2);
		if (EnBias == true)				v_addr_bias = newVgpr("v_addr_bias", 2, 2);
		if (en_l2_sync)					s_addr_sig = newSgpr("s_addr_sig", 2, 2);
		if (c_in_lds_group > 1)			v_addr_lds = newVgpr("v_ds_addr");
		if (c_in_l2_split_group > 1)	v_addr_l2 = newVgpr("v_addr_l2", 2, 2);
#if KERNEL_DEBUG
		v_addr_dbg = newVgpr("v_addr_dbg", 2, 2);
#endif
	}
	
	// -------------------------------------------------------------------------------
	// 计算过程
	// -------------------------------------------------------------------------------
	{
		CheckFunc(loadKernelArgs());
		CheckFunc(calcuWaveIndex());
		CheckFunc(calcuBlkIndex());
		CheckFunc(calcuPosIndex());
		CheckFunc(calcuOffset());
	}

	// -------------------------------------------------------------------------------
	// 销毁 kenrel 参数
	// -------------------------------------------------------------------------------
	{
		if (en_input_offset == false)	delVar(s_ptr_in);
		delVar(s_ptr_wei);
		delVar(s_ptr_out);
		if (EnBias == true)				delVar(s_ptr_bias);
		if (en_l2_sync)					delVar(s_ptr_sig);
		if (c_in_l2_split_group > 1)	delVar(s_ptr_l2);
#if KERNEL_DEBUG
		delVar(s_ptr_dbg);
#endif
	}

	// -------------------------------------------------------------------------------
	// 销毁中间变量
	// -------------------------------------------------------------------------------
	{
		delVar(v_wave_id_x);
		delVar(v_wave_tid_x);
		delVar(v_pix_blk_id);
		delVar(v_c_blk_id);	
		delVar(v_c_l2_blk_id);
		delVar(v_k_blk_id);
		delVar(v_pos_id);
		delVar(v_batch_id);
		delVar(v_out_id);
		delVar(v_lds_pos_id);
		delVar(v_l2_pos_id);
	}

	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelWriterConv1x1::loadKernelArgs()
{
	int offset = 0;

	s_load_dword(2, s_ptr_in, s_kernelArg, offset);			offset += 8;
	s_load_dword(2, s_ptr_wei, s_kernelArg, offset);		offset += 8;
	s_load_dword(2, s_ptr_out, s_kernelArg, offset);		offset += 8;

	if (EnBias == true)
		s_load_dword(2, s_ptr_bias, s_kernelArg, offset);	
	offset += 8;	

	if (en_l2_sync == true)
		s_load_dword(2, s_ptr_sig, s_kernelArg, offset);	
	offset += 8;

	if (c_in_l2_split_group > 1)
		s_load_dword(2, s_ptr_l2, s_kernelArg, offset);
	offset += 8;
	
	if (Relu == PRELU)
		s_load_dword(1, s_slop, s_kernelArg, offset);		
	offset += 8;
	
#if KERNEL_DEBUG
	s_load_dword(2, s_ptr_dbg, s_kernelArg, offset);		offset += 8;
#endif

	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelWriterConv1x1::calcuWaveIndex()
{
	Var * s_tmp1 = newSgpr("s_tmp1");
	Var * v_tmp1 = newVgpr("v_tmp1");

	// -------------------------------------------------------------------------------
	// wave_id_x = (group_wave_num_x * gid_x) + (tid_x / WAVE_SIZE);
	// wave_tid_x = tid_x % WAVE_SIZE;
	// -------------------------------------------------------------------------------
	op3("s_lshl_b32", s_tmp1, s_gid_x, log2(group_wave_num_x));		// s_tmp1 = gid_x * block_per_group
	op3("v_lshrrev_b32", v_tmp1, log2(WAVE_SIZE), v_tid_x);			// v_tmp1 = tid_x / WAVE_SIZE
	op3(v_add_u32, v_wave_id_x, v_tmp1, s_tmp1);					// v_wave_id_x = s_tmp1 + v_tmp1
	op3("v_and_b32", v_wave_tid_x, modMask(WAVE_SIZE), v_tid_x);	// v_wave_tid_x = tid_x % WAVE_SIZE

	delVar(s_tmp1);
	delVar(v_tmp1);

	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelWriterConv1x1::calcuBlkIndex()
{
	Var * v_tmp1 = newVgpr("v_tmp1");
	Var * v_tmp2 = newVgpr("v_tmp2");

	if (PCK_order == 321)
	{
		// -------------------------------------------------------------------------------
		// k_out --> c_in --> pix
		// k_blk_id = wave_id_x % k_out_group;
		// c_l2_blk_id = wave_id_x / k_out_group % c_in_l2_group;
		// pix_blk_id = wave_id_x / k_out_group / c_in_l2_group;
		// -------------------------------------------------------------------------------
		fv_div_u32(v_wave_id_x, k_out_group, v_tmp2, v_k_blk_id);
		fv_div_u32(v_tmp2, c_in_l2_group, v_pix_blk_id, v_c_l2_blk_id);
	}
	else if (PCK_order == 312)
	{
		// -------------------------------------------------------------------------------
		// c_in --> k_out --> pix
		// c_l2_blk_id = wave_id_x % c_in_l2_group;
		// k_blk_id = wave_id_x / c_in_l2_group % k_out_group;
		// pix_blk_id = wave_id_x / c_in_l2_group / k_out_group;
		// -------------------------------------------------------------------------------
		fv_div_u32(v_wave_id_x, c_in_l2_group, v_tmp2, v_c_l2_blk_id);
		fv_div_u32(v_tmp2, k_out_group, v_pix_blk_id, v_k_blk_id);
	}
	else if (PCK_order == 123)
	{
		// -------------------------------------------------------------------------------
		// pix --> c_in --> k_out
		// pix_blk_id = wave_id_x % pix_wave;
		// c_l2_blk_id = wave_id_x / pix_wave % c_in_l2_group;
		// k_blk_id = wave_id_x / pix_wave / c_in_l2_group;
		// -------------------------------------------------------------------------------
		fv_div_u32(v_wave_id_x, pix_wave, v_tmp2, v_pix_blk_id);
		fv_div_u32(v_tmp2, c_in_l2_group, v_k_blk_id, v_c_l2_blk_id);
	}
	else if (PCK_order == 132)
	{
		// -------------------------------------------------------------------------------
		// pix --> k_out --> c_in
		// pix_blk_id = wave_id_x % pix_wave;
		// k_blk_id = wave_id_x / pix_wave % k_out_group;
		// c_l2_blk_id = wave_id_x / pix_wave / k_out_group;
		// -------------------------------------------------------------------------------
		fv_div_u32(v_wave_id_x, pix_wave, v_tmp2, v_pix_blk_id);
		fv_div_u32(v_tmp2, k_out_group, v_c_l2_blk_id, v_k_blk_id);
	}
	else if (PCK_order == 213)
	{
		// -------------------------------------------------------------------------------
		// c_in --> pix --> k_out
		// c_l2_blk_id = wave_id_x % c_in_l2_group;
		// pix_blk_id = wave_id_x / c_in_l2_group % pix_wave;
		// k_blk_id = wave_id_x / c_in_l2_group / pix_wave;
		// -------------------------------------------------------------------------------
		fv_div_u32(v_wave_id_x, c_in_l2_group, v_tmp2, v_c_l2_blk_id);
		fv_div_u32(v_tmp2, pix_wave, v_k_blk_id, v_pix_blk_id);
	}
	else if (PCK_order == 231)
	{
		// -------------------------------------------------------------------------------
		// k_out --> pix --> c_in
		// k_blk_id = wave_id_x % k_out_group;
		// pix_blk_id = wave_id_x / k_out_group % pix_wave;
		// c_l2_blk_id = wave_id_x / k_out_group / pix_wave;
		// -------------------------------------------------------------------------------
		fv_div_u32(v_wave_id_x, k_out_group, v_tmp2, v_k_blk_id);
		fv_div_u32(v_tmp2, pix_wave, v_c_l2_blk_id, v_pix_blk_id);
	}

	// -------------------------------------------------------------------------------
	// c_blk_id = c_in_lds_group * c_l2_blk_id + tid_y;
	// -------------------------------------------------------------------------------
	op2("v_mov_b32", v_tmp1, c_in_lds_group);
	op3("v_mul_u32_u24", v_tmp1, v_tmp1, v_c_l2_blk_id);
	op3(v_add_u32, v_c_blk_id, v_tmp1, v_tid_y);

	op2("v_readfirstlane_b32", s_c_blk_id, v_c_blk_id);
	op2("v_readfirstlane_b32", s_c_l2_blk_id, v_c_l2_blk_id);
	op2("v_readfirstlane_b32", s_c_lds_blk_id, v_tid_y);

	delVar(v_tmp1);
	delVar(v_tmp2);
	
	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelWriterConv1x1::calcuPosIndex()
{
	Var * v_tmp1 = newVgpr("v_tmp1");

	// -------------------------------------------------------------------------------
	// pos_id   = (pix_blk_id * WAVE_SIZE + wave_tid_x) % in_chan_stride;
	// batch_id = (pix_blk_id * WAVE_SIZE + wave_tid_x) / in_chan_stride;
	// out_id   = k_blk_id * k_out_maps;
	// -------------------------------------------------------------------------------
	op3("v_lshlrev_b32", v_tmp1, log2(WAVE_SIZE), v_pix_blk_id);
	op3(v_add_u32, v_tmp1, v_wave_tid_x, v_tmp1);					// v_tmp1 = pix_blk_id * WAVE_SIZE + wave_tid_x
	fv_div_u32(v_tmp1, in_chan_stride, v_batch_id, v_pos_id);
	op3("v_mul_u32_u24", v_out_id, k_out_maps, v_k_blk_id);

	// -------------------------------------------------------------------------------
	// if (batch_id >= extProbCfg->N)
	//		return;
	// -------------------------------------------------------------------------------
	op2("v_mov_b32", v_tmp1, N);
	op3("v_cmpx_lt_u32", "vcc", v_batch_id, v_tmp1); 
	op1("s_cbranch_execz", l_end_prg);
	
	// -------------------------------------------------------------------------------
	// lds_space_size = group_sz.x * k_out_maps * c_in_lds_split_group;
	// lds_pos_id = tid_y % c_in_lds_split_group;
	// -------------------------------------------------------------------------------
	ldsByteCount += group_sz.x * k_out_maps * c_in_lds_split_group * 4;
	fv_div_u32(v_tid_y, c_in_lds_split_group, v_tmp1, v_lds_pos_id);

	// -------------------------------------------------------------------------------
	// l2_space_size = group_sz.x * k_out_maps * c_in_l2_split_group;
	// l2_pos_id = c_l2_blk_id % c_in_l2_split_group;
	// -------------------------------------------------------------------------------
	size_l2 = group_sz.x * k_out_maps * c_in_l2_split_group * 4;
	fv_div_u32(v_c_l2_blk_id, c_in_l2_split_group, v_tmp1, v_l2_pos_id);

	delVar(v_tmp1);
	
	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelWriterConv1x1::calcuOffset()
{
	Var * s_tmp1 = newSgpr("s_tmp1");
	Var * v_tmp1 = newVgpr("v_tmp1");
	Var * v_tmp2 = newVgpr("v_tmp2");
	Var * v_tmp3 = newVgpr("v_tmp3");
	Var * v_out_off_tmp = newVgpr("v_out_off_tmp");

	s_wait_lgkmcnt(0);

	// -------------------------------------------------------------------------------
	// gbl_in_off = (batch_id * in_batch_stride) + (c_blk_id * c_in_maps * in_chan_stride) + pos_id;
	// -------------------------------------------------------------------------------
	{
		op2("v_mov_b32", v_tmp1, in_batch_stride);
		op3("v_mul_u32_u24", v_tmp1, v_tmp1, v_batch_id);	// v_tmp1 = batch_id * in_batch_stride
		op2("v_mov_b32", v_tmp2, c_in_maps);
		op3("v_mul_u32_u24", v_tmp2, v_tmp2, v_c_blk_id);
		op2("v_mov_b32", v_tmp3, in_chan_stride);
		op3("v_mul_u32_u24", v_tmp2, v_tmp3, v_tmp2);		// v_tmp2 = c_blk_id * c_in_maps * in_chan_stride

		if (IsaArch == E_IsaArch::Gfx800)
		{
			op4(v_addc_u32, v_tmp3, "vcc", v_tmp1, v_tmp2);
			op5(v_addc_co_u32, v_tmp3, "vcc", v_tmp3, v_pos_id, "vcc");
		}
		else if (IsaArch == E_IsaArch::Gfx900)
		{
			op4("v_add3_u32", v_tmp3, v_tmp1, v_tmp2, v_pos_id);
		}
		op3("v_lshlrev_b32", v_tmp3, 2, v_tmp3);			// v_tmp3 = gbl_in_off (BYTE)

		if (en_input_offset == true)
		{
			op2("v_mov_b32", v_global_offset, v_tmp3);
			op2("v_mov_b32", v_tmp1, in_chan_stride * 2 * 4);
			for (int i = 0; i < c_in_maps_once_real * 2 / 2; i++)
			{
				op4(v_addc_u32, *v_global_offset + 2 * (i + 1), "vcc", *v_global_offset + 2 * i, v_tmp1);
			}
		}
		else
		{
			op2("v_mov_b32", *v_addr_in + 1, *s_ptr_in + 1);
			op4(v_addc_u32, v_addr_in, "vcc", s_ptr_in, v_tmp3);
			op5(v_addc_co_u32, *v_addr_in + 1, "vcc", 0, *v_addr_in + 1, "vcc");
		}
	}

	// -------------------------------------------------------------------------------
	// wei_off = (out_id * wei_chan_stride) + (c_blk_id * c_in_maps);
	// -------------------------------------------------------------------------------
	{
		op2("v_mov_b32", v_tmp1, wei_chan_stride);
		op3("v_mul_u32_u24", v_tmp1, v_out_id, v_tmp1);		// v_tmp1 = out_id * wei_chan_stride
		op2("v_mov_b32", v_tmp2, c_in_maps);
		op3("v_mul_u32_u24", v_tmp2, v_c_blk_id, v_tmp2);	// v_tmp2 = c_blk_id * c_in_maps
		op3(v_add_u32, v_tmp1, v_tmp1, v_tmp2);
		op2("v_readfirstlane_b32", s_tmp1, v_tmp1);			// s_tmp1 = wei_off
		op3("s_lshl_b32", s_tmp1, s_tmp1, 2);				// s_tmp1 = wei_off (BYTE)
		op3("s_add_u32", s_addr_wei, s_ptr_wei, s_tmp1);
		op3("s_addc_u32", *s_addr_wei + 1, 0, *s_ptr_wei + 1);
	}

	// -------------------------------------------------------------------------------
	// gbl_out_off = (batch_id * out_batch_stride) + (out_id * out_chan_stride) + pos_id;
	// -------------------------------------------------------------------------------
	{
		op2("v_mov_b32", v_tmp1, out_batch_stride);
		op3("v_mul_u32_u24", v_tmp1, v_batch_id, v_tmp1);		// v_tmp1 = batch_id * out_batch_stride
		op2("v_mov_b32", v_tmp2, out_chan_stride);
		op3("v_mul_u32_u24", v_tmp2, v_out_id, v_tmp2);			// v_tmp2 = out_id * out_chan_stride

		if (IsaArch == E_IsaArch::Gfx800)
		{
			op4(v_addc_u32, v_tmp3, "vcc", v_tmp1, v_tmp2);
			op5(v_addc_co_u32, v_tmp3, "vcc", v_tmp3, v_pos_id, "vcc");
		}
		else if (IsaArch == E_IsaArch::Gfx900)
		{
			op4("v_add3_u32", v_tmp3, v_tmp1, v_tmp2, v_pos_id);
		}
		op2("v_mov_b32", v_out_off_tmp, v_tmp3);
		op3("v_lshlrev_b32", v_tmp3, 2, v_tmp3);				// v_tmp3 = gbl_out_off (BYTE)

		op3("v_lshrrev_b32", v_debug2, 2, v_tmp3);

		op2("v_mov_b32", *v_addr_out + 1, *s_ptr_out + 1);
		op4(v_addc_u32, v_addr_out, "vcc", s_ptr_out, v_tmp3);
		op5(v_addc_co_u32, *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");
	}

	// -------------------------------------------------------------------------------
	// bias_off = out_id
	// -------------------------------------------------------------------------------
	if (EnBias == true)
	{
		op3("v_lshlrev_b32", v_tmp1, 2, v_out_id);
		op2("v_mov_b32", *v_addr_bias + 1, *s_ptr_bias + 1);
		op4(v_addc_u32, v_addr_bias, "vcc", s_ptr_bias, v_tmp1);
		op5(v_addc_co_u32, *v_addr_bias + 1, "vcc", 0, *v_addr_bias + 1, "vcc");
	}

	// -------------------------------------------------------------------------------
	// sig_off = pixBlkId * k_out_group + kOutBlkId
	// -------------------------------------------------------------------------------
	if (en_l2_sync)
	{
		op3("v_mul_u32_u24", v_tmp1, v_pix_blk_id, k_out_group);			// v_tmp1 = pixBlkId * k_out_group
		op4(v_addc_u32, v_tmp1, "vcc", v_tmp1, v_k_blk_id);
		op2("v_readfirstlane_b32", s_tmp1, v_tmp1);							// s_tmp1 = sig_off
		op3("s_lshl_b32", s_tmp1, s_tmp1, 2);								// s_tmp1 = sig_off (BYTE)
		s_wait_lgkmcnt(0);
		op3("s_add_u32", s_addr_sig, s_ptr_sig, s_tmp1);
		op3("s_addc_u32", *s_addr_sig + 1, 0, *s_ptr_sig + 1);
	}
	
	// -------------------------------------------------------------------------------
	// lds_off = (group_sz.x * k_out_maps) * lds_pos_id + tid_x
	// -------------------------------------------------------------------------------
	if (c_in_lds_group > 1)
	{
		op2("v_mov_b32", v_tmp1, group_sz.x * k_out_maps);
		op3("v_mul_u32_u24", v_tmp1, v_lds_pos_id, v_tmp1);		// v_tmp1 = (group_sz.x * k_out_maps) * lds_pos_id
		op3(v_add_u32, v_tmp1, v_tmp1, v_tid_x);				// v_tmp1 = lds_off
		op3("v_lshlrev_b32", v_addr_lds, 2, v_tmp1);
	}
	
	// -------------------------------------------------------------------------------
	// l2_off = out_size * l2_pos_id + gbl_out_off
	// -------------------------------------------------------------------------------
	if (c_in_l2_split_group > 1)
	{
		op2("v_mov_b32", v_tmp1, out_size);
		op3("v_mul_u32_u24", v_tmp1, v_l2_pos_id, v_tmp1);		// v_tmp1 = out_size * l2_pos_id
		op3(v_add_u32, v_tmp1, v_tmp1, v_out_off_tmp);			// v_tmp1 = l2_off
		op3("v_lshlrev_b32", v_tmp1, 2, v_tmp1);

		op2("v_mov_b32", *v_addr_l2 + 1, *s_ptr_l2 + 1);
		op4(v_addc_u32, v_addr_l2, "vcc", s_ptr_l2, v_tmp1);
		op5(v_addc_co_u32, *v_addr_l2 + 1, "vcc", 0, *v_addr_l2 + 1, "vcc");
	}

	// -------------------------------------------------------------------------------
	// dbg_off = 线性地址
	// -------------------------------------------------------------------------------
#if KERNEL_DEBUG
	f_linear_addr_2d(s_ptr_dbg, v_addr_dbg);
#endif

	delVar(s_tmp1);
	delVar(v_tmp1);
	delVar(v_tmp2);
	delVar(v_tmp3);
	delVar(v_out_off_tmp);

	return E_ReturnState::SUCCESS;
}

/************************************************************************************/
/* 数据读取与存储																		*/
/************************************************************************************/
void KernelWriterConv1x1::init_output()
{
	Var * v_init = newVgpr("v_init");
	op2("v_mov_b32", v_init, 0);

	for (int i = 0; i < k_out_maps; i++)
		op2("v_mov_b32", *v_acc_buff + i, 0);

	// -------------------------------------------------------------------------------
	// 第一个 tid_y 做LDS ATOMIC 初始化
	// -------------------------------------------------------------------------------
	Var * l_end_lds_atomic_init = newLaber("END_LDS_ATOMIC_INIT");
	if (c_in_lds_atomic_group > 1)
	{
		op2("s_cmpk_eq_i32", s_c_blk_id, 0);
		op1("s_cbranch_scc0", l_end_lds_atomic_init);

		Var * v_lds_addr_tmp = newVgpr("v_lds_addr_tmp");
		op2("v_mov_b32", v_lds_addr_tmp, v_addr_lds);

		for (int i = 0; i < k_out_maps; i++)
		{
			if (group_sz.x * k_out_maps * 4 <= MAX_16BIT_UINT)
			{
				ds_write_dword(1, v_lds_addr_tmp, v_init, group_sz.x * i * 4);
			}
			else
			{
				ds_write_dword(1, v_lds_addr_tmp, v_init, 0);
				op3(v_add_u32, v_lds_addr_tmp, group_sz.x * 4, v_lds_addr_tmp);
			}
		}
		delVar(v_lds_addr_tmp);
	}
	s_wait_lgkmcnt(0);
	wrLaber(l_end_lds_atomic_init);

	// -------------------------------------------------------------------------------
	// 第一个 c_in_blk 的第一个 tid_y 做L2 ATOMIC 初始化
	// -------------------------------------------------------------------------------
	Var * l_end_l2_atomic_init = newLaber("END_L2_ATOMIC_INIT");
	if (c_in_l2_atomic_group > 1)
	{
		op2("s_cmpk_eq_i32", s_c_blk_id, 0);
		op1("s_cbranch_scc0", l_end_l2_atomic_init);

		Var *v_init_addr0;
		Var * v_init_addr_tmp = newVgpr("v_l2_addr_tmp", 2, 2);
		if (c_in_l2_split_group > 1)	v_init_addr0 = v_addr_l2;
		else							v_init_addr0 = v_addr_out;
		op2("v_mov_b32", v_init_addr_tmp, v_init_addr0);
		op2("v_mov_b32", *v_init_addr_tmp + 1, *v_init_addr0 + 1);
		for (int i = 0; i < k_out_maps; i++)
		{
			flat_store_dword(1, v_init_addr_tmp, v_init, "off");
			op4(v_addc_u32, v_init_addr_tmp, "vcc", out_chan_stride * 4, v_init_addr_tmp);
			op5(v_addc_co_u32, *v_init_addr_tmp + 1, "vcc", 0, *v_init_addr_tmp + 1, "vcc");
		}
		delVar(v_init_addr_tmp);
	}
	s_wait_lgkmcnt(0);
	wrLaber(l_end_l2_atomic_init);

	// -------------------------------------------------------------------------------
	// 初始化信号槽
	// -------------------------------------------------------------------------------
	Var * l_end_signal_init = newLaber("END_SIGNAL_INIT");
	if (en_l2_sync == true)
	{
		op2("s_cmpk_eq_i32", s_c_blk_id, 0);
		op1("s_cbranch_scc0", l_end_signal_init);
		
		Var * s_tmp = newSgpr("s_tmp");
		op2("s_mov_b32", s_tmp, 0);
		s_store_dword(1, s_tmp, s_addr_sig, 0, true);
		delVar(s_tmp);
	}
	wrLaber(l_end_signal_init);
	
	delVar(v_init);

	lds_wave_sync();	// just for now
/*
	if (c_in_group == 1)
	{
		if (enBias == true)
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

		op2("v_readfirstlane_b32", s_cInBlkId, v_c_l2_blk_id);
		//op1("s_nop", 5);
		op2("s_cmpk_eq_i32", s_cInBlkId, 0);
		op1("s_cbranch_scc0", l_end_init); 
		{
			if ((enRelu == true) && (en_slop_zero == true))
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
				if (enBias == true)
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

			if (enBias == true)
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


			if ((enRelu == true) && (en_slop_zero == true))
			{
				s_wait_lgkmcnt(0);
			}
		}
		wrLaber(l_end_init);
				
		delVar(s_cInBlkId);
		delVar(v_addr_save);
		delVar(v_init);			// you yinhuan
	}
	*/
}
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

void KernelWriterConv1x1::save_result()
{
	// 销毁数据buffer
	delVar(v_in_buff_a);
	delVar(v_in_buff_b);
	delVar(s_wei_buff_a);
	delVar(s_wei_buff_b);
	delVar(s_prefetch);
	
	if (c_in_lds_group > 1)
	{
		if (c_in_lds_atomic_group > 1)		save_to_lds_atomic();
		if (c_in_lds_split_group > 1)		save_to_lds_split();
	}

	if (c_in_l2_group > 1)
	{
		if (c_in_l2_atomic_group > 1)		save_to_l2_atomic();
	}

	if (en_l2_sync == true)
	{
		save_to_output();
	}
}
void KernelWriterConv1x1::save_to_output()
{
	Var * s_exec_bck = newSgpr("s_exec_bck", 2, 2);

	for (int i = 0; i < k_out_maps; i++)
	{
		// debug
		//op2("v_mov_b32", v_debug, 12345);
		//op2("v_cvt_f32_u32", v_debug, v_debug);
		//op2("v_mov_b32", *v_acc_buff + i, v_debug);

		if (Relu == RELU)
		{
			op2("s_mov_b64", *s_exec_bck ^ 2, "exec");
			op3("v_cmpx_lt_f32", "vcc", *v_acc_buff + i, 0);
			op2("v_mov_b32", *v_acc_buff + i, 0);
			op2("s_mov_b64", "exec", *s_exec_bck ^ 2);
		}

		flat_store_dword(1, v_addr_out, *v_acc_buff + i, "off");
		op4(v_addc_u32, v_addr_out, "vcc", out_chan_stride * 4, v_addr_out);
		op5(v_addc_co_u32, *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");
	}

	delVar(s_exec_bck);
}
void KernelWriterConv1x1::save_to_lds_atomic()
{
	Var * l_end_lds_atomic = newLaber("END_LDS_ATOMIC");

	// -------------------------------------------------------------------------------
	// 将当前wave的结果存到lds
	// -------------------------------------------------------------------------------
	Var * v_lds_addr_tmp = newVgpr("v_lds_addr_bck");
	op2("v_mov_b32", v_lds_addr_tmp, v_addr_lds);

	for (int i = 0; i < k_out_maps; i++)
	{
		// debug
		op2("v_mov_b32", v_debug, 1111);
		op2("v_cvt_f32_u32", v_debug, v_debug);
		op2("v_mov_b32", *v_acc_buff + i, v_debug);

		if (group_sz.x * k_out_maps * 4 <= MAX_16BIT_UINT) // 16-bit uint
		{
			op2("ds_add_f32", v_lds_addr_tmp, *v_acc_buff + i, group_sz.x * i * 4);
		}
		else
		{
			op2("ds_add_f32", v_lds_addr_tmp, *v_acc_buff + i, 0);
			op3(v_add_u32, v_lds_addr_tmp, group_sz.x * 4, v_lds_addr_tmp);
		}
	}
	s_wait_lgkmcnt(0);
	delVar(v_lds_addr_tmp);

	// -------------------------------------------------------------------------------
	// 如果不进行split, 则需要同步后将数据从 LDS 读出到VGPR
	// -------------------------------------------------------------------------------
	if (c_in_lds_split_group < 2)
	{
		// -------------------------------------------------------------------------------
		// 同步
		// -------------------------------------------------------------------------------
		lds_wave_sync();

		// -------------------------------------------------------------------------------
		// 最后一个 tid_y 读取LDS到 v_acc_buff
		// -------------------------------------------------------------------------------
		op3("v_cmpx_eq_u32", "vcc", v_tid_y, group_sz.y - 1);
		op1("s_cbranch_execz", l_end_prg);

		for (int i = 0; i < k_out_maps; i++)
		{
			if (group_sz.x * k_out_maps * 4 <= MAX_16BIT_UINT) // 16-bit uint
			{
				ds_read_dword(1, *v_acc_buff + i, v_addr_lds, group_sz.x * i * 4);
			}
			else
			{
				ds_read_dword(1, *v_acc_buff + i, v_addr_lds, 0);
				op3(v_add_u32, v_addr_lds, group_sz.x * 4, v_addr_lds);
			}
		}
		s_wait_lgkmcnt(0);
	}

	wrLaber(l_end_lds_atomic);
}
void KernelWriterConv1x1::save_to_lds_split()
{
	Var * l_end_lds_split = newLaber("END_LDS_SPLIT");
	Var * v_lds_addr_tmp = newVgpr("v_lds_addr_bck");
	Var * v_tmp1 = newVgpr("v_tmp1");

	// -------------------------------------------------------------------------------
	// 如果不进行atomic, 则需要进行存储 然后同步
	// -------------------------------------------------------------------------------
	if (c_in_lds_atomic_group < 2)
	{
		// -------------------------------------------------------------------------------
		// 将当前wave的结果存到lds
		// -------------------------------------------------------------------------------
		op2("v_mov_b32", v_lds_addr_tmp, v_addr_lds);
		for (int i = 0; i < k_out_maps; i++)
		{
			// debug
			op2("v_mov_b32", v_debug, 1111);
			op2("v_cvt_f32_u32", v_debug, v_debug);
			op2("v_mov_b32", *v_acc_buff + i, v_debug);

			if (group_sz.x * k_out_maps * 4 <= MAX_16BIT_UINT) // 16-bit uint
			{
				ds_write_dword(1, v_lds_addr_tmp, *v_acc_buff + i, group_sz.x * i * 4);
			}
			else
			{
				ds_write_dword(1, v_lds_addr_tmp, *v_acc_buff + i, 0);
				op3(v_add_u32, v_lds_addr_tmp, group_sz.x * 4, v_lds_addr_tmp);
			}
		}
		s_wait_lgkmcnt(0);

		// 同步
		lds_wave_sync();
	}
	
	// -------------------------------------------------------------------------------
	// 最后一个 tid_y 读取LDS并累加到 v_acc_buff
	// -------------------------------------------------------------------------------
	op3("v_cmpx_eq_u32", "vcc", v_tid_y, group_sz.y - 1);
	op1("s_cbranch_execz", l_end_prg);

	Var * v_acc_buff1 = newVgpr("v_accum1", k_out_maps);
	Var * v_acc_buff2 = newVgpr("v_accum2", k_out_maps);
	Var * v_acc_buff0 = v_acc_buff1;

	// 第一轮
	{
		// 地址调整: v_addr_lds 指向第一块LDS
		op3("v_lshlrev_b32", v_addr_lds, 2, v_tid_x);
		// 读取第一组			
		for (int i = 0; i < k_out_maps; i++)
		{
			if (group_sz.x * k_out_maps * 4 <= MAX_16BIT_UINT) // 16-bit uint
			{
				ds_read_dword(1, *v_acc_buff + i, v_addr_lds, group_sz.x * i * 4);
			}
			else
			{
				ds_read_dword(1, *v_acc_buff + i, v_addr_lds, 0);
				op3(v_add_u32, v_addr_lds, group_sz.x * 4, v_addr_lds);
			}
		}
		// 交换buffer
		v_acc_buff0 = v_acc_buff1;
	}
	// 循环
	for (int blk = 0; blk < c_in_lds_split_group - 2; blk++)
	{
		// 地址调整
		if (group_sz.x * k_out_maps * (c_in_lds_split_group - 1) * 4 <= MAX_16BIT_UINT)
		{
			op3(v_add_u32, v_addr_lds, group_sz.x * k_out_maps * 4, v_addr_lds);
		}
		// 读取下一组
		for (int i = 0; i < k_out_maps; i++)
		{
			if (group_sz.x * k_out_maps * 4 <= MAX_16BIT_UINT) // 16-bit uint
			{
				ds_read_dword(1, *v_acc_buff0 + i, v_addr_lds, group_sz.x * i * 4);
			}
			else
			{
				ds_read_dword(1, *v_acc_buff0 + i, v_addr_lds, 0);
				op3(v_add_u32, v_addr_lds, group_sz.x * 4, v_addr_lds);
			}
		}
		// 交换buffer
		if (v_acc_buff0 == v_acc_buff1)
		{
			v_acc_buff0 = v_acc_buff2;
		}
		else
		{
			v_acc_buff0 = v_acc_buff1;
		}
		// 累加上一组
		if (blk > 0)
		{
			if (k_out_maps > 15)
				s_wait_lgkmcnt(15);
			else
				s_wait_lgkmcnt(k_out_maps);
			for (int i = 0; i < k_out_maps; i++)
			{
				op3("v_add_f32", *v_acc_buff + i, *v_acc_buff + i, *v_acc_buff0 + i);
			}
		}
	}
	// 最后一轮
	{
		// 地址调整
		if (group_sz.x * k_out_maps * (c_in_lds_split_group - 1) * 4 <= MAX_16BIT_UINT)
		{
			op3(v_add_u32, v_addr_lds, group_sz.x * k_out_maps * 4, v_addr_lds);
		}
		// 读取最后一组
		for (int i = 0; i < k_out_maps; i++)
		{
			if (group_sz.x * k_out_maps * 4 <= MAX_16BIT_UINT) // 16-bit uint
			{
				ds_read_dword(1, *v_acc_buff0 + i, v_addr_lds, group_sz.x * i * 4);
			}
			else
			{
				ds_read_dword(1, *v_acc_buff0 + i, v_addr_lds, 0);
				op3(v_add_u32, v_addr_lds, group_sz.x * 4, v_addr_lds);
			}
		}
		// 累加
		s_wait_lgkmcnt(0);
		for (int i = 0; i < k_out_maps; i++)
		{
			if (c_in_lds_split_group == 2)
			{
				op3("v_add_f32", *v_acc_buff + i, *v_acc_buff + i, *v_acc_buff1 + i);
			}
			else
			{
				op3("v_add_f32", *v_acc_buff + i, *v_acc_buff + i, *v_acc_buff1 + i);
				op3("v_add_f32", *v_acc_buff + i, *v_acc_buff + i, *v_acc_buff2 + i);
			}
		}
	}

	wrLaber(l_end_lds_split);

	delVar(v_tmp1);
	delVar(v_lds_addr_tmp);
}
void KernelWriterConv1x1::save_to_l2_atomic()
{
	// v_newVal = v_src_cmp
	// v_prevVal = v_src_cmp + 1
	Var * v_src_cmp = newVgpr("v_src_cmp", 2, 2);
	Var * v_rtn = newVgpr("v_rtn");
	Var * v_tmp = newVgpr("v_tmp");
	Var * s_exec2 = newSgpr("s_exec2", 2, 2);
	Var * s_exec_bck = newSgpr("s_exec_bck", 2, 2);
	Var * v_addr_tmp = newVgpr("v_addr_tmp", 2, 2);
	Var * v_atomic_addr;

	// -------------------------------------------------------------------------------
	// 确定地址
	// -------------------------------------------------------------------------------
	if (c_in_l2_split_group > 1)	v_atomic_addr = v_addr_l2;
	else							v_atomic_addr = v_addr_out;

	// -------------------------------------------------------------------------------
	// 将当前wave的结果存到l2
	// -------------------------------------------------------------------------------
	Var * l_l2_atomic;
	op2("s_mov_b64", *s_exec_bck ^ 2, "exec");
	op2("v_mov_b32", v_addr_tmp, v_atomic_addr);
	op2("v_mov_b32", *v_addr_tmp + 1, *v_atomic_addr + 1);
	for (int i = 0; i < k_out_maps; i++)
	{
		// debug
		op2("v_mov_b32", v_debug, 1234);
		op2("v_cvt_f32_u32", v_debug, v_debug);
		op2("v_mov_b32", *v_acc_buff + i, v_debug);

		flat_load_dword(1, *v_src_cmp + 1, v_addr_tmp, "off", 0, true);
		s_wait_vmcnt(0);
		l_l2_atomic = newLaber("L2_ATOMIC_" + d2s(i));
		wrLaber(l_l2_atomic);
		{
			if (Relu == PRELU)
			{
				/* -------------------------------------------------------------------------------
				if (old < 0)
				{
					tmp = 1 / slop;
					old = old * tmp;
				}
				xin = old + acc;
				if (xin < 0)
					xin *= slop;
				------------------------------------------------------------------------------- */
				op2("s_mov_b64", *s_exec2 ^ 2, "exec");
				op2("v_mov_b32", v_tmp, *v_src_cmp + 1);
				op3("v_cmpx_lt_f32", "vcc", *v_src_cmp + 1, 0);
				op2("v_rcp_f32", v_tmp, s_slop);				// v_tmp = 1 / s_slop
				op3("v_mul_f32", v_tmp, *v_src_cmp + 1, v_tmp);	// v_tmp = old / s_slop
				op2("s_mov_b64", "exec", *s_exec2 ^ 2);
				op3("v_add_f32", v_src_cmp, v_tmp, *v_acc_buff + i);
				op3("v_cmpx_lt_f32", "vcc", v_src_cmp, 0);
				op3("v_mul_f32", v_src_cmp, v_src_cmp, s_slop);
				op2("s_mov_b64", "exec", *s_exec2 ^ 2);
			}
			else
			{
				op3("v_add_f32", v_src_cmp, *v_src_cmp + 1, *v_acc_buff + i);
			}

			if (IsaArch == E_IsaArch::Gfx800)
			{
				op3("flat_atomic_cmpswap", v_rtn, *v_addr_tmp ^ 2, *v_src_cmp ^ 2, true);
			}
			else if (IsaArch == E_IsaArch::Gfx900)
			{
				op4("global_atomic_cmpswap", v_rtn, *v_addr_tmp ^ 2, *v_src_cmp ^ 2, "off", true);
			}
			s_wait_vmcnt(0);
			op3("v_cmpx_neq_f32", "vcc", *v_src_cmp + 1, v_rtn);
			op2("v_mov_b32", *v_src_cmp + 1, v_rtn);
			op1("s_cbranch_execnz", l_l2_atomic);
		}

		op0("s_barrier");
		op2("s_mov_b64", "exec", *s_exec_bck ^ 2);
		op4(v_addc_u32, v_addr_tmp, "vcc", out_chan_stride * 4, v_addr_tmp);
		op5(v_addc_co_u32, *v_addr_tmp + 1, "vcc", 0, *v_addr_tmp + 1, "vcc");
	}

	// -------------------------------------------------------------------------------
	// 如果 Relu 或者需要进行 split, 则需要同步后将结果读出
	// -------------------------------------------------------------------------------
	op2("v_mov_b32", v_addr_tmp, v_atomic_addr);
	op2("v_mov_b32", *v_addr_tmp + 1, *v_atomic_addr + 1);
	if (en_l2_sync == true)
	{
		// -------------------------------------------------------------------------------
		// 同步
		// -------------------------------------------------------------------------------
		l2_wave_sync();

		// -------------------------------------------------------------------------------
		// 最后一个 tid_y 读取L2到 v_acc_buff
		// -------------------------------------------------------------------------------
		for (int i = 0; i < k_out_maps; i++)
		{
			flat_load_dword(1, *v_acc_buff + i, v_addr_tmp, "off");
			op4(v_addc_u32, v_addr_tmp, "vcc", out_chan_stride * 4, v_addr_tmp);
			op5(v_addc_co_u32, *v_addr_tmp + 1, "vcc", 0, *v_addr_tmp + 1, "vcc");
		}
		s_wait_vmcnt(0);
	}

	delVar(v_src_cmp);
	delVar(v_rtn);
	delVar(v_tmp);
	delVar(s_exec2);
	delVar(s_exec_bck);
}
void KernelWriterConv1x1::save_to_l2_split()
{
}

/************************************************************************************/
/* wave 间同步																		*/
/************************************************************************************/
void KernelWriterConv1x1::lds_wave_sync()
{
	op0("s_barrier");
}
void KernelWriterConv1x1::l2_wave_sync()
{
	// -------------------------------------------------------------------------------
	// 写信号
	// -------------------------------------------------------------------------------
	Var * s_exec_bck = newSgpr("exec_bck", 2, 2);
	Var * v_addr_sig = newVgpr("v_addr_sig", 2, 2);
	Var * s_sig = newSgpr("s_sig");
	Var * v_sig = newVgpr("v_sig");

	op2("s_mov_b64", *s_exec_bck ^ 2, "exec");
	op2("v_mov_b32", v_addr_sig, s_addr_sig);
	op2("v_mov_b32", *v_addr_sig + 1, *s_addr_sig + 1);
	op2("s_mov_b64", "exec", 1);
	op2("v_mov_b32", v_sig, 1);
	op3("flat_atomic_add", v_sig, *v_addr_sig ^ 2, v_sig);
	op2("s_mov_b64", "exec", *s_exec_bck ^ 2);
	s_wait_vmcnt(0);

	// -------------------------------------------------------------------------------
	// 最后一个 c_in_block 的 wave 读取信号
	// -------------------------------------------------------------------------------
	Var * l_l2_sync = newLaber("BTWN_GP_SYNC");
	op2("s_cmpk_eq_i32", s_c_l2_blk_id, c_in_l2_group - 1);
	op1("s_cbranch_scc0", l_end_prg);
	wrLaber(l_l2_sync);
	{
		s_load_dword(1, s_sig, s_addr_sig, 0, true);
		s_wait_lgkmcnt(0);
		op2("s_cmpk_eq_i32", s_sig, c_in_l2_group);
		op1("s_cbranch_scc0", l_l2_sync);
	}
}

/************************************************************************************/
/* 测试																				*/
/************************************************************************************/
void KernelWriterConv1x1::print_kernel_param()
{
	PRINT_SEPARATOR3();
	OUTPUT("- Kernel Param:");
	OUTPUT("- 	c_lds_atomic = %d, \tc_lds_split = %d", c_in_lds_atomic_group, c_in_lds_split_group);
	OUTPUT("- 	c_l2_atomic = %d, \tc_l2_split = %d", c_in_l2_atomic_group, c_in_l2_split_group);
	OUTPUT("- 	c_in_maps = %d, \tc_in_group = %d", c_in_maps, c_in_group);
	OUTPUT("- 	k_out_maps = %d, \tk_out_group = %d", k_out_maps, k_out_group);
	OUTPUT("- 	align = %d, \t\tpix_group = %d", align, pix_group);
	OUTPUT("- 	group_size = %d, %d", group_sz.x, group_sz.y);
	PRINT_SEPARATOR3();
}
E_ReturnState KernelWriterConv1x1::simulate_index()
{
	uint tid_x = 1, tid_y = 0, gid_x;
	int wave_id_x = -1, wave_tid_x = -1;
	int pix_blk_id = -1, k_blk_id = -1, c_l2_blk_id = -1, c_blk_id = -1;
	int pos_id = -1, batch_id = -1, out_id = -1;
	int gbl_in_off = -1, wei_off = -1, gbl_out_off = -1;
	int lds_space_size = 0, l2_space_size = 0;
	int lds_split_id = -1, l2_split_id = -1;

	int *test_idx1 = (int*)malloc(group_num.x * sizeof(int));
	
	for (int grp = 0; grp < group_num.x; grp++)
	{
		gid_x = grp;

		// calcuWaveIndex()
		// wave_id = (group_wave_num_x * (group_sz.y * gid_x + tid_y)) + (tid_x / WAVE_SIZE);
		wave_id_x = (group_wave_num_x * gid_x) + (tid_x / WAVE_SIZE);
		wave_tid_x = tid_x % WAVE_SIZE;

		// calcuBlkIndex();
		if (PCK_order == 321)
		{
			// k_out --> c_in --> pix
			k_blk_id = wave_id_x % k_out_group;
			c_l2_blk_id = wave_id_x / k_out_group % c_in_l2_group;
			pix_blk_id = wave_id_x / k_out_group / c_in_l2_group;
		}
		else if(PCK_order == 312)
		{
			// c_in --> k_out --> pix
			c_l2_blk_id = wave_id_x % c_in_l2_group;
			k_blk_id = wave_id_x / c_in_l2_group % k_out_group;
			pix_blk_id = wave_id_x / c_in_l2_group / k_out_group;
		}
		else if (PCK_order == 123)
		{
			// pix --> c_in --> k_out
			pix_blk_id = wave_id_x % pix_wave;
			c_l2_blk_id = wave_id_x / pix_wave % c_in_l2_group;
			k_blk_id = wave_id_x / pix_wave / c_in_l2_group;
		}
		else if (PCK_order == 132)
		{
			// pix --> k_out --> c_in
			pix_blk_id = wave_id_x % pix_wave;
			k_blk_id = wave_id_x / pix_wave % k_out_group;
			c_l2_blk_id = wave_id_x / pix_wave / k_out_group;
		}
		else if (PCK_order == 213)
		{
			// c_in --> pix --> k_out
			c_l2_blk_id = wave_id_x % c_in_l2_group;
			pix_blk_id = wave_id_x / c_in_l2_group % pix_wave;
			k_blk_id = wave_id_x / c_in_l2_group / pix_wave;
		}
		else if (PCK_order == 231)
		{
			// k_out --> pix --> c_in
			k_blk_id = wave_id_x % k_out_group;
			pix_blk_id = wave_id_x / k_out_group % pix_wave;
			c_l2_blk_id = wave_id_x / k_out_group / pix_wave;
		}
		c_blk_id = c_in_lds_group * c_l2_blk_id + tid_y;

		// calcuPosIndex()
		pos_id = (pix_blk_id * WAVE_SIZE + wave_tid_x) % in_chan_stride;
		batch_id = (pix_blk_id * WAVE_SIZE + wave_tid_x) / in_chan_stride;
		out_id = k_blk_id * k_out_maps;

		// calcuOffset()
		if (batch_id >= N)
		{
			goto L_END_PRG;
		}
		
		wei_off = (out_id * wei_chan_stride) + (c_blk_id * c_in_maps);
		gbl_in_off = (batch_id * in_batch_stride) + (c_blk_id * c_in_maps * in_chan_stride) + pos_id;
		gbl_out_off = (batch_id * out_batch_stride) + (out_id * out_chan_stride) + pos_id;

		// LDS 
		//if (c_in_lds_group > 1)
		//{
		//	if (lds_method == LDS_SPLIT)
		//	{
		//		lds_space_size = group_sz.x * k_out_maps * c_in_lds_group;
		//		lds_split_id = tid_y % c_in_lds_group;
		//	}
		//	if (lds_method == LDS_ATOMIC)
		//	{
		//		lds_space_size = group_sz.x * k_out_maps;
		//		lds_split_id = 0;
		//	}
		//}

		// L2 
		//if (c_in_l2_group > 1)
		//{
		//	l2_space_size = group_sz.x * k_out_maps * c_in_l2_split_group;
		//
		//	if (l2_method == ATOMIC_SPLIT)
		//	{
		//		l2_split_id = c_l2_blk_id / c_in_l2_atomic_group;
		//	}
		//	if (l2_method == SPLIT_ATOMIC)
		//	{
		//		l2_split_id = c_l2_blk_id % c_in_l2_split_group;
		//	}
		//}

	L_END_PRG:
		test_idx1[grp] = c_blk_id;
	}

	print_index(test_idx1, "c_blk_id");
	free(test_idx1);
	
	return E_ReturnState::FAIL;
}
void KernelWriterConv1x1::save_debug()
{
	//op2("v_cvt_f32_u32", v_debug, v_debug);

	flat_store_dword(1, v_addr_dbg, v_debug, "off");
	op1("s_branch", l_end_prg);
}

