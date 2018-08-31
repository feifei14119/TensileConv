#pragma once
#include "KernelWriter.h" 
#include "IsaWriter.h"

class KernelWriterConv1x1 :public KernelWriterBase
{
public:
	KernelWriterConv1x1():KernelWriterBase()
	{
	}

	bool EnInputOffset = 1;
	bool EnWeightOffset = 1;

	int W = 28, H = 28, C = 192, K = 64, N = 16;
	int K_OUT_MAPS = 16;

protected:
	int arg_in_ptr = 0;
	int arg_wei_ptr = 8;
	int arg_out_ptr = 16;

	int privateSeg;
	int kernelArg;
	int gid_x0;
	int gid_y0;
	int gid_z0;
	int s_ptr_in;
	int s_ptr_wei;
	int s_ptr_out;
	int s_wei_a;
	int s_wei_b;
	int s_fetch;
	int s_loop_cnt;

	int tid_x0;
	int v_addr_in;
	int v_addr_out;
	int v_in_a;
	int v_in_b;
	int v_offset0;
	int v_offset1;
	int v_offset2;
	int v_offset3;
	int v_accum;
	int v_dbg;


	int ENABLE = 1;
	int DISABLE = 0;
	int C_IN_MAPS_ONCE = 8;
	int K_OUT_MAPS_LOG2 = log2(K_OUT_MAPS);
	int K_OUT_GROUP = K / K_OUT_MAPS;
	int K_OUT_GROUP_LOG2 = log2(K_OUT_GROUP);
	int K_OUT_GROUP_MOD_MASK = modMask(K_OUT_GROUP);
	int PIX_PER_GROUP = 64;
	int PIX_PER_GROUP_LOG2 = log2(PIX_PER_GROUP);

	int IN_CHANNEL_STRIDE = W * H;
	int IN_BATCH_STRIDE = W * H * C;
	int OUT_CHANNEL_STRIDE = W * H;
	int OUT_BATCH_STRIDE = W * H * K;
	int WEI_CHANNEL_STRIDE = C;

	int LOOP = C / C_IN_MAPS_ONCE / 2;

	char * m_load_input = "m_load_input";
	char * m_load_weight = "m_load_weight";
	char * m_conv_one_feature = "m_conv_one_feature";
	char * m_conv_all_feature = "m_conv_all_feature";
	char * m_save_output = "m_save_output";
	char * m_weight_pre_fatch = "m_weight_pre_fatch";
	char * m_load_kernel_args = "m_load_kernel_args";

	char * END_PROG = "END_PROG";

	void generateParam()
	{
		ENABLE = 1;
		DISABLE = 0;
		C_IN_MAPS_ONCE = 8;
		K_OUT_MAPS_LOG2 = log2(K_OUT_MAPS);
		K_OUT_GROUP = K / K_OUT_MAPS;
		K_OUT_GROUP_LOG2 = log2(K_OUT_GROUP);
		K_OUT_GROUP_MOD_MASK = modMask(K_OUT_GROUP);
		PIX_PER_GROUP = 64;
		PIX_PER_GROUP_LOG2 = log2(PIX_PER_GROUP);

		IN_CHANNEL_STRIDE = W * H;
		IN_BATCH_STRIDE = W * H * C;
		OUT_CHANNEL_STRIDE = W * H;
		OUT_BATCH_STRIDE = W * H * K;
		WEI_CHANNEL_STRIDE = C;

		LOOP = C / C_IN_MAPS_ONCE / 2;
	}
	
	void startProgram()
	{
		tableCnt = 0;
		sline(KernelName + ":");
		tableCnt = 1;
	}

