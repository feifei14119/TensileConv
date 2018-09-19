.hsa_code_object_version 2,1
.hsa_code_object_isa 9,0,0,"AMD","AMDGPU"

.text
.globl ConvFwd1x1
.p2align 8
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
//.set W,							28
//.set H,							28
//.set C,							192
//.set K,							64

.set MLO_IN_CHANNEL_STRIDE,		(W * H)
.set MLO_IN_BATCH_STRIDE,		(H * W * C)
.set MLO_WEI_CHANNEL_STRIDE,	(1 * 1 * C)
.set MLO_OUT_CHANNEL_STRIDE,	(W * H)
.set MLO_OUT_BATCH_STRIDE,		(H * W * K)



.set MLO_N_IN_GROUPS,			1				// 所有输入通道被分到几个CU
.set MLO_N_IN_GROUPS_LOG2,		0				// 乘除法时的移位
.set MLO_N_IN_GROUPS_DIV_MASK,	0x0				// 求余时的mask

.set MLO_N_LCL_IN_MAPS,			C				// 每个CU负责计算的输入通道个数
.set MLO_N_LCL_IN_MAPS_ONCE,	8				// 每次循环（不展开）负责计算的输入通道个数
.set MLO_N_LCL_OUT_MAPS,		16				// 每个CU负责计算的输出特征数
.set MLO_N_LCL_OUT_MAPS_LOG2,	4
.set MLO_IN_PIX_GROUP,			64				// 每个workgroup处理多少个input pixal
.set MLO_IN_PIX_GROUP_LOG2,		6
.set MLO_IN_PIX_GROUP_DIV_MASK,	0x3F

//;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
.set MLO_N_OUT_GROUPS,			(K / MLO_N_LCL_OUT_MAPS)		// K / MLO_N_LCL_OUT_MAPS 一个输出像素的所有特征，分到几个CU上计算
.set MLO_N_OUT_GROUPS_LOG2,		3				// K / MLO_N_LCL_OUT_MAPS 乘除法时的移位
.set MLO_N_OUT_GROUPS_DIV_MASK, 0x7 			// 求余时的mask
//;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

.set SE_NUM,					4				// shader engin 个数
.set SE_NUM_LOG2,				2
.set FIXED_WORKGROUP_SIZE,		64				// 每个CU的线程数
.set FIXED_WORKGROUP_SIZE_LOG2,	6
.set GROUPS_PER_OUT_BATCH,		(W * H / FIXED_WORKGROUP_SIZE * MLO_N_OUT_GROUPS)	// 48
.set CLOOP0,					(MLO_N_LCL_IN_MAPS / MLO_N_LCL_IN_MAPS_ONCE / 2)

// ==================================================================================
// 输入参数排布
// ==================================================================================
in_ptr_off  = 0x00
wei_ptr_off = 0x08
out_ptr_off = 0x10

