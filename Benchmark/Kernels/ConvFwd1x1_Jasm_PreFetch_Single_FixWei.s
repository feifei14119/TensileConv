.hsa_code_object_version 2,1
.hsa_code_object_isa 9,0,0,"AMD","AMDGPU"

.text
.globl ConvFwd1x1
.p2align 16
.type ConvFwd1x1,@function
.amdgpu_hsa_kernel ConvFwd1x1

.include "gpr_alloc.inc"
.include "common.inc"
.include "inst_wrappers.inc"

/************************************************************************************/
/* 预定义																			*/
/************************************************************************************/
// ==================================================================================
// 常量定义
// ==================================================================================
.set enable,						1
.set disable,						0
//.set W,								28
//.set H,								28
//.set C,								192
//.set K,								64
//.set N,								16
	
.set MLO_IN_CHANNEL_STRIDE,			(W * H)
.set MLO_IN_BATCH_STRIDE,			(H * W * C)
.set MLO_WEI_CHANNEL_STRIDE,		(1 * 1 * C)
.set MLO_WEI_STRIDE,				(1 * 1 * C * K)
.set MLO_OUT_CHANNEL_STRIDE,		(W * H)
.set MLO_OUT_BATCH_STRIDE,			(H * W * K)
	
.set SE_NUM,						(4)
.set CU_PER_SE,						(16)
.set CU_NUM,						(CU_PER_SE * SE_NUM)
.set CU_NUM_LOG2,					(6)
.set CU_NUM_MOD_MASK,				(63)
.set WAVE_SIZE,						(64)
.set WAVE_SIZE_LOG2,				(6)
.set WAVE_SIZE_MOD_MASK,			(63)
	
.set MLO_N_OUT_GROUPS,				4				// 一个输出像素的所有特征，分到几个CU上计算
.set MLO_N_OUT_GROUPS_LOG2,			2				// 乘除法时的移位
.set MLO_N_OUT_GROUPS_MOD_MASK, 	0x3 			// 求余时的mask
.set MLO_N_IN_GROUPS,				1				// 所有输入通道被分到几个CU
.set MLO_N_IN_GROUPS_LOG2,			0				// 乘除法时的移位
.set MLO_N_IN_GROUPS_DIV_MASK,		0x0				// 求余时的mask
	
.set MLO_N_LCL_IN_MAPS,				C				// 每个CU负责计算的输入通道个数
.set MLO_N_LCL_IN_MAPS_ONCE,		8				// 每次循环（不展开）负责计算的输入通道个数
.set MLO_N_LCL_OUT_MAPS,			16				// 每个CU负责计算的输出特征数
.set MLO_N_LCL_OUT_MAPS_LOG2,		4
.set MLO_IN_PIX_GROUP,				64				// 每个workgroup处理多少个input pixal
.set MLO_IN_PIX_GROUP_LOG2,			6
.set MLO_IN_PIX_GROUP_DIV_MASK,		0x3F
	
.set SE_NUM,						4				// shader engin 个数
.set SE_NUM_LOG2,					2
.set SE_NUM_MOD_MASK,				3
.set FIXED_WORKGROUP_SIZE,			64				// 每个CU的线程数
.set FIXED_WORKGROUP_SIZE_LOG2,		6
.set GROUPS_PER_OUT_BATCH,			(W * H / FIXED_WORKGROUP_SIZE * MLO_N_OUT_GROUPS)	// 48
.set CLOOP0,						(MLO_N_LCL_IN_MAPS / MLO_N_LCL_IN_MAPS_ONCE / 2)

.set IN_PIXEL_PER_GROUP,			64
.set IN_PIXEL_PER_GROUP_LOG2,		6
.set IN_PIXEL_PER_GROUP_MOD_MASK,	63

.set WORKGROUP_SIZE,				256
.set WORKGROUP_SIZE_LOG2,			8
.set OUT_BLOCKS_PER_GROUP,			4
.set OUT_BLOCKS_PER_GROUP_LOG2,		2

//.set SIGNAL_NUM_PER_CU,			16				// channel 数除以 catch_line长度取2的n次幂
//.set SIGNAL_NUM_PER_CU_LOG2,		4

.set SIGNAL_REQ_FETCH,				0x1234
.set SIGNAL_EXIT,					0xF0F0
.set SIGNAL_NULL,					0x0

.set Z_BLOCK_GRP_NUM,				(CU_NUM * MLO_N_OUT_GROUPS)
.set Z_BLOCK_GRP_NUM_LOG2,			(CU_NUM_LOG2 + MLO_N_OUT_GROUPS_LOG2)
.set MLO_GRP_NUM,					(W*H*N*K/64/16)
.set MLO_ROUND_NUMBER,				(MLO_GRP_NUM / CU_NUM)
.set MLO_ROUND_LEFT,				(MLO_ROUND_NUMBER * CU_NUM)
.set MLO_Z_ROUND_NUM,			 	(MLO_GRP_NUM / (CU_NUM * MLO_N_OUT_GROUPS))
.set MLO_INBLOCK_LEFT,			 	(MLO_Z_ROUND_NUM * CU_NUM)

.set K_FETCH_SUB,					16				// 每次fetch多少通道的weight: cache_line * K < 4K DWORD
.set K_FETCH_STEP,					16				// cache_line
.set SUB_LOOP,						(K/K_FETCH_SUB)
.set FETCH_LOOP,					(C/K_FETCH_STEP)
.set FETCH_LINE_NUM,				(2)				// 每次fetch 2个catch_line
.set FETCH_LINE_NUM_MOD_MASK,		(1)

// ==================================================================================
// SGPR 初始排布
// ==================================================================================
privateSeg = 0
kernarg = 4
gid_x0 = 6
gid_y0 = 7
gid_z0 = 8

// ==================================================================================
// 输入参数排布
// ==================================================================================
.set in_ptr_off, 0x00
.set wei_ptr_off, 0x08
.set out_ptr_off, 0x10
.set sig_ptr_off, 0x18