	void generateCodeObj(std::string * objString)
	{
		tableCnt = 1;
		std::string tmpStr = sblk();
		sline(&tmpStr, ".amd_kernel_code_t");
		tableCnt++;
		sline(&tmpStr, "enable_sgpr_private_segment_buffer = 1");
		sline(&tmpStr, "enable_sgpr_kernarg_segment_ptr = 1");
		sline(&tmpStr, "enable_sgpr_workgroup_id_x = 1");
		sline(&tmpStr, "enable_sgpr_workgroup_id_y = 1");
		sline(&tmpStr, "enable_sgpr_workgroup_id_z = 1");
		sline(&tmpStr, "enable_vgpr_workitem_id = 0");
		sline(&tmpStr, "is_ptr64 = 1");
		sline(&tmpStr, "float_mode = 240");
		sline(&tmpStr, "granulated_wavefront_sgpr_count = " + d2s((sgprCountMax - 1) / 8));
		sline(&tmpStr, "granulated_workitem_vgpr_count = " + d2s((vgprCountMax - 1) / 4));
		sline(&tmpStr, "user_sgpr_count = 6");
		sline(&tmpStr, "wavefront_sgpr_count = " + d2s(sgprCountMax));
		sline(&tmpStr, "workitem_vgpr_count = " + d2s(vgprCountMax));
		sline(&tmpStr, "kernarg_segment_byte_size = 56");
		sline(&tmpStr, "workgroup_group_segment_byte_size = 0");
		tableCnt--;
		sline(&tmpStr, ".end_amd_kernel_code_t");
		sline(&tmpStr, "");

		objString->append(tmpStr);
	}

	void writeKernel()
	{
		sSimuAlloc();
		writeSubFunc();
		eSimuAlloc();
		writeLoadArgs();
		writeCalcuIndex();
		writeMainConv();
	}

	void endProgram()
	{
		tableCnt = 0;
		sline(c2s(END_PROG) + ":");
		tableCnt = 1;
		sline("s_endpgm\n");
		tableCnt = 0;
	}

	void writeSubFunc()
	{
		writeLoadInputFunc();
		writeLoadWeightFunc();
		writeWeightPreFetch();
		writeConvOneFeatureFunc();
		writeConvAllFeatureFunc();
		writeSaveOutput();
	}

	/************************************************************************************/
	/* 获取传入参数																		*/
	/* 注意: 未加入等待,使用时需要加入s_waitcnt											*/
	/************************************************************************************/
	void writeLoadArgs()
	{
		tableCnt = 0;		sline("START_PROG:");		tableCnt = 1;
		isa->inst3("s_load_dwordx2", sgpr(s_ptr_in, 2), sgpr(kernelArg, 2), d2s(arg_in_ptr), "");
		isa->inst3("s_load_dwordx2", sgpr(s_ptr_wei, 2), sgpr(kernelArg, 2), d2s(arg_wei_ptr), "");
		isa->inst3("s_load_dwordx2", sgpr(s_ptr_out, 2), sgpr(kernelArg, 2), d2s(arg_out_ptr), "");
	}

	/************************************************************************************/
	/* 计算下标																			*/
	/************************************************************************************/
	void writeCalcuBlkIndex()
	{
	}