// ==================================================================================
// SGPR 初始排布
// ==================================================================================
privateSeg = 0
kernarg    = 4
gid_x0 	   = 6
gid_y0 	   = 7
gid_z0 	   = 8

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
		
	.SGPR_ALLOC s_loop_cnt
	.SGPR_ALLOC s_fetchmask
    .SGPR_ALLOC s_ptr_wei_save, 2
    .SGPR_ALLOC s_wei_desc,  4    	// input buffer descriptor
	.SGPR_ALLOC s_tmp1, 2
	.SGPR_ALLOC s_tmp2, 2
	.SGPR_ALLOC s_tmp3
	.SGPR_ALLOC s_tmp4
	.SGPR_ALLOC s_feature_loop_cnt
	.SGPR_ALLOC s_onec_loop_cnt

	// ------------------------------------------------------------------------------
    .VGPR_ALLOC_FROM 0
    .VGPR_ALLOC tid
    .VGPR_ALLOC v_addr_in, 2
    .VGPR_ALLOC v_addr_out, 2
    .VGPR_ALLOC v_addr_dbg, 2
	.VGPR_ALLOC v_tmp1
	.VGPR_ALLOC v_tmp2
	.VGPR_ALLOC v_tmp3
	.VGPR_ALLOC v_tmp4
	.VGPR_ALLOC v_tmp5
	.VGPR_ALLOC v_tmp6
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
		
	.VGPR_ALLOC v_test	
	
	// ------------------------------------------------------------------------------
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
		float_mode 							= 240	// ?
		
		user_sgpr_count 					= 6		// ?
		wavefront_sgpr_count 				= .AUTO_SGPR_COUNT
		workitem_vgpr_count 				= .AUTO_VGPR_COUNT
		granulated_workitem_vgpr_count 		= .AUTO_VGPR_GRANULATED_COUNT
		granulated_wavefront_sgpr_count 	= .AUTO_SGPR_GRANULATED_COUNT
		kernarg_segment_byte_size 			= 56	// kernarg segment size(byte)
		workgroup_group_segment_byte_size 	= 0		//[caculated] group segment memory required by a work-group in bytes		
	.end_amd_kernel_code_t

	
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
.macro m_weight_pre_fatch
	imm_offset = 0
	tmp = s_weic0
	.rept MLO_N_LCL_OUT_MAPS
		s_load_dword 		s[tmp], s[s_ptr_wei:s_ptr_wei+1], 0x0 + imm_offset
		imm_offset = imm_offset + MLO_WEI_CHANNEL_STRIDE * 4
		tmp = tmp + 1
	.endr
.endm
	
/************************************************************************************/
/* 读取8输入通道的input data														*/
/* for (uint j = 0; j < MLO_N_LCL_IN_MAPS_ONCE; ++j)	// read 8 input channel     */
/* {                                                                                */
/*     dat[j] = *p;                                                                 */
/*     p += MLO_IN_CHANNEL_STRIDE;                                                  */
/* }                                                                                */
/************************************************************************************/
.macro m_load_input 		dest_base
	v_dat = \dest_base
	.rept (MLO_N_LCL_IN_MAPS_ONCE)
		global_load_dword 	v[v_dat], v[v_addr_in:v_addr_in+1], off						// v_dat[1..7] = *v_addr_in
		v_add_co_u32 		v[v_addr_in], vcc, 0x0 + MLO_IN_CHANNEL_STRIDE * 4, v[v_addr_in]
		v_addc_co_u32 		v[v_addr_in+1], vcc, 0, v[v_addr_in+1], vcc					// v_addr_in += MLO_IN_CHANNEL_STRIDE
		v_dat = v_dat + 1
	.endr
.endm

/************************************************************************************/
/* 读取1个输出feature的当前通道的8个weight:	                                        */
/* for (uint o = 0; o < MLO_N_LCL_IN_MAPS_ONCE; ++o)	// 8 input channel          */
/* {                                                                                */
/*     weights[o] = *w;                                                             */
/*     w ++;                                                                        */
/* }                                                                                */
/************************************************************************************/
.macro m_load_weight1 wei_base
	s_load_dwordx8 		s[\wei_base:\wei_base+7], s[s_ptr_wei:s_ptr_wei+1], 0x0			// s_weia[0..15] = ptr_wei (s_weia0需要16DWORD对齐)	
	s_add_u32 			s[s_ptr_wei], s[s_ptr_wei], MLO_WEI_CHANNEL_STRIDE * 4			// weight地址调整: w += MLO_WEI_CHANNEL_STRIDE
	s_addc_u32 			s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0								// s_ptr_wei += MLO_WEI_CHANNEL_STRIDE (DWORD寻址)
.endm

.macro m_load_weight2 	wei_base, imm_offset
	s_load_dwordx8 		s[\wei_base:\wei_base+7], s[s_ptr_wei:s_ptr_wei+1], 0x0 + \imm_offset	// s_weia[0..15] = ptr_wei (s_weia0需要16DWORD对齐)	
	\imm_offset = \imm_offset + MLO_WEI_CHANNEL_STRIDE * 4
.endm