// ==================================================================================
// SGPR 使用排布
// ==================================================================================
.GPR_ALLOC_BEGIN
    .SGPR_ALLOC_FROM 9
	.SGPR_ALLOC s_tmp0
    .SGPR_ALLOC s_ptr_in, 2
    .SGPR_ALLOC s_ptr_wei, 2
    .SGPR_ALLOC s_ptr_out, 2
	// wei_a
	.SGPR_ALLOC s_weia0			// 必须16DWORD对齐,以用于s_load_dwordx16!!!!!!!
	.SGPR_ALLOC s_weia1
	.SGPR_ALLOC s_weia2
	.SGPR_ALLOC s_weia3
	.SGPR_ALLOC s_weia4
	.SGPR_ALLOC s_weia5
	.SGPR_ALLOC s_weia6
	.SGPR_ALLOC s_weia7
	// wei_b
	.SGPR_ALLOC s_weib0			// 必须16DWORD对齐,以用于s_load_dwordx16!!!!!!!
	.SGPR_ALLOC s_weib1
	.SGPR_ALLOC s_weib2
	.SGPR_ALLOC s_weib3
	.SGPR_ALLOC s_weib4
	.SGPR_ALLOC s_weib5
	.SGPR_ALLOC s_weib6
	.SGPR_ALLOC s_weib7
	// wei_c
	.SGPR_ALLOC s_weic0			// 必须16DWORD对齐,以用于s_load_dwordx16!!!!!!!
	.SGPR_ALLOC s_weic1
	.SGPR_ALLOC s_weic2
	.SGPR_ALLOC s_weic3
	.SGPR_ALLOC s_weic4
	.SGPR_ALLOC s_weic5
	.SGPR_ALLOC s_weic6
	.SGPR_ALLOC s_weic7
	// wei_d
	.SGPR_ALLOC s_weid0			// 必须16DWORD对齐,以用于s_load_dwordx16!!!!!!!
	.SGPR_ALLOC s_weid1
	.SGPR_ALLOC s_weid2
	.SGPR_ALLOC s_weid3
	.SGPR_ALLOC s_weid4
	.SGPR_ALLOC s_weid5
	.SGPR_ALLOC s_weid6
	.SGPR_ALLOC s_weid7	
	
		
	.SGPR_ALLOC s_ptr_sig, 2
	.SGPR_ALLOC s_ptr_save, 2
	.SGPR_ALLOC s_tmp1, 2
	.SGPR_ALLOC s_tmp2, 2
	.SGPR_ALLOC s_tmp3
	.SGPR_ALLOC s_tmp4
	.SGPR_ALLOC s_loop_cnt
	.SGPR_ALLOC s_signal
	.SGPR_ALLOC s_sig_cnt
	.SGPR_ALLOC s_flag
	
	// ------------------------------------------------------------------------------
    .VGPR_ALLOC_FROM 0
    .VGPR_ALLOC tid
    .VGPR_ALLOC v_addr_in, 2
    .VGPR_ALLOC v_addr_out, 2
	// a
	.VGPR_ALLOC v_data0
	.VGPR_ALLOC v_data1
	.VGPR_ALLOC v_data2
	.VGPR_ALLOC v_data3
	.VGPR_ALLOC v_data4
	.VGPR_ALLOC v_data5
	.VGPR_ALLOC v_data6
	.VGPR_ALLOC v_data7
	// b
	.VGPR_ALLOC v_datb0
	.VGPR_ALLOC v_datb1
	.VGPR_ALLOC v_datb2
	.VGPR_ALLOC v_datb3
	.VGPR_ALLOC v_datb4
	.VGPR_ALLOC v_datb5
	.VGPR_ALLOC v_datb6
	.VGPR_ALLOC v_datb7
	// acc
	.VGPR_ALLOC v_acc0
	.VGPR_ALLOC v_acc1
	.VGPR_ALLOC v_acc2
	.VGPR_ALLOC v_acc3
	.VGPR_ALLOC v_acc4
	.VGPR_ALLOC v_acc5
	.VGPR_ALLOC v_acc6
	.VGPR_ALLOC v_acc7
	.VGPR_ALLOC v_acc8
	.VGPR_ALLOC v_acc9
	.VGPR_ALLOC v_acc10
	.VGPR_ALLOC v_acc11
	.VGPR_ALLOC v_acc12
	.VGPR_ALLOC v_acc13
	.VGPR_ALLOC v_acc14
	.VGPR_ALLOC v_acc15
	
	// offsets
	.VGPR_ALLOC v_io_offset0, 2
	.VGPR_ALLOC v_io_offset1, 2
	.VGPR_ALLOC v_io_offset2, 2
	.VGPR_ALLOC v_io_offset3, 2	
	
	
    //.VGPR_ALLOC v_addr_dbg, 2
	//.VGPR_ALLOC v_dbg0, 2
	//.VGPR_ALLOC v_dbg1
	
	
    .LDS_ALLOC_FROM 0
.GPR_ALLOC_END


/************************************************************************************/
/* 主程序																			*/
/************************************************************************************/
ConvFwd1x1:
    .amd_kernel_code_t	
		enable_sgpr_private_segment_buffer	= 1		// needed by this kernel specially
		enable_sgpr_kernarg_segment_ptr 	= 1		//(use 1 SGPR) 64 bit address of Kernarg segment.
		enable_sgpr_workgroup_id_x 			= 1		//(use 1 SGPR) 32 bit work group id in X dimension of grid for wavefront. Always present.
		enable_sgpr_workgroup_id_y 			= 1		//(use 1 SGPR) 32 bit work group id in Y dimension of grid for wavefront
		enable_sgpr_workgroup_id_z 			= 1		//(use 1 SGPR) 32 bit work group id in Z dimension of grid for wavefront. If present then Work-group Id Y will also be present
		
		enable_vgpr_workitem_id 			= 0		//(use 1 SGPR) 32 bit work item id in Y dimension of work-group for wavefront lane.
		
		is_ptr64 							= 1		// ?
		granulated_wavefront_sgpr_count 	= .AUTO_SGPR_GRANULATED_COUNT
		granulated_workitem_vgpr_count 		= .AUTO_VGPR_GRANULATED_COUNT
		user_sgpr_count 					= 6		// ?
		kernarg_segment_byte_size 			= 56	// kernarg segment size(byte)
		wavefront_sgpr_count 				= .AUTO_SGPR_COUNT
		workitem_vgpr_count 				= .AUTO_VGPR_COUNT
		float_mode 							= 240	// ?
		workgroup_group_segment_byte_size 	= 0		//[caculated] group segment memory required by a work-group in bytes		
	.end_amd_kernel_code_t
		