	void writeCalcuIndex()
	{
		int offset;
		int v_tmp1, v_tmp2, v_tmp3, v_tmp4;
		int s_tmp1, s_tmp2, s_tmp3, s_tmp4;
		int v_weiBlkId, v_inBlkId, v_posId, v_batchId, v_outId;

		newSgpr(&s_tmp1);

		newVgpr(&v_tmp1);
		newVgpr(&v_tmp2);
		newVgpr(&v_tmp3);
		newVgpr(&v_weiBlkId);
		newVgpr(&v_inBlkId);
		newVgpr(&v_posId);
		newVgpr(&v_batchId);
		newVgpr(&v_outId);

		// -------------------------------------------------------------------------------
		// out_grp_block = gid % MLO_N_OUT_GROUPS;
		// grp_id0_faked = gid / MLO_N_OUT_GROUPS;
		// -------------------------------------------------------------------------------
		isa->inst3("v_and_b32", vgpr(v_weiBlkId), d2s(K_OUT_GROUP_MOD_MASK), sgpr(gid_x0), "");
		isa->inst3("v_lshrrev_b32", vgpr(v_inBlkId), d2s(K_OUT_GROUP_LOG2), sgpr(gid_x0), "");

		// -------------------------------------------------------------------------------
		// pos_id   = (grp_id0_faked * FIXED_WORKGROUP_SIZE + tid) % MLO_IN_CHANNEL_STRIDE;
		// batch_id = (grp_id0_faked * FIXED_WORKGROUP_SIZE + tid) / MLO_IN_CHANNEL_STRIDE;
		// out_id   = out_grp_block * MLO_N_LCL_OUT_MAPS;
		// -------------------------------------------------------------------------------
		isa->inst4("v_lshl_add_u32", vgpr(v_tmp1), vgpr(v_inBlkId), d2s(PIX_PER_GROUP_LOG2), vgpr(tid_x0), "");
		isa->inst2("v_mov_b32", vgpr(v_tmp2), d2s(IN_CHANNEL_STRIDE), "");
		gas->FUNC("mv_div_u32", 4, s2c(vgpr(v_tmp1)), s2c(vgpr(v_tmp2)), s2c(vgpr(v_batchId)), s2c(vgpr(v_posId)));
		isa->inst3("v_lshlrev_b32", vgpr(v_outId), d2s(K_OUT_MAPS_LOG2), vgpr(v_weiBlkId), "");

		// -------------------------------------------------------------------------------
		// if (batch_id >= BATCHSIZE)
		//		return;
		// -------------------------------------------------------------------------------
		isa->inst2("v_mov_b32", vgpr(v_tmp1), d2s(N), "");
		isa->inst3("v_cmpx_lt_u32", "exec", vgpr(v_batchId), vgpr(v_tmp1), "");

		// -------------------------------------------------------------------------------
		// gbl_in_off  = batch_id * MLO_IN_BATCH_STRIDE + pos;
		// -------------------------------------------------------------------------------
		isa->inst2("v_mov_b32", vgpr(v_tmp1), d2s(IN_BATCH_STRIDE), "");
		isa->inst3("v_mul_u32_u24", vgpr(v_tmp2), vgpr(v_batchId), vgpr(v_tmp1), "");
		isa->inst4("v_add_co_u32", vgpr(v_tmp3), "vcc", vgpr(v_tmp2), vgpr(v_posId), "");
		isa->inst3("v_lshlrev_b32", vgpr(v_tmp3), d2s(2), vgpr(v_tmp3), "");					// gbl_in_off
		offset = IN_CHANNEL_STRIDE * 2 * 4;
		isa->inst2("v_mov_b32", vgpr(v_tmp1), d2hs(offset), "");
		isa->inst2("v_mov_b32", vgpr(v_offset0), vgpr(v_tmp3), "");
		isa->inst4("v_add_co_u32", vgpr(v_offset1), "vcc", vgpr(v_offset0), vgpr(v_tmp1), "");
		isa->inst4("v_add_co_u32", vgpr(v_offset2), "vcc", vgpr(v_offset1), vgpr(v_tmp1), "");
		isa->inst4("v_add_co_u32", vgpr(v_offset3), "vcc", vgpr(v_offset2), vgpr(v_tmp1), "");

		// -------------------------------------------------------------------------------
		// wei_off = out_id * MLO_WEI_CHANNEL_STRIDE;
		// -------------------------------------------------------------------------------
		isa->inst2("v_mov_b32", vgpr(v_tmp1), d2s(WEI_CHANNEL_STRIDE), "");
		isa->inst3("v_mul_u32_u24", vgpr(v_tmp1), vgpr(v_outId), vgpr(v_tmp1), "");
		isa->inst2("v_readfirstlane_b32", sgpr(s_tmp1), vgpr(v_tmp1), "");
		isa->inst3("s_lshl_b32", sgpr(s_tmp1), sgpr(s_tmp1), d2s(2), "");
		isa->s_waitcnt("lgkmcnt", 0);
		isa->inst3("s_add_u32", sgpr(s_ptr_wei), sgpr(s_ptr_wei), sgpr(s_tmp1), "");
		isa->inst3("s_addc_u32", sgpr_h(s_ptr_wei), d2s(0), sgpr_h(s_ptr_wei), "");

		// -------------------------------------------------------------------------------
		// gbl_out_off = batch_id * MLO_OUT_BATCH_STRIDE + out_id * MLO_OUT_CHANNEL_STRIDE + pos;
		// -------------------------------------------------------------------------------
		isa->inst2("v_mov_b32", vgpr(v_tmp1), d2s(OUT_BATCH_STRIDE), "");
		isa->inst3("v_mul_u32_u24", vgpr(v_tmp1), vgpr(v_batchId), vgpr(v_tmp1), "");
		isa->inst2("v_mov_b32", vgpr(v_tmp2), d2s(OUT_CHANNEL_STRIDE), "");
		isa->inst3("v_mul_u32_u24", vgpr(v_tmp2), vgpr(v_outId), vgpr(v_tmp2), "");
		isa->inst4("v_add3_u32", vgpr(v_tmp3), vgpr(v_tmp1), vgpr(v_tmp2), vgpr(v_posId), "");
		isa->inst3("v_lshlrev_b32", vgpr(v_tmp3), d2s(2), vgpr(v_tmp3), "");					// gbl_out_off
		isa->inst2("v_mov_b32", vgpr_h(v_addr_out), sgpr_h(s_ptr_out), "");
		isa->inst4("v_add_co_u32", vgpr(v_addr_out), "vcc", sgpr(s_ptr_out), vgpr(v_tmp3), "");
		isa->inst5("v_addc_co_u32", vgpr_h(v_addr_out), "vcc", d2s(0), vgpr_h(v_addr_out), "vcc", "");
		sline("");

		delSgpr(&s_tmp1);

		delVgpr(&v_outId);
		delVgpr(&v_batchId);
		delVgpr(&v_posId);
		delVgpr(&v_inBlkId);
		delVgpr(&v_weiBlkId);
		delVgpr(&v_tmp3);
		delVgpr(&v_tmp2);
		delVgpr(&v_tmp1);
	}

