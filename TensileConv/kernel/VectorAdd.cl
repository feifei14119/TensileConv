__kernel void VectorAdd(__global  float * a, __global  float * b, __global  float * c)
{
    uint tid = get_global_id(0);
    
    c[tid] = a[tid] + b[tid];
}

