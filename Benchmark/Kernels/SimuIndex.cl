
#define		W					(28)
#define		H					(28)
#define		C					(192)
#define		K					(64)

#define		IN_CHANNEL_STRIDE	(W * H)
#define		IN_BATCH_STRIDE		(W * H * C)
#define		WEI_CHANNEL_STRIDE	(C)
#define		OUT_CHANNEL_STRIDE	(W * H)
#define		OUT_BATCH_STRIDE	(W * H * K)

#define		K_BLK_SIZE			(16)
#define		K_OUT_GROUPS		(K / K_BLK_SIZE)
#define		PIX_MAPS			(64)
#define		GROUP_SIZE			(64)

__kernel void SimuIndex(__global int* in_off_buff,__global int* wei_off_buff,__global int* out_off_buff)
{
    uint local_id0 = get_local_id(0);
    uint group_id0 = get_group_id(0);
	
    uint weiBlkId = group_id0 % K_OUT_GROUPS;
    uint pixBlkId = group_id0 / K_OUT_GROUPS;

    uint pos      = (pixBlkId * PIX_MAPS + local_id0) % IN_CHANNEL_STRIDE;
    uint batch_id = (pixBlkId * PIX_MAPS + local_id0) / IN_CHANNEL_STRIDE;
    uint out_id   = weiBlkId * K_BLK_SIZE;

    uint in_off  = batch_id * IN_BATCH_STRIDE + pos;
    uint wei_off = out_id * WEI_CHANNEL_STRIDE;
    uint out_off = batch_id * OUT_BATCH_STRIDE + out_id * OUT_CHANNEL_STRIDE + pos;
	
	barrier(CLK_LOCAL_MEM_FENCE);
	
	uint glb_idx0 = group_id0 * GROUP_SIZE + local_id0;
	*(in_off_buff + glb_idx0) = in_off;
	*(wei_off_buff + glb_idx0) = wei_off;
	*(out_off_buff + glb_idx0) = out_off;
}
