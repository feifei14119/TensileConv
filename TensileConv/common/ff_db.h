#pragma once

#include <map>

namespace feifei
{
	typedef struct SaveDataType
	{
		int N, C, H, W, K;
		bool bias; int relu;
		int PCK_order;
		int c_in_lds_atomic_group;
		int c_in_lds_split_group;
		int c_in_l2_atomic_group;
		int c_in_l2_split_group;
		int k_out_maps;
		int group_size_x;
		double elapsedSec;
	} T_SaveData;

	class Database
	{
	public:
		Database(std::string db_file);

		void LoadDbFile();
		void ResaveDbFile();
		void SaveRcd(T_SaveData rcd);
		void AppendRcd(T_SaveData rcd);

		std::map<std::string, T_SaveData> SaveDataMap;
		std::string GenKeyStr(int N, int C, int H, int W, int K, bool bias, int relu);
		std::string GenKeyStr(T_SaveData rcd);
		T_SaveData Find(std::string key);

	private:
		std::string dbDirPath;
		std::string dbFileName;

		char checkSum(std::string key, T_SaveData rcd);
	};
}