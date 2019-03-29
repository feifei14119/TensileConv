
#include "ff_timer.h"

namespace feifei 
{
#ifdef _WIN32
	void WinTimer::Restart()
	{
		QueryPerformanceFrequency(&cpuFreqHz);
		QueryPerformanceCounter(&startCnt);
	}

	void WinTimer::Stop()
	{
		QueryPerformanceCounter(&stopCnt);
		ElapsedMilliSec = (stopCnt.QuadPart - startCnt.QuadPart) * 1000.0 / cpuFreqHz.QuadPart;
	}

	void WinTimer::SleepSec(int sec)
	{
		Sleep(sec * 1000);
	}
	void WinTimer::SleepSec(double sec)
	{
		Sleep((unsigned int)(sec * 1000));
	}
	void WinTimer::SleepMilliSec(int ms)
	{
		Sleep(ms);
	}
	void WinTimer::SleepMilliSec(double ms)
	{
		Sleep((unsigned int)ms);
	}
#else
	void UnixTimer::Restart()
	{
		clock_gettime(CLOCK_MONOTONIC, &startTime);
	}

	void UnixTimer::Stop()
	{
		clock_gettime(CLOCK_MONOTONIC, &stopTime);
		double d_startTime = static_cast<double>(startTime.tv_sec)*1e9 + static_cast<double>(startTime.tv_nsec);
		double d_currentTime = static_cast<double>(stopTime.tv_sec)*1e9 + static_cast<double>(stopTime.tv_nsec);
		ElapsedNanoSec = d_currentTime - d_startTime;
		ElapsedMilliSec = ElapsedNanoSec / 1e6;
	}

	void UnixTimer::SleepSec(int sec)
	{
		sleep(sec);
	}
	void UnixTimer::SleepSec(double sec)
	{
		usleep((unsigned int)(sec * 1000 * 1000));
	}
	void UnixTimer::SleepMilliSec(int ms)
	{
		usleep(ms * 1000);
	}
	void UnixTimer::SleepMilliSec(double ms)
	{
		usleep((unsigned int)(ms * 1000));
	}
#endif
}
