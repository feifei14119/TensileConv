/************************************************************************************/
// buffer指令包含MUBUF和MTBUF指令. 用于读写线性buffer内存, 每thread每指令可以操作1~4DWORD
// buffer_load_dword            dst,  src0, src1, src2				idxen offen buf_offset12 glc slc lds
//								vgpr, vgpr, sreg, sreg/-1~-16/64
//
// flat_offset13:
//			Specifies an immediate signed 13-bit offset, in bytes.
//			offset:{-4096..+4095}
//
// Global instructions offer two types of addressing:
//			Memory_addr = VGPR-address + instruction-offset.
//			Memory_addr = SGPR-address + VGPR-offset + instruction-offset.
/************************************************************************************/
.hsa_code_object_version 2,1
.hsa_code_object_isa 9,0,0,"AMD","AMDGPU"

.text
.globl IsaVMBuf
.p2align 8
.type IsaVMBuf,@function
.amdgpu_hsa_kernel IsaVMBuf

/************************************************************************************/
/* 预定义																			*/
/************************************************************************************/
// ==================================================================================
// 常数定义
// ==================================================================================
.set LOCAL_SIZE_LOG2,	6
.set VECTOR_SIZE, 512

// ==================================================================================
// 输入参数排布
// ==================================================================================
a_ptr_off = 0x00
b_ptr_off = 0x08
c_ptr_off = 0x10

// ==================================================================================
// SGPR 排布
// ==================================================================================
s_arg = 4
s_gidx = 6

s_temp0 = 7
s_a_ptr = 8
s_b_ptr = 10
s_c_ptr = 12
s_temp1 = 14
s_temp2 = 16
s_temp3 = 18
s_desc0 = 20
s_desc1 = 21
s_desc2 = 22
s_desc3 = 23
s_base_offset = 24

// ==================================================================================
// VGPR 排布
// ==================================================================================
v_tid0 = 0
v_a_addr = 2
v_b_addr = 4
v_c_addr = 6
v_temp1 = 8
v_temp2 = 10
v_temp3 = 12
v_index = 14
v_offset = 15
v_input = 16


/************************************************************************************/
/* 主程序																			*/
/************************************************************************************/
IsaVMBuf:
	// ===============================================================================
	// ===============================================================================
	.amd_kernel_code_t
		enable_sgpr_private_segment_buffer = 1
		enable_sgpr_kernarg_segment_ptr = 1
		enable_sgpr_workgroup_id_x = 1
		enable_sgpr_workgroup_id_y = 0
		enable_sgpr_workgroup_id_z = 0
		
		enable_vgpr_workitem_id = 0
	    
		is_ptr64 = 1
		float_mode = 240
		
		granulated_workitem_vgpr_count = 15
		granulated_wavefront_sgpr_count = 7
		user_sgpr_count = 6
		kernarg_segment_byte_size = 56
		wavefront_sgpr_count = 64
		workitem_vgpr_count = 64
		workgroup_group_segment_byte_size = 0		
	.end_amd_kernel_code_t
	
	// ===============================================================================
	// ===============================================================================