	/************************************************************************************/
	/* 卷积主体																			*/
	/************************************************************************************/
	void writeMainConv()
	{
		char * t_acc = "t_acc";
		int loop;

		tableCnt = 0;
		sline("MAIN_CONV:");
		tableCnt = 1;

		// -------------------------------------------------------------------------------
		// 初始化:
		// -------------------------------------------------------------------------------
		newSgpr(&s_wei_a, C_IN_MAPS_ONCE, 8);
		newSgpr(&s_wei_b, C_IN_MAPS_ONCE, 8);
		newSgpr(&s_fetch, K_OUT_MAPS);
		newSgpr(&s_loop_cnt);

		newVgpr(&v_accum, K_OUT_MAPS);
		newVgpr(&v_in_a, C_IN_MAPS_ONCE);
		newVgpr(&v_in_b, C_IN_MAPS_ONCE);

		gas->setGPR(t_acc, v_accum);
		loop = K_OUT_MAPS;
		gas->sFOR(loop);
		isa->inst2("v_mov_b32", vgpr(t_acc), d2s(0), "");
		sline(t_acc + c2s(" = ") + t_acc + c2s(" + ") + d2s(1));
		gas->eFOR();
		sline("");

		// -------------------------------------------------------------------------------
		// 循环填充 :
		// -------------------------------------------------------------------------------
		char * wei_offset = "wei_offset";
		gas->FUNC(m_weight_pre_fatch, 0);
		gas->FUNC(m_load_input, 1, d2c(v_in_a));
		isa->inst2("s_mov_b32", sgpr(s_loop_cnt), d2s(LOOP - 1), "");
		sline(wei_offset + c2s(" = 0"));
		sline("");

		// -------------------------------------------------------------------------------
		// 循环体 :
		// -------------------------------------------------------------------------------
		tableCnt = 0;
		sline("CONV_LOOP:");
		tableCnt = 1;

		gas->FUNC(m_load_input, 1, d2c(v_in_b));
		gas->FUNC(m_conv_all_feature, 5, d2c(v_in_a), wei_offset, d2c(8), d2c(DISABLE), d2c(DISABLE));
		gas->FUNC(m_load_input, 1, d2c(v_in_a));
		gas->FUNC(m_conv_all_feature, 5, d2c(v_in_b), wei_offset, d2c(8), d2c(ENABLE), d2c(ENABLE));
		gas->FUNC(m_weight_pre_fatch, 0);
		sline("");

		// -------------------------------------------------------------------------------
		// 循环控制 :
		// -------------------------------------------------------------------------------
		isa->inst3("s_sub_u32", sgpr(s_loop_cnt), sgpr(s_loop_cnt), d2s(1), "");
		//isa->inst3("s_and_b32", sgpr(s_loop_cnt), sgpr(s_loop_cnt), "exec_lo", "");
		isa->inst2("s_cmpk_eq_u32", sgpr(s_loop_cnt), d2s(0), "");
		isa->inst1("s_cbranch_scc0", "CONV_LOOP", "");
		sline("");

		// -------------------------------------------------------------------------------
		// 循环排空 :
		// -------------------------------------------------------------------------------
		gas->FUNC(m_load_input, 1, d2c(v_in_b));
		gas->FUNC(m_conv_all_feature, 5, d2c(v_in_a), wei_offset, d2c(8), d2c(DISABLE), d2c(DISABLE));
		gas->FUNC(m_conv_all_feature, 5, d2c(v_in_b), wei_offset, d2c(0), d2c(ENABLE), d2c(DISABLE));
		sline("");

		// -------------------------------------------------------------------------------
		// 存储 :
		// -------------------------------------------------------------------------------
		gas->FUNC(m_save_output, 0);
		sline("");
	}

