#include <memory.h>
#include <fstream>
#include <stdio.h>
#include "ff_db.h"
#include "ff_basic.h"
#include "ff_file_opt.h"

namespace feifei 
{
	Database::Database(std::string db_name)
	{
		dbDirPath = get_work_path() + DIR_SPT + "db" + DIR_SPT;
		ensure_dir(dbDirPath.c_str());
		dbFileName = db_name + ".db";

		std::string full_name = dbDirPath + dbFileName;
		std::ofstream fout(full_name.c_str(), std::ios::out | std::ios::binary | std::ios::app);
		fout.seekp(0, std::ios::end);

		size_t fSize = 0;
		fSize = (size_t)fout.tellp();
		if (fSize == 0)
			fout << "<db_file_start>" << std::endl;
		
		fout.close();
	}

	/* NEED TODO */
	std::string Database::GenKeyStr(int N, int C, int H, int W, int K, bool bias, int relu)
	{
		char tmpc[1024];
		memset(tmpc, 0, 1024);
		sprintf(tmpc, "N%02dC%04dH%04dW%04dK%04db%dr%02d", N, C, H, W, K, bias, relu);
		return std::string(tmpc);
	}
	std::string Database::GenKeyStr(T_SaveData sd)
	{
		char tmpc[1024];
		memset(tmpc, 0, 1024);
		sprintf(tmpc, "N%02dC%04dH%04dW%04dK%04db%dr%02d", 
			sd.N, sd.C, sd.H, sd.W, sd.K, sd.bias, sd.relu);
		return std::string(tmpc);
	}

	char Database::checkSum(std::string key, T_SaveData rcd)
	{
		char ck = '<';

		for (int i = 0; i < key.size(); i++)
			ck ^= key[i];
		ck ^= '>';

		char * prcd = (char*)(&rcd);
		for (int i = 0; i < sizeof(T_SaveData); i++)
			ck ^= prcd[i];

		return ck;
	}

	T_SaveData Database::Find(std::string key)
	{
		T_SaveData rcd;
		memset(&rcd, 0, sizeof(rcd));
		std::map<std::string, T_SaveData>::iterator it;
		rcd.elapsedSec = -1;
		it = SaveDataMap.find(key);
		if (it != SaveDataMap.end())
		{
			rcd = it->second;
		}
		return rcd;
	}
	void Database::AppendRcd(T_SaveData rcd)
	{
		std::string key = GenKeyStr(rcd);
		SaveDataMap.insert(std::pair<std::string, T_SaveData>(key, rcd));
	}
	
	void Database::SaveRcd(T_SaveData rcd)
	{
		std::string full_name = dbDirPath + dbFileName;
		std::ofstream fout(full_name.c_str(), std::ios::out | std::ios::binary | std::ios::app);
		fout.seekp(0, std::ios::end);

		std::string key = GenKeyStr(rcd);
		fout << '<';
		fout.write(key.c_str(), key.size());
		fout << '>';

		fout.write((char*)(&rcd), sizeof(T_SaveData));

		fout << '*';
		fout << checkSum(key, rcd);
		fout << '\n';

		fout.close();

		SaveDataMap.insert(std::pair<std::string, T_SaveData>(key, rcd));
	}

	void Database::ResaveDbFile()
	{
		std::string full_name = dbDirPath + dbFileName;
		std::ofstream fout(full_name.c_str(), std::ios::out | std::ios::binary);
		fout << "<db_file_start>" << std::endl;

		for (auto rcd : SaveDataMap)
		{
			SaveRcd(rcd.second);
		}
	}

	void Database::LoadDbFile()
	{
		std::string full_name = dbDirPath + dbFileName;
		std::ifstream fin(full_name.c_str(), std::ios::in | std::ios::binary);

		char tmp_key[1024];
		char a1 = 0, a2 = 0, a3 = 0;

		while (!fin.eof())
		{
			fin.get(a2);
			if (a2 == '<')
			{
				memset(tmp_key, 0, 1024);
				int idx = 0;

				fin.get(a2);
				while (a2 != '>')
				{
					tmp_key[idx++] = a2;
					fin.get(a2);
				}
				std::string key(tmp_key);

				if (key == "db_file_start")
					break;
			}
		}

		while (!fin.eof())
		{
			a1 = a2;
			fin.get(a2);
			if (a1 == '\n' && a2 == '<')
			{
				memset(tmp_key, 0, 1024);
				int idx = 0;

				fin.get(a2);
				while (a2 != '>')
				{
					tmp_key[idx++] = a2;
					fin.get(a2);
				}
				std::string key(tmp_key);

				T_SaveData rcd;
				char * prcd = (char*)&rcd;
				idx = 0;
				fin.get(a3); a1 = a2; a2 = a3;
				while (!fin.eof())
				{
					fin.get(a3);
					a1 = a2; a2 = a3;
					if ((a1 == '*'&&a3 == '\n')||(idx == sizeof(T_SaveData)))
						break;

					prcd[idx++] = a1;
				}

				char chk1 = a2;
				char chk2 = checkSum(key, rcd);
				if (chk1 == chk2)
				{
					SaveDataMap.insert(std::pair<std::string, T_SaveData>(key, rcd));
				}
			}
		}
	}
}