START_PROG:        
	s_load_dwordx2 s[s_a_ptr:s_a_ptr+1], s[s_arg:s_arg+1], 0x0+a_ptr_off   
	s_load_dwordx2 s[s_b_ptr:s_b_ptr+1], s[s_arg:s_arg+1], 0x0+b_ptr_off
	s_load_dwordx2 s[s_c_ptr:s_c_ptr+1], s[s_arg:s_arg+1], 0x0+c_ptr_off
	s_waitcnt lgkmcnt(0)
	
	s_cmpk_gt_u32		s[s_gidx], 0
	s_cbranch_scc1		END_PROG
	v_cmpx_lt_u32		vcc, v[v_tid0], 16
	
	// -------------------------------------------------------------------------------
	// 计算输入a下标: 
	// a_addr = a_ptr + (gid_x * local_size) + tid_x
	// -------------------------------------------------------------------------------
	v_lshlrev_b32 		v[v_temp1], 0x0 + LOCAL_SIZE_LOG2, s[s_gidx]
	v_add_lshl_u32 		v[v_temp1], v[v_temp1], v[v_tid0], 2
		
	v_mov_b32			v[v_temp2], s[s_a_ptr+1]
	v_add_co_u32 		v[v_a_addr], vcc, s[s_a_ptr], v[v_temp1]
	v_addc_co_u32 		v[v_a_addr+1], vcc, 0, v[v_temp2], vcc

	// -------------------------------------------------------------------------------
	// 计算输出下标: 
	// b_addr = b_ptr + (gid_x * local_size) + tid_x
	// -------------------------------------------------------------------------------
	v_lshlrev_b32 		v[v_temp1], 0x0 + LOCAL_SIZE_LOG2, s[s_gidx]
	v_add_lshl_u32 		v[v_temp1], v[v_temp1], v[v_tid0], 2

	v_mov_b32			v[v_temp2], s[s_b_ptr + 1]
	v_add_co_u32 		v[v_b_addr], vcc, s[s_b_ptr], v[v_temp1]
	v_addc_co_u32 		v[v_b_addr + 1], vcc, 0, v[v_temp2], vcc
		
	// -------------------------------------------------------------------------------
	// 计算输出下标: 
	// c_addr = c_ptr + (gid_x * local_size) + tid_x
	// -------------------------------------------------------------------------------
	v_lshlrev_b32 		v[v_temp1], 0x0 + LOCAL_SIZE_LOG2, s[s_gidx]
	v_add_lshl_u32 		v[v_temp1], v[v_temp1], v[v_tid0], 2
		
	v_mov_b32			v[v_temp2], s[s_c_ptr+1]
	v_add_co_u32 		v[v_c_addr], vcc, s[s_c_ptr], v[v_temp1]
	v_addc_co_u32 		v[v_c_addr+1], vcc, 0, v[v_temp2], vcc
  
	//// -------------------------------------------------------------------------------
	//// 计算1
	//// -------------------------------------------------------------------------------
	//stride = (64*4) << 16												// ;记录长度(BYTE)
	//s_mov_b64			s[s_desc0:s_desc1], s[s_a_ptr:s_a_ptr+1]
	//s_or_b32			s[s_desc1], s[s_desc1], 0x0 + stride
	//s_mov_b32			s[s_desc2], 16									// ;记录条数
	//s_mov_b32			s[s_desc3], 0x00027000
	//
	//s_mov_b32			s[s_base_offset], 0x0 
	//v_mov_b32			v[v_index],	 v[v_tid0]
	//v_mov_b32			v[v_offset], 4*1								// (BYTE)
	//
	//buffer_load_dwordx4	v[v_input:v_input+3], v[v_index:v_offset], s[s_desc0:s_desc3] s[s_base_offset] idxen offen
	//s_waitcnt			vmcnt(0)
	//
	//v_mov_b32			v[v_temp1], v[v_input+3]
	//global_store_dword	v[v_c_addr:v_c_addr+1], v[v_temp1], off
    
	//// -------------------------------------------------------------------------------
	//// 计算2
	//// -------------------------------------------------------------------------------
	//stride = (64*4) << 16												// ;记录长度(BYTE)
	//s_mov_b64			s[s_desc0:s_desc1], s[s_a_ptr:s_a_ptr+1]
	//s_or_b32			s[s_desc1], s[s_desc1], 0x0 + stride
	//s_mov_b32			s[s_desc2], 16									// ;记录条数
	//s_mov_b32			s[s_desc3], 0x00807000							// add_tid_en = 1, ; dat_fmt存储stride[17:15]
	//
	//s_mov_b32			s[s_base_offset], 0x0 
	//v_mov_b32			v[v_index],	 0
	//v_mov_b32			v[v_offset], v[v_tid0]
	//
	//buffer_load_dwordx4	v[v_input:v_input+3], off, s[s_desc0:s_desc3], s[s_base_offset]
	//s_waitcnt			vmcnt(0)
	//
	//v_mov_b32			v[v_temp1], v[v_input+3]
	//global_store_dword	v[v_c_addr:v_c_addr+1], v[v_temp1], off
	
    
	// -------------------------------------------------------------------------------
	// 计算3
	// -------------------------------------------------------------------------------
	stride = (64*4) << 16												// ;记录长度(BYTE)
	s_mov_b64			s[s_desc0:s_desc1], s[s_a_ptr:s_a_ptr+1]
	s_or_b32			s[s_desc1], s[s_desc1], (0x0 + stride) | 0x80000000 // swizzle_enable
	s_mov_b32			s[s_desc2], 16									// ;记录条数
	s_mov_b32			s[s_desc3], 0x00027000
	
	s_mov_b32			s[s_base_offset], 0x0
	v_mov_b32			v[v_index],	 0
	v_mov_b32			v[v_offset], v[v_tid0]
	v_lshlrev_b32 		v[v_offset], 2, v[v_offset]
	
	buffer_load_dwordx2	v[v_input+2:v_input+3], v[v_index:v_offset], s[s_desc0:s_desc3], s[s_base_offset] idxen offen	
	s_waitcnt			vmcnt(0)
	
	v_mov_b32			v[v_temp1], v[v_input+3]
	global_store_dword	v[v_c_addr:v_c_addr+1], v[v_temp1], off
	
END_PROG:
	s_endpgm                              
	
/************************************************************************************/
/* metadata																			*/
/************************************************************************************/	 
.amd_amdgpu_hsa_metadata
{ Version: [ 1, 0 ],
  Kernels: 
    - { Name: IsaVMBuf, SymbolName: 'IsaVMBuf', Language: OpenCL C, LanguageVersion: [ 1, 2 ],
        Attrs: { ReqdWorkGroupSize: [ 64, 1, 1 ] }
        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize: 0, PrivateSegmentFixedSize: 0, KernargSegmentAlign: 8, WavefrontSize: 64, MaxFlatWorkGroupSize: 512 }
        Args:
        - { Name: a, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: b, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: c, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global  }
      }
}
.end_amd_amdgpu_hsa_metadata