/************************************************************************************/
/* 读取8输入通道的input data														*/
/* for (uint j = 0; j < MLO_N_LCL_IN_MAPS_ONCE; ++j)	// read 8 input channel     */
/* {                                                                                */
/*     dat[j] = *p;                                                                 */
/*     p += MLO_IN_CHANNEL_STRIDE;                                                  */
/* }                                                                                */
/*																					*/
/* offset 为12 bit，故此当W/H <= 28时(W*H < 12bit)时可使用							*/
/************************************************************************************/
.macro m_load_input 			dest_base, en_offset
	v_dat = \dest_base	
	.if (\en_offset == enable)
		voffset = v_io_offset0
		.rept (MLO_N_LCL_IN_MAPS_ONCE / 2)
			global_load_dword 	v[v_dat], v[voffset:voffset + 1], s[s_ptr_in:s_ptr_in + 1]	offset:0
			v_dat = v_dat + 1
			global_load_dword 	v[v_dat], v[voffset:voffset + 1], s[s_ptr_in:s_ptr_in + 1]	offset : 0x0 + MLO_IN_CHANNEL_STRIDE * 4
			v_dat = v_dat + 1
			voffset = voffset + 2
		.endr
		s_add_u32    			s[s_ptr_in], 0x0 + MLO_IN_CHANNEL_STRIDE * 4 * 8, s[s_ptr_in]
		s_addc_u32   			s[s_ptr_in + 1], 0, s[s_ptr_in + 1]
	.else
		.rept (MLO_N_LCL_IN_MAPS_ONCE)
			global_load_dword 	v[v_dat], v[v_addr_in:v_addr_in+1], off						// v_dat[1..7] = *v_addr_in
			v_add_co_u32 		v[v_addr_in], vcc, 0x0 + MLO_IN_CHANNEL_STRIDE * 4, v[v_addr_in]
			v_addc_co_u32 		v[v_addr_in+1], vcc, 0, v[v_addr_in+1], vcc					// v_addr_in += MLO_IN_CHANNEL_STRIDE
			v_dat = v_dat + 1
		.endr
	.endif
.endm

/************************************************************************************/
/* 读取1个输出feature的当前通道的8个weight:	                                        */
/* for (uint o = 0; o < MLO_N_LCL_IN_MAPS_ONCE; ++o)	// 8 input channel          */
/* {                                                                                */
/*     weights[o] = *w;                                                             */
/*     w ++;                                                                        */
/* }                                                                                */
/************************************************************************************/
.macro m_load_weight 			wei_base, imm_offset, en_offset
	.if (\en_offset == enable)
		s_load_dwordx8 			s[\wei_base:\wei_base+7], s[s_ptr_wei:s_ptr_wei+1], 0x0 + \imm_offset	// s_weia[0..15] = ptr_wei (s_weia0需要16DWORD对齐)	
		\imm_offset = \imm_offset + MLO_WEI_CHANNEL_STRIDE * 4
	.else
		s_load_dwordx8 			s[\wei_base:\wei_base+7], s[s_ptr_wei:s_ptr_wei+1], 0x0			// s_weia[0..15] = ptr_wei (s_weia0需要16DWORD对齐)	
		s_add_u32 				s[s_ptr_wei], s[s_ptr_wei], MLO_WEI_CHANNEL_STRIDE * 4			// weight地址调整: w += MLO_WEI_CHANNEL_STRIDE
		s_addc_u32 				s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0								// s_ptr_wei += MLO_WEI_CHANNEL_STRIDE (DWORD寻址)
	.endif
.endm

/************************************************************************************/
/* 小循环: 计算1个输出特征值的8次乘加, 每次计算1次乘加								*/
/* for (uint c = 0; c < MLO_N_LCL_IN_MAPS_ONCE; ++c)                                */
/* {                                                                                */
/*     accum[o] += dat[c] * weights[c];                                             */
/* }                                                                                */
/************************************************************************************/
.macro m_conv_once 				input, weight, output
	v_dat = \input
	s_wei = \weight
	.rept MLO_N_LCL_IN_MAPS_ONCE
		//v_mov_b32 			v[\output], s[s_tmp1]											// ;for debug
		//v_cvt_f32_u32			v[\output], v[\output]											// ;for debug
		//v_add_f32 			v[\output], s[s_wei], v[\output]								// ;for debug		
		//s_mov_b32				s[s_wei], 0x01													// ;for debug
		//v_mov_b32				v[v_dat], 0x01													// ;for debug
		v_fma_f32 				v[\output], v[v_dat], s[s_wei], v[\output]						// v_tmp1 = accum += v_dat[0..7] * s_wei[0..7]
		v_dat = v_dat + 1
		s_wei = s_wei + 1
	.endr
	\output = \output + 1
.endm

/************************************************************************************/
/* 中循环: 计算一轮循环(16个输出特征值的)8次乘加. 每次计算1个输出特征				*/
/* for (uint o = 0; o < MLO_N_LCL_OUT_MAPS; ++o)		// 16 output feature        */
/* {                                                                                */
/*     ... ...                                                                      */
/* }                                                                                */
/************************************************************************************/
.macro m_cacul_all_feature 		input, wei_offset, wait_cnt, en_offset_adj, en_addr_adj
	// -------------------------------------------------------------------------------
	// 地址指针: 指向16个输出feature的第一个
	// -------------------------------------------------------------------------------
	v_acc = v_acc0
	.if (\en_offset_adj == enable)
		\wei_offset = \wei_offset + (MLO_N_LCL_IN_MAPS_ONCE - MLO_WEI_CHANNEL_STRIDE * MLO_N_LCL_OUT_MAPS) * 4
	.else
		\wei_offset = 0
	.endif
	
	m_load_weight 				s_weia0, \wei_offset, enable
	s_waitcnt 					lgkmcnt(0)
	m_load_weight 				s_weib0, \wei_offset, enable
	s_waitcnt					vmcnt(\wait_cnt)
	m_conv_once 				\input, s_weia0, v_acc
	
	.rept MLO_N_LCL_OUT_MAPS / 2 - 1
		s_waitcnt 				lgkmcnt(0)
		m_load_weight 			s_weia0, \wei_offset, enable
		m_conv_once 			\input, s_weib0, v_acc
				
		s_waitcnt 				lgkmcnt(0)		
		m_load_weight 			s_weib0, \wei_offset, enable
		m_conv_once 			\input, s_weia0, v_acc
	.endr
	
	// -------------------------------------------------------------------------------
	// weight地址调整: 指向下一组8个的weight
	// -------------------------------------------------------------------------------
	.if (\en_addr_adj == enable)
		s_add_u32 				s[s_ptr_wei], s[s_ptr_wei], 0x0 + MLO_N_LCL_IN_MAPS_ONCE * 4 * 2
		s_addc_u32 				s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0									// s_ptr_wei ++ (DWORD寻址)
	.endif
	
	s_waitcnt 					lgkmcnt(0)	
	//m_weight_pre_fatch	
	m_conv_once 				\input, s_weib0, v_acc
