
#include "ProblemControl.h"

using namespace AutoTune;

/************************************************************************/
/* 问题句柄																*/
/************************************************************************/
void ProblemCtrlBase::RunAllProblem()
{
	// ======================================================================
	// 遍历problem参数空间,搜索参数空间
	// ======================================================================
	std::list<T_ProblemConfig*>::iterator problemCfgIt;
	for (problemCfgIt = ProblemConfigList->begin(); problemCfgIt != ProblemConfigList->end(); problemCfgIt++)
	{
		ProblemConfig = *problemCfgIt;

		PRINT_SEPARATOR1();
		INFO(" Problem Name: %s.", ProblemName.c_str());
		INFO(" Problem Config: %s.", ProblemConfig->ConfigName.c_str());
		PRINT_SEPARATOR1();

		RunOneProblem();
	}
}

E_ReturnState ProblemCtrlBase::RunOneProblem()
{
	while (true)
	{
		INFO("initialize host.");				InitHost();
		INFO("run host calculate.");			Host();
		INFO("solve this problem.");			Solution->RunAllSolution(ProblemConfig);
		INFO("verify device calculation.");		Verify();
		INFO("release host.");					ReleaseHost();

		if (ProblemConfig->ProblemParamSpace.ParamNum > 0)
		{
			INFO("search problem parameters.");
			if (ProblemConfig->ProblemParamSpace.GetNexComb() == E_ReturnState::FAIL)
			{
				INFO("search problem parameters finished.");
				break;
			}
		}
		else
		{
			break;
		}
	}

	return E_ReturnState::SUCCESS;
}
#pragma endregion
