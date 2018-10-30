
#include "ConvFwd1x1KernelWriter.h"

using namespace AutoGen;

/************************************************************************************/
/* 计算下标																			*/
/************************************************************************************/
void KernelWriterConv1x1::calcuIndex()
{
	s_ptr_in = newSgpr("s_ptr_in", 2, 2);
	s_ptr_wei = newSgpr("s_ptr_wei", 2, 2);
	s_ptr_out = newSgpr("s_ptr_out", 2, 2);

	v_pixBlkId = newVgpr("v_pixBlkId");
	v_weiBlkId = newVgpr("v_weiBlkId");
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

	s_load_dword(2, s_ptr_in, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_wei, s_kernelArg, 0x08);
	s_load_dword(2, s_ptr_out, s_kernelArg, 0x10);

	calcuBlkIndex();
	calcuPosIndex();
	calcuOffset();

	if (en_input_offset == false)
	{
		delVar(s_ptr_in);
	}
	delVar(s_ptr_wei);
	delVar(s_ptr_out);

	delVar(v_pixBlkId);
	delVar(v_weiBlkId);
	delVar(v_posId);
	delVar(v_batchId);
	delVar(v_outId);
}
void KernelWriterConv1x1::calcuBlkIndex()
{
	Var * s_tmp1 = newSgpr("s_tmp1");
	Var * v_tmp1 = newVgpr("v_tmp1");

	// -------------------------------------------------------------------------------
	// weiBlkId = (gid * FIX_BLK_PER_GROUP + tid / FIX_BLK_SIZE) % MLO_N_OUT_GROUPS;
	// pixBlkId = (gid * FIX_BLK_PER_GROUP + tid / FIX_BLK_SIZE) / MLO_N_OUT_GROUPS;
	// -------------------------------------------------------------------------------
	op3("s_lshl_b32", s_tmp1, s_gid_x, log2(blk_per_group));					// s_tmp1 = gid_x * block_per_group
	op3("v_lshrrev_b32", v_tmp1, log2(FIX_BLK_SIZE), v_tid_x);					// v_tmp1 = tid_x / 
	//isa->inst2("v_readfirstlane_b32", s[s_block_id], vgpr(v_tmp1), "");
	op4("v_add_co_u32", v_tmp1, "vcc", v_tmp1, s_tmp1);							// v_tmp1 = (gid * FIX_BLK_PER_GROUP + tid / FIX_BLK_SIZE
	op3("v_and_b32", v_weiBlkId, modMask(k_out_group), v_tmp1);					// v_weiBlkId = weiBlkId (in_grp_block)
	op3("v_lshrrev_b32", v_pixBlkId, log2(k_out_group), v_tmp1);				// v_pixBlkId = pixBlkId (grp_id0_faked)

	delVar(s_tmp1);
	delVar(v_tmp1);
}
void KernelWriterConv1x1::calcuPosIndex()
{
	Var * v_tmp1 = newVgpr("v_tmp1");
	Var * v_tmp2 = newVgpr("v_tmp2");

	// -------------------------------------------------------------------------------
	// pos_id   = (pixBlkId * PIX_BLK_SIZE + tid % PIX_BLK_SIZE) % MLO_IN_CHANNEL_STRIDE;
	// batch_id = (pixBlkId * PIX_BLK_SIZE + tid % PIX_BLK_SIZE) / MLO_IN_CHANNEL_STRIDE;
	// out_id   = weiBlkId * MLO_N_LCL_OUT_MAPS;
	// -------------------------------------------------------------------------------
	op3("v_lshlrev_b32", v_tmp1, log2(FIX_BLK_SIZE), v_pixBlkId);
	op3("v_and_b32", v_tmp2, modMask(FIX_BLK_SIZE), v_tid_x);
	op4("v_add_co_u32", v_tmp1, "vcc", v_tmp2, v_tmp1);
	op2("v_mov_b32", v_tmp2, in_chan_stride);
	fv_div_u32(v_tmp1, v_tmp2, v_batchId, v_posId);								// v_batchId = batch_id; v_posId = pos
	op3("v_lshlrev_b32", v_outId, log2(k_out_maps), v_weiBlkId);				// v_outId = out_id

	// -------------------------------------------------------------------------------
	// if (batch_id >= BATCHSIZE)
	//		return;
	// -------------------------------------------------------------------------------
	op2("v_mov_b32", v_tmp1, extProbCfg->N);
	op3("v_cmpx_lt_u32", "exec", v_batchId, v_tmp1);

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
	// gbl_in_off  = batch_id * MLO_IN_BATCH_STRIDE + pos;
	// -------------------------------------------------------------------------------
	op2("v_mov_b32", v_tmp1, in_batch_stride);
	op3("v_mul_u32_u24", v_tmp2, v_batchId, v_tmp1);
	op4("v_add_co_u32", v_tmp3, "vcc", v_tmp2, v_posId);
	op3("v_lshlrev_b32", v_tmp3, 2, v_tmp3);								// gbl_in_off

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
	// wei_off = out_id * MLO_WEI_CHANNEL_STRIDE;
	// -------------------------------------------------------------------------------
	op2("v_mov_b32", v_tmp1, wei_chan_stride);
	op3("v_mul_u32_u24", v_tmp1, v_outId, v_tmp1);
	op2("v_readfirstlane_b32", s_tmp1, v_tmp1);
	op3("s_lshl_b32", s_tmp1, s_tmp1, 2);
	s_wait_lgkmcnt(0);
	op3("s_add_u32", s_addr_wei, s_ptr_wei, s_tmp1);
	op3("s_addc_u32", *s_addr_wei + 1, 0, *s_ptr_wei + 1);

	// -------------------------------------------------------------------------------
	// gbl_out_off = batch_id * MLO_OUT_BATCH_STRIDE + out_id * MLO_OUT_CHANNEL_STRIDE + pos;
	// -------------------------------------------------------------------------------
	op2("v_mov_b32", v_tmp1, out_batch_stride);
	op3("v_mul_u32_u24", v_tmp1, v_batchId, v_tmp1);
	op2("v_mov_b32", v_tmp2, out_chan_stride);
	op3("v_mul_u32_u24", v_tmp2, v_outId, v_tmp2);
	op4("v_add3_u32", v_tmp3, v_tmp1, v_tmp2, v_posId);
	op3("v_lshlrev_b32", v_tmp3, 2, v_tmp3);								// gbl_out_off
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
void KernelWriterConv1x1::load_input(Var * in_buff)
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
void KernelWriterConv1x1::load_weight(Var * wei_buff)
{
	s_load_dword(8, wei_buff, s_addr_wei, wei_offset);
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
void KernelWriterConv1x1::save_output()
{
	for (int i = 0; i < k_out_maps; i++)
	{
		// debug
		//op2("v_mov_b32", *v_acc_buff + i, 1.234);
		//op2("v_cvt_f32_u32", *v_acc_buff + i, *v_acc + i);

		flat_store_dword(1, v_addr_out, *v_acc_buff + i, "off");
		op4("v_add_co_u32", v_addr_out, "vcc", out_chan_stride*4, v_addr_out);
		op5("v_addc_co_u32", *v_addr_out + 1, "vcc", 0, *v_addr_out + 1, "vcc");
	}

}
void KernelWriterConv1x1::atomic_add(int n, Var * addr_out, Var * accum)
{
	Var * v_src_cmp = newVgpr("v_src_cmp", 2, 2);
	Var * v_rtn = newVgpr("v_rtn");

	Var * l_atomic_add = newLaber("ATOMIC_ADD_" + d2s(n));
	Var * s_exec_save = newSgpr("s_exec_save");


	// v_prevVal = v_src_cmp + 1
	// v_newVal = v_src_cmp

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

/************************************************************************************/
/* 卷积主体																			*/
/************************************************************************************/
void KernelWriterConv1x1::main_conv()
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
	for (int i = 0; i < k_out_maps; i++)
		op2("v_mov_b32", *v_acc_buff + i, 0);

	// 循环填充
	wei_offset = 0;
	prefetch_weight();
	load_input(v_in_buff_a);

	// 循环体
	Var * s_loop_cnt = newSgpr("s_loop_cnt");
	f_s_loop(s_loop_cnt, conv_loop - 1, "CONV_LOOP");

	load_input(v_in_buff_b);
	conv_one_loop(v_in_buff_a, false);
	load_input(v_in_buff_a);
	conv_one_loop(v_in_buff_b, true);

	f_e_loop(s_loop_cnt, "CONV_LOOP");

	// 循环排空
	load_input(v_in_buff_b);
	conv_one_loop(v_in_buff_a, false);
	conv_last_loop(v_in_buff_b);

	// -------------------------------------------------------------------------------
	// 存储结果:
	// -------------------------------------------------------------------------------
	save_output();

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
void KernelWriterConv1x1::conv_last_loop(Var * in_buff)
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
void KernelWriterConv1x1::conv_one_accum(Var * in_buff, Var * wei_buff, Var * accum)
{
	for (int i = 0; i < c_in_maps_once; i++)
	{
		// debug
		//op2("v_mov_b32", *in_buff + i, 1.23);

		//op4("v_fma_f32", accum, *in_buff + i, *wei_buff + i, accum);
		op3("v_mac_f32", accum, *in_buff + i, *wei_buff + i);
	}
}