.endm

// ===================================================================================
// ===================================================================================
.macro m_cacul_all_feature_save input wei_offset
	// -------------------------------------------------------------------------------
	// 地址指针: 指向16个输出feature的第一个
	// -------------------------------------------------------------------------------
	v_acc = v_acc0
	\wei_offset = \wei_offset + (MLO_N_LCL_IN_MAPS_ONCE - MLO_WEI_CHANNEL_STRIDE * MLO_N_LCL_OUT_MAPS) * 4
	//vout_offset = v_io_offset0
		
	m_load_weight 				s_weia0, \wei_offset, enable	
	s_waitcnt 					lgkmcnt(0)
	m_load_weight 				s_weib0, \wei_offset, enable	
	s_waitcnt					vmcnt(0)
	m_conv_once 				\input, s_weia0, v_acc	
	//m_save_one_output			v_acc
	//global_store_dword    	v[vout_offset:vout_offset + 1], v[v_acc-1], s[s_ptr_out:s_ptr_out+1]	offset:0x0
	
	.rept MLO_N_LCL_OUT_MAPS / 2 - 1
		s_waitcnt 				lgkmcnt(0)
		m_load_weight 			s_weia0, \wei_offset, enable
		m_conv_once 			\input, s_weib0, v_acc		
		//m_save_one_output		v_acc
		//global_store_dword    v[vout_offset:vout_offset + 1], v[v_acc-1], s[s_ptr_out:s_ptr_out+1]	offset:0x0 + MLO_OUT_CHANNEL_STRIDE * 4
		//vout_offset = vout_offset + 2
		
		s_waitcnt 				lgkmcnt(0)		
		m_load_weight 			s_weib0, \wei_offset, enable
		m_conv_once 			\input, s_weia0, v_acc		
		//m_save_one_output		v_acc
		//global_store_dword    v[vout_offset:vout_offset + 1], v[v_acc-1], s[s_ptr_out:s_ptr_out+1]	offset:0x0
	.endr
	
	s_waitcnt 					lgkmcnt(0)	
	m_conv_once 				\input, s_weib0, v_acc
	//m_save_one_output			v_acc
	//global_store_dword    	v[vout_offset:vout_offset + 1], v[v_acc-1], s[s_ptr_out:s_ptr_out+1]	offset:0x0 + MLO_OUT_CHANNEL_STRIDE * 4	
.endm

/************************************************************************************/
/* 存储:																			*/
/* 存储16个输出feature的output: 													*/
/* for (uint j = 0; j < MLO_N_LCL_OUT_MAPS; ++j) // 16 output feature				*/
/* {																				*/
/*     *q = weights[j] + dat[j/2];													*/
/*     q += MLO_OUT_CHANNEL_STRIDE;													*/
/* } 																				*/
/************************************************************************************/
.macro m_save_output
	v_sum = v_acc0
	.rept MLO_N_LCL_OUT_MAPS
		//v_mov_b32				v[v_sum], 0x0 + .AUTO_VGPR_COUNT							// ; for debug only
		//v_cvt_f32_u32			v[v_sum], v[v_sum]
			
		global_store_dword 		v[v_addr_out:v_addr_out+1], v[v_sum], off					// *v_addr_out = v_vdat[1..15]
		v_add_co_u32 			v[v_addr_out], vcc, 0x0 + MLO_OUT_CHANNEL_STRIDE * 4, v[v_addr_out]
		v_addc_co_u32 			v[v_addr_out+1], vcc, 0, v[v_addr_out+1], vcc				// v_addr_out += MLO_OUT_CHANNEL_STRIDE
		v_sum = v_sum + 1
	.endr
.endm

/************************************************************************************/
/* 向prefetch kernel发射信号														*/
/************************************************************************************/
.macro m_send_signal 			signal_type, index_reg
	.if (\signal_type == SIGNAL_REQ_FETCH)
		s_lshl_b32				s[s_tmp1], s[\index_reg], 0x02
		s_store_dword			s[s_signal], s[s_ptr_sig:s_ptr_sig+1], s[s_tmp1]
	.elseif (\signal_type == SIGNAL_EXIT)
		s_mov_b32				s[s_signal], 0x0 + SIGNAL_EXIT
		s_mov_b32				s[s_tmp1], CLOOP0
		s_lshl_b32				s[s_tmp1], s[s_tmp1], 0x02
		s_store_dword			s[s_signal], s[s_ptr_sig:s_ptr_sig+1], s[s_tmp1]
	.endif
.endm

/************************************************************************************/
/* 等待信号 																		*/
/************************************************************************************/
.macro m_wait_signal			index_reg
	s_mov_b32					s[s_signal], SIGNAL_NULL
FETCH_WAIT:	
	s_sleep						0x10
		
	s_lshl_b32					s[s_tmp1], s[\index_reg], 0x02
	s_load_dword				s[s_signal], s[s_ptr_sig:s_ptr_sig+1], s[s_tmp1]
	s_waitcnt 					lgkmcnt(0)
		
	s_cmp_eq_u32				s[s_signal], SIGNAL_REQ_FETCH								// if(signal == SIGNAL_NULL) wait
	s_cbranch_scc0				FETCH_WAIT
.endm

/************************************************************************************/
/* 调整fetch指针: 指向下一个循环													*/
/************************************************************************************/
.macro m_point_nx_round
	//s_sub_u32 				s[s_ptr_wei], s[s_ptr_wei], 0x0 + (MLO_WEI_STRIDE - K_FETCH_STEP) * 4
	//s_subb_u32 				s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0
		
	s_mov_b64					s[s_ptr_wei:s_ptr_wei+1], s[s_ptr_save:s_ptr_save+1]
	s_add_u32 					s[s_ptr_wei], s[s_ptr_wei], 0x0 + MLO_N_LCL_IN_MAPS_ONCE * 4 * 2
	s_addc_u32 					s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0								
.endm

/************************************************************************************/
/* 预读取 																			*/
/************************************************************************************/
.macro m_fetch_sub			wei_fetch
	imm_offset = 0
	//s_fetch = s_weia0
	
	.rept MLO_N_LCL_OUT_MAPS
		s_load_dword 			s[\wei_fetch], s[s_ptr_wei:s_ptr_wei+1], 0x0 + imm_offset
		imm_offset = imm_offset + MLO_WEI_CHANNEL_STRIDE * 4
		\wei_fetch = \wei_fetch + 1		
	.endr
	
	// ------------------------------------------------------------------------------
	// 调整指针: 到下一个 16 k_out
	// ------------------------------------------------------------------------------
	s_add_u32 					s[s_ptr_wei], s[s_ptr_wei], 0x0 + MLO_WEI_CHANNEL_STRIDE * K_FETCH_SUB * 4
	s_addc_u32 					s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0
