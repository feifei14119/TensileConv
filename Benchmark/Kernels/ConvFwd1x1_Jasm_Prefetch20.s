.hsa_code_object_version 2,1
.hsa_code_object_isa 9,0,0,"AMD","AMDGPU"

.text
.globl ConvFwd1x1_Jasm_Prefetch
.p2align 16
.type ConvFwd1x1_Jasm_Prefetch,@function
.amdgpu_hsa_kernel ConvFwd1x1_Jasm_Prefetch

.include "gpr_alloc.inc"
.include "common.inc"
.include "inst_wrappers.inc"

/************************************************************************************/
/* 预定义																			*/
/************************************************************************************/
// ==================================================================================
// 常量定义
// ==================================================================================
.set W,							28
.set H,							28
.set C,							192
.set K,							64

.set MLO_IN_CHANNEL_STRIDE,		(W * H)
.set MLO_IN_BATCH_STRIDE,		(H * W * C)
.set MLO_WEI_CHANNEL_STRIDE,	(1 * 1 * C)
.set MLO_WEI_STRIDE,			(1 * 1 * C * K)
.set MLO_OUT_CHANNEL_STRIDE,	(W * H)
.set MLO_OUT_BATCH_STRIDE,		(H * W * K)

.set MLO_N_OUT_GROUPS,			4				// 一个输出像素的所有特征，分到几个CU上计算
.set MLO_N_OUT_GROUPS_LOG2,		2				// 乘除法时的移位
.set MLO_N_OUT_GROUPS_DIV_MASK, 0x3 			// 求余时的mask
.set MLO_N_IN_GROUPS,			1				// 所有输入通道被分到几个CU
.set MLO_N_IN_GROUPS_LOG2,		0				// 乘除法时的移位
.set MLO_N_IN_GROUPS_DIV_MASK,	0x0				// 求余时的mask

.set MLO_N_LCL_IN_MAPS,			192				// 每个CU负责计算的输入通道个数
.set MLO_N_LCL_IN_MAPS_ONCE,	8				// 每次循环（不展开）负责计算的输入通道个数
.set MLO_N_LCL_OUT_MAPS,		16				// 每个CU负责计算的输出特征数
.set MLO_N_LCL_OUT_MAPS_LOG2,	4
.set MLO_IN_PIX_GROUP,			64				// 每个workgroup处理多少个input pixal
.set MLO_IN_PIX_GROUP_LOG2,		6
.set MLO_IN_PIX_GROUP_DIV_MASK,	0x3F

.set SE_NUM,					4				// shader engin 个数
.set SE_NUM_LOG2,				2
.set FIXED_WORKGROUP_SIZE,		64				// 每个CU的线程数
.set FIXED_WORKGROUP_SIZE_LOG2,	6
.set GROUPS_PER_OUT_BATCH,		(W * H / FIXED_WORKGROUP_SIZE * MLO_N_OUT_GROUPS)	// 48

.set K_FETCH_SUB,				16				// 每次fetch多少通道的weight: cache_line * K < 4K DWORD
.set K_FETCH_STEP,				16				// cache_line
.set SUB_LOOP,					(K/K_FETCH_SUB)
.set FETCH_LOOP,				(C/K_FETCH_STEP)
.set CLOOP0,					(MLO_N_LCL_IN_MAPS / MLO_N_LCL_IN_MAPS_ONCE / 2)

.set IN_PIXEL_PER_GROUP,			64
.set IN_PIXEL_PER_GROUP_LOG2,		6
.set IN_PIXEL_PER_GROUP_MOD_MASK,	63

.set CU_NUM,						64
.set CU_NUM_MOD_MASK,				63
.set SIGNAL_NUM_PER_CU,				16			// channel 数除以 catch_line长度取2的n次幂
.set SIGNAL_NUM_PER_CU_LOG2,		4

