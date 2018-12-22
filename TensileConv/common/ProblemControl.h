#pragma once

#include <stdarg.h>
#include <vector>
#include "ff_utils.h"
#include "SolutionControl.h"
#include "AutoTuning.h"

#include "unistd.h"

using namespace AutoTune;


/************************************************************************/
/* 问题句柄																*/
/************************************************************************/
class ProblemCtrlBase
{
public:
	ProblemCtrlBase();
	ProblemCtrlBase(std::string name);

public:
	void RunProblem();

	E_ReturnState RunOneProblemConfig();

	E_ReturnState RunProblemOnce();

	virtual E_ReturnState GenerateProblemConfigs() = 0;
	virtual E_ReturnState InitHost() = 0;
	virtual E_ReturnState Host() = 0;
	virtual E_ReturnState Verify() = 0;
	virtual void ReleaseHost() = 0;

	std::string ProblemName;
	std::list<T_ProblemConfig*> *ProblemConfigList;	// 所有问题配置
	T_ProblemConfig *ProblemConfig;					// 当前正在处理的问题配置
	SolutionCtrlBase * Solution;

protected:
	CmdArgs * cmdArgs;
	BackendEngineBase * hwBackend;
};

