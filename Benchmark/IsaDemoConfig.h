#pragma once 

#include "BasicClass.h" 

/************************************************************************/
/* SMEM                                                                 */
/************************************************************************/
typedef struct ExtSmemSolutionConfigTpye
{
}T_ExtSmemSolutionConfig;

typedef struct ExtSmemProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_out, *c_ref;
}T_ExtSmemProblemConfig;
