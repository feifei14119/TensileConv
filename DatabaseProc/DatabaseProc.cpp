// DatabaseProc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <iostream>
#include <fstream>
#include <map> 

typedef struct SaveParamStruct
{
	char key[30];
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
} T_SaveParamOld;
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

std::string oldDbFileName;
std::string newDbFileName;
std::string dbFileName;
std::string txtFileName;
std::map<std::string, T_SaveData> SaveDataMap;
std::map<std::string, T_SaveParamOld> OldDataMap;

char checkSum(std::string key, T_SaveData rcd)
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

std::string GenOldKeyStr(T_SaveParamOld d)
{
	char tmpc[1024];
	memset(tmpc, 0, 1024);
	sprintf_s(tmpc, "N%04dC%04dH%04dW%04dK%04db%dr%02d",
		d.N, d.C, d.H, d.W, d.K, (int)d.bias, d.relu);
	return std::string(tmpc);
}
std::string GenKeyStr(T_SaveData sd)
{
	char tmpc[1024];
	memset(tmpc, 0, 1024);
	sprintf_s(tmpc, "N%02dC%04dH%04dW%04dK%04db%dr%02d",
		sd.N, sd.C, sd.H, sd.W, sd.K, sd.bias, sd.relu);
	return std::string(tmpc);
}

void loadOldDbFile()
{
	std::ifstream fin(oldDbFileName.c_str(), std::ios::in | std::ios::binary);

	unsigned int fileSize;
	unsigned int rcdNum;
	unsigned int rcdSize = sizeof(T_SaveParamOld);

	fin.seekg(0, std::ios::end);
	fileSize = (size_t)fin.tellg();
	fin.seekg(0, std::ios::beg);

	int t = 0;
	char a1 = 0, a2 = 0;
	while (!fin.eof())
	{
		t++;
		a1 = a2;
		fin.get(a2);
		if (a1 == '\n' && a2 == 'N')
			break;
		if (a1 == 0 && a2 == 'N')
			break;
	}

	fileSize -= (t - 1);
	fin.seekg(t - 1, std::ios::beg);
	rcdNum = fileSize / rcdSize;

	for (int i = 0; i < rcdNum; i++)
	{
		T_SaveParamOld * pSaveParam = new T_SaveParamOld;
		fin.read((char*)pSaveParam, rcdSize);
		std::string key = GenOldKeyStr(*pSaveParam);
		if(key == pSaveParam->key)
			OldDataMap.insert(std::pair<std::string, T_SaveParamOld>(pSaveParam->key, *pSaveParam));
	}

	fin.close();
}
void LoadDbFile()
{
	std::ifstream fin(dbFileName.c_str(), std::ios::in | std::ios::binary);

	char tmp_key[1024];
	char a = 0, a1 = 0, a2 = 0, a3 = 0;

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
				if ((a1 == '*'&&a3 == '\n') || (idx == sizeof(T_SaveData)))
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

void SaveNewDbFile()
{
	std::ofstream fout(newDbFileName.c_str(), std::ios::out | std::ios::binary);
	fout << "<db_file_start>" << std::endl;

	for (auto rcd : OldDataMap)
	{
		T_SaveParamOld d1 = rcd.second;
		T_SaveData d;
		d.N = d1.N;
		d.C = d1.C;
		d.H = d1.H;
		d.W = d1.W;
		d.K = d1.K;
		d.bias = d1.bias;
		d.relu = d1.relu;
		d.PCK_order = d1.PCK_order;
		d.c_in_l2_atomic_group = d1.c_in_l2_atomic_group;
		d.c_in_l2_split_group = d1.c_in_l2_split_group;
		d.c_in_lds_atomic_group = d1.c_in_lds_atomic_group;
		d.c_in_lds_split_group = d1.c_in_lds_split_group;
		d.k_out_maps = d1.k_out_maps;
		d.group_size_x = d1.group_size_x;
		d.elapsedSec = d1.elapsedSec;

		std::string key = GenKeyStr(d);
		fout << '<';
		fout.write(key.c_str(), key.size());
		fout << '>';

		fout.write((char*)(&d), sizeof(T_SaveData));

		fout << '*';
		fout << checkSum(key, d);
		fout << '\n';
	}

	fout.close();
}
void SaveTxtFile()
{
	std::ofstream fout(txtFileName.c_str(), std::ios::out);

	for (auto rcd : SaveDataMap)
	{
		T_SaveData d = rcd.second;
		
		// 只翻译有效项目,便于后继处理
		std::cout << d.N << "," << d.C << "," << d.H << "," << d.W << "," << d.K << "," <<
			d.bias << "," << d.relu << "," <<
			d.PCK_order << "," << d.c_in_lds_split_group << "," << d.c_in_l2_split_group << "," <<
			d.k_out_maps << "," << d.group_size_x << "," << d.elapsedSec << std::endl;

		fout << d.N << "," << d.C << "," << d.H << "," << d.W << "," << d.K << "," <<
			d.bias << "," << d.relu << "," <<
			d.PCK_order << "," << d.c_in_lds_split_group << "," << d.c_in_l2_split_group << "," <<
			d.k_out_maps << "," << d.group_size_x << "," << d.elapsedSec << std::endl;
	}
}

int main()
{
	oldDbFileName = ".\\conv1x1_fwd_dir_gfx803";
	newDbFileName = ".\\conv1x1_fwd_dir_gfx803.db";
	loadOldDbFile();
	SaveNewDbFile();

	dbFileName = ".\\conv1x1_fwd_dir_gfx803.db";
	txtFileName = ".\\conv1x1_fwd_dir_gfx803.txt";
	LoadDbFile();
	SaveTxtFile();
}
