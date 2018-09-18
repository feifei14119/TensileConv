/************************************************************************************/

/************************************************************************************/
.hsa_code_object_version 2,1
.hsa_code_object_isa 9,0,0,"AMD","AMDGPU"

.text
.globl IsaSmem
.p2align 8
.type IsaSmem,@function
.amdgpu_hsa_kernel IsaSmem

/************************************************************************************/
/* 预定义																			*/
/************************************************************************************/
// ==================================================================================
// 常数定义
// ==================================================================================
.set WAVE_SIZE,			64
.set WAVE_SIZE_LOG2,	6

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
IsaSmem:
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
	s_load_dwordx2 		s[s_a_ptr:s_a_ptr+1], s[s_arg:s_arg+1], 0x0+a_ptr_off   
	s_load_dwordx2 		s[s_b_ptr:s_b_ptr+1], s[s_arg:s_arg+1], 0x0+b_ptr_off
	s_load_dwordx2 		s[s_c_ptr:s_c_ptr+1], s[s_arg:s_arg+1], 0x0+c_ptr_off
	s_waitcnt 			lgkmcnt(0)
		
	// -------------------------------------------------------------------------------
	// offset = WAVE_SIZE * gid_x
	// -------------------------------------------------------------------------------
	s_lshl_b32			s[s_temp0], s[s_gidx], 0x2 + WAVE_SIZE_LOG2						// s_temp0 = offset (BYTE寻址)
	
	// -------------------------------------------------------------------------------
	// 计算输入a下标: 
	// a_addr = a_ptr + (gid_x * WAVE_SIZE)
	// -------------------------------------------------------------------------------
	s_add_u32			s[s_a_ptr], s[s_a_ptr], s[s_temp0]
	s_addc_u32			s[s_a_ptr+1], 0, s[s_a_ptr+1]

	// -------------------------------------------------------------------------------
	// 计算输出下标: 
	// b_addr = b_ptr + (gid_x * WAVE_SIZE)
	// -------------------------------------------------------------------------------
	s_add_u32			s[s_b_ptr], s[s_b_ptr], s[s_temp0]
	s_addc_u32			s[s_b_ptr+1], 0, s[s_b_ptr+1]
		
	// -------------------------------------------------------------------------------
	// 计算输出下标: 
	// c_addr = c_ptr + (gid_x * WAVE_SIZE)
	// -------------------------------------------------------------------------------
	s_add_u32			s[s_c_ptr], s[s_c_ptr], s[s_temp0]
	s_addc_u32			s[s_c_ptr+1], 0, s[s_c_ptr+1]
    
	// -------------------------------------------------------------------------------
	// 计算
	// -------------------------------------------------------------------------------
	s_load_dwordx2		s[s_temp1:s_temp1+1], s[s_a_ptr:s_a_ptr+1], 0
	s_waitcnt 			lgkmcnt(0)
	
	s_memtime			s[s_temp2:s_temp2+1]											// read 64-bit timestamp
	s_memrealtime		s[s_temp3:s_temp3+1]											// read 64-bit RTC
	
	s_store_dwordx2		s[s_temp1:s_temp1+1], s[s_c_ptr:s_c_ptr+1], 0
	s_store_dwordx2		s[s_temp2:s_temp2+1], s[s_c_ptr:s_c_ptr+1], 4*2
	s_store_dwordx2		s[s_temp3:s_temp3+1], s[s_c_ptr:s_c_ptr+1], 4*4
	s_waitcnt 			lgkmcnt(0)
	s_mov_b32			s[s_temp0], 1.23
	s_atomic_add		s[s_temp0], s[s_c_ptr:s_c_ptr+1], 0
	s_waitcnt 			lgkmcnt(0)
	s_dcache_wb
	
END_PROG:
	s_endpgm                              
	
/************************************************************************************/
/* metadata																			*/
/************************************************************************************/	 
.amd_amdgpu_hsa_metadata
{ Version: [ 1, 0 ],
  Kernels: 
    - { Name: IsaSmem, SymbolName: 'IsaSmem', Language: OpenCL C, LanguageVersion: [ 1, 2 ],
        Attrs: { ReqdWorkGroupSize: [ 64, 1, 1 ] }
        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize: 0, PrivateSegmentFixedSize: 0, KernargSegmentAlign: 8, WavefrontSize: 64, MaxFlatWorkGroupSize: 512 }
        Args:
        - { Name: a, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: b, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: c, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global  }
      }
}
.end_amd_amdgpu_hsa_metadata
