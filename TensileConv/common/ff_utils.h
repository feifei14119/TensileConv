#pragma once

#include "ff_basic.h"
#include "ff_file_opt.h"
#include "ff_log.h"
#include "ff_timer.h"
#include "ff_cmd_args.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <list>
#include <algorithm>
#include <math.h>
#include <stdarg.h>

#include <map>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#if RUNTIME_CUDA
#include "cuda.h"
#include "cuda_runtime.h"
#endif

#if RUNTIME_OCL
#include <CL/cl.h>
#endif

#include "ff_cl_helper.h"

using namespace feifei;

/*
	<ff_directory>
	ensure_dir(".//log//");

	<ff_timer>
	UnixTimer *tm = new UnixTimer();
	tm->Restart();
	sleep(0.01);
	tm->Stop();
	printf("elpsed %.2f ms.\n", tm->ElapsedMilliSec);

	<ff_log>
	float a = 1.23;
	INFO("a = %.2f.", a);
	WARN("a = %.2f.", a);
	ERR("a = %.2f.", a);
	LogFile * flog = new LogFile("test");
	flog->Log("a = %.2f.", a);

	<ff_cmd_args>
	// 1. in main():
	CmdArgs * cmdArgs = new CmdArgs(argc, argv);
	....
	// 2. in other file:
	CmdArgs * args = CmdArgs::GetCmdArgs();
	int a = *(int*)args->GetOneArg(E_ArgId::CMD_ARG_DEVICE);
*/

static inline std::string getKernelDirectory() {
#ifdef KERNEL_DIR
	return std::string(KERNEL_DIR);
#else
	return get_curr_path() + "/Kernels/";
#endif
}