/************************************************************************************/
/* 存储16个输出feature的output														*/
/* for (uint j = 0; j < MLO_N_LCL_IN_MAPS_ONCE; ++j)	// read 8 input channel     */
/* {                                                                                */
/*     *q = acc[j];                                                                 */
/*     *q += MLO_OUT_CHANNEL_STRIDE;                                                */
/* }                                                                                */
/************************************************************************************/
.macro m_save_output
	v_sum = v_acc0
	.rept MLO_N_LCL_OUT_MAPS		
		global_store_dword    	v[v_addr_out:v_addr_out + 1], v[v_sum], off
		v_add_co_u32          	v[v_addr_out], vcc, 0x0 + MLO_OUT_CHANNEL_STRIDE * 4, v[v_addr_out]
		v_addc_co_u32         	v[v_addr_out + 1], vcc, 0, v[v_addr_out + 1], vcc
		v_sum = v_sum + 1
	.endr
.endm

/************************************************************************************/
/* 小循环: 计算1个输出特征值的8次乘加, 每次计算1次乘加								*/
/* for (uint c = 0; c < MLO_N_LCL_IN_MAPS_ONCE; ++c)                                */
/* {                                                                                */
/*     accum[o] += dat[c] * weights[c];                                             */
/* }                                                                                */
/************************************************************************************/
.macro m_conv_once input, weight, output
	v_dat = \input
	s_wei = \weight
	.rept MLO_N_LCL_IN_MAPS_ONCE
		//v_mov_b32 			v[\output], s[s_tmp1]									// ;for debug
		//v_cvt_f32_u32			v[\output], v[\output]									// ;for debug
		//v_add_f32 			v[\output], s[s_wei], v[\output]						// ;for debug
		v_fma_f32 				v[\output], v[v_dat], s[s_wei], v[\output]				// v_tmp1 = accum += v_dat[0..7] * s_wei[0..7]
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
.macro m_cacul_all_feature_ping input wei_offset
	v_acc = v_acc0
	\wei_offset = 0	
	
	m_load_weight2 		s_weia0, \wei_offset	
	s_waitcnt 			lgkmcnt(0)
	m_load_weight2 		s_weib0, \wei_offset	
	s_waitcnt			vmcnt(8)
	m_conv_once 		\input, s_weia0, v_acc
	
	.rept MLO_N_LCL_OUT_MAPS / 2 - 1
		s_waitcnt 		lgkmcnt(0)
		m_load_weight2 	s_weia0, \wei_offset
		m_conv_once 	\input, s_weib0, v_acc
		
		s_waitcnt 		lgkmcnt(0)		
		m_load_weight2 	s_weib0, \wei_offset
		m_conv_once 	\input, s_weia0, v_acc
	.endr
	
	s_waitcnt 			lgkmcnt(0)	
	m_conv_once 		\input, s_weib0, v_acc
.endm

// ---------------------------------------------------------------------------------
.macro m_cacul_all_feature_pang input wei_offset
	v_acc = v_acc0
	\wei_offset = \wei_offset + (MLO_N_LCL_IN_MAPS_ONCE - MLO_WEI_CHANNEL_STRIDE * MLO_N_LCL_OUT_MAPS) * 4
	
	m_load_weight2 		s_weia0, \wei_offset	
	s_waitcnt 			lgkmcnt(0)
	m_load_weight2 		s_weib0, \wei_offset	
	s_waitcnt			vmcnt(8)
	m_conv_once 		\input, s_weia0, v_acc
	
	.rept MLO_N_LCL_OUT_MAPS / 2 - 1
		s_waitcnt 		lgkmcnt(0)
		m_load_weight2 	s_weia0, \wei_offset
		m_conv_once 	\input, s_weib0, v_acc
		
		s_waitcnt 		lgkmcnt(0)		
		m_load_weight2 	s_weib0, \wei_offset
		m_conv_once 	\input, s_weia0, v_acc
	.endr
	
	s_add_u32 			s[s_ptr_wei], s[s_ptr_wei], 0x0 + MLO_N_LCL_IN_MAPS_ONCE * 4 * 2
	s_addc_u32 			s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0									// s_ptr_wei ++ (DWORD寻址)	
	
	s_waitcnt 			lgkmcnt(0)	
	m_weight_pre_fatch
	m_conv_once 		\input, s_weib0, v_acc
