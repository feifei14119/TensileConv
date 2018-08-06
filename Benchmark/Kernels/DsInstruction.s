/************************************************************************************/
// ds_write_b32			src0, src1                ds_offset16 gds
//
// ds_offset16:
//			Specifies an immediate unsigned 16-bit offset, in bytes.
//			offset:{0..0xFFFF}
/************************************************************************************/
.hsa_code_object_version 2,1
.hsa_code_object_isa 9,0,0,"AMD","AMDGPU"

.text
.globl DsInstruction
.p2align 8
.type DsInstruction,@function
.amdgpu_hsa_kernel DsInstruction

/************************************************************************************/
/* 预定义																			*/
/************************************************************************************/
// ==================================================================================
// 常数定义
// ==================================================================================
.set LOCAL_SIZE_LOG2,	6

// ==================================================================================
// 输入参数排布
// ==================================================================================
a_ptr_off = 0x00
b_ptr_off = 0x08
c_ptr_off = 0x10
lds_ptr_off = 0x18

// ==================================================================================
// SGPR 排布
// ==================================================================================
s_arg = 4
s_gidx = 6

s_temp0 = 7
s_a_ptr = 8
s_b_ptr = 10
s_c_ptr = 12
s_lds_ptr = 14
s_temp1 = 16
s_temp2 = 18
s_temp3 = 20

// ==================================================================================
// VGPR 排布
// ==================================================================================
v_tid0 = 0
v_a_addr = 2
v_b_addr = 4
v_c_addr = 6
v_lds_addr = 8
v_temp1 = 10
v_temp2 = 12
v_temp3 = 14


/************************************************************************************/
/* 主程序																			*/
/************************************************************************************/
DsInstruction:
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
// Disassembly:
	s_load_dwordx2 s[s_a_ptr:s_a_ptr+1], s[s_arg:s_arg+1], 0x0+a_ptr_off
	s_load_dwordx2 s[s_b_ptr:s_b_ptr+1], s[s_arg:s_arg+1], 0x0+b_ptr_off
	s_load_dwordx2 s[s_c_ptr:s_c_ptr+1], s[s_arg:s_arg+1], 0x0+c_ptr_off
	s_load_dword   s[s_lds_ptr], s[s_arg:s_arg+1], 0x0+lds_ptr_off
	s_waitcnt lgkmcnt(0)
	
	// -------------------------------------------------------------------------------
	// 计算输入a下标: 
	// a_addr = a_ptr + (gid_x * local_size) + 16
	// -------------------------------------------------------------------------------
	v_lshlrev_b32 		v[v_temp1], 0x0 + LOCAL_SIZE_LOG2, s[s_gidx]
	v_add_lshl_u32 		v[v_temp1], v[v_temp1], 0x10, 2
		
	v_mov_b32			v[v_temp2], s[s_a_ptr+1]
	v_add_co_u32 		v[v_a_addr], vcc, s[s_a_ptr], v[v_temp1]
	v_addc_co_u32 		v[v_a_addr+1], vcc, 0, v[v_temp2], vcc
		
	// -------------------------------------------------------------------------------
	// 计算输出下标: 
	// c_addr = c_ptr + (gid_x * local_size) + tid_x
	// -------------------------------------------------------------------------------
	v_lshlrev_b32 		v[v_temp1], 0x0 + LOCAL_SIZE_LOG2, s[s_gidx]
	v_add_lshl_u32 		v[v_temp1], v[v_temp1], v[v_tid0], 2
		
	v_mov_b32			v[v_temp2], s[s_c_ptr+1]
	v_add_co_u32 		v[v_c_addr], vcc, s[s_c_ptr], v[v_temp1]
	v_addc_co_u32 		v[v_c_addr+1], vcc, 0, v[v_temp2], vcc
  
	// -------------------------------------------------------------------------------
	// 计算
	// -------------------------------------------------------------------------------
	//s_mov_b32			m0, -1

	v_lshlrev_b32		v[v_lds_addr], 0x2, v[v_tid0]
	v_add_co_u32		v[v_lds_addr],vcc, s[s_lds_ptr], v[v_lds_addr]
	v_cvt_f32_u32		v[v_temp1],v[v_tid0]
	ds_write_b32		v[v_lds_addr], v[v_temp1]
	s_waitcnt			0

	v_mov_b32			v[v_temp3], 0
	v_add_co_u32		v[v_lds_addr],vcc, 0x4, v[v_lds_addr]
	ds_read_b32			v[v_temp3], v[v_lds_addr]
	s_waitcnt			0

	global_store_dword	v[v_c_addr:v_c_addr+1], v[v_temp3], off

	s_endpgm
	
/************************************************************************************/
/* metadata																			*/
/************************************************************************************/	 
.amd_amdgpu_hsa_metadata
{ Version: [ 1, 0 ],
  Kernels: 
    - { Name: DsInstruction, SymbolName: 'DsInstruction', Language: OpenCL C, LanguageVersion: [ 1, 2 ],
        Attrs: { ReqdWorkGroupSize: [ 64, 1, 1 ] }
        CodeProps: { KernargSegmentSize: 32, GroupSegmentFixedSize: 0, PrivateSegmentFixedSize: 0, KernargSegmentAlign: 8, WavefrontSize: 64, MaxFlatWorkGroupSize: 512 }
        Args:
        - { Name: a		, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: b		, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: c		, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global  }
        - { Name: lds	, Size: 8, Align: 8, ValueKind: DynamicSharedPointer, ValueType: F32, TypeName: 'lds', AddrSpaceQual: Local }
      }
}
.end_amd_amdgpu_hsa_metadata