	void sSimuAlloc()
	{
		newSgpr(&privateSeg, 4);
		newSgpr(&kernelArg, 2);
		newSgpr(&gid_x0);
		newSgpr(&gid_y0);
		newSgpr(&gid_z0);
		newSgpr(&s_ptr_in, 2, 2);
		newSgpr(&s_ptr_wei, 2, 2);
		newSgpr(&s_ptr_out, 2, 2);
		newSgpr(&s_wei_a, C_IN_MAPS_ONCE, 8);
		newSgpr(&s_wei_b, C_IN_MAPS_ONCE, 8);
		newSgpr(&s_fetch, K_OUT_MAPS);
		newSgpr(&s_loop_cnt);

		newVgpr(&tid_x0);
		newVgpr(&v_addr_in, 2, 2);
		newVgpr(&v_addr_out, 2, 2);
		newVgpr(&v_offset0, 2, 2);
		newVgpr(&v_offset1, 2, 2);
		newVgpr(&v_offset2, 2, 2);
		newVgpr(&v_offset3, 2, 2);
		newVgpr(&v_accum, K_OUT_MAPS);
		newVgpr(&v_in_a, C_IN_MAPS_ONCE);
		newVgpr(&v_in_b, C_IN_MAPS_ONCE);
		newVgpr(&v_dbg, 1);
	}

	void eSimuAlloc()
	{
		delVgpr(&v_in_b);
		delVgpr(&v_in_a);
		delVgpr(&v_accum);

		delSgpr(&s_loop_cnt);
		delSgpr(&s_fetch);
		delSgpr(&s_wei_b);
		delSgpr(&s_wei_a);
	}
	 