.endm
// ---------------------------------------------------------------------------------
.macro m_cacul_all_feature_last input wei_offset
	v_acc = v_acc0
	\wei_offset = \wei_offset + (MLO_N_LCL_IN_MAPS_ONCE - MLO_WEI_CHANNEL_STRIDE * MLO_N_LCL_OUT_MAPS) * 4
		
	m_load_weight2 		s_weia0, \wei_offset	
	s_waitcnt 			lgkmcnt(0)
	m_load_weight2 		s_weib0, \wei_offset	
	s_waitcnt			vmcnt(0)
	m_conv_once 		\input, s_weia0, v_acc
	
	.rept MLO_N_LCL_OUT_MAPS / 2 - 1
		s_waitcnt 			lgkmcnt(0)
		m_load_weight2 		s_weia0, \wei_offset
		m_conv_once 		\input, s_weib0, v_acc
		
		s_waitcnt 			lgkmcnt(0)		
		m_load_weight2 		s_weib0, \wei_offset
		m_conv_once 		\input, s_weia0, v_acc
	.endr
	
	s_waitcnt 				lgkmcnt(0)	
	m_conv_once 			\input, s_weib0, v_acc
.endm

	// ===============================================================================
	// 获取计算参数
	//// ===============================================================================
	s_load_dwordx2 			s[s_ptr_in :s_ptr_in +1], s[kernarg:kernarg+1], 0x0 + in_ptr_off	// desc_in = in_ptr
	s_load_dwordx2 			s[s_ptr_wei:s_ptr_wei+1], s[kernarg:kernarg+1], 0x0 + wei_ptr_off	// desc_wei = wei_ptr
	s_load_dwordx2 			s[s_ptr_out:s_ptr_out+1], s[kernarg:kernarg+1], 0x0 + out_ptr_off	// desc_out = out_ptr
	s_waitcnt 				lgkmcnt(0)															//; 放在真正用到s地址的地方
    
	// -------------------------------------------------------------------------------
	// workload参数 
	// -------------------------------------------------------------------------------
	// tid = v[tid]
	// gid_x = s[gid_x0]
	// v_lshlrev_b32 	v[v_tmp1], 2, v[tid]											// v_tmp1 = tid * 4 (DWORD寻址) 
    
	// ===============================================================================
	// 下标与地址计算 
	// ===============================================================================
	//v_lshrrev_b32		v[v_acc0], 0x0 + MLO_IN_PIX_GROUP_LOG2, v[tid]					// v_acc0 = tid_sub_grp = tid / MLO_IN_PIX_GROUP
	//v_and_b32			v[v_acc1], 0x0 + MLO_IN_PIX_GROUP_DIV_MASK, v[tid]				// v_acc1 = tid_left = tid % MLO_IN_PIX_GROUP
    
	// -------------------------------------------------------------------------------	
	// uint out_grp_block = gid % MLO_N_OUT_GROUPS;										// 第几个weight的组
    // uint in_grp_block  = (uint)(gid / MLO_N_OUT_GROUPS) % MLO_N_IN_GROUPS;		
    // uint grp_id0_faked = (uint)(gid / MLO_N_OUT_GROUPS) / MLO_N_IN_GROUPS;			// 第几个input的组
	// -------------------------------------------------------------------------------
	v_and_b32 			v[v_acc1], 0x0 + MLO_N_OUT_GROUPS_DIV_MASK, s[gid_x0] 			// v_acc1 = out_grp_block = gid % MLO_N_OUT_GROUPS	
	v_lshrrev_b32 		v[v_acc2], 0x0 + MLO_N_OUT_GROUPS_LOG2, s[gid_x0] 				// v_acc2 = gid / MLO_N_OUT_GROUPS
	v_lshrrev_b32 		v[v_acc3], 0x0 + MLO_N_IN_GROUPS_LOG2, v[v_acc2] 				// v_acc3 = grp_id0_faked = (uint)(gid / MLO_N_OUT_GROUPS) / MLO_N_IN_GROUPS
	v_and_b32 			v[v_acc4], 0x0 + MLO_N_IN_GROUPS_DIV_MASK, v[v_acc2] 			// v_acc4 = in_grp_block = (uint)(gid / MLO_N_OUT_GROUPS) % MLO_N_IN_GROUPS
    
	// -------------------------------------------------------------------------------	
	// uint pos_id = (grp_id0_faked * FIXED_WORKGROUP_SIZE + tid) % MLO_IN_CHANNEL_STRIDE;
	// uint out_id = out_grp_block * MLO_N_LCL_OUT_MAPS;
	// uint batch_id = (grp_id0_faked * FIXED_WORKGROUP_SIZE + tid) / MLO_IN_CHANNEL_STRIDE;
	// -------------------------------------------------------------------------------
	v_lshl_add_u32		v[v_acc5], v[v_acc3], 0x0 + MLO_IN_PIX_GROUP_LOG2, v[tid]		// v_acc5 = grp_id0_faked * MLO_IN_PIX_GROUP + tid
	v_mov_b32			v[v_acc6], 0x0 + MLO_IN_CHANNEL_STRIDE							// v_acc8 = pos_id = (grp_id0_faked * MLO_IN_PIX_GROUP + tid_left) % MLO_IN_CHANNEL_STRIDE
	mv_div_u32			v[v_acc5], v[v_acc6], v[v_acc7], v[v_acc8]						// v_acc7 = batch_id = (grp_id0_faked * MLO_IN_PIX_GROUP + tid_left) / MLO_IN_CHANNEL_STRIDE	
	v_lshlrev_b32		v[v_acc6], 0x0 + MLO_N_LCL_OUT_MAPS_LOG2, v[v_acc1]				// v_acc6 = out_id = out_grp_block * MLO_N_LCL_OUT_MAPS
	
	// -------------------------------------------------------------------------------
	// 计算 input 地址
	// uint gbl_in_off = batch_id * MLO_IN_BATCH_STRIDE + in_grp_block * MLO_N_LCL_IN_MAPS * MLO_IN_CHANNEL_STRIDE + pos_id;
	// -------------------------------------------------------------------------------
	v_mov_b32 			v[v_acc9], 0x0 + MLO_IN_BATCH_STRIDE							// v_acc9 = MLO_IN_BATCH_STRIDE
	v_mul_u32_u24		v[v_acc10], v[v_acc7], v[v_acc9]								// v_acc10 = batch_id * MLO_IN_BATCH_STRIDE
	v_mov_b32 			v[v_acc11], 0x0 + MLO_N_LCL_IN_MAPS * MLO_IN_CHANNEL_STRIDE		// v_acc11 = MLO_N_LCL_IN_MAPS * MLO_IN_CHANNEL_STRIDE
	v_mul_u32_u24		v[v_acc12], v[v_acc4], s[v_acc11]								// v_acc12 = in_grp_block * MLO_N_LCL_IN_MAPS * MLO_IN_CHANNEL_STRIDE
	v_add3_u32			v[v_acc13], v[v_acc10], v[v_acc12], v[v_acc8]					// v_acc13 = gbl_in_off	
	v_lshlrev_b32 		v[v_acc13], 2, v[v_acc13]										// v_acc13 = gbl_in_off * 4 (DWORD寻址)
	v_mov_b32			v[v_addr_in], s[s_ptr_in]										// ...
	v_mov_b32			v[v_addr_in+1], s[s_ptr_in+1]									// v_addr_in = s_ptr_in
	v_add_co_u32		v[v_addr_in], vcc, v[v_acc13], v[v_addr_in]						// ...
	v_addc_co_u32		v[v_addr_in+1], vcc, 0, v[v_addr_in+1], vcc	
	
	// -------------------------------------------------------------------------------
	// 计算 weight 地址
	// uint wei_off = out_id * MLO_WEI_CHANNEL_STRIDE + in_grp_block * MLO_N_LCL_IN_MAPS;
	// -------------------------------------------------------------------------------
	v_mov_b32 			v[v_acc9], 0x0 + MLO_WEI_CHANNEL_STRIDE							// v_acc9 = MLO_WEI_CHANNEL_STRIDE
	v_mul_u32_u24		v[v_acc9], v[v_acc6], v[v_acc9]									// v_acc9 = out_id * MLO_WEI_CHANNEL_STRIDE
	v_mov_b32 			v[v_acc10], 0x0 + MLO_N_LCL_IN_MAPS								// v_acc10 = MLO_WEI_CHANNEL_STRIDE
	v_mul_u32_u24		v[v_acc10], v[v_acc4], v[v_acc10]								// v_acc10 = in_grp_block * MLO_N_LCL_IN_MAPS
	v_add_u32			v[v_acc9], v[v_acc9], v[v_acc10]								// v_acc9 = wei_off = out_id * MLO_WEI_CHANNEL_STRIDE + in_grp_block * MLO_N_LCL_IN_MAPS
	v_readfirstlane_b32	s[s_tmp0], v[v_acc9]											// s_tmp0 = wei_off	
	s_lshl_b32			s[s_tmp1], s[s_tmp0], 2											// s_tmp1 = wei_off * 4 (DWORD寻址)
	s_waitcnt 			lgkmcnt(0)														//; !!!
	s_add_u32			s[s_ptr_wei], s[s_ptr_wei], s[s_tmp1]							// ...
	s_addc_u32			s[s_ptr_wei+1], 0x0, s[s_ptr_wei+1]								// s_ptr_wei = wei_ptr + wei_off
	
	// -------------------------------------------------------------------------------
	// 计算 output 地址 
	// uint gbl_out_off = batch_id * MLO_OUT_BATCH_STRIDE + out_id * MLO_OUT_CHANNEL_STRIDE + pos_id;
	// -------------------------------------------------------------------------------
	v_mov_b32 			v[v_acc9], 0x0 + MLO_OUT_BATCH_STRIDE							// v_acc9 = MLO_OUT_BATCH_STRIDE
	v_mul_u32_u24		v[v_acc9], v[v_acc7], v[v_acc9]									// v_acc9 = batch_id * MLO_OUT_BATCH_STRIDE	
	v_mov_b32 			v[v_acc10], 0x0 + MLO_OUT_CHANNEL_STRIDE						// v_acc10 = MLO_OUT_CHANNEL_STRIDE
	v_mul_u32_u24		v[v_acc11], v[v_acc6], v[v_acc10]								// v_acc11 = out_id * MLO_OUT_CHANNEL_STRIDE	
	v_add3_u32			v[v_acc12], v[v_acc9], v[v_acc11], v[v_acc8]					// v_acc12 = gbl_out_off	
	v_lshlrev_b32 				v[v_acc12], 2, v[v_acc12]
	v_mov_b32					v[v_addr_out + 1], s[s_ptr_out + 1]
	v_add_co_u32				v[v_addr_out], vcc, s[s_ptr_out], v[v_acc12]
	v_addc_co_u32				v[v_addr_out + 1], vcc, 0, v[v_addr_out + 1], vcc

