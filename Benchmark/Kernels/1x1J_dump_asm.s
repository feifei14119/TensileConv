	.hsa_code_object_version 2,1

	.hsa_code_object_isa 9,0,0,"AMD","AMDGPU"

	.amdgpu_hsa_kernel ConvFwd1x1_Jcl

ConvFwd1x1_Jcl:
	.amd_kernel_code_t
		amd_code_version_major = 1
		amd_code_version_minor = 1
		amd_machine_kind = 1
		amd_machine_version_major = 9
		amd_machine_version_minor = 0
		amd_machine_version_stepping = 0
		kernel_code_entry_byte_offset = 256
		kernel_code_prefetch_byte_size = 0
		max_scratch_backing_memory_byte_size = 0
		granulated_workitem_vgpr_count = 11
		granulated_wavefront_sgpr_count = 12
		priority = 0
		float_mode = 240
		priv = 0
		enable_dx10_clamp = 1
		debug_mode = 0
		enable_ieee_mode = 1
		enable_sgpr_private_segment_wave_byte_offset = 0
		user_sgpr_count = 6
		enable_trap_handler = 1
		enable_sgpr_workgroup_id_x = 1
		enable_sgpr_workgroup_id_y = 0
		enable_sgpr_workgroup_id_z = 1
		enable_sgpr_workgroup_info = 0
		enable_vgpr_workitem_id = 0
		enable_exception_msb = 0
		granulated_lds_size = 0
		enable_exception = 0
		enable_sgpr_private_segment_buffer = 1
		enable_sgpr_dispatch_ptr = 0
		enable_sgpr_queue_ptr = 0
		enable_sgpr_kernarg_segment_ptr = 1
		enable_sgpr_dispatch_id = 0
		enable_sgpr_flat_scratch_init = 0
		enable_sgpr_private_segment_size = 0
		enable_sgpr_grid_workgroup_count_x = 0
		enable_sgpr_grid_workgroup_count_y = 0
		enable_sgpr_grid_workgroup_count_z = 0
		enable_ordered_append_gds = 0
		private_element_size = 1
		is_ptr64 = 1
		is_dynamic_callstack = 0
		is_debug_enabled = 0
		is_xnack_enabled = 0
		workitem_private_segment_byte_size = 0
		workgroup_group_segment_byte_size = 0
		gds_segment_byte_size = 0
		kernarg_segment_byte_size = 56
		workgroup_fbarrier_count = 0
		wavefront_sgpr_count = 98
		workitem_vgpr_count = 46
		reserved_vgpr_first = 0
		reserved_vgpr_count = 0
		reserved_sgpr_first = 0
		reserved_sgpr_count = 0
		debug_wavefront_private_segment_offset_sgpr = 0
		debug_private_segment_buffer_sgpr = 0
		kernarg_segment_alignment = 4
		group_segment_alignment = 4
		private_segment_alignment = 4
		wavefront_size = 6
		call_convention = -1
		runtime_loader_kernel_symbol = 0
	.end_amd_kernel_code_t

// Disassembly:
	s_lshl_b32 s2, s6, 4                                       // 000000001100: 8E028406
	s_and_b32 s0, s2, 0xffffffc0                               // 000000001104: 8600FF02 FFFFFFC0
	v_or_b32_e32 v0, s0, v0                                    // 00000000110C: 28000000
	v_mov_b32_e32 v1, 0x3100                                   // 000000001110: 7E0202FF 00003100
	v_cmp_lt_u32_e32 vcc, v0, v1                               // 000000001118: 7D920300
	s_and_saveexec_b64 s[0:1], vcc                             // 00000000111C: BE80206A
	s_cbranch_execz BB0_5                                      // 000000001120: BF8807FF
BB0_1:
	v_mov_b32_e32 v1, 0x5397829d                               // 000000001124: 7E0202FF 5397829D
	v_mul_hi_u32 v1, v0, v1                                    // 00000000112C: D2860001 00020300
	s_load_dwordx2 s[8:9], s[4:5], 0x0                         // 000000001134: C0060202 00000000
	v_mov_b32_e32 v33, 0                                       // 00000000113C: 7E420280
	s_and_b32 s2, s2, 48                                       // 000000001140: 8602B002
	v_lshrrev_b32_e32 v16, 8, v1                               // 000000001144: 20200288
	v_mov_b32_e32 v1, 0xfffffcf0                               // 000000001148: 7E0202FF FFFFFCF0
	v_mad_i32_i24 v14, v16, v1, v0                             // 000000001150: D1C2000E 04020310
	v_mul_u32_u24_e32 v0, 0x24c00, v16                         // 000000001158: 100020FF 00024C00
	v_or_b32_e32 v32, v14, v0                                  // 000000001160: 2840010E
	v_mov_b32_e32 v0, 0xc0                                     // 000000001164: 7E0002FF 000000C0
	v_mul_u32_u24_e32 v12, s2, v0                              // 00000000116C: 10180002
	v_lshlrev_b64 v[0:1], 2, v[32:33]                          // 000000001170: D28F0000 00024082
	s_load_dwordx2 s[10:11], s[4:5], 0x8                       // 000000001178: C0060282 00000008
	s_load_dwordx2 s[0:1], s[4:5], 0x10                        // 000000001180: C0060002 00000010
	s_waitcnt lgkmcnt(0)                                       // 000000001188: BF8CC07F
	v_mov_b32_e32 v2, s9                                       // 00000000118C: 7E040209
	v_add_co_u32_e32 v8, vcc, s8, v0                           // 000000001190: 32100008
	v_addc_co_u32_e32 v9, vcc, v2, v1, vcc                     // 000000001194: 38120302
	s_movk_i32 s3, 0x1880                                      // 000000001198: B0031880
	v_add_co_u32_e32 v0, vcc, s3, v8                           // 00000000119C: 32001003
	v_addc_co_u32_e32 v1, vcc, 0, v9, vcc                      // 0000000011A0: 38021280
	s_movk_i32 s4, 0x24c0                                      // 0000000011A4: B00424C0
	v_add_co_u32_e32 v4, vcc, s4, v8                           // 0000000011A8: 32081004
	v_addc_co_u32_e32 v5, vcc, 0, v9, vcc                      // 0000000011AC: 380A1280
	s_movk_i32 s5, 0x3100                                      // 0000000011B0: B0053100
	v_add_co_u32_e32 v2, vcc, s5, v8                           // 0000000011B4: 32041005
	v_addc_co_u32_e32 v3, vcc, 0, v9, vcc                      // 0000000011B8: 38061280
	s_movk_i32 s6, 0x3d40                                      // 0000000011BC: B0063D40
	global_load_dword v3, v[2:3], off                          // 0000000011C0: DC508000 037F0002
	global_load_dword v4, v[4:5], off                          // 0000000011C8: DC508000 047F0004
	global_load_dword v5, v[0:1], off                          // 0000000011D0: DC508000 057F0000
	v_add_co_u32_e32 v0, vcc, s6, v8                           // 0000000011D8: 32001006
	v_addc_co_u32_e32 v1, vcc, 0, v9, vcc                      // 0000000011DC: 38021280
	s_movk_i32 s8, 0x4980                                      // 0000000011E0: B0084980
	global_load_dword v2, v[0:1], off                          // 0000000011E4: DC508000 027F0000
	v_add_co_u32_e32 v0, vcc, s8, v8                           // 0000000011EC: 32001008
	v_addc_co_u32_e32 v1, vcc, 0, v9, vcc                      // 0000000011F0: 38021280
	s_movk_i32 s9, 0x55c0                                      // 0000000011F4: B00955C0
	v_add_co_u32_e32 v10, vcc, s9, v8                          // 0000000011F8: 32141009
	v_addc_co_u32_e32 v11, vcc, 0, v9, vcc                     // 0000000011FC: 38161280
	global_load_dword v1, v[0:1], off                          // 000000001200: DC508000 017F0000
	global_load_dword v7, v[8:9], off                          // 000000001208: DC508000 077F0008
	global_load_dword v6, v[8:9], off offset:3136              // 000000001210: DC508C40 067F0008
	global_load_dword v0, v[10:11], off                        // 000000001218: DC508000 007F000A
	v_lshlrev_b32_e32 v10, 2, v12                              // 000000001220: 24141882
	v_add_co_u32_e32 v23, vcc, s10, v10                        // 000000001224: 322E140A
	v_mov_b32_e32 v11, s11                                     // 000000001228: 7E16020B
	v_addc_co_u32_e32 v24, vcc, 0, v11, vcc                    // 00000000122C: 38301680
	s_movk_i32 s10, 0x6200                                     // 000000001230: B00A6200
	v_add_co_u32_e32 v29, vcc, s10, v8                         // 000000001234: 323A100A
	v_addc_co_u32_e32 v30, vcc, 0, v9, vcc                     // 000000001238: 383C1280
	v_cmp_eq_u32_e64 vcc, s7, 31                               // 00000000123C: D0CA006A 00013E07
	v_mov_b32_e32 v8, 0x177                                    // 000000001244: 7E1002FF 00000177
	v_cndmask_b32_e32 v28, 22, v8, vcc                         // 00000000124C: 00381096
	v_add_co_u32_e32 v8, vcc, -1, v28                          // 000000001250: 321038C1
	v_lshrrev_b32_e32 v8, 1, v8                                // 000000001254: 20101081
	v_lshlrev_b32_e32 v27, 4, v8                               // 000000001258: 24361084
	v_mul_lo_u32 v8, v8, s5                                    // 00000000125C: D2850008 00000B08
	v_mov_b32_e32 v35, v24                                     // 000000001264: 7E460318
	v_mov_b32_e32 v37, v30                                     // 000000001268: 7E4A031E
	s_movk_i32 s7, 0x6e40                                      // 00000000126C: B0076E40
	v_add_co_u32_e32 v31, vcc, s5, v8                          // 000000001270: 323E1005
	s_movk_i32 s11, 0x7a80                                     // 000000001274: B00B7A80
	s_mov_b32 s12, 0x86c0                                      // 000000001278: BE8C00FF 000086C0
	s_mov_b32 s13, 0x9300                                      // 000000001280: BE8D00FF 00009300
	s_mov_b32 s14, 0x9f40                                      // 000000001288: BE8E00FF 00009F40
	s_mov_b32 s15, 0xab80                                      // 000000001290: BE8F00FF 0000AB80
	s_mov_b32 s16, 0xb7c0                                      // 000000001298: BE9000FF 0000B7C0
	s_mov_b32 s17, 0xc400                                      // 0000000012A0: BE9100FF 0000C400
	v_mov_b32_e32 v8, v33                                      // 0000000012A8: 7E100321
	v_mov_b32_e32 v9, v33                                      // 0000000012AC: 7E120321
	v_mov_b32_e32 v10, v33                                     // 0000000012B0: 7E140321
	v_mov_b32_e32 v11, v33                                     // 0000000012B4: 7E160321
	v_mov_b32_e32 v12, v33                                     // 0000000012B8: 7E180321
	v_mov_b32_e32 v13, v33                                     // 0000000012BC: 7E1A0321
	v_mov_b32_e32 v15, v33                                     // 0000000012C0: 7E1E0321
	v_mov_b32_e32 v17, v33                                     // 0000000012C4: 7E220321
	v_mov_b32_e32 v18, v33                                     // 0000000012C8: 7E240321
	v_mov_b32_e32 v19, v33                                     // 0000000012CC: 7E260321
	v_mov_b32_e32 v20, v33                                     // 0000000012D0: 7E280321
	v_mov_b32_e32 v21, v33                                     // 0000000012D4: 7E2A0321
	v_mov_b32_e32 v22, v33                                     // 0000000012D8: 7E2C0321
	v_mov_b32_e32 v25, v33                                     // 0000000012DC: 7E320321
	v_mov_b32_e32 v26, v33                                     // 0000000012E0: 7E340321
	v_mov_b32_e32 v32, v33                                     // 0000000012E4: 7E400321
	v_mov_b32_e32 v34, v23                                     // 0000000012E8: 7E440317
	v_mov_b32_e32 v36, v29                                     // 0000000012EC: 7E48031D
	s_branch BB0_3                                             // 0000000012F0: BF820004
BB0_2:
	v_add_co_u32_e32 v34, vcc, 64, v34                         // 0000000012F4: 324444C0
	v_addc_co_u32_e32 v35, vcc, 0, v35, vcc                    // 0000000012F8: 38464680
	v_add_co_u32_e32 v36, vcc, s17, v36                        // 0000000012FC: 32484811
	v_addc_co_u32_e32 v37, vcc, 0, v37, vcc                    // 000000001300: 384A4A80