.set SIGNAL_REQ_FETCH,				0x1234
.set SIGNAL_EXIT,					0xF0F0
.set SIGNAL_NULL,					0x0

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
	.SGPR_ALLOC s_ptr_sig, 2
    .SGPR_ALLOC s_save_wei, 2
	.SGPR_ALLOC s_wei1
	.SGPR_ALLOC s_wei2
	.SGPR_ALLOC s_wei3
	.SGPR_ALLOC s_wei4
	.SGPR_ALLOC s_wei5
	.SGPR_ALLOC s_wei6
	.SGPR_ALLOC s_wei7
	.SGPR_ALLOC s_wei8
	.SGPR_ALLOC s_wei9
	.SGPR_ALLOC s_wei10
	.SGPR_ALLOC s_wei11
	.SGPR_ALLOC s_wei12
	.SGPR_ALLOC s_wei13
	.SGPR_ALLOC s_wei14
	.SGPR_ALLOC s_wei15
	.SGPR_ALLOC s_wei16
	
	.SGPR_ALLOC s_loop_cnt
	.SGPR_ALLOC s_sub_loop_cnt
	.SGPR_ALLOC s_block_id
	.SGPR_ALLOC s_signal
	.SGPR_ALLOC s_tmp1

	// ------------------------------------------------------------------------------
    .VGPR_ALLOC_FROM 0
    .VGPR_ALLOC tid
	
    .LDS_ALLOC_FROM 0
.GPR_ALLOC_END


/************************************************************************************/
/* 主程序																			*/
/************************************************************************************/
ConvFwd1x1_Jasm_Prefetch:
    .amd_kernel_code_t	
		enable_sgpr_private_segment_buffer	= 1		// needed by this kernel specially
		enable_sgpr_kernarg_segment_ptr 	= 1		//(use 1 SGPR) 64 bit address of Kernarg segment.
		enable_sgpr_workgroup_id_x 			= 1		//(use 1 SGPR) 32 bit work group id in X dimension of grid for wavefront. Always present.
		enable_sgpr_workgroup_id_y 			= 1		//(use 1 SGPR) 32 bit work group id in Y dimension of grid for wavefront
		enable_sgpr_workgroup_id_z 			= 1		//(use 1 SGPR) 32 bit work group id in Z dimension of grid for wavefront. If present then Work-group Id Y will also be present
		
		enable_vgpr_workitem_id 			= 0		//(use 1 SGPR) 32 bit work item id in Y dimension of work-group for wavefront lane.
		
		is_ptr64 							= 1		// ?
		granulated_workitem_vgpr_count 		= .AUTO_VGPR_GRANULATED_COUNT
		granulated_wavefront_sgpr_count 	= .AUTO_SGPR_GRANULATED_COUNT
		user_sgpr_count 					= 6		// ?
		kernarg_segment_byte_size 			= 56	// kernarg segment size(byte)
		wavefront_sgpr_count 				= .AUTO_SGPR_COUNT
		workitem_vgpr_count 				= .AUTO_VGPR_COUNT
		float_mode 							= 240	// ?
		workgroup_group_segment_byte_size 	= 0		//[caculated] group segment memory required by a work-group in bytes		
	.end_amd_kernel_code_t


/************************************************************************************/
/* 测试 																			*/
/************************************************************************************/
.macro m_debug_func
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
.endm	
	
	

/************************************************************************************/
/* 等待信号 																		*/
/************************************************************************************/
.macro m_wait_signal
	s_mov_b32				s[s_signal], SIGNAL_NULL
FETCH_WAIT:
	//s_sleep					0x01
	
	s_lshl_b32				s[s_tmp1], s[s_loop_cnt], 0x02
	s_load_dword			s[s_signal], s[s_ptr_sig:s_ptr_sig+1], s[s_tmp1]
	s_waitcnt 				lgkmcnt(0)
	
	s_cmp_eq_u32			s[s_signal], SIGNAL_REQ_FETCH					// if(signal == SIGNAL_NULL) wait
	s_cbranch_scc0			FETCH_WAIT
.endm	

/************************************************************************************/
/* 预读取 																			*/
/************************************************************************************/
.macro m_fetch_sub
	imm_offset = 0
	s_fetch = s_wei1
	
	.rept K_FETCH_SUB / 2
		s_load_dword 		s[s_fetch], s[s_ptr_wei:s_ptr_wei+1], 0x0 + imm_offset
		imm_offset = imm_offset + MLO_WEI_CHANNEL_STRIDE * 4
		s_fetch = s_fetch + 1
		
		s_load_dword 		s[s_fetch], s[s_ptr_wei:s_ptr_wei+1], 0x0 + imm_offset
		imm_offset = imm_offset + MLO_WEI_CHANNEL_STRIDE * 4
		s_fetch = s_fetch + 1
		
		s_waitcnt lgkmcnt(0)															// 最多15条s_load_指令(lgkmcnt为4bit计数器)		
	.endr
	
	// ------------------------------------------------------------------------------
	// 调整指针: 到下一个 16 k_out
	// ------------------------------------------------------------------------------
	s_add_u32 				s[s_ptr_wei], s[s_ptr_wei], 0x0 + MLO_WEI_CHANNEL_STRIDE * K_FETCH_SUB * 4
	s_addc_u32 				s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0
