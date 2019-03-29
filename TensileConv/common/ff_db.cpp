#include <memory.h>
#include <fstream>
#include <stdio.h>
#include "ff_db.h"
#include "ff_basic.h"
#include "ff_file_opt.h"

namespace feifei 
{
	Db::Db(std::string db_name)
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

	std::string Db::genKeyStr(T_SaveData sd)
	{
		char tmpc[1024];
		memset(tmpc, 0, 1024);
#ifdef _WIN32
		sprintf_s(tmpc, "A%d", sd.a);
#else
		sprintf(tmpc, "A%d", sd.a);
#endif
		return std::string(tmpc);
	}
	char Db::checkSum(std::string key, T_SaveData rcd)
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

	void Db::AppendRcd(T_SaveData rcd)
	{
		std::string key = genKeyStr(rcd);
		SaveDataMap.insert(std::pair<std::string, T_SaveData>(key, rcd));
	}
	
	void Db::SaveRcd(T_SaveData rcd)
	{
		std::string full_name = dbDirPath + dbFileName;
		std::ofstream fout(full_name.c_str(), std::ios::out | std::ios::binary | std::ios::app);
		fout.seekp(0, std::ios::end);

		std::string key = genKeyStr(rcd);
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

	void Db::ResaveDbFile()
	{
		std::string full_name = dbDirPath + dbFileName;
		std::ofstream fout(full_name.c_str(), std::ios::out | std::ios::binary);
		fout << "<db_file_start>" << std::endl;

		for (auto rcd : SaveDataMap)
		{
			SaveRcd(rcd.second);
		}
	}

	void Db::LoadDbFile()
	{
		std::string full_name = dbDirPath + dbFileName;
		std::ifstream fin(full_name.c_str(), std::ios::in | std::ios::binary);

		char tmp_key[1024];
		char a1 = 0, a2 = 0, a3 = 0;
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
