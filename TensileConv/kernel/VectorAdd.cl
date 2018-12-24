__kernel void VectorAdd(__global  int * a, __global  int * b, __global  int * c)
{
    uint tid = get_global_id(0);
    
    c[tid] = a[tid] + b[tid];
}