/*
	// ------------------------------------------------------------------------------
	// 计算输出地址(线性测试用) 
	// __global float* q = out_ptr + FIXED_WORKGROUP_SIZE * gid_x + tid;
	// ------------------------------------------------------------------------------
	s_lshl_b32 			s[s_tmp1], s[gid_x0], .WAVE_SIZE_LOG2							// s_tmp1 = grp_idx * FIXED_WORKGROUP_SIZE 
	s_mov_b32 			s[s_tmp1+1], 0													// ...
	s_lshl_b64 			s[s_tmp1:s_tmp1+1], s[s_tmp1:s_tmp1+1], 2						// s_tmp1:s_tmp1+1 = grp_idx * FIXED_WORKGROUP_SIZE (DWORD寻址)
	s_add_u32 			s[s_ptr_out], s[s_ptr_out], s[s_tmp1]							// ...
	s_addc_u32 			s[s_ptr_out+1], s[s_ptr_out+1], s[s_tmp1+1]						// s_ptr_outp = s_ptr_out + grp_idx * FIXED_WORKGROUP_SIZE
	v_lshlrev_b32 		v[v_tmp1], 2, v[tid]											// v_tmp1 = tid * 4 (DWORD寻址) 
	v_mov_b32 			v[v_tmp2], s[s_ptr_out+1]										// v_tmp2 = s_ptr_outp[31:16]
	v_add_co_u32 		v[v_addr_dbg], vcc, s[s_ptr_out], v[v_tmp1]						// ...
	v_addc_co_u32 		v[v_addr_dbg+1], vcc, 0, v[v_tmp2], vcc							// v_addr_dbg = out_ptr + FIXED_WORKGROUP_SIZE * gid_x + tid
*/


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
		v_mov_b32 		v[v_acc], 0														// v_tmp1 = accum = 0
		v_acc = v_acc + 1
	.endr
	