	/************************************************************************************/
	/* 读取8输入通道的input data															*/
	/* for (uint j = 0; j < MLO_N_LCL_IN_MAPS_ONCE; ++j)	// read 8 input channel     */
	/* {                                                                                */
	/*     dat[j] = *p;                                                                 */
	/*     p += MLO_IN_CHANNEL_STRIDE;                                                  */
	/* }                                                                                */
	/*																					*/
	/* offset 为12 bit，故此当W/H <= 28时(W*H < 12bit)时可使用							*/
	/************************************************************************************/
	void writeLoadInputFunc()
	{
		char * destBase = "param1";
		char * r_destBase = "\\param1";

		char * t_dat = "tmp1";
		char * t_voffset = "tmp2";

		int loop, step;

		gas->sFUNC(m_load_input, 1, destBase);
		gas->refGPR(t_dat, destBase);

		if (EnInputOffset)
		{
			gas->setGPR(t_voffset, v_offset0);

			loop = C_IN_MAPS_ONCE / 2;
			gas->sFOR(loop);
			
			isa->inst3("global_load_dword", vgpr(t_dat), vgpr(t_voffset, 2), sgpr(s_ptr_in, 2), "", 0);
			sline(t_dat + c2s(" = ") + t_dat + c2s(" + ") + d2s(1));
			isa->inst3("global_load_dword", vgpr(t_dat), vgpr(t_voffset, 2), sgpr(s_ptr_in, 2), "", IN_CHANNEL_STRIDE * 4);
			sline(t_dat + c2s(" = ") + t_dat + c2s(" + ") + d2s(1));
			sline(t_voffset + c2s(" = ") + t_voffset + c2s(" + ") + d2s(2));
			gas->eFOR();
			step = IN_CHANNEL_STRIDE * 4 * 8;
			isa->inst3("s_add_u32", sgpr(s_ptr_in), d2s(step), sgpr(s_ptr_in), "");
			isa->inst3("s_addc_u32", sgpr_h(s_ptr_in), d2s(0), sgpr_h(s_ptr_in), "");
		}
		else
		{
			loop = C_IN_MAPS_ONCE;
			gas->sFOR(loop);
			isa->inst3("global_load_dword", vgpr(t_dat), vgpr(v_addr_in, 2), "off", "");
			step = IN_CHANNEL_STRIDE * 4;
			isa->inst4("v_add_co_u32", vgpr(v_addr_in), "vcc", d2s(step), vgpr(v_addr_in), "");
			isa->inst5("v_addc_co_u32", vgpr_h(v_addr_in), "vcc", d2s(0), vgpr_h(v_addr_in), "vcc", "");
			sline(t_dat + c2s(" = ") + t_dat + c2s(" + ") + d2s(1));
			gas->eFOR();
		}

		gas->eFUNC();
	}

	/************************************************************************************/
	/* 读取1个输出feature的当前通道的8个weight:	                                        */
	/* for (uint o = 0; o < MLO_N_LCL_IN_MAPS_ONCE; ++o)	// 8 input channel          */
	/* {                                                                                */
	/*     weights[o] = *w;                                                             */
	/*     w ++;                                                                        */
	/* }                                                                                */
	/************************************************************************************/
	void writeLoadWeightFunc()
	{
		char * weiBase = "param1";
		char * offset = "param2";
		char * r_weiBase = "\\param1";
		char * r_offset = "\\param2";

		int step;

		gas->sFUNC(m_load_weight, 2, weiBase, offset);

		//gas->sIF(r_enOffset, "==", d2s(ENABLE));
		if (EnWeightOffset)
		{
			isa->inst3("s_load_dwordx8", sgpr(r_weiBase, 8), sgpr(s_ptr_wei, 2), c2hs(r_offset), "");
			step = WEI_CHANNEL_STRIDE * 4;
			sline(r_offset + c2s(" = ") + r_offset + c2s(" + ") + d2s(step));
		}
		//gas->sELSE();
		else
		{
			isa->inst3("s_load_dwordx8", sgpr(r_weiBase, 8), sgpr(s_ptr_wei, 2), d2s(0), "");
			step = WEI_CHANNEL_STRIDE * 4;
			isa->inst3("s_add_u32", sgpr(s_ptr_wei), sgpr(s_ptr_wei), d2s(step), "");
			isa->inst3("s_addc_u32", sgpr_h(s_ptr_wei), sgpr_h(s_ptr_wei), d2s(0), "");
			//gas->eIF();
		}

		gas->eFUNC();
	}

