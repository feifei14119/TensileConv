
#include "ProblemControl.h"

using namespace AutoTune;

/************************************************************************/
/* 问题句柄																*/
/************************************************************************/
ProblemCtrlBase::ProblemCtrlBase()
{
	ProblemConfigList = new std::list<T_ProblemConfig*>;

	cmdArgs = CmdArgs::GetCmdArgs();
	hwBackend = BackendEngine::Get("OpenCL");
}

ProblemCtrlBase::ProblemCtrlBase(std::string name)
{
	ProblemName = name;
	ProblemConfigList = new std::list<T_ProblemConfig*>;

	cmdArgs = CmdArgs::GetCmdArgs();
	hwBackend = BackendEngine::Get("OpenCL");
}

void ProblemCtrlBase::RunProblem()
{
	printf("************************************************************************\n");
	printf("* Problem Name: %s.\n", ProblemName.c_str());
	printf("************************************************************************\n");

	// ======================================================================
	// 生成问题空间
	// ======================================================================
	INFO("generate problem config list.");
	ProblemConfigList->clear();
	GenerateProblemConfigs();

	// ======================================================================
	// 遍历problem参数空间,搜索参数空间
	// ======================================================================
	std::list<T_ProblemConfig*>::iterator problemCfgIt;
	for (problemCfgIt = ProblemConfigList->begin(); 
		problemCfgIt != ProblemConfigList->end(); 
		problemCfgIt++)
	{
		ProblemConfig = *problemCfgIt;

		printf("************************************************************************\n");
		printf("* Problem Name: %s.\n", ProblemName.c_str());
		printf("* Problem Config: %s.\n", ProblemConfig->ConfigName.c_str());
		printf("************************************************************************\n");

		if (ProblemConfig->ProblemParamSpace.ParamNum > 0)
		{
			RunOneProblemConfig();
		}
		else
		{
			RunProblemOnce();
		}
	}
}

E_ReturnState ProblemCtrlBase::RunOneProblemConfig()
{
	while (true)
	{
		Solution->ProblemBestTime = -1;
		INFO("initialize host.");			InitHost();
		INFO("run host calculate.");		Host();
		INFO("solve this problem.");		Solution->RunSolution(ProblemConfig);
		INFO("verify device calculation.");	Verify();
		INFO("release host.");				ReleaseHost();

		INFO("search problem parameters.");
		if (ProblemConfig->ProblemParamSpace.GetNexComb() == E_ReturnState::FAIL)
		{
			INFO("search problem parameters finished.");
			break;
		}
	}
}

E_ReturnState ProblemCtrlBase::RunProblemOnce()
{
	Solution->ProblemBestTime = -1;
	INFO("initialize host.");				InitHost();
	INFO("run host calculate.");			Host();
	INFO("solve this problem.");			Solution->RunSolution(ProblemConfig);
	INFO("verify device calculation.");		Verify();
	INFO("release host.");					ReleaseHost();

	return E_ReturnState::SUCCESS;
}
