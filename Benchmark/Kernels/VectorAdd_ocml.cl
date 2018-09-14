
#define	WAVE_SIZE (64)

__kernel void VectorAdd(__global const float *a, __global const float *b, __global float *result)
{	
    uint local_idx = get_local_id(0);
    uint group_idx = get_group_id(0);
	uint gidx = group_idx * WAVE_SIZE + local_idx;
	
	float tmpa;
	float tmpb;	
	float tmpc = 0;
	
	__global float * addr_a;
	__global float * addr_b;	
	addr_a = a + gidx;
	addr_b = b + gidx;
	
	__asm volatile("global_load_dword %[a], %[pA], off\n" :[a]"=v"(tmpa) : [pA] "v" (addr_a));
	__asm volatile("global_load_dword %[b], %[pB], off\n" :[b]"=v"(tmpb) : [pB] "v" (addr_b));
	__asm volatile("s_waitcnt vmcnt(0)\n");
	__asm volatile("v_add_f32 %[c], %[a], %[b]\n" :[c]"=v"(tmpc) : [a] "v" (tmpa), [b] "v" (tmpb));
	 
	result[gidx] = tmpc;
	
}
