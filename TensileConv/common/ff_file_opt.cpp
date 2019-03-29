#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <string>
#include <fstream>
#include <vector>

#include "ff_basic.h"
#include "ff_log.h"
#include "ff_file_opt.h"

namespace feifei
{
	void ensure_dir(const char * dir)
	{
#ifdef _WIN32
		if (_access(dir, 2) == -1)
		{
			_mkdir(dir);
		}
#else
		if (access(dir, F_OK) == -1)
		{
			::mkdir(dir, 0777);
		}
#endif
	}

	void exec_cmd(std::string cmd)
	{
#ifdef _WIN32
		system(cmd.c_str());
#else
		FILE * pfp = popen(cmd.c_str(), "r");
		auto status = pclose(pfp);
		WEXITSTATUS(status);
#endif
	}
	
	std::string work_path = "";
	std::string get_work_path()
	{
		if (work_path == "")
		{
#ifdef _WIN32
			work_path = ".\\";
#else
			work_path = "./";
#endif
		}

		return work_path;
	}
	void set_work_path(std::string path)
	{
		work_path = path;
		ensure_dir(work_path.c_str());
	}

	E_ReturnState dump2_bin_file(std::string file_name, std::vector<char> *binary)
	{
		std::ofstream fout(file_name.c_str(), std::ios::out | std::ios::binary);
		if (!fout.is_open())
		{
			ERR("can't open save file: " + file_name);
		}
		fout.write(binary->data(), binary->size());
		fout.close();
		return E_ReturnState::SUCCESS;
	}

	E_ReturnState dump2_txt_file(std::string file_name, std::string str)
	{
		std::ofstream fout(file_name.c_str(), std::ios::out|std::ios::ate);
		if (!fout.is_open())
		{
			ERR("can't open save file: " + file_name);
		}
		fout.write(str.c_str(), str.length());
		fout.close();
		return E_ReturnState::SUCCESS;
	}

	size_t read_bin_file(std::string file_name, char * binary)
	{
		std::ifstream fin(file_name.c_str(), std::ios::in | std::ios::binary);
		if (!fin.is_open())
		{
			WARN("can't open bin file: " + file_name);
			return 0;
		}

		size_t binSize;
		fin.seekg(0, std::ios::end); 
		binSize = (size_t)fin.tellg();

		fin.seekg(0, std::ios::beg);
		fin.read(binary, binSize);

		fin.close();
		return binSize;
	}

	std::string get_curr_path()
	{
#ifdef _WIN32
		char * buf = _getcwd(nullptr, 0);
#else
		char * buf = getcwd(nullptr, 0);
#endif
		std::string path(buf);
		free(buf);
		return path;
	}

	std::string get_file_path(std::string fileName)
	{
		size_t p;
		p = fileName.rfind(DIR_SPT);
		return fileName.substr(0, p);
	}

	std::string get_file_name(std::string fileName)
	{
		size_t p1, p2;
		p1 = fileName.rfind(DIR_SPT);
		p2 = fileName.rfind(".");
		return fileName.substr(p1 + 1, p2 - p1 - 1);
	}
}