// =================================================================================================================
	s_mov_b32 					s[s_loop_cnt], CLOOP0 - 1								// s_loop_cnt = CLOOP0 - 1
	
	// -------------------------------------------------------------------------------
	// 循环填充 :
	// 读取8输入通道的input data
	// -------------------------------------------------------------------------------
	m_weight_pre_fatch
	m_load_input 				v_data0
	weight_offset = 0
	
LOOP_CONV:
	m_load_input 				v_datb0
	m_cacul_all_feature_ping 	v_data0, weight_offset
	
	m_load_input 				v_data0
	m_cacul_all_feature_pang 	v_datb0, weight_offset
		
	// -------------------------------------------------------------------------------
	// 循环控制 :
	// -------------------------------------------------------------------------------
	s_sub_u32 					s[s_loop_cnt], s[s_loop_cnt], 0x01						// s_loop_cnt--
	s_cmpk_eq_i32 				s[s_loop_cnt], 0x0										// for(s_loop_cnt == CLOOP0)
	s_cbranch_scc0 				LOOP_CONV

	// -------------------------------------------------------------------------------
	// 循环排空 :
	// -------------------------------------------------------------------------------
LAST_CYCLE:
	m_load_input 				v_datb0	
	m_cacul_all_feature_ping 	v_data0, weight_offset
	m_cacul_all_feature_last 	v_datb0, weight_offset