	/************************************************************************************/
	/* 预读取16个output channel的weight													*/
	/*  	for(c=0;c<192;c+16)//12														*/
	/*  	{																			*/
	/*  		for(k=0;k<16;k++)														*/
	/*  		{																		*/
	/*  			temp[c] = s_load wei;												*/
	/*  		}																		*/
	/* 		s_wait0																		*/
	/* }																				*/
	/************************************************************************************/
	void writeWeightPreFetch()
	{
		char * t_fetch = "tmp1";
		char * imm_offset = "tmp2";
		int loop, step;

		gas->sFUNC(m_weight_pre_fatch, 0);
		gas->setGPR(t_fetch, s_fetch); 
		sline(imm_offset + c2s(" = 0"));

		loop = K_OUT_MAPS;
		step = WEI_CHANNEL_STRIDE * 4;

		gas->sFOR(loop);
		isa->inst3("s_load_dword", sgpr(t_fetch), sgpr(s_ptr_wei, 2), c2hs(imm_offset), "");
		sline(imm_offset + c2s(" = ") + imm_offset + c2s(" + ") + d2s(step));
		sline(t_fetch + c2s(" = ") + t_fetch + c2s(" + ") + d2s(1));
		gas->eFOR();
		gas->eFUNC();
	}

	/************************************************************************************/
	/* 小循环: 计算1个输出特征值的8次乘加, 每次计算1次乘加								*/
	/* for (uint c = 0; c < MLO_N_LCL_IN_MAPS_ONCE; ++c)                                */
	/* {                                                                                */
	/*     accum[o] += dat[c] * weights[c];                                             */
	/* }                                                                                */
	/************************************************************************************/
	void writeConvOneFeatureFunc()
	{
		char * input = "param1";
		char * weight = "param2";
		char * output = "param3";
		char * r_input = "\\param1";
		char * r_weight = "\\param2";
		char * r_output = "\\param3";

		char * t_dat = "tmp3";
		char * t_wei = "tmp4";

		int loop;

		gas->sFUNC(m_conv_one_feature, 3, input, weight, output);
		gas->refGPR(t_dat, input);
		gas->refGPR(t_wei, weight);

		loop = C_IN_MAPS_ONCE;
		gas->sFOR(loop);
		// debug
		//isa->inst2("v_mov_b32", vgpr(t_dat), d2s(1.0), "");
		//isa->inst2("s_mov_b32", sgpr(t_wei), d2s(1.0), "");
		isa->inst4("v_fma_f32", vgpr(r_output), vgpr(t_dat), sgpr(t_wei), vgpr(r_output), "");
		sline(t_dat + c2s(" = ") + t_dat + c2s(" + ") + d2s(1));
		sline(t_wei + c2s(" = ") + t_wei + c2s(" + ") + d2s(1));
		gas->eFOR(); 

		sline(r_output + c2s(" = ") + r_output + c2s(" + ") + d2s(1));

		gas->eFUNC();
	}

