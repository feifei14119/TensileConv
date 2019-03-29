#include <string.h> // linux: C style memset
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

#include "ff_log.h"

namespace feifei 
{
	static const int PrintBufferSize = 1024;
	static char PrintBuffer[PrintBufferSize];

	static time_t t;
#ifdef _WIN32
	static struct tm stm;
#endif
	static std::string getCurrentTime()
	{
		memset(PrintBuffer, 0, PrintBufferSize);
		t = time(0);
#ifdef _WIN32
		localtime_s(&stm, &t);
		strftime(PrintBuffer, PrintBufferSize, "[%H:%M:%S]", &stm);
#else
		strftime(PrintBuffer, PrintBufferSize, "[%H:%M:%S]", localtime(&t));
#endif
		return std::string(PrintBuffer);
	}

	static const int CommentLength = 73;
	void PrintSeperator(const char c, std::ostream *sm)
	{
		*sm << std::string(CommentLength, c) << std::endl;
	}

	void PrintInfo(const char * format, ...)
	{
		memset(PrintBuffer, 0, PrintBufferSize);
		va_list args;
		va_start(args, format);
#ifdef _WIN32
		vsprintf_s(PrintBuffer, PrintBufferSize, format, args);
#else
		vsprintf(PrintBuffer, format, args);
#endif
		printf("%s", PrintBuffer);
		va_end(args);
		printf("\n");
	}
	void PrintInfo(std::string msg, ...)
	{
		printf(msg.c_str());
		printf("\n");
	}
	
	void PrintLog(const char * format, ...)
	{
		std::clog << "[  LOG  ]" << getCurrentTime() << std::flush;

		memset(PrintBuffer, 0, PrintBufferSize);
		va_list args;
		va_start(args, format);
#ifdef _WIN32
		vsprintf_s(PrintBuffer, PrintBufferSize, format, args);
#else
		vsprintf(PrintBuffer, format, args);
#endif
		va_end(args);
		std::clog << std::string(PrintBuffer) << std::endl;
	}
	void PrintLog(std::string msg, ...)
	{
		std::clog << "[  LOG  ]" << getCurrentTime() << msg << std::endl;
	}

	void PrintWarning(const char *file, int line, const char * format, ...)
	{
		std::clog << "[WARNING]" << getCurrentTime() << std::flush;

		int pos;
		char * p = (char *)file;
		memset(PrintBuffer, 0, PrintBufferSize);
		va_list args;
		va_start(args, format);
#ifdef _WIN32
		pos = vsprintf_s(PrintBuffer, PrintBufferSize, format, args);
		p = strrchr(p, DIR_SPT) + 1;
		sprintf_s(PrintBuffer + pos, PrintBufferSize, " @%s:%d", p, line);
#else
		pos = vsprintf(PrintBuffer, format, args);
		p = strrchr(p, DIR_SPT) + 1;
		sprintf(PrintBuffer + pos, " @%s:%d", p, line);
#endif
		va_end(args);
		std::clog << std::string(PrintBuffer) << std::endl;
	}
	void PrintWarning(const char *file, int line, std::string msg, ...)
	{
		std::clog << "[WARNING]" << getCurrentTime() << msg << std::flush;

		std::string sf(file);
		std::clog << " @" << sf.erase(0, sf.find_last_of(DIR_SPT) + 1)
			<< ":" << line << std::endl;
	}

	E_ReturnState PrintError(const char *file, int line, const char * format, ...)
	{
		std::cerr << "[ERROR]" << getCurrentTime() << std::flush;

		int pos;
		char * p = (char *)file;
		memset(PrintBuffer, 0, PrintBufferSize);
		va_list args;
		va_start(args, format);
#ifdef _WIN32
		pos = vsprintf_s(PrintBuffer, PrintBufferSize, format, args);
		p = strrchr(p, DIR_SPT) + 1;
		sprintf_s(PrintBuffer + pos, PrintBufferSize, " @%s:%d", p, line);
#else
		pos = vsprintf(PrintBuffer, format, args);
		p = strrchr(p, DIR_SPT) + 1;
		sprintf(PrintBuffer + pos, " @%s:%d", p, line);
#endif
		va_end(args);
		std::cerr << std::string(PrintBuffer) << std::endl;

		return E_ReturnState::RTN_ERR;
	}
	E_ReturnState PrintError(const char *file, int line, std::string msg, ...)
	{
		std::cerr << "[ERROR]" << getCurrentTime() << msg << std::flush;

		std::string sf(file);

		std::cerr << " @" << sf.erase(0, sf.find_last_of(DIR_SPT) + 1)
			<< ":" << line << std::endl;

		return E_ReturnState::RTN_ERR;
	}

