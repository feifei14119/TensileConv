#pragma once

#include <map>

namespace feifei
{
	typedef struct SaveDataType
	{
		int a;
		float b;
		char c;
		double d;
	} T_SaveData;

	class Db
	{
	public:
		Db(std::string db_file);

		void LoadDbFile();
		void ResaveDbFile();
		void SaveRcd(T_SaveData rcd);
		void AppendRcd(T_SaveData rcd);

		std::map<std::string, T_SaveData> SaveDataMap;

	private:
		std::string dbDirPath;
		std::string dbFileName;

		std::string genKeyStr(T_SaveData rcd);
		char checkSum(std::string key, T_SaveData rcd);
	};
}