.endm

/************************************************************************************/
/* 预读取n个输出feature block, 即n个k_out_maps										*/
/************************************************************************************/
.macro m_fetch_k_out_blk		k_out_block_num
	s_mov_b64					s[s_ptr_save:s_ptr_save+1], s[s_ptr_wei:s_ptr_wei+1]
	s_wei_fetch = s_weia0

	.rept  \k_out_block_num
		s_wei_fetch = s_weia0
		m_fetch_sub 			s_wei_fetch
		//s_waitcnt 			lgkmcnt(0)				
	.endr
	
	m_point_nx_round	
.endm
	
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
.macro m_fetch_all_in_chan
	imm_offset = 0
	tmp = s_weic0
	.rept MLO_N_LCL_OUT_MAPS
		s_load_dword 			s[tmp], s[s_ptr_wei:s_ptr_wei+1], 0x0 + imm_offset
		imm_offset = imm_offset + MLO_WEI_CHANNEL_STRIDE * 4
		tmp = tmp + 1
	.endr
.endm

/************************************************************************************/
/* 测试 																			*/
/************************************************************************************/
.macro m_debug_func
	// ------------------------------------------------------------------------------
	// 计算输出地址(线性测试用) 
	// __global float* q = out_ptr + FIXED_WORKGROUP_SIZE * gid_x + tid;
	// ------------------------------------------------------------------------------
	s_waitcnt lgkmcnt(0)
	
	v_lshlrev_b32 				v[v_tmp1], 0x0 + FIXED_WORKGROUP_SIZE_LOG2, s[gid_x0]
	v_add_lshl_u32 				v[v_tmp1], v[v_tmp1], v[tid], 2
	
	v_mov_b32					v[v_tmp2], s[s_ptr_out+1]
	v_add_co_u32 				v[v_addr_dbg], vcc, s[s_ptr_out], v[v_tmp1]
	v_addc_co_u32 				v[v_addr_dbg+1], vcc, 0, v[v_tmp2], vcc
	
	// ------------------------------------------------------------------------------
	// 输出测试数据
	// * q = ;
	// ------------------------------------------------------------------------------
	//v_mov_b32					v[v_tmp1],0x0 + MLO_ROUND_LEFT
	v_cvt_f32_u32				v[v_tmp1], v[v_acc12]
		
	global_store_dword 			v[v_addr_dbg:v_addr_dbg+1], v[v_tmp1], off					// v_addr_dbg = v_tmp1 (测试单个输出)	
.endm

/************************************************************************************/
/* 用于测试的等待与同步																*/
/************************************************************************************/
.macro m_debug_wait
	s_barrier
	.rept 100
		s_nop					0x0F
	.endr
	s_barrier
.endm

// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

// Disassembly:
	// ===============================================================================
	// 获取计算参数
	// ===============================================================================
	s_load_dwordx2 				s[s_ptr_in :s_ptr_in +1], s[kernarg:kernarg+1], 0x0 + in_ptr_off
	s_load_dwordx2 				s[s_ptr_wei:s_ptr_wei+1], s[kernarg:kernarg+1], 0x0 + wei_ptr_off
	s_load_dwordx2 				s[s_ptr_out:s_ptr_out+1], s[kernarg:kernarg+1], 0x0 + out_ptr_off
	s_load_dwordx2 				s[s_ptr_sig:s_ptr_sig+1], s[kernarg:kernarg+1], 0x0 + sig_ptr_off
		
	s_cmp_lt_u32				s[gid_x0], 0x0 + CU_NUM										// if(!(grp_id0 < CU_NUM)) goto noraml_index
	s_cbranch_scc0				CALCU_GROUP
	
	s_branch					END_PROG
// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
	// -------------------------------------------------------------------------------
	// int se_id = group_id0 % SE_NUM;														// SE编号
	// int cu_id = (group_id0 % CU_NUM) / SE_NUM;											// 一个SE内的CU编号
	// int cu_id2 = cu_id / 2 * 2;
	// int col_id = group_id0 / CU_NUM;
	// int raw_id = se_id * CU_PER_SE + cu_id;
	// -------------------------------------------------------------------------------
	s_and_b32 					s[s_tmp3], s[gid_x0], 0x0 + SE_NUM_MOD_MASK					// s_tmp3 = se_id
	s_and_b32 					s[s_tmp1], s[gid_x0], 0x0 + CU_NUM_MOD_MASK					// s_tmp1 = group_id0 % CU_NUM	
	s_lshr_b32 					s[s_tmp1], s[s_tmp1], 0x0 + SE_NUM_LOG2						// s_tmp1 = cu_id
	s_and_b32 					s[s_tmp1+1], s[s_tmp1], 0xFFFFFFFE							// s_tmp1+1 = cu_id2	
	s_lshr_b32					s[s_tmp2], s[gid_x0], CU_NUM_LOG2							// s_tmp2 = col_id
	s_lshl4_add_u32				s[s_tmp2+1], s[s_tmp3], s[s_tmp1]							// s_tmp2+1 = raw_id
	
	// -------------------------------------------------------------------------------
	// int sr_id = MAX_GRP_CU_NUM;
	// sr_id += GRP_NUM_SE * se_id;															// 前继SE上所有wave
	// sr_id += GRP_NUM_CU_MIN * cu_id2;													// 前继CU上所有wave
	// if (cu_id < MAX_GRP_CU_NUM)
	// 		sr_id -= (MAX_GRP_CU_NUM - cu_id2);
	// sr_id = col_id * 2 + cu_id % 2 + sr_id;
	// -------------------------------------------------------------------------------
	v_mov_b32					v[v_acc0], 0x0 + MAX_GRP_CU_NUM								// v_acc0 = MAX_GRP_CU_NUM
	v_mov_b32					v[v_acc1], 0x0 + GRP_NUM_SE
	v_mad_u32_u24				v[v_acc0], v[v_acc1], s[s_tmp3], v[v_acc0]					// v_acc0 += GRP_NUM_SE * se_id
	v_mov_b32					v[v_acc1], 0x0 + GRP_NUM_CU_MIN
	v_mad_u32_u24				v[v_acc0], v[v_acc1], s[s_tmp1+1], v[v_acc0]				// v_acc0 += GRP_NUM_CU_MIN * cu_id2
	v_cmpx_lt_u32				vcc, s[s_tmp1], 0x0 + MAX_GRP_CU_NUM						// if (cu_id < MAX_GRP_CU_NUM)
	v_mov_b32					v[v_acc1], 0x0 - MAX_GRP_CU_NUM								// 		v_acc1 = -MAX_GRP_CU_NUM
	v_add3_u32					v[v_acc0], v[v_acc1], s[s_tmp1+1], v[v_acc0]				// 		v_acc0 = v_acc0 + (-MAX_GRP_CU_NUM) + cu_id2
	s_mov_b64					exec, 0xFFFFFFFFFFFFFFFF
	v_lshlrev_b32				v[v_acc1], 1, s[s_tmp2]
	v_and_b32					v[v_acc2], s[s_tmp1], 1
	v_add3_u32					v[v_acc0], v[v_acc0], v[v_acc1], v[v_acc2]					// v_acc0 = sr_id
	
	// -------------------------------------------------------------------------------
	// pixBlkId = sr_id % in_pix_groups;
	// weiBlkId = sr_id / in_pix_groups;
	// -------------------------------------------------------------------------------
	v_mov_b32					v[v_acc1], 0x0 + PIX_GRP_NUM
	mv_div_u32					v[v_acc0], v[v_acc1], v[v_acc12], v[v_acc13]				// v_acc12 = weiBlkId
																							// v_acc13 = pixBlkId
	
	// -------------------------------------------------------------------------------
    // uint batch_id = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0) / MLO_IN_CHANNEL_STRIDE;
    // uint pos      = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0) % MLO_IN_CHANNEL_STRIDE;
    // uint out_id   = out_grp_block * MLO_N_LCL_OUT_MAPS;
	// -------------------------------------------------------------------------------
	v_lshlrev_b32				v[v_acc13], 0x0 + IN_PIXEL_PER_GROUP_LOG2, v[v_acc13]
	v_add_co_u32				v[v_acc13], vcc, v[v_acc13], v[tid]
	v_mov_b32					v[v_acc6], 0x0 + MLO_IN_CHANNEL_STRIDE
	mv_div_u32					v[v_acc13], v[v_acc6], v[v_acc7], v[v_acc8]					// v_acc7 = batch_id v_acc8 = pos
	v_lshlrev_b32				v[v_acc6], 0x0 + MLO_N_LCL_OUT_MAPS_LOG2, v[v_acc12]		// v_acc6 = out_id
	
	// -------------------------------------------------------------------------------
	// 计算 weight 地址
    // uint wei_off = out_id * MLO_WEI_CHANNEL_STRIDE;
	// -------------------------------------------------------------------------------
	v_mov_b32 					v[v_acc9], 0x0 + MLO_WEI_CHANNEL_STRIDE
	v_mul_u32_u24				v[v_acc9], v[v_acc6], v[v_acc9]
	v_readfirstlane_b32			s[s_tmp0], v[v_acc9]
	s_lshl_b32					s[s_tmp1], s[s_tmp0], 2
	s_waitcnt 					lgkmcnt(0)
	s_add_u32					s[s_ptr_wei], s[s_ptr_wei], s[s_tmp1]
	s_addc_u32					s[s_ptr_wei + 1], 0x0, s[s_ptr_wei + 1]	
	
	// -------------------------------------------------------------------------------
	// PING_PANG_CU
	// -------------------------------------------------------------------------------
	s_and_b32 					s[s_tmp1], s[gid_x0], 0x0 + CU_NUM_MOD_MASK					// s_tmp1 = group_id0 % CU_NUM	
	s_lshr_b32 					s[s_tmp1], s[s_tmp1], 0x0 + SE_NUM_LOG2						// s_tmp1 = cu_id
	s_and_b32					s[s_tmp1], s[s_tmp1], 0x01
	s_cmp_eq_u32				s[s_tmp1], 0x0	
	s_cbranch_scc1				FETCH_GROUP
	
	// ------------------------------------------------------------------------------
	// PANG_CU调整指针: 到下一个 weight_block 的 k_out
	// ------------------------------------------------------------------------------
	s_waitcnt 					lgkmcnt(0)
	s_add_u32 					s[s_ptr_wei], s[s_ptr_wei], 0x0 + MLO_WEI_CHANNEL_STRIDE * K_FETCH_SUB * 4
	s_addc_u32 					s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0

FETCH_GROUP:	
	// -------------------------------------------------------------------------------
	// 计算 signal 地址 
	// uint glb_sig_off = (grp_id0 % 64) * CLOOP0
	// -------------------------------------------------------------------------------
	s_waitcnt 					lgkmcnt(0)
	s_and_b32					s[s_tmp0], s[gid_x0], 0x0 + CU_NUM_MOD_MASK
	s_lshl_b32					s[s_tmp0], s[s_tmp0], 0x0 + SIG_NUM_PER_CU_LOG2 + 0x2		// (+2:dword)
	s_add_u32					s[s_ptr_sig], s[s_ptr_sig], s[s_tmp0]
	s_addc_u32					s[s_ptr_sig+1], s[s_ptr_sig+1], 0x0
	
	// -------------------------------------------------------------------------------
	// 初始化消息队列
	// -------------------------------------------------------------------------------
	s_mov_b32					s[s_tmp1], 0x0
	s_gpr_idx = 0
	.rept CLOOP0
		s_store_dword			s[s_tmp1], s[s_ptr_sig:s_ptr_sig+1], 0x0 + s_gpr_idx
		s_gpr_idx = s_gpr_idx  + 4
	.endr
	
	s_mov_b32 					s[s_loop_cnt], CLOOP0-1										// channel 的循环	
	
	m_fetch_k_out_blk			1
		
PRE_FETCH:
	m_wait_signal				s_loop_cnt
	m_fetch_k_out_blk			1
	
	// -------------------------------------------------------------------------------
	// 循环控制 :
	// -------------------------------------------------------------------------------
	s_sub_u32 					s[s_loop_cnt], s[s_loop_cnt], 0x1							// s_loop_cnt--
	s_cmpk_le_i32 				s[s_loop_cnt], 0x1
	s_cbranch_scc0 				PRE_FETCH
	
	// 最后一笔预取不再等待,转而去做指令预取
	m_fetch_k_out_blk			1
	
	s_branch					END_PROG
	
// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INSTR_FETCH_GROUP:
	s_mov_b64					exec, 0x0
	s_branch					LAST_CYCLE

// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

