# VectorAdd example

SMUL = 1
GCN1_2_4 = 0

.ifarch GCN1.2
    GCN1_2_4 = 1
    SMUL = 4
.elseifarch GCN1.4
    GCN1_2_4 = 1
    SMUL = 4
.endif

.ifarch GCN1.4
    # helper macros for integer add/sub instructions
    .macro VADD_U32 dest, cdest, src0, src1, mods:vararg
        v_add_co_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VADDC_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_addc_co_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
    .macro VSUB_U32 dest, cdest, src0, src1, mods:vararg
        v_sub_co_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VSUBB_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_subb_co_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
    .macro VSUBREV_U32 dest, cdest, src0, src1, mods:vararg
        v_subrev_co_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VSUBBREV_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_subbrev_co_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
.else
    # helper macros for integer add/sub instructions
    .macro VADD_U32 dest, cdest, src0, src1, mods:vararg
        v_add_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VADDC_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_addc_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
    .macro VSUB_U32 dest, cdest, src0, src1, mods:vararg
        v_sub_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VSUBB_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_subb_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
    .macro VSUBREV_U32 dest, cdest, src0, src1, mods:vararg
        v_subrev_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VSUBBREV_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_subbrev_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
.endif

.kernel vectorAdd
    .config
        .dims x
        .useargs
        .usesetup
        .setupargs
        .arg n, uint                        # argument uint n
        .arg aBuf, float*, global, const    # argument const float* aBuf
        .arg bBuf, float*, global, const    # argument const float* bBuf
        .arg cBuf, float*, global           # argument float* cBuf
    .text

    .ifarch GCN1.4
        GID = %s10
    .else
        GID = %s8
    .endif

        s_load_dwordx4 s[0:3], s[6:7], 14*SMUL  # get aBuf and bBuf pointers
        s_load_dword s4, s[4:5], 1*SMUL         # get local info dword
        s_load_dword s9, s[6:7], 0              # get global offset (32-bit)
        s_load_dword s5, s[6:7], 12*SMUL        # get n - number of elems
        s_waitcnt lgkmcnt(0)                    # wait for data
        s_and_b32 s4, s4, 0xffff            # only first localsize(0)
        s_mul_i32 s4, GID, s4                # localsize*groupId
        s_add_u32 s4, s9, s4                # localsize*groupId+offset
        VADD_U32 v0, vcc, s4, v0           # final global_id
        v_cmp_gt_u32 vcc, s5, v0            # global_id(0) < n
        s_and_saveexec_b64 s[4:5], vcc          # lock all threads with id>=n
        s_cbranch_execz end                     # no active threads, we jump to end
        
        v_lshrrev_b32 v1, 30, v0
        v_lshlrev_b32 v0, 2, v0             # v[0:1] - global_id(0)*4
        VADD_U32 v2, vcc, s0, v0           # aBuf+get_global_id(0)
        v_mov_b32 v3, s1
        VADDC_U32 v3, vcc, 0, v3, vcc      # aBuf+get_global_id(0) - higher part
        VADD_U32 v4, vcc, s2, v0           # bBuf+get_global_id(0)
        v_mov_b32 v5, s3
        VADDC_U32 v5, vcc, 0, v5, vcc      # bBuf+get_global_id(0) - higher part
        flat_load_dword v2, v[2:3]          # load value from aBuf
        flat_load_dword v4, v[4:5]          # load value from bBuf
        s_waitcnt vmcnt(0) & lgkmcnt(0)     # wait for data
        v_add_f32 v2, v2, v4                # add values
        
        s_load_dwordx2 s[0:1], s[6:7], 18*SMUL  # get cBuf pointer
        s_waitcnt lgkmcnt(0)
        VADD_U32 v0, vcc, s0, v0           # cBuf+get_global_id(0)
        v_mov_b32 v3, s1
        VADDC_U32 v1, vcc, 0, v3, vcc      # cBuf+get_global_id(0) - higher part
        flat_store_dword v[0:1], v2         # store value to cBuf
end:
        s_endpgm

