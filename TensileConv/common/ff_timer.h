#pragma once

#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "ff_basic.h"

namespace feifei
{
#ifdef _WIN32 
#define ffSleepSec(t)	WinTimer::SleepSec(t)
#define ffSleepMS(t)	WinTimer::SleepMilliSec(t)
#else
#define ffSleepSec(t)	UnixTimer::SleepSec(t)
#define ffSleepMS(t)	UnixTimer::SleepMilliSec(t)
#endif
	class TimerBase
	{
	protected:
		timespec startTime;
		timespec stopTime;

	public:
		virtual void Restart() = 0;
		virtual void Stop() = 0;

		double ElapsedMilliSec = 0;
		double ElapsedNanoSec = 0;
	};

#ifdef _WIN32
	class WinTimer:public TimerBase
	{
	public:
		void Restart();
		void Stop();
		static void SleepSec(int sec);
		static void SleepSec(double sec);
		static void SleepMilliSec(int ms);
		static void SleepMilliSec(double ms);

	protected:
		LARGE_INTEGER cpuFreqHz;
		LARGE_INTEGER startCnt;
		LARGE_INTEGER stopCnt;
	};
#else
	class UnixTimer:public TimerBase
	{
	public:
		void Restart();
		void Stop();
		static void SleepSec(int sec);
		static void SleepSec(double sec);
		static void SleepMilliSec(int ms);
		static void SleepMilliSec(double ms);
	};
#endif
}