CALCU_GROUP:
	s_sub_u32					s[gid_x0], s[gid_x0], 0x0 + CU_NUM
	
	// -------------------------------------------------------------------------------
	// int se_id = group_id0 % SE_NUM;														// SE编号
	// int cu_id = (group_id0 % CU_NUM) / SE_NUM;											// 一个SE内的CU编号
	// int cu_id2 = cu_id / 2 * 2;
	// int col_id = group_id0 / CU_NUM;
	// int raw_id = se_id * CU_PER_SE + cu_id;
	// -------------------------------------------------------------------------------
	s_and_b32 					s[s_tmp3], s[gid_x0], 0x0 + SE_NUM_MOD_MASK					// s_tmp3 = se_id
	s_and_b32 					s[s_tmp1], s[gid_x0], 0x0 + CU_NUM_MOD_MASK					// s_tmp1 = group_id0 % CU_NUM	
	s_lshr_b32 					s[s_tmp1], s[s_tmp1], 0x0 + SE_NUM_LOG2						// s_tmp1 = cu_id
	s_and_b32 					s[s_tmp1+1], s[s_tmp1], 0xFFFFFFFE							// s_tmp1+1 = cu_id2	
	s_lshr_b32					s[s_tmp2], s[gid_x0], CU_NUM_LOG2							// s_tmp2 = col_id
	s_lshl4_add_u32				s[s_tmp2+1], s[s_tmp3], s[s_tmp1]							// s_tmp2+1 = raw_id
	
	// -------------------------------------------------------------------------------
	// int sr_id = MAX_GRP_CU_NUM;
	// sr_id += GRP_NUM_SE * se_id;															// 前继SE上所有wave
	// sr_id += GRP_NUM_CU_MIN * cu_id2;													// 前继CU上所有wave
	// if (cu_id < MAX_GRP_CU_NUM)
	// 		sr_id -= (MAX_GRP_CU_NUM - cu_id2);
	// sr_id = col_id * 2 + cu_id % 2 + sr_id;
	// -------------------------------------------------------------------------------
	v_mov_b32					v[v_acc0], 0x0 + MAX_GRP_CU_NUM								// v_acc0 = MAX_GRP_CU_NUM
	v_mov_b32					v[v_acc1], 0x0 + GRP_NUM_SE
	v_mad_u32_u24				v[v_acc0], v[v_acc1], s[s_tmp3], v[v_acc0]					// v_acc0 += GRP_NUM_SE * se_id
	v_mov_b32					v[v_acc1], 0x0 + GRP_NUM_CU_MIN
	v_mad_u32_u24				v[v_acc0], v[v_acc1], s[s_tmp1+1], v[v_acc0]				// v_acc0 += GRP_NUM_CU_MIN * cu_id2
	v_cmpx_lt_u32				vcc, s[s_tmp1], 0x0 + MAX_GRP_CU_NUM						// if (cu_id < MAX_GRP_CU_NUM)
	v_mov_b32					v[v_acc1], 0x0 - MAX_GRP_CU_NUM								// 		v_acc1 = -MAX_GRP_CU_NUM
	v_add3_u32					v[v_acc0], v[v_acc1], s[s_tmp1+1], v[v_acc0]				// 		v_acc0 = v_acc0 + (-MAX_GRP_CU_NUM) + cu_id2
	s_mov_b64					exec, 0xFFFFFFFFFFFFFFFF
	v_lshlrev_b32				v[v_acc1], 1, s[s_tmp2]
	v_and_b32					v[v_acc2], s[s_tmp1], 1
	v_add3_u32					v[v_acc0], v[v_acc0], v[v_acc1], v[v_acc2]					// v_acc0 = sr_id
	
	// -------------------------------------------------------------------------------
	// pixBlkId = sr_id % in_pix_groups;
	// weiBlkId = sr_id / in_pix_groups;
	// -------------------------------------------------------------------------------
	v_mov_b32					v[v_acc1], 0x0 + PIX_GRP_NUM
	mv_div_u32					v[v_acc0], v[v_acc1], v[v_acc12], v[v_acc13]				// v_acc12 = weiBlkId
																							// v_acc13 = pixBlkId
	
	// -------------------------------------------------------------------------------
    // uint batch_id = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0) / MLO_IN_CHANNEL_STRIDE;
    // uint pos      = (grp_id0_faked * FIXED_WORKGROUP_SIZE + local_id0) % MLO_IN_CHANNEL_STRIDE;
    // uint out_id   = out_grp_block * MLO_N_LCL_OUT_MAPS;
	// -------------------------------------------------------------------------------
	v_lshlrev_b32				v[v_acc13], 0x0 + IN_PIXEL_PER_GROUP_LOG2, v[v_acc13]
	v_add_co_u32				v[v_acc13], vcc, v[v_acc13], v[tid]
	v_mov_b32					v[v_acc6], 0x0 + MLO_IN_CHANNEL_STRIDE
	mv_div_u32					v[v_acc13], v[v_acc6], v[v_acc7], v[v_acc8]					// v_acc7 = batch_id v_acc8 = pos
	v_lshlrev_b32				v[v_acc6], 0x0 + MLO_N_LCL_OUT_MAPS_LOG2, v[v_acc12]		// v_acc6 = out_id
	
	// -------------------------------------------------------------------------------
	// 计算 input 地址
    // uint gbl_in_off  = batch_id * MLO_IN_BATCH_STRIDE + pos;
	// -------------------------------------------------------------------------------
	v_mov_b32 					v[v_acc9], 0x0 + MLO_IN_BATCH_STRIDE
	v_mul_u32_u24				v[v_acc10], v[v_acc7], v[v_acc9]
	v_add_co_u32				v[v_acc13], vcc, v[v_acc10], v[v_acc8]						// v_acc13 = gbl_in_off
	
	// offset_list		
	v_lshlrev_b32 				v[v_io_offset0], 2, v[v_acc13]								// v_io_offset0 = gbl_in_off(DWORD)
	v_mov_b32					v[v_acc1], 0x0 + MLO_IN_CHANNEL_STRIDE * 2 * 4
	v_add_co_u32				v[v_io_offset1], vcc, v[v_io_offset0], v[v_acc1]
	v_add_co_u32				v[v_io_offset2], vcc, v[v_io_offset1], v[v_acc1]
	v_add_co_u32				v[v_io_offset3], vcc, v[v_io_offset2], v[v_acc1]
	
	// -------------------------------------------------------------------------------
	// 计算 weight 地址
    // uint wei_off = out_id * MLO_WEI_CHANNEL_STRIDE;
	// -------------------------------------------------------------------------------
	v_mov_b32 					v[v_acc9], 0x0 + MLO_WEI_CHANNEL_STRIDE
	v_mul_u32_u24				v[v_acc9], v[v_acc6], v[v_acc9]
	v_readfirstlane_b32			s[s_tmp0], v[v_acc9]
	s_lshl_b32					s[s_tmp1], s[s_tmp0], 2
	s_waitcnt 					lgkmcnt(0)
	s_add_u32					s[s_ptr_wei], s[s_ptr_wei], s[s_tmp1]
	s_addc_u32					s[s_ptr_wei + 1], 0x0, s[s_ptr_wei + 1]
	
	
	// -------------------------------------------------------------------------------
	// 计算 output 地址 
	// uint gbl_out_off = batch_id * MLO_OUT_BATCH_STRIDE + out_id * MLO_OUT_CHANNEL_STRIDE + pos;
	// -------------------------------------------------------------------------------
	v_mov_b32 					v[v_acc9], 0x0 + MLO_OUT_BATCH_STRIDE
	v_mul_u32_u24				v[v_acc9], v[v_acc7], v[v_acc9]								// v_acc9 = batch_id * MLO_OUT_BATCH_STRIDE
	v_mov_b32 					v[v_acc10], 0x0 + MLO_OUT_CHANNEL_STRIDE					// v_acc8 = pos
	v_mul_u32_u24				v[v_acc11], v[v_acc6], v[v_acc10]							// v_acc11 = out_id * MLO_OUT_CHANNEL_STRIDE
	v_add3_u32					v[v_acc12], v[v_acc9], v[v_acc11], v[v_acc8]				// v_acc12 = gbl_out_off
	v_lshlrev_b32 				v[v_acc12], 2, v[v_acc12]
	
	v_mov_b32					v[v_addr_out + 1], s[s_ptr_out + 1]
	v_add_co_u32				v[v_addr_out], vcc, s[s_ptr_out], v[v_acc12]
	v_addc_co_u32				v[v_addr_out + 1], vcc, 0, v[v_addr_out + 1], vcc
	
	// -------------------------------------------------------------------------------
	// 计算 signal 地址 
	// uint glb_sig_off = (grp_id0 % 64) * CLOOP0
	// -------------------------------------------------------------------------------
	s_and_b32					s[s_tmp0], s[gid_x0], 0x0 + CU_NUM_MOD_MASK
	s_lshl_b32					s[s_tmp0], s[s_tmp0], 0x0 + SIG_NUM_PER_CU_LOG2 + 0x2		// (+2:dword)
	s_add_u32					s[s_ptr_sig], s[s_ptr_sig], s[s_tmp0]
	s_addc_u32					s[s_ptr_sig+1], s[s_ptr_sig+1], 0x0
	
	s_mov_b32					s[s_signal], SIGNAL_REQ_FETCH
	
	