	/************************************************************************************/
	/* 中循环: 计算一轮循环(16个输出特征值的)8次乘加. 每次计算1个输出特征						*/
	/* for (uint o = 0; o < MLO_N_LCL_OUT_MAPS; ++o)		// 16 output feature        */
	/* {                                                                                */
	/*     ... ...                                                                      */
	/* }                                                                                */
	/************************************************************************************/
	void writeConvAllFeatureFunc()
	{
		char * input = "param1";
		char * wei_offset = "param2";
		char * wait_cnt = "param3";
		char * en_offset_adj = "param4";
		char * en_addr_adj = "param5";
		char * r_input = "\\param1";
		char * r_wei_offset = "\\param2";
		char * r_wait_cnt = "\\param3";
		char * r_en_offset_adj = "\\param4";
		char * r_en_addr_adj = "\\param5";

		char * t_acc = "tmp1";

		int loop, step;

		gas->sFUNC(m_conv_all_feature, 5, input, wei_offset, wait_cnt, en_offset_adj, en_addr_adj);
		gas->setGPR(t_acc, v_accum);

		gas->sIF(r_en_offset_adj, "==", d2s(ENABLE));
		step = (C_IN_MAPS_ONCE - WEI_CHANNEL_STRIDE * K_OUT_MAPS) * 4;
		sline(r_wei_offset + c2s(" = ") + r_wei_offset + c2s(" + ") + d2s(step));
		gas->sELSE();
		sline(r_wei_offset + c2s(" = ") + d2s(0));
		gas->eIF();
		
		gas->FUNC(m_load_weight, 2, d2c(s_wei_a), r_wei_offset);
		isa->s_waitcnt("lgkmcnt", 0);
		gas->FUNC(m_load_weight, 2, d2c(s_wei_b), r_wei_offset);
		isa->s_waitcnt("vmcnt", r_wait_cnt);
		gas->FUNC(m_conv_one_feature, 3, r_input, d2c(s_wei_a), t_acc);

		loop = K_OUT_MAPS / 2 - 1;
		gas->sFOR(loop);
		isa->s_waitcnt("lgkmcnt", 0);
		gas->FUNC(m_load_weight, 2, d2c(s_wei_a), r_wei_offset);
		gas->FUNC(m_conv_one_feature, 3, r_input, d2c(s_wei_b), t_acc);

		isa->s_waitcnt("lgkmcnt", 0);
		gas->FUNC(m_load_weight, 2, d2c(s_wei_b), r_wei_offset);
		gas->FUNC(m_conv_one_feature, 3, r_input, d2c(s_wei_a), t_acc);
		gas->eFOR();

		gas->sIF(r_en_addr_adj, "==", d2s(ENABLE));
		step = C_IN_MAPS_ONCE * 4 * 2;
		isa->inst3("s_add_u32", sgpr(s_ptr_wei), sgpr(s_ptr_wei), d2s(step), "");
		isa->inst3("s_addc_u32", sgpr_h(s_ptr_wei), sgpr_h(s_ptr_wei), d2s(0), "");
		gas->eIF();

		isa->s_waitcnt("lgkmcnt", 0);
		gas->FUNC(m_conv_one_feature, 3, r_input, d2c(s_wei_b), t_acc);

		gas->eFUNC();
	}

	/************************************************************************************/
	/* 存储:																				*/
	/* 存储16个输出feature的output: 														*/
	/* for (uint j = 0; j < MLO_N_LCL_OUT_MAPS; ++j) // 16 output feature				*/
	/* {																				*/
	/*     *q = weights[j] + dat[j/2];													*/
	/*     q += MLO_OUT_CHANNEL_STRIDE;													*/
	/* } 																				*/
	/************************************************************************************/
	void writeSaveOutput()
	{
		char * t_acc = "tmp1";

		int loop;

		gas->sFUNC(m_save_output, 0);
		gas->setGPR(t_acc, v_accum);
		loop = K_OUT_MAPS;

		gas->sFOR(loop);
		// debug
		//isa->inst2("v_mov_b32", vgpr(t_acc), d2s(192), "");
		//isa->inst2("v_cvt_f32_u32", vgpr(t_acc), vgpr(t_acc), "");

		isa->inst3("global_store_dword", vgpr(v_addr_out, 2), vgpr(t_acc), "off", "");
		isa->inst4("v_add_co_u32", vgpr(v_addr_out), "vcc", d2s(OUT_CHANNEL_STRIDE * 4), vgpr(v_addr_out), "");
		isa->inst5("v_addc_co_u32", vgpr_h(v_addr_out), "vcc", d2s(0), vgpr_h(v_addr_out), "vcc", "");
		sline(t_acc + c2s(" = ") + t_acc + c2s(" + ") + d2s(1));

		gas->eFOR();
		gas->eFUNC();
	}	
};
