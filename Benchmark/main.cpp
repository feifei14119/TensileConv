#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "RuntimeControl.h"
#include "ProblemControl.h"

#include "ConvFwd1x1.h"

int main(int argc, char *argv[])
{
	RuntimeCtrl::InitRuntime(argc, argv);
	 
	// 建立问题
	ConvFwd1x1Problem *conv1x1 = new ConvFwd1x1Problem("DirConv1x1Fwd");	
//	conv1x1->TurnProblem();							// 搜索全部尺寸范围	
	conv1x1->TurnProblem(56, 56, 1024, 256, 4);		// 运行 WH = 28, C = 2048, K = 64, N = 1 的问题

//	// 对该问题创建一个solution
//	T_SolutionConfig *solCfg = conv1x1->NewSolutionConfig("TensileConv");
//
//	// 添加调节参数
//	conv1x1->NewTurnParam(solCfg,
//		//ConvFwd1x1Problem::E_TurnParam::TURN_PARAM_C_IN_GROUP, std::vector<int>({ 1, 2, 4, 8, 16, 32 }));
//		ConvFwd1x1Problem::E_TurnParam::TURN_PARAM_C_IN_GROUP, std::vector<int>({ 1}));
//	conv1x1->NewTurnParam(solCfg,
//		ConvFwd1x1Problem::E_TurnParam::TURN_PARAM_K_OUT_MAPS, std::vector<int>({ 4 , 8, 16 }));
//	conv1x1->NewTurnParam(solCfg,
//		ConvFwd1x1Problem::E_TurnParam::TURN_PARAM_GROUP_SIZE, std::vector<int>({ 64,128,256 }));
//

	RuntimeCtrl::CleanRuntime();
	return 0;
}