MAIN_CONV:
	// ===============================================================================
	// 大循环: 计算一个像素全部输入通道和8个输出特征值. 一轮循环计算8个输入通道的累加和
	// 一个像素16个输出特征,全部输入通道的计算:
	// for (uint loopCnt = 0; loopCnt < MLO_CLOOP0; loopCnt++)
	// {
	//     ... ...
	// }
	// ===============================================================================	
	// -------------------------------------------------------------------------------
	// 初始化:
	// for (uint o = 0; o < MLO_N_LCL_OUT_MAPS; ++o)
	// {
	//     accum[o] = (_FLOAT)0;
	// }
	// -------------------------------------------------------------------------------
	v_acc = v_acc0
	.rept MLO_N_LCL_OUT_MAPS
		v_mov_b32 				v[v_acc], 0												// v_tmp1 = accum = 0
		v_acc = v_acc + 1
	.endr
		
	// -------------------------------------------------------------------------------
	// 循环填充 :
	// 读取8输入通道的input data
	// -------------------------------------------------------------------------------
	//m_weight_pre_fatch
	s_mov_b32 					s[s_loop_cnt], CLOOP0 - 1								// s_loop_cnt = CLOOP0 - 1
	m_load_input 				v_data0, enable
	weight_offset = 0
	
	// -------------------------------------------------------------------------------
	// 循环体 :
	// -------------------------------------------------------------------------------
LOOP_CONV:	
	//m_debug_wait				// ;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	m_load_input 				v_datb0, enable
	m_cacul_all_feature		 	v_data0, weight_offset, 8, disable, disable	
	m_send_signal				SIGNAL_REQ_FETCH, s_loop_cnt	
	m_load_input 				v_data0, enable
	m_cacul_all_feature		 	v_datb0, weight_offset, 8, enable, enable
	
	// -------------------------------------------------------------------------------
	// 循环控制 :
	// -------------------------------------------------------------------------------
END_LOOP_CONV:
	s_sub_u32 					s[s_loop_cnt], s[s_loop_cnt], 0x01						// s_loop_cnt--
	s_and_b32					s[s_loop_cnt], s[s_loop_cnt], exec_lo
	s_cmpk_eq_i32 				s[s_loop_cnt], 0x0										// for(s_loop_cnt == CLOOP0)
	s_cbranch_scc0 				LOOP_CONV

	// -------------------------------------------------------------------------------
	// 循环排空 :
	// -------------------------------------------------------------------------------
	//m_debug_wait				// ;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
LAST_CYCLE:
	m_load_input 				v_datb0, enable	
	m_cacul_all_feature		 	v_data0, weight_offset, 8, disable, disable
	m_cacul_all_feature		 	v_datb0, weight_offset, 0, enable,  disable
	
	// -------------------------------------------------------------------------------
	// 存储结果
	// -------------------------------------------------------------------------------
	m_save_output
	
END_PROG:
	s_endpgm
	
/************************************************************************************/
/* metadate																			*/
/************************************************************************************/
.amd_amdgpu_hsa_metadata
{ Version: [ 1, 0 ],
  Kernels: 
    - { Name: ConvFwd1x1, SymbolName: 'ConvFwd1x1', Language: OpenCL C, LanguageVersion: [ 1, 2 ],
        Attrs: { ReqdWorkGroupSize: [ 64, 1, 1 ] }
        CodeProps: { KernargSegmentSize: 32, GroupSegmentFixedSize: 0, PrivateSegmentFixedSize: 0, KernargSegmentAlign: 8, WavefrontSize: 64, MaxFlatWorkGroupSize: 64 }
        Args:
        - { Name: d_in  , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: d_wei , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: d_out , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global  }
        - { Name: d_sig , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: U32, TypeName: 'float*', AddrSpaceQual: Global  }
      }
}
.end_amd_amdgpu_hsa_metadata