BB0_3:
	v_readfirstlane_b32 s18, v34                               // 000000001304: 7E240522
	v_readfirstlane_b32 s19, v35                               // 000000001308: 7E260523
	s_load_dwordx4 s[32:35], s[18:19], 0x0                     // 00000000130C: C00A0809 00000000
	s_load_dwordx4 s[28:31], s[18:19], 0x10                    // 000000001314: C00A0709 00000010
	v_add_co_u32_e32 v32, vcc, 2, v32                          // 00000000131C: 32404082
	v_add_co_u32_e32 v38, vcc, s3, v36                         // 000000001320: 324C4803
	s_load_dwordx4 s[24:27], s[18:19], 0x2d00                  // 000000001324: C00A0609 00002D00
	s_waitcnt vmcnt(2) lgkmcnt(0)                              // 00000000132C: BF8C0072
	v_fma_f32 v26, v7, s32, v26                                // 000000001330: D1CB001A 04684107
	s_waitcnt vmcnt(1)                                         // 000000001338: BF8C0F71
	v_fma_f32 v26, v6, s33, v26                                // 00000000133C: D1CB001A 04684306
	v_fma_f32 v26, v5, s34, v26                                // 000000001344: D1CB001A 04684505
	v_fma_f32 v26, v4, s35, v26                                // 00000000134C: D1CB001A 04684704
	s_load_dwordx4 s[36:39], s[18:19], 0x300                   // 000000001354: C00A0909 00000300
	s_load_dwordx4 s[32:35], s[18:19], 0x310                   // 00000000135C: C00A0809 00000310
	v_addc_co_u32_e32 v39, vcc, 0, v37, vcc                    // 000000001364: 384E4A80
	global_load_dword v40, v[38:39], off                       // 000000001368: DC508000 287F0026
	v_add_co_u32_e32 v38, vcc, s9, v36                         // 000000001370: 324C4809
	s_waitcnt lgkmcnt(0)                                       // 000000001374: BF8CC07F
	v_fma_f32 v25, v7, s36, v25                                // 000000001378: D1CB0019 04644907
	v_fma_f32 v25, v6, s37, v25                                // 000000001380: D1CB0019 04644B06
	v_fma_f32 v25, v5, s38, v25                                // 000000001388: D1CB0019 04644D05
	v_fma_f32 v25, v4, s39, v25                                // 000000001390: D1CB0019 04644F04
	s_load_dwordx4 s[40:43], s[18:19], 0x600                   // 000000001398: C00A0A09 00000600
	s_load_dwordx4 s[36:39], s[18:19], 0x610                   // 0000000013A0: C00A0909 00000610
	v_addc_co_u32_e32 v39, vcc, 0, v37, vcc                    // 0000000013A8: 384E4A80
	s_load_dwordx4 s[20:23], s[18:19], 0x2d10                  // 0000000013AC: C00A0509 00002D10
	global_load_dword v41, v[38:39], off                       // 0000000013B4: DC508000 297F0026
	s_waitcnt lgkmcnt(0)                                       // 0000000013BC: BF8CC07F
	v_fma_f32 v22, v7, s40, v22                                // 0000000013C0: D1CB0016 04585107
	v_fma_f32 v22, v6, s41, v22                                // 0000000013C8: D1CB0016 04585306
	v_fma_f32 v22, v5, s42, v22                                // 0000000013D0: D1CB0016 04585505
	v_fma_f32 v22, v4, s43, v22                                // 0000000013D8: D1CB0016 04585704
	s_load_dwordx4 s[44:47], s[18:19], 0x900                   // 0000000013E0: C00A0B09 00000900
	s_load_dwordx4 s[40:43], s[18:19], 0x910                   // 0000000013E8: C00A0A09 00000910
	v_add_co_u32_e32 v38, vcc, s8, v36                         // 0000000013F0: 324C4808
	v_addc_co_u32_e32 v39, vcc, 0, v37, vcc                    // 0000000013F4: 384E4A80
	v_fma_f32 v33, v7, s24, v33                                // 0000000013F8: D1CB0021 04843107
	s_waitcnt lgkmcnt(0)                                       // 000000001400: BF8CC07F
	v_fma_f32 v21, v7, s44, v21                                // 000000001404: D1CB0015 04545907
	v_fma_f32 v21, v6, s45, v21                                // 00000000140C: D1CB0015 04545B06
	v_fma_f32 v21, v5, s46, v21                                // 000000001414: D1CB0015 04545D05
	v_fma_f32 v21, v4, s47, v21                                // 00000000141C: D1CB0015 04545F04
	s_load_dwordx4 s[48:51], s[18:19], 0xc00                   // 000000001424: C00A0C09 00000C00
	s_load_dwordx4 s[44:47], s[18:19], 0xc10                   // 00000000142C: C00A0B09 00000C10
	global_load_dword v42, v[38:39], off                       // 000000001434: DC508000 2A7F0026
	v_add_co_u32_e32 v38, vcc, s6, v36                         // 00000000143C: 324C4806
	v_addc_co_u32_e32 v39, vcc, 0, v37, vcc                    // 000000001440: 384E4A80
	s_waitcnt lgkmcnt(0)                                       // 000000001444: BF8CC07F
	v_fma_f32 v20, v7, s48, v20                                // 000000001448: D1CB0014 04506107
	v_fma_f32 v20, v6, s49, v20                                // 000000001450: D1CB0014 04506306
	v_fma_f32 v20, v5, s50, v20                                // 000000001458: D1CB0014 04506505
	v_fma_f32 v20, v4, s51, v20                                // 000000001460: D1CB0014 04506704
	s_load_dwordx4 s[48:51], s[18:19], 0xf00                   // 000000001468: C00A0C09 00000F00
	s_load_dwordx4 s[52:55], s[18:19], 0xf10                   // 000000001470: C00A0D09 00000F10
	global_load_dword v43, v[38:39], off                       // 000000001478: DC508000 2B7F0026
	v_add_co_u32_e32 v38, vcc, s5, v36                         // 000000001480: 324C4805
	v_fma_f32 v20, v3, s44, v20                                // 000000001484: D1CB0014 04505903
	s_waitcnt lgkmcnt(0)                                       // 00000000148C: BF8CC07F
	v_fma_f32 v19, v7, s48, v19                                // 000000001490: D1CB0013 044C6107
	v_fma_f32 v19, v6, s49, v19                                // 000000001498: D1CB0013 044C6306
	v_fma_f32 v19, v5, s50, v19                                // 0000000014A0: D1CB0013 044C6505
	v_fma_f32 v19, v4, s51, v19                                // 0000000014A8: D1CB0013 044C6704
	s_load_dwordx4 s[48:51], s[18:19], 0x1200                  // 0000000014B0: C00A0C09 00001200
	s_load_dwordx4 s[56:59], s[18:19], 0x1210                  // 0000000014B8: C00A0E09 00001210
	v_fma_f32 v19, v3, s52, v19                                // 0000000014C0: D1CB0013 044C6903
	v_addc_co_u32_e32 v39, vcc, 0, v37, vcc                    // 0000000014C8: 384E4A80
	v_fma_f32 v21, v3, s40, v21                                // 0000000014CC: D1CB0015 04545103
	s_waitcnt lgkmcnt(0)                                       // 0000000014D4: BF8CC07F
	v_fma_f32 v18, v7, s48, v18                                // 0000000014D8: D1CB0012 04486107
	v_fma_f32 v18, v6, s49, v18                                // 0000000014E0: D1CB0012 04486306
	v_fma_f32 v18, v5, s50, v18                                // 0000000014E8: D1CB0012 04486505
	v_fma_f32 v18, v4, s51, v18                                // 0000000014F0: D1CB0012 04486704
	s_load_dwordx4 s[48:51], s[18:19], 0x1500                  // 0000000014F8: C00A0C09 00001500
	s_load_dwordx4 s[60:63], s[18:19], 0x1510                  // 000000001500: C00A0F09 00001510
	v_fma_f32 v18, v3, s56, v18                                // 000000001508: D1CB0012 04487103
	v_fma_f32 v19, v2, s53, v19                                // 000000001510: D1CB0013 044C6B02
	v_fma_f32 v18, v2, s57, v18                                // 000000001518: D1CB0012 04487302
	s_waitcnt lgkmcnt(0)                                       // 000000001520: BF8CC07F
	v_fma_f32 v17, v7, s48, v17                                // 000000001524: D1CB0011 04446107
	v_fma_f32 v17, v6, s49, v17                                // 00000000152C: D1CB0011 04446306
	v_fma_f32 v17, v5, s50, v17                                // 000000001534: D1CB0011 04446505
	v_fma_f32 v17, v4, s51, v17                                // 00000000153C: D1CB0011 04446704
	s_load_dwordx4 s[48:51], s[18:19], 0x1800                  // 000000001544: C00A0C09 00001800
	s_load_dwordx4 s[64:67], s[18:19], 0x1810                  // 00000000154C: C00A1009 00001810
	v_fma_f32 v17, v3, s60, v17                                // 000000001554: D1CB0011 04447903
	v_fma_f32 v17, v2, s61, v17                                // 00000000155C: D1CB0011 04447B02
	global_load_dword v44, v[38:39], off                       // 000000001564: DC508000 2C7F0026
	s_waitcnt lgkmcnt(0)                                       // 00000000156C: BF8CC07F
	v_fma_f32 v15, v7, s48, v15                                // 000000001570: D1CB000F 043C6107
	v_fma_f32 v15, v6, s49, v15                                // 000000001578: D1CB000F 043C6306
	v_fma_f32 v15, v5, s50, v15                                // 000000001580: D1CB000F 043C6505
	v_fma_f32 v15, v4, s51, v15                                // 000000001588: D1CB000F 043C6704
	s_load_dwordx4 s[48:51], s[18:19], 0x1b00                  // 000000001590: C00A0C09 00001B00
	s_load_dwordx4 s[68:71], s[18:19], 0x1b10                  // 000000001598: C00A1109 00001B10
	v_fma_f32 v15, v3, s64, v15                                // 0000000015A0: D1CB000F 043C8103
	v_add_co_u32_e32 v38, vcc, s4, v36                         // 0000000015A8: 324C4804
	v_fma_f32 v15, v2, s65, v15                                // 0000000015AC: D1CB000F 043C8302
	s_waitcnt lgkmcnt(0)                                       // 0000000015B4: BF8CC07F
	v_fma_f32 v13, v7, s48, v13                                // 0000000015B8: D1CB000D 04346107
	v_fma_f32 v13, v6, s49, v13                                // 0000000015C0: D1CB000D 04346306
	v_fma_f32 v13, v5, s50, v13                                // 0000000015C8: D1CB000D 04346505
	v_fma_f32 v13, v4, s51, v13                                // 0000000015D0: D1CB000D 04346704
	s_load_dwordx4 s[48:51], s[18:19], 0x1e00                  // 0000000015D8: C00A0C09 00001E00
	s_load_dwordx4 s[72:75], s[18:19], 0x1e10                  // 0000000015E0: C00A1209 00001E10
	v_fma_f32 v13, v3, s68, v13                                // 0000000015E8: D1CB000D 04348903
	v_fma_f32 v13, v2, s69, v13                                // 0000000015F0: D1CB000D 04348B02
	v_fma_f32 v20, v2, s45, v20                                // 0000000015F8: D1CB0014 04505B02
	s_waitcnt lgkmcnt(0)                                       // 000000001600: BF8CC07F
	v_fma_f32 v12, v7, s48, v12                                // 000000001604: D1CB000C 04306107
	v_fma_f32 v12, v6, s49, v12                                // 00000000160C: D1CB000C 04306306
	v_fma_f32 v12, v5, s50, v12                                // 000000001614: D1CB000C 04306505
	v_fma_f32 v12, v4, s51, v12                                // 00000000161C: D1CB000C 04306704
	s_load_dwordx4 s[48:51], s[18:19], 0x2100                  // 000000001624: C00A0C09 00002100
	s_load_dwordx4 s[76:79], s[18:19], 0x2110                  // 00000000162C: C00A1309 00002110
	v_fma_f32 v12, v3, s72, v12                                // 000000001634: D1CB000C 04309103
	v_fma_f32 v12, v2, s73, v12                                // 00000000163C: D1CB000C 04309302
	v_fma_f32 v19, v1, s54, v19                                // 000000001644: D1CB0013 044C6D01
	s_waitcnt lgkmcnt(0)                                       // 00000000164C: BF8CC07F
	v_fma_f32 v11, v7, s48, v11                                // 000000001650: D1CB000B 042C6107
	v_fma_f32 v11, v6, s49, v11                                // 000000001658: D1CB000B 042C6306
	v_fma_f32 v11, v5, s50, v11                                // 000000001660: D1CB000B 042C6505
	v_fma_f32 v11, v4, s51, v11                                // 000000001668: D1CB000B 042C6704
	s_load_dwordx4 s[48:51], s[18:19], 0x2400                  // 000000001670: C00A0C09 00002400
	s_load_dwordx4 s[80:83], s[18:19], 0x2410                  // 000000001678: C00A1409 00002410
	v_fma_f32 v11, v3, s76, v11                                // 000000001680: D1CB000B 042C9903
	v_fma_f32 v11, v2, s77, v11                                // 000000001688: D1CB000B 042C9B02
	v_fma_f32 v18, v1, s58, v18                                // 000000001690: D1CB0012 04487501
	s_waitcnt lgkmcnt(0)                                       // 000000001698: BF8CC07F
	v_fma_f32 v10, v7, s48, v10                                // 00000000169C: D1CB000A 04286107
	v_fma_f32 v10, v6, s49, v10                                // 0000000016A4: D1CB000A 04286306
	v_fma_f32 v10, v5, s50, v10                                // 0000000016AC: D1CB000A 04286505
	v_fma_f32 v10, v4, s51, v10                                // 0000000016B4: D1CB000A 04286704
	s_load_dwordx4 s[48:51], s[18:19], 0x2700                  // 0000000016BC: C00A0C09 00002700
	s_load_dwordx4 s[84:87], s[18:19], 0x2710                  // 0000000016C4: C00A1509 00002710
	v_fma_f32 v10, v3, s80, v10                                // 0000000016CC: D1CB000A 0428A103
	v_fma_f32 v10, v2, s81, v10                                // 0000000016D4: D1CB000A 0428A302
	v_fma_f32 v17, v1, s62, v17                                // 0000000016DC: D1CB0011 04447D01
	s_waitcnt lgkmcnt(0)                                       // 0000000016E4: BF8CC07F
	v_fma_f32 v9, v7, s48, v9                                  // 0000000016E8: D1CB0009 04246107
	v_fma_f32 v9, v6, s49, v9                                  // 0000000016F0: D1CB0009 04246306
	v_fma_f32 v9, v5, s50, v9                                  // 0000000016F8: D1CB0009 04246505
	v_fma_f32 v9, v4, s51, v9                                  // 000000001700: D1CB0009 04246704
	s_load_dwordx4 s[48:51], s[18:19], 0x2a00                  // 000000001708: C00A0C09 00002A00
	s_load_dwordx4 s[88:91], s[18:19], 0x2a10                  // 000000001710: C00A1609 00002A10
	v_fma_f32 v9, v3, s84, v9                                  // 000000001718: D1CB0009 0424A903
	v_fma_f32 v9, v2, s85, v9                                  // 000000001720: D1CB0009 0424AB02
	v_fma_f32 v15, v1, s66, v15                                // 000000001728: D1CB000F 043C8501
	s_waitcnt lgkmcnt(0)                                       // 000000001730: BF8CC07F
	v_fma_f32 v7, v7, s48, v8                                  // 000000001734: D1CB0007 04206107
	v_fma_f32 v7, v6, s49, v7                                  // 00000000173C: D1CB0007 041C6306
	v_fma_f32 v6, v6, s25, v33                                 // 000000001744: D1CB0006 04843306
	v_fma_f32 v7, v5, s50, v7                                  // 00000000174C: D1CB0007 041C6505
	v_fma_f32 v5, v5, s26, v6                                  // 000000001754: D1CB0005 04183505
	v_fma_f32 v6, v4, s51, v7                                  // 00000000175C: D1CB0006 041C6704
	v_fma_f32 v4, v4, s27, v5                                  // 000000001764: D1CB0004 04143704
	v_fma_f32 v6, v3, s88, v6                                  // 00000000176C: D1CB0006 0418B103
	v_fma_f32 v5, v3, s28, v26                                 // 000000001774: D1CB0005 04683903
	v_fma_f32 v7, v3, s32, v25                                 // 00000000177C: D1CB0007 04644103
	v_fma_f32 v8, v3, s36, v22                                 // 000000001784: D1CB0008 04584903
	v_fma_f32 v3, v3, s20, v4                                  // 00000000178C: D1CB0003 04102903
	v_fma_f32 v6, v2, s89, v6                                  // 000000001794: D1CB0006 0418B302
	v_fma_f32 v4, v2, s29, v5                                  // 00000000179C: D1CB0004 04143B02
	v_fma_f32 v5, v2, s33, v7                                  // 0000000017A4: D1CB0005 041C4302
	v_fma_f32 v7, v2, s37, v8                                  // 0000000017AC: D1CB0007 04204B02
	v_fma_f32 v8, v2, s41, v21                                 // 0000000017B4: D1CB0008 04545302
	v_fma_f32 v2, v2, s21, v3                                  // 0000000017BC: D1CB0002 040C2B02
	v_fma_f32 v13, v1, s70, v13                                // 0000000017C4: D1CB000D 04348D01
	v_fma_f32 v12, v1, s74, v12                                // 0000000017CC: D1CB000C 04309501
	v_fma_f32 v11, v1, s78, v11                                // 0000000017D4: D1CB000B 042C9D01
	v_fma_f32 v10, v1, s82, v10                                // 0000000017DC: D1CB000A 0428A501
	v_addc_co_u32_e32 v39, vcc, 0, v37, vcc                    // 0000000017E4: 384E4A80
	v_fma_f32 v3, v1, s30, v4                                  // 0000000017E8: D1CB0003 04103D01
	v_fma_f32 v4, v1, s34, v5                                  // 0000000017F0: D1CB0004 04144501
	v_fma_f32 v5, v1, s38, v7                                  // 0000000017F8: D1CB0005 041C4D01
	v_fma_f32 v7, v1, s42, v8                                  // 000000001800: D1CB0007 04205501
	v_fma_f32 v8, v1, s46, v20                                 // 000000001808: D1CB0008 04505D01
	v_fma_f32 v9, v1, s86, v9                                  // 000000001810: D1CB0009 0424AD01
	v_fma_f32 v6, v1, s90, v6                                  // 000000001818: D1CB0006 0418B501
	v_fma_f32 v1, v1, s22, v2                                  // 000000001820: D1CB0001 04082D01
	global_load_dword v38, v[38:39], off                       // 000000001828: DC508000 267F0026
	s_waitcnt vmcnt(6)                                         // 000000001830: BF8C0F76
	v_fma_f32 v39, v0, s91, v6                                 // 000000001834: D1CB0027 0418B700
	v_fma_f32 v45, v0, s23, v1                                 // 00000000183C: D1CB002D 04042F00
	v_fma_f32 v20, v0, s31, v3                                 // 000000001844: D1CB0014 040C3F00
	v_fma_f32 v21, v0, s35, v4                                 // 00000000184C: D1CB0015 04104700
	v_fma_f32 v22, v0, s39, v5                                 // 000000001854: D1CB0016 04144F00
	v_fma_f32 v25, v0, s43, v7                                 // 00000000185C: D1CB0019 041C5700
	v_fma_f32 v26, v0, s47, v8                                 // 000000001864: D1CB001A 04205F00
	v_fma_f32 v19, v0, s55, v19                                // 00000000186C: D1CB0013 044C6F00
	v_fma_f32 v18, v0, s59, v18                                // 000000001874: D1CB0012 04487700
	v_fma_f32 v17, v0, s63, v17                                // 00000000187C: D1CB0011 04447F00
	v_fma_f32 v15, v0, s67, v15                                // 000000001884: D1CB000F 043C8700
	v_fma_f32 v13, v0, s71, v13                                // 00000000188C: D1CB000D 04348F00
	v_fma_f32 v12, v0, s75, v12                                // 000000001894: D1CB000C 04309700
	v_fma_f32 v11, v0, s79, v11                                // 00000000189C: D1CB000B 042C9F00
	v_fma_f32 v10, v0, s83, v10                                // 0000000018A4: D1CB000A 0428A700
	v_fma_f32 v33, v0, s87, v9                                 // 0000000018AC: D1CB0021 0424AF00
	v_add_co_u32_e32 v0, vcc, s10, v36                         // 0000000018B4: 3200480A
	v_addc_co_u32_e32 v1, vcc, 0, v37, vcc                     // 0000000018B8: 38024A80
	global_load_dword v7, v[0:1], off                          // 0000000018BC: DC508000 077F0000
	v_add_co_u32_e32 v0, vcc, s7, v36                          // 0000000018C4: 32004807
	v_addc_co_u32_e32 v1, vcc, 0, v37, vcc                     // 0000000018C8: 38024A80
	global_load_dword v6, v[0:1], off                          // 0000000018CC: DC508000 067F0000
	v_add_co_u32_e32 v0, vcc, s11, v36                         // 0000000018D4: 3200480B
	v_addc_co_u32_e32 v1, vcc, 0, v37, vcc                     // 0000000018D8: 38024A80
	global_load_dword v5, v[0:1], off                          // 0000000018DC: DC508000 057F0000
	v_add_co_u32_e32 v0, vcc, s12, v36                         // 0000000018E4: 3200480C
	v_addc_co_u32_e32 v1, vcc, 0, v37, vcc                     // 0000000018E8: 38024A80
	global_load_dword v4, v[0:1], off                          // 0000000018EC: DC508000 047F0000
	v_add_co_u32_e32 v0, vcc, s13, v36                         // 0000000018F4: 3200480D
	v_addc_co_u32_e32 v1, vcc, 0, v37, vcc                     // 0000000018F8: 38024A80
	global_load_dword v3, v[0:1], off                          // 0000000018FC: DC508000 037F0000
	v_add_co_u32_e32 v0, vcc, s14, v36                         // 000000001904: 3200480E
	v_addc_co_u32_e32 v1, vcc, 0, v37, vcc                     // 000000001908: 38024A80
	global_load_dword v2, v[0:1], off                          // 00000000190C: DC508000 027F0000
	v_add_co_u32_e32 v0, vcc, s15, v36                         // 000000001914: 3200480F
	v_addc_co_u32_e32 v1, vcc, 0, v37, vcc                     // 000000001918: 38024A80
	v_add_co_u32_e32 v8, vcc, s16, v36                         // 00000000191C: 32104810
	v_addc_co_u32_e32 v9, vcc, 0, v37, vcc                     // 000000001920: 38124A80
	global_load_dword v1, v[0:1], off                          // 000000001924: DC508000 017F0000
	global_load_dword v0, v[8:9], off                          // 00000000192C: DC508000 007F0008
	global_load_dword v8, v[36:37], off                        // 000000001934: DC508000 087F0024
	global_load_dword v9, v[36:37], off offset:3136            // 00000000193C: DC508C40 097F0024
	s_load_dwordx4 s[24:27], s[18:19], 0x20                    // 000000001944: C00A0609 00000020
	s_load_dwordx4 s[20:23], s[18:19], 0x30                    // 00000000194C: C00A0509 00000030
	s_load_dwordx4 s[80:83], s[18:19], 0x2a20                  // 000000001954: C00A1409 00002A20
	s_load_dwordx4 s[88:91], s[18:19], 0x2d20                  // 00000000195C: C00A1609 00002D20
	s_load_dwordx4 s[84:87], s[18:19], 0x2a30                  // 000000001964: C00A1509 00002A30
	s_load_dwordx4 s[92:95], s[18:19], 0x2d30                  // 00000000196C: C00A1709 00002D30
	v_cmp_ge_u32_e32 vcc, v32, v28                             // 000000001974: 7D9C3920
	s_and_b64 vcc, exec, vcc                                   // 000000001978: 86EA6A7E
	s_waitcnt vmcnt(1) lgkmcnt(0)                              // 00000000197C: BF8C0071
	v_fma_f32 v20, v8, s24, v20                                // 000000001980: D1CB0014 04503108
	s_waitcnt vmcnt(0)                                         // 000000001988: BF8C0F70
	v_fma_f32 v20, v9, s25, v20                                // 00000000198C: D1CB0014 04503309
	v_fma_f32 v20, v40, s26, v20                               // 000000001994: D1CB0014 04503528
	v_fma_f32 v20, v38, s27, v20                               // 00000000199C: D1CB0014 04503726
	s_load_dwordx4 s[28:31], s[18:19], 0x320                   // 0000000019A4: C00A0709 00000320
	s_load_dwordx4 s[24:27], s[18:19], 0x330                   // 0000000019AC: C00A0609 00000330
	v_fma_f32 v39, v8, s80, v39                                // 0000000019B4: D1CB0027 049CA108
	v_fma_f32 v39, v9, s81, v39                                // 0000000019BC: D1CB0027 049CA309
	v_fma_f32 v20, v44, s20, v20                               // 0000000019C4: D1CB0014 0450292C
	s_waitcnt lgkmcnt(0)                                       // 0000000019CC: BF8CC07F
	v_fma_f32 v21, v8, s28, v21                                // 0000000019D0: D1CB0015 04543908
	v_fma_f32 v21, v9, s29, v21                                // 0000000019D8: D1CB0015 04543B09
	v_fma_f32 v21, v40, s30, v21                               // 0000000019E0: D1CB0015 04543D28
	v_fma_f32 v21, v38, s31, v21                               // 0000000019E8: D1CB0015 04543F26
	s_load_dwordx4 s[32:35], s[18:19], 0x620                   // 0000000019F0: C00A0809 00000620
	s_load_dwordx4 s[28:31], s[18:19], 0x630                   // 0000000019F8: C00A0709 00000630
	v_fma_f32 v21, v44, s24, v21                               // 000000001A00: D1CB0015 0454312C
	v_fma_f32 v20, v43, s21, v20                               // 000000001A08: D1CB0014 04502B2B
	v_fma_f32 v21, v43, s25, v21                               // 000000001A10: D1CB0015 0454332B
	s_waitcnt lgkmcnt(0)                                       // 000000001A18: BF8CC07F
	v_fma_f32 v22, v8, s32, v22                                // 000000001A1C: D1CB0016 04584108
	v_fma_f32 v22, v9, s33, v22                                // 000000001A24: D1CB0016 04584309
	v_fma_f32 v22, v40, s34, v22                               // 000000001A2C: D1CB0016 04584528
	v_fma_f32 v22, v38, s35, v22                               // 000000001A34: D1CB0016 04584726
	s_load_dwordx4 s[36:39], s[18:19], 0x920                   // 000000001A3C: C00A0909 00000920
	s_load_dwordx4 s[32:35], s[18:19], 0x930                   // 000000001A44: C00A0809 00000930
	v_fma_f32 v22, v44, s28, v22                               // 000000001A4C: D1CB0016 0458392C
	v_fma_f32 v22, v43, s29, v22                               // 000000001A54: D1CB0016 04583B2B
	v_fma_f32 v22, v42, s30, v22                               // 000000001A5C: D1CB0016 04583D2A
	s_waitcnt lgkmcnt(0)                                       // 000000001A64: BF8CC07F
	v_fma_f32 v25, v8, s36, v25                                // 000000001A68: D1CB0019 04644908
	v_fma_f32 v25, v9, s37, v25                                // 000000001A70: D1CB0019 04644B09
	v_fma_f32 v25, v40, s38, v25                               // 000000001A78: D1CB0019 04644D28
	v_fma_f32 v25, v38, s39, v25                               // 000000001A80: D1CB0019 04644F26
	s_load_dwordx4 s[40:43], s[18:19], 0xc20                   // 000000001A88: C00A0A09 00000C20
	s_load_dwordx4 s[36:39], s[18:19], 0xc30                   // 000000001A90: C00A0909 00000C30
	v_fma_f32 v25, v44, s32, v25                               // 000000001A98: D1CB0019 0464412C
	v_fma_f32 v25, v43, s33, v25                               // 000000001AA0: D1CB0019 0464432B
	v_fma_f32 v20, v42, s22, v20                               // 000000001AA8: D1CB0014 04502D2A
	s_waitcnt lgkmcnt(0)                                       // 000000001AB0: BF8CC07F
	v_fma_f32 v26, v8, s40, v26                                // 000000001AB4: D1CB001A 04685108
	v_fma_f32 v26, v9, s41, v26                                // 000000001ABC: D1CB001A 04685309
	v_fma_f32 v26, v40, s42, v26                               // 000000001AC4: D1CB001A 04685528
	v_fma_f32 v26, v38, s43, v26                               // 000000001ACC: D1CB001A 04685726
	s_load_dwordx4 s[44:47], s[18:19], 0xf20                   // 000000001AD4: C00A0B09 00000F20
	s_load_dwordx4 s[40:43], s[18:19], 0xf30                   // 000000001ADC: C00A0A09 00000F30
	v_fma_f32 v26, v44, s36, v26                               // 000000001AE4: D1CB001A 0468492C
	v_fma_f32 v26, v43, s37, v26                               // 000000001AEC: D1CB001A 04684B2B
	v_fma_f32 v21, v42, s26, v21                               // 000000001AF4: D1CB0015 0454352A
	s_waitcnt lgkmcnt(0)                                       // 000000001AFC: BF8CC07F
	v_fma_f32 v19, v8, s44, v19                                // 000000001B00: D1CB0013 044C5908
	v_fma_f32 v19, v9, s45, v19                                // 000000001B08: D1CB0013 044C5B09
	v_fma_f32 v19, v40, s46, v19                               // 000000001B10: D1CB0013 044C5D28
	v_fma_f32 v19, v38, s47, v19                               // 000000001B18: D1CB0013 044C5F26
	s_load_dwordx4 s[48:51], s[18:19], 0x1220                  // 000000001B20: C00A0C09 00001220
	s_load_dwordx4 s[44:47], s[18:19], 0x1230                  // 000000001B28: C00A0B09 00001230
	v_fma_f32 v19, v44, s40, v19                               // 000000001B30: D1CB0013 044C512C
	v_fma_f32 v19, v43, s41, v19                               // 000000001B38: D1CB0013 044C532B
	v_fma_f32 v19, v42, s42, v19                               // 000000001B40: D1CB0013 044C552A
	s_waitcnt lgkmcnt(0)                                       // 000000001B48: BF8CC07F
	v_fma_f32 v18, v8, s48, v18                                // 000000001B4C: D1CB0012 04486108
	v_fma_f32 v18, v9, s49, v18                                // 000000001B54: D1CB0012 04486309
	v_fma_f32 v18, v40, s50, v18                               // 000000001B5C: D1CB0012 04486528
	v_fma_f32 v18, v38, s51, v18                               // 000000001B64: D1CB0012 04486726
	s_load_dwordx4 s[52:55], s[18:19], 0x1520                  // 000000001B6C: C00A0D09 00001520
	s_load_dwordx4 s[48:51], s[18:19], 0x1530                  // 000000001B74: C00A0C09 00001530
	v_fma_f32 v18, v44, s44, v18                               // 000000001B7C: D1CB0012 0448592C
	v_fma_f32 v18, v43, s45, v18                               // 000000001B84: D1CB0012 04485B2B
	v_fma_f32 v18, v42, s46, v18                               // 000000001B8C: D1CB0012 04485D2A
	s_waitcnt lgkmcnt(0)                                       // 000000001B94: BF8CC07F
	v_fma_f32 v17, v8, s52, v17                                // 000000001B98: D1CB0011 04446908
	v_fma_f32 v17, v9, s53, v17                                // 000000001BA0: D1CB0011 04446B09
	v_fma_f32 v17, v40, s54, v17                               // 000000001BA8: D1CB0011 04446D28
	v_fma_f32 v17, v38, s55, v17                               // 000000001BB0: D1CB0011 04446F26
	s_load_dwordx4 s[56:59], s[18:19], 0x1820                  // 000000001BB8: C00A0E09 00001820
	s_load_dwordx4 s[52:55], s[18:19], 0x1830                  // 000000001BC0: C00A0D09 00001830
	v_fma_f32 v17, v44, s48, v17                               // 000000001BC8: D1CB0011 0444612C
	v_fma_f32 v17, v43, s49, v17                               // 000000001BD0: D1CB0011 0444632B
	v_fma_f32 v17, v42, s50, v17                               // 000000001BD8: D1CB0011 0444652A
	s_waitcnt lgkmcnt(0)                                       // 000000001BE0: BF8CC07F
	v_fma_f32 v15, v8, s56, v15                                // 000000001BE4: D1CB000F 043C7108
	v_fma_f32 v15, v9, s57, v15                                // 000000001BEC: D1CB000F 043C7309
	v_fma_f32 v15, v40, s58, v15                               // 000000001BF4: D1CB000F 043C7528
	v_fma_f32 v15, v38, s59, v15                               // 000000001BFC: D1CB000F 043C7726
	s_load_dwordx4 s[60:63], s[18:19], 0x1b20                  // 000000001C04: C00A0F09 00001B20
	s_load_dwordx4 s[56:59], s[18:19], 0x1b30                  // 000000001C0C: C00A0E09 00001B30
	v_fma_f32 v15, v44, s52, v15                               // 000000001C14: D1CB000F 043C692C
	v_fma_f32 v15, v43, s53, v15                               // 000000001C1C: D1CB000F 043C6B2B
	v_fma_f32 v15, v42, s54, v15                               // 000000001C24: D1CB000F 043C6D2A
	s_waitcnt lgkmcnt(0)                                       // 000000001C2C: BF8CC07F
	v_fma_f32 v13, v8, s60, v13                                // 000000001C30: D1CB000D 04347908
	v_fma_f32 v13, v9, s61, v13                                // 000000001C38: D1CB000D 04347B09
	v_fma_f32 v13, v40, s62, v13                               // 000000001C40: D1CB000D 04347D28
	v_fma_f32 v13, v38, s63, v13                               // 000000001C48: D1CB000D 04347F26
	s_load_dwordx4 s[64:67], s[18:19], 0x1e20                  // 000000001C50: C00A1009 00001E20
	s_load_dwordx4 s[60:63], s[18:19], 0x1e30                  // 000000001C58: C00A0F09 00001E30
	v_fma_f32 v13, v44, s56, v13                               // 000000001C60: D1CB000D 0434712C
	v_fma_f32 v13, v43, s57, v13                               // 000000001C68: D1CB000D 0434732B
	v_fma_f32 v13, v42, s58, v13                               // 000000001C70: D1CB000D 0434752A
	s_waitcnt lgkmcnt(0)                                       // 000000001C78: BF8CC07F
	v_fma_f32 v12, v8, s64, v12                                // 000000001C7C: D1CB000C 04308108
	v_fma_f32 v12, v9, s65, v12                                // 000000001C84: D1CB000C 04308309
	v_fma_f32 v12, v40, s66, v12                               // 000000001C8C: D1CB000C 04308528
	v_fma_f32 v12, v38, s67, v12                               // 000000001C94: D1CB000C 04308726
	s_load_dwordx4 s[64:67], s[18:19], 0x2120                  // 000000001C9C: C00A1009 00002120
	s_load_dwordx4 s[68:71], s[18:19], 0x2130                  // 000000001CA4: C00A1109 00002130
	v_fma_f32 v12, v44, s60, v12                               // 000000001CAC: D1CB000C 0430792C
	v_fma_f32 v12, v43, s61, v12                               // 000000001CB4: D1CB000C 04307B2B
	v_fma_f32 v12, v42, s62, v12                               // 000000001CBC: D1CB000C 04307D2A
	s_waitcnt lgkmcnt(0)                                       // 000000001CC4: BF8CC07F
	v_fma_f32 v11, v8, s64, v11                                // 000000001CC8: D1CB000B 042C8108
	v_fma_f32 v11, v9, s65, v11                                // 000000001CD0: D1CB000B 042C8309
	v_fma_f32 v11, v40, s66, v11                               // 000000001CD8: D1CB000B 042C8528
	v_fma_f32 v11, v38, s67, v11                               // 000000001CE0: D1CB000B 042C8726
	s_load_dwordx4 s[64:67], s[18:19], 0x2420                  // 000000001CE8: C00A1009 00002420
	s_load_dwordx4 s[72:75], s[18:19], 0x2430                  // 000000001CF0: C00A1209 00002430
	v_fma_f32 v11, v44, s68, v11                               // 000000001CF8: D1CB000B 042C892C
	v_fma_f32 v11, v43, s69, v11                               // 000000001D00: D1CB000B 042C8B2B
	v_fma_f32 v11, v42, s70, v11                               // 000000001D08: D1CB000B 042C8D2A
	s_waitcnt lgkmcnt(0)                                       // 000000001D10: BF8CC07F
	v_fma_f32 v10, v8, s64, v10                                // 000000001D14: D1CB000A 04288108
	v_fma_f32 v10, v9, s65, v10                                // 000000001D1C: D1CB000A 04288309
	v_fma_f32 v10, v40, s66, v10                               // 000000001D24: D1CB000A 04288528
	v_fma_f32 v10, v38, s67, v10                               // 000000001D2C: D1CB000A 04288726
	s_load_dwordx4 s[64:67], s[18:19], 0x2720                  // 000000001D34: C00A1009 00002720
	s_load_dwordx4 s[76:79], s[18:19], 0x2730                  // 000000001D3C: C00A1309 00002730
	v_fma_f32 v10, v44, s72, v10                               // 000000001D44: D1CB000A 0428912C
	v_fma_f32 v10, v43, s73, v10                               // 000000001D4C: D1CB000A 0428932B
	v_fma_f32 v10, v42, s74, v10                               // 000000001D54: D1CB000A 0428952A
	s_waitcnt lgkmcnt(0)                                       // 000000001D5C: BF8CC07F
	v_fma_f32 v33, v8, s64, v33                                // 000000001D60: D1CB0021 04848108
	v_fma_f32 v8, v8, s88, v45                                 // 000000001D68: D1CB0008 04B4B108
	v_fma_f32 v8, v9, s89, v8                                  // 000000001D70: D1CB0008 0420B309
	v_fma_f32 v33, v9, s65, v33                                // 000000001D78: D1CB0021 04848309
	v_fma_f32 v9, v40, s66, v33                                // 000000001D80: D1CB0009 04848528
	v_fma_f32 v33, v40, s82, v39                               // 000000001D88: D1CB0021 049CA528
	v_fma_f32 v8, v40, s90, v8                                 // 000000001D90: D1CB0008 0420B528
	v_fma_f32 v9, v38, s67, v9                                 // 000000001D98: D1CB0009 04248726
	v_fma_f32 v33, v38, s83, v33                               // 000000001DA0: D1CB0021 0484A726
	v_fma_f32 v8, v38, s91, v8                                 // 000000001DA8: D1CB0008 0420B726
	v_fma_f32 v9, v44, s76, v9                                 // 000000001DB0: D1CB0009 0424992C
	v_fma_f32 v33, v44, s84, v33                               // 000000001DB8: D1CB0021 0484A92C
	v_fma_f32 v8, v44, s92, v8                                 // 000000001DC0: D1CB0008 0420B92C
	v_fma_f32 v9, v43, s77, v9                                 // 000000001DC8: D1CB0009 04249B2B
	v_fma_f32 v33, v43, s85, v33                               // 000000001DD0: D1CB0021 0484AB2B
	v_fma_f32 v8, v43, s93, v8                                 // 000000001DD8: D1CB0008 0420BB2B
	v_fma_f32 v9, v42, s78, v9                                 // 000000001DE0: D1CB0009 04249D2A
	v_fma_f32 v38, v42, s34, v25                               // 000000001DE8: D1CB0026 0464452A
	v_fma_f32 v39, v42, s38, v26                               // 000000001DF0: D1CB0027 04684D2A
	v_fma_f32 v33, v42, s86, v33                               // 000000001DF8: D1CB0021 0484AD2A
	v_fma_f32 v40, v42, s94, v8                                // 000000001E00: D1CB0028 0420BD2A
	v_fma_f32 v26, v41, s23, v20                               // 000000001E08: D1CB001A 04502F29
	v_fma_f32 v25, v41, s27, v21                               // 000000001E10: D1CB0019 04543729
	v_fma_f32 v8, v41, s87, v33                                // 000000001E18: D1CB0008 0484AF29
	v_fma_f32 v22, v41, s31, v22                               // 000000001E20: D1CB0016 04583F29
	v_fma_f32 v21, v41, s35, v38                               // 000000001E28: D1CB0015 04984729
	v_fma_f32 v20, v41, s39, v39                               // 000000001E30: D1CB0014 049C4F29
	v_fma_f32 v19, v41, s43, v19                               // 000000001E38: D1CB0013 044C5729
	v_fma_f32 v18, v41, s47, v18                               // 000000001E40: D1CB0012 04485F29
	v_fma_f32 v17, v41, s51, v17                               // 000000001E48: D1CB0011 04446729
	v_fma_f32 v15, v41, s55, v15                               // 000000001E50: D1CB000F 043C6F29
	v_fma_f32 v13, v41, s59, v13                               // 000000001E58: D1CB000D 04347729
	v_fma_f32 v12, v41, s63, v12                               // 000000001E60: D1CB000C 04307F29
	v_fma_f32 v11, v41, s71, v11                               // 000000001E68: D1CB000B 042C8F29
	v_fma_f32 v10, v41, s75, v10                               // 000000001E70: D1CB000A 04289729
	v_fma_f32 v9, v41, s79, v9                                 // 000000001E78: D1CB0009 04249F29
	v_fma_f32 v33, v41, s95, v40                               // 000000001E80: D1CB0021 04A0BF29
	s_cbranch_vccz BB0_2                                       // 000000001E88: BF86FD1A
	s_mov_b32 s3, 0xc400                                       // 000000001E8C: BE8300FF 0000C400
	v_mul_lo_u32 v16, v16, s3                                  // 000000001E94: D2850010 00000710
	v_mov_b32_e32 v32, 0                                       // 000000001E9C: 7E400280
	v_lshlrev_b64 v[34:35], 2, v[31:32]                        // 000000001EA0: D28F0022 00023E82
	v_add_co_u32_e32 v34, vcc, v29, v34                        // 000000001EA8: 3244451D
	v_mov_b32_e32 v29, 0x310                                   // 000000001EAC: 7E3A02FF 00000310
	v_mad_i32_i24 v16, s2, v29, v16                            // 000000001EB4: D1C20010 04423A02
	v_addc_co_u32_e32 v35, vcc, v30, v35, vcc                  // 000000001EBC: 3846471E
	v_add_co_u32_e32 v31, vcc, v16, v14                        // 000000001EC0: 323E1D10
	v_lshlrev_b64 v[29:30], 2, v[31:32]                        // 000000001EC4: D28F001D 00023E82
	v_ashrrev_i32_e32 v28, 31, v27                             // 000000001ECC: 2238369F
	v_mov_b32_e32 v14, s1                                      // 000000001ED0: 7E1C0201
	v_add_co_u32_e32 v29, vcc, s0, v29                         // 000000001ED4: 323A3A00
	v_addc_co_u32_e32 v30, vcc, v14, v30, vcc                  // 000000001ED8: 383C3D0E
	v_lshlrev_b64 v[27:28], 2, v[27:28]                        // 000000001EDC: D28F001B 00023682
	v_add_co_u32_e32 v14, vcc, v23, v27                        // 000000001EE4: 321C3717
	v_addc_co_u32_e32 v16, vcc, v24, v28, vcc                  // 000000001EE8: 38203918
	s_movk_i32 s0, 0x24c0                                      // 000000001EEC: B00024C0
	v_add_co_u32_e32 v23, vcc, s0, v34                         // 000000001EF0: 322E4400
	v_addc_co_u32_e32 v24, vcc, 0, v35, vcc                    // 000000001EF4: 38304680
	s_movk_i32 s5, 0x1880                                      // 000000001EF8: B0051880
	v_add_co_u32_e32 v27, vcc, s5, v34                         // 000000001EFC: 32364405
	v_addc_co_u32_e32 v28, vcc, 0, v35, vcc                    // 000000001F00: 38384680
	s_movk_i32 s4, 0x3100                                      // 000000001F04: B0043100
	v_readfirstlane_b32 s6, v14                                // 000000001F08: 7E0C050E
	v_readfirstlane_b32 s7, v16                                // 000000001F0C: 7E0E0510
	global_load_dword v14, v[23:24], off                       // 000000001F10: DC508000 0E7F0017
	global_load_dword v16, v[27:28], off                       // 000000001F18: DC508000 107F001B
	v_add_co_u32_e32 v23, vcc, s4, v34                         // 000000001F20: 322E4404
	v_addc_co_u32_e32 v24, vcc, 0, v35, vcc                    // 000000001F24: 38304680
	s_movk_i32 s3, 0x3d40                                      // 000000001F28: B0033D40
	global_load_dword v27, v[23:24], off                       // 000000001F2C: DC508000 1B7F0017
	v_add_co_u32_e32 v23, vcc, s3, v34                         // 000000001F34: 322E4403
	v_addc_co_u32_e32 v24, vcc, 0, v35, vcc                    // 000000001F38: 38304680
	s_load_dword s16, s[6:7], 0x2a74                           // 000000001F3C: C0020403 00002A74
	s_load_dword s17, s[6:7], 0x2a78                           // 000000001F44: C0020443 00002A78
	s_load_dword s18, s[6:7], 0x2a7c                           // 000000001F4C: C0020483 00002A7C
	s_load_dword s8, s[6:7], 0x2d40                            // 000000001F54: C0020203 00002D40
	s_movk_i32 s2, 0x4980                                      // 000000001F5C: B0024980
	global_load_dword v28, v[23:24], off                       // 000000001F60: DC508000 1C7F0017
	v_add_co_u32_e32 v23, vcc, s2, v34                         // 000000001F68: 322E4402
	v_addc_co_u32_e32 v24, vcc, 0, v35, vcc                    // 000000001F6C: 38304680
	s_movk_i32 s1, 0x55c0                                      // 000000001F70: B00155C0
	global_load_dword v31, v[23:24], off                       // 000000001F74: DC508000 1F7F0017
	v_add_co_u32_e32 v23, vcc, s1, v34                         // 000000001F7C: 322E4401
	v_addc_co_u32_e32 v24, vcc, 0, v35, vcc                    // 000000001F80: 38304680
	global_load_dword v32, v[34:35], off                       // 000000001F84: DC508000 207F0022
	global_load_dword v34, v[34:35], off offset:3136           // 000000001F8C: DC508C40 227F0022
	global_load_dword v23, v[23:24], off                       // 000000001F94: DC508000 177F0017
	s_load_dword s19, s[6:7], 0x2d44                           // 000000001F9C: C00204C3 00002D44
	s_waitcnt lgkmcnt(0)                                       // 000000001FA4: BF8CC07F
	v_fma_f32 v24, v7, s8, v33                                 // 000000001FA8: D1CB0018 04841107
	s_load_dwordx4 s[8:11], s[6:7], 0x40                       // 000000001FB0: C00A0203 00000040
	s_load_dword s20, s[6:7], 0x50                             // 000000001FB8: C0020503 00000050
	s_load_dword s21, s[6:7], 0x54                             // 000000001FC0: C0020543 00000054
	s_load_dword s22, s[6:7], 0x58                             // 000000001FC8: C0020583 00000058
	s_load_dword s23, s[6:7], 0x5c                             // 000000001FD0: C00205C3 0000005C
	s_waitcnt lgkmcnt(0)                                       // 000000001FD8: BF8CC07F
	v_fma_f32 v26, v7, s8, v26                                 // 000000001FDC: D1CB001A 04681107
	s_load_dwordx2 s[12:13], s[6:7], 0x340                     // 000000001FE4: C0060303 00000340
	s_load_dword s8, s[6:7], 0x348                             // 000000001FEC: C0020203 00000348
	s_load_dword s24, s[6:7], 0x34c                            // 000000001FF4: C0020603 0000034C
	s_load_dword s25, s[6:7], 0x350                            // 000000001FFC: C0020643 00000350
	s_load_dword s26, s[6:7], 0x354                            // 000000002004: C0020683 00000354
	s_waitcnt lgkmcnt(0)                                       // 00000000200C: BF8CC07F
	v_fma_f32 v25, v7, s12, v25                                // 000000002010: D1CB0019 04641907
	s_load_dword s27, s[6:7], 0x37c                            // 000000002018: C00206C3 0000037C
	s_load_dwordx2 s[14:15], s[6:7], 0x640                     // 000000002020: C0060383 00000640
	s_load_dword s12, s[6:7], 0x648                            // 000000002028: C0020303 00000648
	s_load_dword s28, s[6:7], 0x64c                            // 000000002030: C0020703 0000064C
	s_load_dword s29, s[6:7], 0x650                            // 000000002038: C0020743 00000650
	s_load_dword s34, s[6:7], 0x978                            // 000000002040: C0020883 00000978
	s_waitcnt lgkmcnt(0)                                       // 000000002048: BF8CC07F
	v_fma_f32 v22, v7, s14, v22                                // 00000000204C: D1CB0016 04581D07
	s_load_dword s30, s[6:7], 0x67c                            // 000000002054: C0020783 0000067C
	s_load_dword s14, s[6:7], 0x940                            // 00000000205C: C0020383 00000940
	s_load_dword s31, s[6:7], 0x944                            // 000000002064: C00207C3 00000944
	s_load_dword s32, s[6:7], 0x948                            // 00000000206C: C0020803 00000948
	s_load_dword s33, s[6:7], 0x94c                            // 000000002074: C0020843 0000094C
	s_load_dword s38, s[6:7], 0xc74                            // 00000000207C: C0020983 00000C74
	s_waitcnt lgkmcnt(0)                                       // 000000002084: BF8CC07F
	v_fma_f32 v21, v7, s14, v21                                // 000000002088: D1CB0015 04541D07
	s_load_dword s35, s[6:7], 0x97c                            // 000000002090: C00208C3 0000097C
	s_load_dword s14, s[6:7], 0xc40                            // 000000002098: C0020383 00000C40
	s_load_dword s36, s[6:7], 0xc44                            // 0000000020A0: C0020903 00000C44
	s_load_dword s37, s[6:7], 0xc48                            // 0000000020A8: C0020943 00000C48
	s_load_dword s39, s[6:7], 0xc78                            // 0000000020B0: C00209C3 00000C78
	s_load_dword s42, s[6:7], 0xf70                            // 0000000020B8: C0020A83 00000F70
	s_waitcnt lgkmcnt(0)                                       // 0000000020C0: BF8CC07F
	v_fma_f32 v20, v7, s14, v20                                // 0000000020C4: D1CB0014 04501D07
	s_load_dword s40, s[6:7], 0xc7c                            // 0000000020CC: C0020A03 00000C7C
	s_load_dword s14, s[6:7], 0xf40                            // 0000000020D4: C0020383 00000F40
	s_load_dword s41, s[6:7], 0xf44                            // 0000000020DC: C0020A43 00000F44
	s_load_dword s43, s[6:7], 0xf74                            // 0000000020E4: C0020AC3 00000F74
	s_load_dword s44, s[6:7], 0xf78                            // 0000000020EC: C0020B03 00000F78
	s_load_dword s54, s[6:7], 0x1878                           // 0000000020F4: C0020D83 00001878
	s_waitcnt lgkmcnt(0)                                       // 0000000020FC: BF8CC07F
	v_fma_f32 v19, v7, s14, v19                                // 000000002100: D1CB0013 044C1D07
	s_load_dword s45, s[6:7], 0xf7c                            // 000000002108: C0020B43 00000F7C
	s_load_dword s14, s[6:7], 0x1240                           // 000000002110: C0020383 00001240
	s_load_dword s58, s[6:7], 0x1b74                           // 000000002118: C0020E83 00001B74
	s_load_dword s59, s[6:7], 0x1b78                           // 000000002120: C0020EC3 00001B78
	s_load_dword s62, s[6:7], 0x1e70                           // 000000002128: C0020F83 00001E70
	s_load_dword s63, s[6:7], 0x1e74                           // 000000002130: C0020FC3 00001E74
	s_waitcnt lgkmcnt(0)                                       // 000000002138: BF8CC07F
	v_fma_f32 v18, v7, s14, v18                                // 00000000213C: D1CB0012 04481D07
	s_load_dword s14, s[6:7], 0x1540                           // 000000002144: C0020383 00001540
	s_load_dword s46, s[6:7], 0x1544                           // 00000000214C: C0020B83 00001544
	s_load_dword s47, s[6:7], 0x1548                           // 000000002154: C0020BC3 00001548
	s_load_dword s48, s[6:7], 0x154c                           // 00000000215C: C0020C03 0000154C
	s_load_dword s49, s[6:7], 0x1550                           // 000000002164: C0020C43 00001550
	s_waitcnt lgkmcnt(0)                                       // 00000000216C: BF8CC07F
	v_fma_f32 v17, v7, s14, v17                                // 000000002170: D1CB0011 04441D07
	s_load_dword s50, s[6:7], 0x157c                           // 000000002178: C0020C83 0000157C
	s_load_dword s14, s[6:7], 0x1840                           // 000000002180: C0020383 00001840
	s_load_dword s51, s[6:7], 0x1844                           // 000000002188: C0020CC3 00001844
	s_load_dword s52, s[6:7], 0x1848                           // 000000002190: C0020D03 00001848
	s_load_dword s53, s[6:7], 0x184c                           // 000000002198: C0020D43 0000184C
	s_load_dword s64, s[6:7], 0x1e78                           // 0000000021A0: C0021003 00001E78
	s_waitcnt lgkmcnt(0)                                       // 0000000021A8: BF8CC07F
	v_fma_f32 v15, v7, s14, v15                                // 0000000021AC: D1CB000F 043C1D07
	s_load_dword s55, s[6:7], 0x187c                           // 0000000021B4: C0020DC3 0000187C
	s_load_dword s14, s[6:7], 0x1b40                           // 0000000021BC: C0020383 00001B40
	s_load_dword s56, s[6:7], 0x1b44                           // 0000000021C4: C0020E03 00001B44
	s_load_dword s57, s[6:7], 0x1b48                           // 0000000021CC: C0020E43 00001B48
	s_load_dword s74, s[6:7], 0x2778                           // 0000000021D4: C0021283 00002778
	v_fma_f32 v25, v6, s13, v25                                // 0000000021DC: D1CB0019 04641B06
	s_waitcnt lgkmcnt(0)                                       // 0000000021E4: BF8CC07F
	v_fma_f32 v13, v7, s14, v13                                // 0000000021E8: D1CB000D 04341D07
	s_load_dword s60, s[6:7], 0x1b7c                           // 0000000021F0: C0020F03 00001B7C
	s_load_dword s14, s[6:7], 0x1e40                           // 0000000021F8: C0020383 00001E40
	s_load_dword s61, s[6:7], 0x1e44                           // 000000002200: C0020F43 00001E44
	v_fma_f32 v22, v6, s15, v22                                // 000000002208: D1CB0016 04581F06
	v_fma_f32 v21, v6, s31, v21                                // 000000002210: D1CB0015 04543F06
	v_fma_f32 v20, v6, s36, v20                                // 000000002218: D1CB0014 04504906
	s_waitcnt lgkmcnt(0)                                       // 000000002220: BF8CC07F
	v_fma_f32 v12, v7, s14, v12                                // 000000002224: D1CB000C 04301D07
	s_load_dword s65, s[6:7], 0x1e7c                           // 00000000222C: C0021043 00001E7C
	s_load_dword s14, s[6:7], 0x2140                           // 000000002234: C0020383 00002140
	v_fma_f32 v17, v6, s46, v17                                // 00000000223C: D1CB0011 04445D06
	v_fma_f32 v15, v6, s51, v15                                // 000000002244: D1CB000F 043C6706
	v_fma_f32 v13, v6, s56, v13                                // 00000000224C: D1CB000D 04347106
	v_fma_f32 v19, v6, s41, v19                                // 000000002254: D1CB0013 044C5306
	s_waitcnt lgkmcnt(0)                                       // 00000000225C: BF8CC07F
	v_fma_f32 v11, v7, s14, v11                                // 000000002260: D1CB000B 042C1D07
	s_load_dword s14, s[6:7], 0x2440                           // 000000002268: C0020383 00002440
	s_load_dword s66, s[6:7], 0x2444                           // 000000002270: C0021083 00002444
	s_load_dword s67, s[6:7], 0x2448                           // 000000002278: C00210C3 00002448
	s_load_dword s68, s[6:7], 0x244c                           // 000000002280: C0021103 0000244C
	s_load_dword s69, s[6:7], 0x2450                           // 000000002288: C0021143 00002450
	s_waitcnt lgkmcnt(0)                                       // 000000002290: BF8CC07F
	v_fma_f32 v10, v7, s14, v10                                // 000000002294: D1CB000A 04281D07
	s_load_dword s70, s[6:7], 0x247c                           // 00000000229C: C0021183 0000247C
	s_load_dword s14, s[6:7], 0x2740                           // 0000000022A4: C0020383 00002740
	s_load_dword s71, s[6:7], 0x2744                           // 0000000022AC: C00211C3 00002744
	s_load_dword s72, s[6:7], 0x2748                           // 0000000022B4: C0021203 00002748
	s_load_dword s73, s[6:7], 0x274c                           // 0000000022BC: C0021243 0000274C
	v_fma_f32 v10, v6, s66, v10                                // 0000000022C4: D1CB000A 04288506
	s_waitcnt lgkmcnt(0)                                       // 0000000022CC: BF8CC07F
	v_fma_f32 v9, v7, s14, v9                                  // 0000000022D0: D1CB0009 04241D07
	s_load_dword s75, s[6:7], 0x277c                           // 0000000022D8: C00212C3 0000277C
	s_load_dword s14, s[6:7], 0x2a40                           // 0000000022E0: C0020383 00002A40
	s_load_dword s76, s[6:7], 0x2a44                           // 0000000022E8: C0021303 00002A44
	s_load_dword s77, s[6:7], 0x2a48                           // 0000000022F0: C0021343 00002A48
	v_fma_f32 v9, v6, s71, v9                                  // 0000000022F8: D1CB0009 04248F06
	v_fma_f32 v12, v6, s61, v12                                // 000000002300: D1CB000C 04307B06
	s_waitcnt lgkmcnt(0)                                       // 000000002308: BF8CC07F
	v_fma_f32 v7, v7, s14, v8                                  // 00000000230C: D1CB0007 04201D07
	v_fma_f32 v8, v6, s9, v26                                  // 000000002314: D1CB0008 04681306
	s_load_dword s9, s[6:7], 0x1244                            // 00000000231C: C0020243 00001244
	s_load_dword s13, s[6:7], 0x1248                           // 000000002324: C0020343 00001248
	s_load_dword s14, s[6:7], 0x124c                           // 00000000232C: C0020383 0000124C
	s_load_dword s15, s[6:7], 0x1250                           // 000000002334: C00203C3 00001250
	s_load_dword s31, s[6:7], 0x1254                           // 00000000233C: C00207C3 00001254
	s_waitcnt lgkmcnt(0)                                       // 000000002344: BF8CC07F
	v_fma_f32 v18, v6, s9, v18                                 // 000000002348: D1CB0012 04481306
	s_load_dword s9, s[6:7], 0x2144                            // 000000002350: C0020243 00002144
	v_fma_f32 v7, v6, s76, v7                                  // 000000002358: D1CB0007 041C9906
	s_load_dword s36, s[6:7], 0x2148                           // 000000002360: C0020903 00002148
	s_load_dword s41, s[6:7], 0x214c                           // 000000002368: C0020A43 0000214C
	s_load_dword s46, s[6:7], 0x2150                           // 000000002370: C0020B83 00002150
	s_load_dword s51, s[6:7], 0x2154                           // 000000002378: C0020CC3 00002154
	s_waitcnt lgkmcnt(0)                                       // 000000002380: BF8CC07F
	v_fma_f32 v11, v6, s9, v11                                 // 000000002384: D1CB000B 042C1306
	v_fma_f32 v6, v6, s19, v24                                 // 00000000238C: D1CB0006 04602706
	v_fma_f32 v24, v5, s8, v25                                 // 000000002394: D1CB0018 04641105
	s_load_dword s8, s[6:7], 0xf48                             // 00000000239C: C0020203 00000F48
	v_fma_f32 v22, v5, s12, v22                                // 0000000023A4: D1CB0016 04581905
	v_fma_f32 v21, v5, s32, v21                                // 0000000023AC: D1CB0015 04544105
	v_fma_f32 v8, v5, s10, v8                                  // 0000000023B4: D1CB0008 04201505
	s_load_dword s9, s[6:7], 0xf4c                             // 0000000023BC: C0020243 00000F4C
	s_load_dword s10, s[6:7], 0xf50                            // 0000000023C4: C0020283 00000F50
	s_load_dword s19, s[6:7], 0xf54                            // 0000000023CC: C00204C3 00000F54
	s_load_dword s32, s[6:7], 0xf58                            // 0000000023D4: C0020803 00000F58
	s_waitcnt lgkmcnt(0)                                       // 0000000023DC: BF8CC07F
	v_fma_f32 v19, v5, s8, v19                                 // 0000000023E0: D1CB0013 044C1105
	s_load_dword s8, s[6:7], 0x1e48                            // 0000000023E8: C0020203 00001E48
	v_fma_f32 v18, v5, s13, v18                                // 0000000023F0: D1CB0012 04481B05
	v_fma_f32 v17, v5, s47, v17                                // 0000000023F8: D1CB0011 04445F05
	v_fma_f32 v11, v5, s36, v11                                // 000000002400: D1CB000B 042C4905
	v_fma_f32 v20, v5, s37, v20                                // 000000002408: D1CB0014 04504B05
	s_load_dword s12, s[6:7], 0x1e4c                           // 000000002410: C0020303 00001E4C
	s_load_dword s13, s[6:7], 0x1e50                           // 000000002418: C0020343 00001E50
	s_load_dword s37, s[6:7], 0x1e54                           // 000000002420: C0020943 00001E54
	s_load_dword s47, s[6:7], 0x1e58                           // 000000002428: C0020BC3 00001E58
	s_waitcnt lgkmcnt(0)                                       // 000000002430: BF8CC07F
	v_fma_f32 v12, v5, s8, v12                                 // 000000002434: D1CB000C 04301105
	s_load_dword s8, s[6:7], 0x2d48                            // 00000000243C: C0020203 00002D48
	v_fma_f32 v15, v5, s52, v15                                // 000000002444: D1CB000F 043C6905
	v_fma_f32 v10, v5, s67, v10                                // 00000000244C: D1CB000A 04288705
	v_fma_f32 v9, v5, s72, v9                                  // 000000002454: D1CB0009 04249105
	s_load_dword s36, s[6:7], 0x2d4c                           // 00000000245C: C0020903 00002D4C
	v_fma_f32 v13, v5, s57, v13                                // 000000002464: D1CB000D 04347305
	s_load_dword s52, s[6:7], 0x2d50                           // 00000000246C: C0020D03 00002D50
	s_load_dword s56, s[6:7], 0x2d54                           // 000000002474: C0020E03 00002D54
	v_fma_f32 v7, v5, s77, v7                                  // 00000000247C: D1CB0007 041C9B05
	s_load_dword s57, s[6:7], 0x2d58                           // 000000002484: C0020E43 00002D58
	s_waitcnt lgkmcnt(0)                                       // 00000000248C: BF8CC07F
	v_fma_f32 v5, v5, s8, v6                                   // 000000002490: D1CB0005 04181105
	v_fma_f32 v6, v4, s11, v8                                  // 000000002498: D1CB0006 04201704
	s_load_dword s8, s[6:7], 0xc4c                             // 0000000024A0: C0020203 00000C4C
	v_fma_f32 v19, v4, s9, v19                                 // 0000000024A8: D1CB0013 044C1304
	v_fma_f32 v8, v4, s24, v24                                 // 0000000024B0: D1CB0008 04603104
	v_fma_f32 v22, v4, s28, v22                                // 0000000024B8: D1CB0016 04583904
	s_load_dword s11, s[6:7], 0xc50                            // 0000000024C0: C00202C3 00000C50
	v_fma_f32 v21, v4, s33, v21                                // 0000000024C8: D1CB0015 04544304
	s_load_dword s24, s[6:7], 0xc54                            // 0000000024D0: C0020603 00000C54
	s_load_dword s28, s[6:7], 0xc58                            // 0000000024D8: C0020703 00000C58
	s_load_dword s33, s[6:7], 0xc5c                            // 0000000024E0: C0020843 00000C5C
	s_waitcnt lgkmcnt(0)                                       // 0000000024E8: BF8CC07F
	v_fma_f32 v20, v4, s8, v20                                 // 0000000024EC: D1CB0014 04501104
	s_load_dword s8, s[6:7], 0x1b4c                            // 0000000024F4: C0020203 00001B4C
	v_fma_f32 v18, v4, s14, v18                                // 0000000024FC: D1CB0012 04481D04
	v_fma_f32 v17, v4, s48, v17                                // 000000002504: D1CB0011 04446104
	v_fma_f32 v12, v4, s12, v12                                // 00000000250C: D1CB000C 04301904
	s_load_dword s9, s[6:7], 0x1b50                            // 000000002514: C0020243 00001B50
	v_fma_f32 v15, v4, s53, v15                                // 00000000251C: D1CB000F 043C6B04
	s_load_dword s14, s[6:7], 0x1b54                           // 000000002524: C0020383 00001B54
	s_load_dword s48, s[6:7], 0x1b58                           // 00000000252C: C0020C03 00001B58
	s_load_dword s53, s[6:7], 0x1b5c                           // 000000002534: C0020D43 00001B5C
	s_waitcnt lgkmcnt(0)                                       // 00000000253C: BF8CC07F
	v_fma_f32 v13, v4, s8, v13                                 // 000000002540: D1CB000D 04341104
	s_load_dword s8, s[6:7], 0x2a4c                            // 000000002548: C0020203 00002A4C
	v_fma_f32 v11, v4, s41, v11                                // 000000002550: D1CB000B 042C5304
	v_fma_f32 v10, v4, s68, v10                                // 000000002558: D1CB000A 04288904
	s_load_dword s12, s[6:7], 0x2a50                           // 000000002560: C0020303 00002A50
	s_load_dword s41, s[6:7], 0x2a54                           // 000000002568: C0020A43 00002A54
	s_load_dword s61, s[6:7], 0x2a58                           // 000000002570: C0020F43 00002A58
	s_load_dword s66, s[6:7], 0x2a5c                           // 000000002578: C0021083 00002A5C
	s_waitcnt lgkmcnt(0)                                       // 000000002580: BF8CC07F
	v_fma_f32 v7, v4, s8, v7                                   // 000000002584: D1CB0007 041C1104
	v_fma_f32 v9, v4, s73, v9                                  // 00000000258C: D1CB0009 04249304
	v_fma_f32 v4, v4, s36, v5                                  // 000000002594: D1CB0004 04144904
	v_fma_f32 v5, v3, s20, v6                                  // 00000000259C: D1CB0005 04182903
	s_load_dword s8, s[6:7], 0x950                             // 0000000025A4: C0020203 00000950
	v_fma_f32 v6, v3, s25, v8                                  // 0000000025AC: D1CB0006 04203303
	v_fma_f32 v19, v3, s10, v19                                // 0000000025B4: D1CB0013 044C1503
	v_fma_f32 v8, v3, s29, v22                                 // 0000000025BC: D1CB0008 04583B03
	s_load_dword s20, s[6:7], 0x954                            // 0000000025C4: C0020503 00000954
	s_load_dword s25, s[6:7], 0x958                            // 0000000025CC: C0020643 00000958
	s_load_dword s29, s[6:7], 0x95c                            // 0000000025D4: C0020743 0000095C
	s_load_dword s36, s[6:7], 0x960                            // 0000000025DC: C0020903 00000960
	s_waitcnt lgkmcnt(0)                                       // 0000000025E4: BF8CC07F
	v_fma_f32 v21, v3, s8, v21                                 // 0000000025E8: D1CB0015 04541103
	s_load_dword s8, s[6:7], 0x1850                            // 0000000025F0: C0020203 00001850
	v_fma_f32 v20, v3, s11, v20                                // 0000000025F8: D1CB0014 04501703
	v_fma_f32 v13, v3, s9, v13                                 // 000000002600: D1CB000D 04341303
	v_fma_f32 v17, v3, s49, v17                                // 000000002608: D1CB0011 04446303
	s_load_dword s10, s[6:7], 0x1854                           // 000000002610: C0020283 00001854
	s_load_dword s11, s[6:7], 0x1858                           // 000000002618: C00202C3 00001858
	s_load_dword s49, s[6:7], 0x185c                           // 000000002620: C0020C43 0000185C
	s_load_dword s67, s[6:7], 0x1860                           // 000000002628: C00210C3 00001860
	s_waitcnt lgkmcnt(0)                                       // 000000002630: BF8CC07F
	v_fma_f32 v15, v3, s8, v15                                 // 000000002634: D1CB000F 043C1103
	s_load_dword s8, s[6:7], 0x2750                            // 00000000263C: C0020203 00002750
	v_fma_f32 v11, v3, s46, v11                                // 000000002644: D1CB000B 042C5D03
	v_fma_f32 v18, v3, s15, v18                                // 00000000264C: D1CB0012 04481F03
	v_fma_f32 v12, v3, s13, v12                                // 000000002654: D1CB000C 04301B03
	v_fma_f32 v7, v3, s12, v7                                  // 00000000265C: D1CB0007 041C1903
	s_load_dword s9, s[6:7], 0x2754                            // 000000002664: C0020243 00002754
	v_fma_f32 v10, v3, s69, v10                                // 00000000266C: D1CB000A 04288B03
	s_load_dword s46, s[6:7], 0x2758                           // 000000002674: C0020B83 00002758
	s_load_dword s68, s[6:7], 0x275c                           // 00000000267C: C0021103 0000275C
	s_load_dword s69, s[6:7], 0x2760                           // 000000002684: C0021143 00002760
	s_waitcnt lgkmcnt(0)                                       // 00000000268C: BF8CC07F
	v_fma_f32 v9, v3, s8, v9                                   // 000000002690: D1CB0009 04241103
	v_fma_f32 v3, v3, s52, v4                                  // 000000002698: D1CB0003 04106903
	v_fma_f32 v4, v2, s21, v5                                  // 0000000026A0: D1CB0004 04142B02
	s_load_dword s8, s[6:7], 0x654                             // 0000000026A8: C0020203 00000654
	v_fma_f32 v19, v2, s19, v19                                // 0000000026B0: D1CB0013 044C2702
	v_fma_f32 v5, v2, s26, v6                                  // 0000000026B8: D1CB0005 04183502
	s_load_dword s21, s[6:7], 0x658                            // 0000000026C0: C0020543 00000658
	s_load_dword s26, s[6:7], 0x65c                            // 0000000026C8: C0020683 0000065C
	s_load_dwordx2 s[12:13], s[6:7], 0x660                     // 0000000026D0: C0060303 00000660
	s_waitcnt lgkmcnt(0)                                       // 0000000026D8: BF8CC07F
	v_fma_f32 v6, v2, s8, v8                                   // 0000000026DC: D1CB0006 04201102
	s_load_dword s8, s[6:7], 0x1554                            // 0000000026E4: C0020203 00001554
	v_fma_f32 v15, v2, s10, v15                                // 0000000026EC: D1CB000F 043C1502
	v_fma_f32 v8, v2, s20, v21                                 // 0000000026F4: D1CB0008 04542902
	v_fma_f32 v20, v2, s24, v20                                // 0000000026FC: D1CB0014 04503102
	s_load_dword s19, s[6:7], 0x1558                           // 000000002704: C00204C3 00001558
	v_fma_f32 v18, v2, s31, v18                                // 00000000270C: D1CB0012 04483F02
	s_load_dword s20, s[6:7], 0x155c                           // 000000002714: C0020503 0000155C
	s_load_dword s24, s[6:7], 0x1560                           // 00000000271C: C0020603 00001560
	s_load_dword s31, s[6:7], 0x1564                           // 000000002724: C00207C3 00001564
	s_waitcnt lgkmcnt(0)                                       // 00000000272C: BF8CC07F
	v_fma_f32 v17, v2, s8, v17                                 // 000000002730: D1CB0011 04441102
	s_load_dword s8, s[6:7], 0x2454                            // 000000002738: C0020203 00002454
	v_fma_f32 v12, v2, s37, v12                                // 000000002740: D1CB000C 04304B02
	v_fma_f32 v9, v2, s9, v9                                   // 000000002748: D1CB0009 04241302
	v_fma_f32 v11, v2, s51, v11                                // 000000002750: D1CB000B 042C6702
	s_load_dword s10, s[6:7], 0x2458                           // 000000002758: C0020283 00002458
	s_load_dword s37, s[6:7], 0x245c                           // 000000002760: C0020943 0000245C
	s_load_dword s51, s[6:7], 0x2460                           // 000000002768: C0020CC3 00002460
	s_load_dword s52, s[6:7], 0x2464                           // 000000002770: C0020D03 00002464
	s_waitcnt lgkmcnt(0)                                       // 000000002778: BF8CC07F
	v_fma_f32 v10, v2, s8, v10                                 // 00000000277C: D1CB000A 04281102
	s_load_dword s8, s[6:7], 0x358                             // 000000002784: C0020203 00000358
	v_fma_f32 v13, v2, s14, v13                                // 00000000278C: D1CB000D 04341D02
	v_fma_f32 v7, v2, s41, v7                                  // 000000002794: D1CB0007 041C5302
	v_fma_f32 v2, v2, s56, v3                                  // 00000000279C: D1CB0002 040C7102
	s_load_dword s9, s[6:7], 0x35c                             // 0000000027A4: C0020243 0000035C
	v_fma_f32 v3, v1, s22, v4                                  // 0000000027AC: D1CB0003 04102D01
	s_load_dwordx2 s[14:15], s[6:7], 0x360                     // 0000000027B4: C0060383 00000360
	s_waitcnt lgkmcnt(0)                                       // 0000000027BC: BF8CC07F
	v_fma_f32 v4, v1, s8, v5                                   // 0000000027C0: D1CB0004 04141101
	v_fma_f32 v5, v1, s21, v6                                  // 0000000027C8: D1CB0005 04182B01
	s_load_dword s8, s[6:7], 0x1258                            // 0000000027D0: C0020203 00001258
	v_fma_f32 v15, v1, s11, v15                                // 0000000027D8: D1CB000F 043C1701
	v_fma_f32 v6, v1, s25, v8                                  // 0000000027E0: D1CB0006 04203301
	s_load_dword s21, s[6:7], 0x125c                           // 0000000027E8: C0020543 0000125C
	v_fma_f32 v8, v1, s28, v20                                 // 0000000027F0: D1CB0008 04503901
	s_load_dword s22, s[6:7], 0x1260                           // 0000000027F8: C0020583 00001260
	s_load_dword s25, s[6:7], 0x1264                           // 000000002800: C0020643 00001264
	s_load_dword s28, s[6:7], 0x1268                           // 000000002808: C0020703 00001268
	s_waitcnt lgkmcnt(0)                                       // 000000002810: BF8CC07F
	v_fma_f32 v18, v1, s8, v18                                 // 000000002814: D1CB0012 04481101
	s_load_dword s8, s[6:7], 0x2158                            // 00000000281C: C0020203 00002158
	v_fma_f32 v17, v1, s19, v17                                // 000000002824: D1CB0011 04442701
	v_fma_f32 v13, v1, s48, v13                                // 00000000282C: D1CB000D 04346101
	v_fma_f32 v10, v1, s10, v10                                // 000000002834: D1CB000A 04281501
	v_fma_f32 v9, v1, s46, v9                                  // 00000000283C: D1CB0009 04245D01
	v_fma_f32 v7, v1, s61, v7                                  // 000000002844: D1CB0007 041C7B01
	v_fma_f32 v19, v1, s32, v19                                // 00000000284C: D1CB0013 044C4101
	s_load_dword s11, s[6:7], 0x215c                           // 000000002854: C00202C3 0000215C
	s_load_dword s19, s[6:7], 0x2160                           // 00000000285C: C00204C3 00002160
	s_load_dword s32, s[6:7], 0x2164                           // 000000002864: C0020803 00002164
	s_load_dword s41, s[6:7], 0x2168                           // 00000000286C: C0020A43 00002168
	s_waitcnt lgkmcnt(0)                                       // 000000002874: BF8CC07F
	v_fma_f32 v11, v1, s8, v11                                 // 000000002878: D1CB000B 042C1101
	v_fma_f32 v12, v1, s47, v12                                // 000000002880: D1CB000C 04305F01
	v_fma_f32 v1, v1, s57, v2                                  // 000000002888: D1CB0001 04087301
	v_fma_f32 v2, v0, s23, v3                                  // 000000002890: D1CB0002 040C2F00
	s_load_dword s8, s[6:7], 0xf5c                             // 000000002898: C0020203 00000F5C
	v_fma_f32 v17, v0, s20, v17                                // 0000000028A0: D1CB0011 04442900
	v_fma_f32 v3, v0, s9, v4                                   // 0000000028A8: D1CB0003 04101300
	v_fma_f32 v4, v0, s26, v5                                  // 0000000028B0: D1CB0004 04143500
	v_fma_f32 v5, v0, s29, v6                                  // 0000000028B8: D1CB0005 04183B00
	s_load_dword s23, s[6:7], 0xf60                            // 0000000028C0: C00205C3 00000F60
	v_fma_f32 v6, v0, s33, v8                                  // 0000000028C8: D1CB0006 04204300
	s_load_dword s26, s[6:7], 0xf64                            // 0000000028D0: C0020683 00000F64
	s_load_dword s29, s[6:7], 0xf68                            // 0000000028D8: C0020743 00000F68
	s_load_dword s33, s[6:7], 0xf6c                            // 0000000028E0: C0020843 00000F6C
	s_waitcnt lgkmcnt(0)                                       // 0000000028E8: BF8CC07F
	v_fma_f32 v8, v0, s8, v19                                  // 0000000028EC: D1CB0008 044C1100
	s_load_dword s8, s[6:7], 0x1e5c                            // 0000000028F4: C0020203 00001E5C
	v_fma_f32 v10, v0, s37, v10                                // 0000000028FC: D1CB000A 04284B00
	v_fma_f32 v18, v0, s21, v18                                // 000000002904: D1CB0012 04482B00
	s_load_dword s20, s[6:7], 0x1e60                           // 00000000290C: C0020503 00001E60
	s_load_dword s21, s[6:7], 0x1e64                           // 000000002914: C0020543 00001E64
	s_load_dword s46, s[6:7], 0x1e68                           // 00000000291C: C0020B83 00001E68
	s_load_dword s47, s[6:7], 0x1e6c                           // 000000002924: C0020BC3 00001E6C
	s_waitcnt lgkmcnt(0)                                       // 00000000292C: BF8CC07F
	v_fma_f32 v12, v0, s8, v12                                 // 000000002930: D1CB000C 04301100
	s_load_dword s8, s[6:7], 0x2d5c                            // 000000002938: C0020203 00002D5C
	v_fma_f32 v15, v0, s49, v15                                // 000000002940: D1CB000F 043C6300
	v_fma_f32 v11, v0, s11, v11                                // 000000002948: D1CB000B 042C1700
	v_fma_f32 v9, v0, s68, v9                                  // 000000002950: D1CB0009 04248900
	s_load_dword s37, s[6:7], 0x2d60                           // 000000002958: C0020943 00002D60
	v_fma_f32 v13, v0, s53, v13                                // 000000002960: D1CB000D 04346B00
	s_load_dword s48, s[6:7], 0x2d64                           // 000000002968: C0020C03 00002D64
	s_load_dword s49, s[6:7], 0x2d68                           // 000000002970: C0020C43 00002D68
	v_fma_f32 v7, v0, s66, v7                                  // 000000002978: D1CB0007 041C8500
	s_load_dword s53, s[6:7], 0x2d6c                           // 000000002980: C0020D43 00002D6C
	s_waitcnt lgkmcnt(0)                                       // 000000002988: BF8CC07F
	v_fma_f32 v0, v0, s8, v1                                   // 00000000298C: D1CB0000 04041100
	s_load_dwordx4 s[8:11], s[6:7], 0x60                       // 000000002994: C00A0203 00000060
	s_load_dword s56, s[6:7], 0x70                             // 00000000299C: C0020E03 00000070
	s_load_dword s57, s[6:7], 0x74                             // 0000000029A4: C0020E43 00000074
	s_load_dword s61, s[6:7], 0x78                             // 0000000029AC: C0020F43 00000078
	s_load_dword s66, s[6:7], 0x7c                             // 0000000029B4: C0021083 0000007C
	s_waitcnt vmcnt(2) lgkmcnt(0)                              // 0000000029BC: BF8C0072
	v_fma_f32 v1, v32, s8, v2                                  // 0000000029C0: D1CB0001 04081120
	v_fma_f32 v2, v32, s14, v3                                 // 0000000029C8: D1CB0002 040C1D20
	v_fma_f32 v3, v32, s12, v4                                 // 0000000029D0: D1CB0003 04101920
	s_load_dword s8, s[6:7], 0xc60                             // 0000000029D8: C0020203 00000C60
	v_fma_f32 v4, v32, s36, v5                                 // 0000000029E0: D1CB0004 04144920
	s_load_dword s12, s[6:7], 0xc64                            // 0000000029E8: C0020303 00000C64
	s_load_dword s14, s[6:7], 0xc68                            // 0000000029F0: C0020383 00000C68
	s_load_dword s36, s[6:7], 0xc6c                            // 0000000029F8: C0020903 00000C6C
	s_load_dword s68, s[6:7], 0xc70                            // 000000002A00: C0021103 00000C70
	s_waitcnt lgkmcnt(0)                                       // 000000002A08: BF8CC07F
	v_fma_f32 v5, v32, s8, v6                                  // 000000002A0C: D1CB0005 04181120
	v_fma_f32 v6, v32, s23, v8                                 // 000000002A14: D1CB0006 04202F20
	v_fma_f32 v8, v32, s22, v18                                // 000000002A1C: D1CB0008 04482D20
	s_load_dword s8, s[6:7], 0x1b60                            // 000000002A24: C0020203 00001B60
	v_fma_f32 v17, v32, s24, v17                               // 000000002A2C: D1CB0011 04443120
	v_fma_f32 v11, v32, s19, v11                               // 000000002A34: D1CB000B 042C2720
	s_load_dword s22, s[6:7], 0x1b64                           // 000000002A3C: C0020583 00001B64
	v_fma_f32 v15, v32, s67, v15                               // 000000002A44: D1CB000F 043C8720
	s_load_dword s23, s[6:7], 0x1b68                           // 000000002A4C: C00205C3 00001B68
	s_load_dword s24, s[6:7], 0x1b6c                           // 000000002A54: C0020603 00001B6C
	s_load_dword s67, s[6:7], 0x1b70                           // 000000002A5C: C00210C3 00001B70
	s_waitcnt lgkmcnt(0)                                       // 000000002A64: BF8CC07F
	v_fma_f32 v13, v32, s8, v13                                // 000000002A68: D1CB000D 04341120
	s_load_dword s8, s[6:7], 0x2a60                            // 000000002A70: C0020203 00002A60
	v_fma_f32 v12, v32, s20, v12                               // 000000002A78: D1CB000C 04302920
	v_fma_f32 v10, v32, s51, v10                               // 000000002A80: D1CB000A 04286720
	s_waitcnt vmcnt(1)                                         // 000000002A88: BF8C0F71
	v_fma_f32 v1, v34, s9, v1                                  // 000000002A8C: D1CB0001 04041322
	s_load_dword s19, s[6:7], 0x2a64                           // 000000002A94: C00204C3 00002A64
	v_fma_f32 v9, v32, s69, v9                                 // 000000002A9C: D1CB0009 04248B20
	s_load_dword s20, s[6:7], 0x2a68                           // 000000002AA4: C0020503 00002A68
	s_load_dword s51, s[6:7], 0x2a6c                           // 000000002AAC: C0020CC3 00002A6C
	s_load_dword s69, s[6:7], 0x2a70                           // 000000002AB4: C0021143 00002A70
	s_waitcnt lgkmcnt(0)                                       // 000000002ABC: BF8CC07F
	v_fma_f32 v7, v32, s8, v7                                  // 000000002AC0: D1CB0007 041C1120
	s_load_dword s8, s[6:7], 0x964                             // 000000002AC8: C0020203 00000964
	v_fma_f32 v0, v32, s37, v0                                 // 000000002AD0: D1CB0000 04004B20
	v_fma_f32 v5, v34, s12, v5                                 // 000000002AD8: D1CB0005 04141922
	v_fma_f32 v3, v34, s13, v3                                 // 000000002AE0: D1CB0003 040C1B22
	v_fma_f32 v2, v34, s15, v2                                 // 000000002AE8: D1CB0002 04081F22
	s_load_dword s9, s[6:7], 0x968                             // 000000002AF0: C0020243 00000968
	s_load_dword s13, s[6:7], 0x96c                            // 000000002AF8: C0020343 0000096C
	s_load_dword s15, s[6:7], 0x970                            // 000000002B00: C00203C3 00000970
	s_load_dword s37, s[6:7], 0x974                            // 000000002B08: C0020943 00000974
	s_waitcnt lgkmcnt(0)                                       // 000000002B10: BF8CC07F
	v_fma_f32 v4, v34, s8, v4                                  // 000000002B14: D1CB0004 04101122
	s_load_dword s8, s[6:7], 0x1864                            // 000000002B1C: C0020203 00001864
	v_fma_f32 v8, v34, s25, v8                                 // 000000002B24: D1CB0008 04203322
	v_fma_f32 v6, v34, s26, v6                                 // 000000002B2C: D1CB0006 04183522
	v_fma_f32 v12, v34, s21, v12                               // 000000002B34: D1CB000C 04302B22
	s_load_dword s12, s[6:7], 0x1868                           // 000000002B3C: C0020303 00001868
	v_fma_f32 v17, v34, s31, v17                               // 000000002B44: D1CB0011 04443F22
	s_load_dword s25, s[6:7], 0x186c                           // 000000002B4C: C0020643 0000186C
	s_load_dword s26, s[6:7], 0x1870                           // 000000002B54: C0020683 00001870
	s_load_dword s31, s[6:7], 0x1874                           // 000000002B5C: C00207C3 00001874
	s_waitcnt lgkmcnt(0)                                       // 000000002B64: BF8CC07F
	v_fma_f32 v15, v34, s8, v15                                // 000000002B68: D1CB000F 043C1122
	s_load_dword s8, s[6:7], 0x2764                            // 000000002B70: C0020203 00002764
	v_fma_f32 v13, v34, s22, v13                               // 000000002B78: D1CB000D 04342D22
	v_fma_f32 v11, v34, s32, v11                               // 000000002B80: D1CB000B 042C4122
	v_fma_f32 v1, v16, s10, v1                                 // 000000002B88: D1CB0001 04041510
	s_load_dword s21, s[6:7], 0x2768                           // 000000002B90: C0020543 00002768
	v_fma_f32 v10, v34, s52, v10                               // 000000002B98: D1CB000A 04286922
	s_load_dword s22, s[6:7], 0x276c                           // 000000002BA0: C0020583 0000276C
	s_load_dword s32, s[6:7], 0x2770                           // 000000002BA8: C0020803 00002770
	s_load_dword s52, s[6:7], 0x2774                           // 000000002BB0: C0020D03 00002774
	s_waitcnt lgkmcnt(0)                                       // 000000002BB8: BF8CC07F
	v_fma_f32 v9, v34, s8, v9                                  // 000000002BBC: D1CB0009 04241122
	s_load_dword s8, s[6:7], 0x368                             // 000000002BC4: C0020203 00000368
	v_fma_f32 v7, v34, s19, v7                                 // 000000002BCC: D1CB0007 041C2722
	v_fma_f32 v0, v34, s48, v0                                 // 000000002BD4: D1CB0000 04006122
	s_load_dword s10, s[6:7], 0x36c                            // 000000002BDC: C0020283 0000036C
	s_load_dword s19, s[6:7], 0x370                            // 000000002BE4: C00204C3 00000370
	s_load_dword s48, s[6:7], 0x374                            // 000000002BEC: C0020C03 00000374
	s_load_dword s71, s[6:7], 0x378                            // 000000002BF4: C00211C3 00000378
	s_waitcnt lgkmcnt(0)                                       // 000000002BFC: BF8CC07F
	v_fma_f32 v2, v16, s8, v2                                  // 000000002C00: D1CB0002 04081110
	s_load_dword s8, s[6:7], 0x668                             // 000000002C08: C0020203 00000668
	v_fma_f32 v4, v16, s9, v4                                  // 000000002C10: D1CB0004 04101310
	s_load_dword s72, s[6:7], 0x66c                            // 000000002C18: C0021203 0000066C
	s_load_dword s73, s[6:7], 0x670                            // 000000002C20: C0021243 00000670
	s_load_dword s76, s[6:7], 0x674                            // 000000002C28: C0021303 00000674
	s_load_dword s77, s[6:7], 0x678                            // 000000002C30: C0021343 00000678
	s_waitcnt lgkmcnt(0)                                       // 000000002C38: BF8CC07F
	v_fma_f32 v3, v16, s8, v3                                  // 000000002C3C: D1CB0003 040C1110
	s_load_dword s8, s[6:7], 0x1568                            // 000000002C44: C0020203 00001568
	v_fma_f32 v5, v16, s14, v5                                 // 000000002C4C: D1CB0005 04141D10
	v_fma_f32 v6, v16, s29, v6                                 // 000000002C54: D1CB0006 04183B10
	v_fma_f32 v15, v16, s12, v15                               // 000000002C5C: D1CB000F 043C1910
	v_fma_f32 v8, v16, s28, v8                                 // 000000002C64: D1CB0008 04203910
	s_load_dword s9, s[6:7], 0x156c                            // 000000002C6C: C0020243 0000156C
	s_load_dword s14, s[6:7], 0x1570                           // 000000002C74: C0020383 00001570
	s_load_dword s28, s[6:7], 0x1574                           // 000000002C7C: C0020703 00001574
	s_load_dword s29, s[6:7], 0x1578                           // 000000002C84: C0020743 00001578
	s_waitcnt lgkmcnt(0)                                       // 000000002C8C: BF8CC07F
	v_fma_f32 v17, v16, s8, v17                                // 000000002C90: D1CB0011 04441110
	s_load_dword s8, s[6:7], 0x2468                            // 000000002C98: C0020203 00002468
	v_fma_f32 v13, v16, s23, v13                               // 000000002CA0: D1CB000D 04342F10
	v_fma_f32 v12, v16, s46, v12                               // 000000002CA8: D1CB000C 04305D10
	v_fma_f32 v2, v14, s10, v2                                 // 000000002CB0: D1CB0002 0408150E
	v_fma_f32 v11, v16, s41, v11                               // 000000002CB8: D1CB000B 042C5310
	s_load_dword s12, s[6:7], 0x246c                           // 000000002CC0: C0020303 0000246C
	s_load_dword s23, s[6:7], 0x2470                           // 000000002CC8: C00205C3 00002470
	s_load_dword s41, s[6:7], 0x2474                           // 000000002CD0: C0020A43 00002474
	s_load_dword s46, s[6:7], 0x2478                           // 000000002CD8: C0020B83 00002478
	s_waitcnt lgkmcnt(0)                                       // 000000002CE0: BF8CC07F
	v_fma_f32 v10, v16, s8, v10                                // 000000002CE4: D1CB000A 04281110
	s_load_dword s8, s[6:7], 0x126c                            // 000000002CEC: C0020203 0000126C
	v_fma_f32 v1, v14, s11, v1                                 // 000000002CF4: D1CB0001 0404170E
	v_fma_f32 v9, v16, s21, v9                                 // 000000002CFC: D1CB0009 04242B10
	v_fma_f32 v7, v16, s20, v7                                 // 000000002D04: D1CB0007 041C2910
	v_fma_f32 v0, v16, s49, v0                                 // 000000002D0C: D1CB0000 04006310
	v_fma_f32 v4, v14, s13, v4                                 // 000000002D14: D1CB0004 04101B0E
	s_load_dword s10, s[6:7], 0x1270                           // 000000002D1C: C0020283 00001270
	s_load_dword s11, s[6:7], 0x1274                           // 000000002D24: C00202C3 00001274
	s_load_dword s13, s[6:7], 0x1278                           // 000000002D2C: C0020343 00001278
	s_load_dword s20, s[6:7], 0x127c                           // 000000002D34: C0020503 0000127C
	s_waitcnt lgkmcnt(0)                                       // 000000002D3C: BF8CC07F
	v_fma_f32 v8, v14, s8, v8                                  // 000000002D40: D1CB0008 0420110E
	v_fma_f32 v16, v14, s9, v17                                // 000000002D48: D1CB0010 0444130E
	s_load_dword s8, s[6:7], 0x216c                            // 000000002D50: C0020203 0000216C
	s_load_dword s9, s[6:7], 0x2170                            // 000000002D58: C0020243 00002170
	v_fma_f32 v13, v14, s24, v13                               // 000000002D60: D1CB000D 0434310E
	v_fma_f32 v15, v14, s25, v15                               // 000000002D68: D1CB000F 043C330E
	s_load_dword s21, s[6:7], 0x2174                           // 000000002D70: C0020543 00002174
	s_load_dword s24, s[6:7], 0x2178                           // 000000002D78: C0020603 00002178
	s_load_dword s25, s[6:7], 0x217c                           // 000000002D80: C0020643 0000217C
	s_waitcnt lgkmcnt(0)                                       // 000000002D88: BF8CC07F
	v_fma_f32 v11, v14, s8, v11                                // 000000002D8C: D1CB000B 042C110E
	v_fma_f32 v11, v27, s9, v11                                // 000000002D94: D1CB000B 042C131B
	s_load_dword s8, s[6:7], 0x2d70                            // 000000002D9C: C0020203 00002D70
	v_fma_f32 v8, v27, s10, v8                                 // 000000002DA4: D1CB0008 0420151B
	s_load_dword s9, s[6:7], 0x2d74                            // 000000002DAC: C0020243 00002D74
	s_load_dword s10, s[6:7], 0x2d78                           // 000000002DB4: C0020283 00002D78
	s_load_dword s6, s[6:7], 0x2d7c                            // 000000002DBC: C0020183 00002D7C
	v_fma_f32 v0, v14, s53, v0                                 // 000000002DC4: D1CB0000 04006B0E
	v_fma_f32 v3, v14, s72, v3                                 // 000000002DCC: D1CB0003 040C910E
	v_fma_f32 v1, v27, s56, v1                                 // 000000002DD4: D1CB0001 0404711B
	v_fma_f32 v2, v27, s19, v2                                 // 000000002DDC: D1CB0002 0408271B
	s_waitcnt lgkmcnt(0)                                       // 000000002DE4: BF8CC07F
	v_fma_f32 v0, v27, s8, v0                                  // 000000002DE8: D1CB0000 0400111B
	v_fma_f32 v3, v27, s73, v3                                 // 000000002DF0: D1CB0003 040C931B
	v_fma_f32 v1, v28, s57, v1                                 // 000000002DF8: D1CB0001 0404731C
	v_fma_f32 v2, v28, s48, v2                                 // 000000002E00: D1CB0002 0408611C
	v_fma_f32 v0, v28, s9, v0                                  // 000000002E08: D1CB0000 0400131C
	v_fma_f32 v3, v28, s76, v3                                 // 000000002E10: D1CB0003 040C991C
	v_fma_f32 v1, v31, s61, v1                                 // 000000002E18: D1CB0001 04047B1F
	v_fma_f32 v2, v31, s71, v2                                 // 000000002E20: D1CB0002 04088F1F
	v_fma_f32 v0, v31, s10, v0                                 // 000000002E28: D1CB0000 0400151F
	v_fma_f32 v5, v14, s36, v5                                 // 000000002E30: D1CB0005 0414490E
	v_fma_f32 v6, v14, s33, v6                                 // 000000002E38: D1CB0006 0418430E
	v_fma_f32 v12, v14, s47, v12                               // 000000002E40: D1CB000C 04305F0E
	v_fma_f32 v10, v14, s12, v10                               // 000000002E48: D1CB000A 0428190E
	v_fma_f32 v9, v14, s22, v9                                 // 000000002E50: D1CB0009 04242D0E
	v_fma_f32 v7, v14, s51, v7                                 // 000000002E58: D1CB0007 041C670E
	v_fma_f32 v14, v27, s14, v16                               // 000000002E60: D1CB000E 04401D1B
	v_fma_f32 v4, v27, s15, v4                                 // 000000002E68: D1CB0004 04101F1B
	v_fma_f32 v3, v31, s77, v3                                 // 000000002E70: D1CB0003 040C9B1F
	s_waitcnt vmcnt(0)                                         // 000000002E78: BF8C0F70
	v_fma_f32 v16, v23, s6, v0                                 // 000000002E7C: D1CB0010 04000D17
	v_fma_f32 v1, v23, s66, v1                                 // 000000002E84: D1CB0001 04048517
	v_fma_f32 v2, v23, s27, v2                                 // 000000002E8C: D1CB0002 04083717
	v_add_co_u32_e32 v0, vcc, s5, v29                          // 000000002E94: 32003A05
	v_fma_f32 v4, v28, s37, v4                                 // 000000002E98: D1CB0004 04104B1C
	global_store_dword v[29:30], v1, off                       // 000000002EA0: DC708000 007F011D
	global_store_dword v[29:30], v2, off offset:3136           // 000000002EA8: DC708C40 007F021D
	v_fma_f32 v3, v23, s30, v3                                 // 000000002EB0: D1CB0003 040C3D17
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 000000002EB8: 38023C80
	v_fma_f32 v5, v27, s68, v5                                 // 000000002EBC: D1CB0005 0414891B
	v_fma_f32 v4, v31, s34, v4                                 // 000000002EC4: D1CB0004 0410451F
	global_store_dword v[0:1], v3, off                         // 000000002ECC: DC708000 007F0300
	v_add_co_u32_e32 v0, vcc, s0, v29                          // 000000002ED4: 32003A00
	v_fma_f32 v5, v28, s38, v5                                 // 000000002ED8: D1CB0005 04144D1C
	v_fma_f32 v4, v23, s35, v4                                 // 000000002EE0: D1CB0004 04104717
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 000000002EE8: 38023C80
	v_fma_f32 v6, v27, s42, v6                                 // 000000002EEC: D1CB0006 0418551B
	v_fma_f32 v5, v31, s39, v5                                 // 000000002EF4: D1CB0005 04144F1F
	global_store_dword v[0:1], v4, off                         // 000000002EFC: DC708000 007F0400
	v_add_co_u32_e32 v0, vcc, s4, v29                          // 000000002F04: 32003A04
	v_fma_f32 v6, v28, s43, v6                                 // 000000002F08: D1CB0006 0418571C
	v_fma_f32 v5, v23, s40, v5                                 // 000000002F10: D1CB0005 04145117
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 000000002F18: 38023C80
	v_fma_f32 v6, v31, s44, v6                                 // 000000002F1C: D1CB0006 0418591F
	global_store_dword v[0:1], v5, off                         // 000000002F24: DC708000 007F0500
	v_add_co_u32_e32 v0, vcc, s3, v29                          // 000000002F2C: 32003A03
	v_fma_f32 v8, v28, s11, v8                                 // 000000002F30: D1CB0008 0420171C
	v_fma_f32 v6, v23, s45, v6                                 // 000000002F38: D1CB0006 04185B17
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 000000002F40: 38023C80
	v_fma_f32 v8, v31, s13, v8                                 // 000000002F44: D1CB0008 04201B1F
	global_store_dword v[0:1], v6, off                         // 000000002F4C: DC708000 007F0600
	v_add_co_u32_e32 v0, vcc, s2, v29                          // 000000002F54: 32003A02
	v_fma_f32 v14, v28, s28, v14                               // 000000002F58: D1CB000E 0438391C
	v_fma_f32 v8, v23, s20, v8                                 // 000000002F60: D1CB0008 04202917
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 000000002F68: 38023C80
	v_fma_f32 v15, v27, s26, v15                               // 000000002F6C: D1CB000F 043C351B
	v_fma_f32 v14, v31, s29, v14                               // 000000002F74: D1CB000E 04383B1F
	global_store_dword v[0:1], v8, off                         // 000000002F7C: DC708000 007F0800
	v_add_co_u32_e32 v0, vcc, s1, v29                          // 000000002F84: 32003A01
	v_fma_f32 v15, v28, s31, v15                               // 000000002F88: D1CB000F 043C3F1C
	v_fma_f32 v14, v23, s50, v14                               // 000000002F90: D1CB000E 04386517
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 000000002F98: 38023C80
	s_movk_i32 s0, 0x6200                                      // 000000002F9C: B0006200
	v_fma_f32 v13, v27, s67, v13                               // 000000002FA0: D1CB000D 0434871B
	v_fma_f32 v15, v31, s54, v15                               // 000000002FA8: D1CB000F 043C6D1F
	global_store_dword v[0:1], v14, off                        // 000000002FB0: DC708000 007F0E00
	v_add_co_u32_e32 v0, vcc, s0, v29                          // 000000002FB8: 32003A00
	v_fma_f32 v13, v28, s58, v13                               // 000000002FBC: D1CB000D 0434751C
	v_fma_f32 v15, v23, s55, v15                               // 000000002FC4: D1CB000F 043C6F17
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 000000002FCC: 38023C80
	s_movk_i32 s0, 0x6e40                                      // 000000002FD0: B0006E40
	v_fma_f32 v12, v27, s62, v12                               // 000000002FD4: D1CB000C 04307D1B
	v_fma_f32 v13, v31, s59, v13                               // 000000002FDC: D1CB000D 0434771F
	global_store_dword v[0:1], v15, off                        // 000000002FE4: DC708000 007F0F00
	v_add_co_u32_e32 v0, vcc, s0, v29                          // 000000002FEC: 32003A00
	v_fma_f32 v12, v28, s63, v12                               // 000000002FF0: D1CB000C 04307F1C
	v_fma_f32 v13, v23, s60, v13                               // 000000002FF8: D1CB000D 04347917
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 000000003000: 38023C80
	s_movk_i32 s0, 0x7a80                                      // 000000003004: B0007A80
	v_fma_f32 v12, v31, s64, v12                               // 000000003008: D1CB000C 0430811F
	global_store_dword v[0:1], v13, off                        // 000000003010: DC708000 007F0D00
	v_add_co_u32_e32 v0, vcc, s0, v29                          // 000000003018: 32003A00
	v_fma_f32 v11, v28, s21, v11                               // 00000000301C: D1CB000B 042C2B1C
	v_fma_f32 v12, v23, s65, v12                               // 000000003024: D1CB000C 04308317
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 00000000302C: 38023C80
	s_mov_b32 s0, 0x86c0                                       // 000000003030: BE8000FF 000086C0
	v_fma_f32 v10, v27, s23, v10                               // 000000003038: D1CB000A 04282F1B
	v_fma_f32 v11, v31, s24, v11                               // 000000003040: D1CB000B 042C311F
	global_store_dword v[0:1], v12, off                        // 000000003048: DC708000 007F0C00
	v_add_co_u32_e32 v0, vcc, s0, v29                          // 000000003050: 32003A00
	v_fma_f32 v10, v28, s41, v10                               // 000000003054: D1CB000A 0428531C
	v_fma_f32 v11, v23, s25, v11                               // 00000000305C: D1CB000B 042C3317
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 000000003064: 38023C80
	s_mov_b32 s0, 0x9300                                       // 000000003068: BE8000FF 00009300
	v_fma_f32 v9, v27, s32, v9                                 // 000000003070: D1CB0009 0424411B
	v_fma_f32 v10, v31, s46, v10                               // 000000003078: D1CB000A 04285D1F
	global_store_dword v[0:1], v11, off                        // 000000003080: DC708000 007F0B00
	v_add_co_u32_e32 v0, vcc, s0, v29                          // 000000003088: 32003A00
	v_fma_f32 v9, v28, s52, v9                                 // 00000000308C: D1CB0009 0424691C
	v_fma_f32 v10, v23, s70, v10                               // 000000003094: D1CB000A 04288D17
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 00000000309C: 38023C80
	s_mov_b32 s0, 0x9f40                                       // 0000000030A0: BE8000FF 00009F40
	v_fma_f32 v7, v27, s69, v7                                 // 0000000030A8: D1CB0007 041C8B1B
	v_fma_f32 v9, v31, s74, v9                                 // 0000000030B0: D1CB0009 0424951F
	global_store_dword v[0:1], v10, off                        // 0000000030B8: DC708000 007F0A00
	v_add_co_u32_e32 v0, vcc, s0, v29                          // 0000000030C0: 32003A00
	v_fma_f32 v7, v28, s16, v7                                 // 0000000030C4: D1CB0007 041C211C
	v_fma_f32 v9, v23, s75, v9                                 // 0000000030CC: D1CB0009 04249717
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 0000000030D4: 38023C80
	s_mov_b32 s0, 0xab80                                       // 0000000030D8: BE8000FF 0000AB80
	v_fma_f32 v7, v31, s17, v7                                 // 0000000030E0: D1CB0007 041C231F
	global_store_dword v[0:1], v9, off                         // 0000000030E8: DC708000 007F0900
	v_add_co_u32_e32 v0, vcc, s0, v29                          // 0000000030F0: 32003A00
	v_fma_f32 v7, v23, s18, v7                                 // 0000000030F4: D1CB0007 041C2517
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 0000000030FC: 38023C80
	s_mov_b32 s0, 0xb7c0                                       // 000000003100: BE8000FF 0000B7C0
	global_store_dword v[0:1], v7, off                         // 000000003108: DC708000 007F0700
	v_add_co_u32_e32 v0, vcc, s0, v29                          // 000000003110: 32003A00
	v_addc_co_u32_e32 v1, vcc, 0, v30, vcc                     // 000000003114: 38023C80
	global_store_dword v[0:1], v16, off                        // 000000003118: DC708000 007F1000
BB0_5:
	s_endpgm                                                   // 000000003120: BF810000