// =================================================================================================================
	
DBG_SEG:
	// ===============================================================================
	// 测试 
	// ===============================================================================
/*	
	//v_mov_b32 		v[v_tmp2], s[s_tmp1]											// v_tmp2 = s_tmp1
	//v_cvt_f32_u32 	v[v_tmp2], v[tid]												// v_tmp2 = (float)tid

	v_mov_b32 			v[v_tmp1], 0													// v_tmp1 = accum = 0
	s_wei = s_weia0
	v_dat = v_dat0
	v_acc = v_acc

	.rept MLO_N_LCL_IN_MAPS_ONCE
		v_add_f32 		v[v_tmp1], v[v_dat], v[v_tmp1]									// v_tmp1 = accum += v_dat[0..7]
		//v_fma_f32 	v[v_tmp1], v[v_dat], s[s_wei], v[v_tmp1]						// v_tmp1 = accum += v_dat[0..7] * s_wei[0..7]
		v_dat = v_dat + 1
		s_wei = s_wei + 1
		v_acc = v_acc + 1
	.endr
	
	//v_mov_b32			v[v_tmp1], v[v_acc0]
	v_cvt_f32_u32 		v[v_tmp1], v[v_acc0]											// v_tmp2 = (float)tid
	
	s_waitcnt lgkmcnt(0)
	global_store_dword v[v_addr_dbg:v_addr_dbg+1], v[v_tmp1], off						// v_addr_dbg = v_tmp1 (测试单个输出)
	
	s_branch END_PROG
*/

	// ===============================================================================
	// 存取 
	// =============================================================================== 
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
        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize: 0, PrivateSegmentFixedSize: 0, KernargSegmentAlign: 8, WavefrontSize: 64, MaxFlatWorkGroupSize: 512 }
        Args:
        - { Name: d_in  , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: d_wei , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: d_out , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global  }
      }
}
.end_amd_amdgpu_hsa_metadata