	void PrintFatal(const char *file, int line, const char * format, ...)
	{
		std::cerr << "[RTN_FATAL]" << getCurrentTime() << std::flush;

		int pos;
		char * p = (char *)file;
		memset(PrintBuffer, 0, PrintBufferSize);
		va_list args;
		va_start(args, format);
#ifdef _WIN32
		pos = vsprintf_s(PrintBuffer, PrintBufferSize, format, args);
		p = strrchr(p, DIR_SPT) + 1;
		sprintf_s(PrintBuffer + pos, PrintBufferSize, " @%s:%d", p, line);
#else
		pos = vsprintf(PrintBuffer, format, args);
		p = strrchr(p, DIR_SPT) + 1;
		sprintf(PrintBuffer + pos, " @%s:%d", p, line);
#endif
		va_end(args);
		std::cerr << std::string(PrintBuffer) << std::endl;

		exit(EXIT_FAILURE);
	}
	void PrintFatal(const char *file, int line, std::string msg, ...)
	{
		std::cerr << "[RTN_FATAL]" << getCurrentTime() << msg << std::flush;

		std::string sf(file);

		std::cerr << " @" << sf.erase(0, sf.find_last_of(DIR_SPT) + 1)
			<< ":" << line << std::endl;

		exit(EXIT_FAILURE);
	}

#define chkFStream() do{\
		if ((fstream == nullptr)||(!fstream->is_open())){\
			WARN("can't open log file");\
			fstream = nullptr;\
			return;\
		}}while(0)

	void LogFile::ensureLogDir()
	{
#ifdef _WIN32
		if (_access(log_dir.c_str(), 2) == -1)
		{
			_mkdir(log_dir.c_str());
		}
#else
		if (access(log_dir.c_str(), F_OK) == -1)
		{
			::mkdir(log_dir.c_str(), 0777);
		}
#endif
	}

	LogFile::LogFile(std::string file_name, bool isNew)
	{
		PrintBuffer = (char *)malloc(PrintBufferSize);

		log_dir = std::string(".") + DIR_SPT + "log" + DIR_SPT;
		ensureLogDir();

		this->file_name = log_dir + file_name;

		if (isNew == true)
		{
			memset(PrintBuffer, 0, PrintBufferSize);
			t = time(0);
#ifdef _WIN32
			localtime_s(&stm, &t);
			strftime(PrintBuffer, PrintBufferSize, "-%H-%M-%S", &stm);
#else
			strftime(PrintBuffer, PrintBufferSize, "-%H-%M-%S", localtime(&t));
#endif
			this->file_name += std::string(PrintBuffer) + ".log";
			fstream = new std::ofstream(this->file_name, std::ios::out | std::ios::trunc);
		}
		else
		{
			this->file_name += ".log";
			fstream = new std::ofstream(this->file_name, std::ios::out | std::ios::app);
		}

		chkFStream();
	}
	LogFile::~LogFile()
	{
		free(PrintBuffer);
		if ((fstream != nullptr) && fstream->is_open())
		{
			fstream->close();
		}
		delete fstream;
	}
	
	void LogFile::Log(const char * format, ...)
	{
		chkFStream();

		*fstream << getCurrentTime() << std::flush;

		memset(PrintBuffer, 0, PrintBufferSize);
		va_list args;
		va_start(args, format);
#ifdef _WIN32
		vsprintf_s(PrintBuffer, PrintBufferSize, format, args);
#else
		vsprintf(PrintBuffer, format, args);
#endif
		va_end(args);
		*fstream << std::string(PrintBuffer) << std::endl;
	}
	void LogFile::Log(std::string msg, ...)
	{
		chkFStream();
		*fstream << getCurrentTime() << msg << std::endl;
	}
}