.endm

/************************************************************************************/
/* 预读取一轮																		*/
/************************************************************************************/
.macro m_fetch_round
	.rept  SUB_LOOP	
		m_fetch_sub
	.endr
	m_point_nx_round
.endm

/************************************************************************************/
/* 调整指针: 指向下一组																*/
/************************************************************************************/
.macro m_point_nx_round
	s_sub_u32 				s[s_ptr_wei], s[s_ptr_wei], 0x0 + (MLO_WEI_STRIDE - K_FETCH_STEP) * 4
	s_subb_u32 				s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0
	// ;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//s_sub_u32 				s[s_ptr_wei], s[s_ptr_wei], 0x0 + MLO_WEI_STRIDE * 4
	//s_subb_u32 				s[s_ptr_wei+1], s[s_ptr_wei+1], 0x0
.endm

// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
	
// Disassembly:	
	// ===============================================================================
	// 获取计算参数
	// ===============================================================================
	s_load_dwordx2 			s[s_ptr_in :s_ptr_in +1], s[kernarg:kernarg+1], 0x0 + in_ptr_off	// desc_in = in_ptr
	s_load_dwordx2 			s[s_ptr_wei:s_ptr_wei+1], s[kernarg:kernarg+1], 0x0 + wei_ptr_off	// desc_wei = wei_ptr
	s_load_dwordx2 			s[s_ptr_out:s_ptr_out+1], s[kernarg:kernarg+1], 0x0 + out_ptr_off	// desc_out = out_ptr
	s_load_dwordx2			s[s_ptr_sig:s_ptr_sig+1], s[kernarg:kernarg+1], 0x0 + sig_ptr_off
	s_waitcnt 				lgkmcnt(0)
		
	// -------------------------------------------------------------------------------
	// 计算 signal 地址 
	// uint glb_sig_off = (grp_id0 % 64) * CLOOP0
	// -------------------------------------------------------------------------------
	s_and_b32				s[s_tmp0], s[gid_x0], 0x0 + CU_NUM_MOD_MASK
	s_lshl_b32				s[s_tmp0], s[s_tmp0], 0x0 + SIGNAL_NUM_PER_CU_LOG2 + 0x2	// dword
	s_add_u32				s[s_ptr_sig], s[s_ptr_sig], s[s_tmp0]
	s_addc_u32				s[s_ptr_sig+1], s[s_ptr_sig+1], 0x0
	
		
	//m_point_nx_round	// ;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//m_point_nx_round	// ;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		
	// ------------------------------------------------------------------------------
	//s_mov_b64				exec, 0x01
	s_mov_b32 				s[s_loop_cnt], CLOOP0 - 1									// channel 的循环	
	
	m_fetch_round	
	
PRE_FETCH:
	m_fetch_round
	
	// ------------------------------------------------------------------------------
	// 等待信号
	// ------------------------------------------------------------------------------
	//m_wait_signal
	
	// -------------------------------------------------------------------------------
	// 循环控制 :
	// -------------------------------------------------------------------------------
	s_sub_u32 				s[s_loop_cnt], s[s_loop_cnt], 0x1							// s_loop_cnt--
	//s_cmpk_eq_i32 		s[s_loop_cnt], 0x0											// ;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	s_cmpk_eq_i32 			s[s_loop_cnt], 0x0+ CLOOP0-3
	s_cbranch_scc0 			PRE_FETCH
	
END_PROG:
	s_endpgm
	
/************************************************************************************/
/* metadate																			*/
/************************************************************************************/
.amd_amdgpu_hsa_metadata
{ Version: [ 1, 0 ],
  Kernels: 
    - { Name: ConvFwd1x1_Jasm_Prefetch, SymbolName: 'ConvFwd1x1_Jasm_Prefetch', Language: OpenCL C, LanguageVersion: [ 1, 2 ],
        Attrs: { ReqdWorkGroupSize: [ 1, 1, 1 ] }
        CodeProps: { KernargSegmentSize: 32, GroupSegmentFixedSize: 0, PrivateSegmentFixedSize: 0, KernargSegmentAlign: 8, WavefrontSize: 64, MaxFlatWorkGroupSize: 512 }
        Args:
        - { Name: d_in  , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: d_wei , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: d_out , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global  }
        - { Name: d_sig , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: U32, TypeName: 'float*', AddrSpaceQual: Global  }
      }
}
.end_amd_amdgpu_hsa_metadata
