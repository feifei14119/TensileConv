#pragma once

#include "BasicClass.h"

namespace AutoTune
{
	using namespace std;
	/************************************************************************/
	/* 搜索参数					                                             */
	/************************************************************************/
	typedef struct SearchParamType
	{
		SearchParamType(std::string name)
		{
			Name = name;
		}
		SearchParamType()
		{
		}

		std::string Name;
		std::vector<int> ValueArray;
		int CurrIdx;
		int CurrValue;
		int BestIdx;
		int BestValue;
		int MinValue;
		int MaxValue;
		int Step;
		int ValueNum;

		SearchParamType operator=(SearchParamType &p)
		{
			Name = p.Name;
			CurrValue = p.CurrValue;
			CurrIdx = p.CurrIdx;
			MinValue = p.MinValue;
			MaxValue = p.MaxValue;
			Step = p.Step;
			ValueNum = p.ValueNum;

			for (int i = 0; i < p.ValueArray.size(); i++)
			{
				int val = p.ValueArray[i];
				ValueArray.push_back(val);
			}

			return *this;
		}
	} T_SearchParam;

	class SearchSpace
	{
	public:
		SearchSpace()
		{
			searchParams = new std::vector<T_SearchParam>;
		}

		~SearchSpace()
		{
			delete searchParams;
		}

	public:
		int ParamNum = 0;

	private:
		std::vector<T_SearchParam> * searchParams;	// 搜索参数数组列表

		int searchParamIdx = 0;
		bool moveCurrIdx = true;
		int getParamIdx = 0;

	public:
		/************************************************************************/
		/* 获取一组新的参数组合													*/
		/************************************************************************/
		E_ReturnState GetNexComb()
		{
			T_SearchParam * currParam;
			currParam = &((*searchParams)[searchParamIdx]);

			// 遍历完成: 如果已经指向最后一个参数且仍需调整指针,则搜索完成
			if ((searchParamIdx >= ParamNum - 1) && (currParam->CurrIdx >= currParam->ValueNum - 1) && moveCurrIdx)
			{
				moveCurrIdx = true;
				searchParamIdx = 0;
				return E_ReturnState::FAIL;
			}

			// 调整当前数组指针
			bool moveNextIdx;
			if (moveCurrIdx)
			{
				if (currParam->CurrIdx >= currParam->ValueNum - 1)
				{
					currParam->CurrIdx = 0;
					moveNextIdx = true;
				}
				else
				{
					currParam->CurrIdx++;
					moveNextIdx = false;
				}

				currParam->CurrValue = currParam->ValueArray[currParam->CurrIdx];
			}

			// 搜索完一轮完成: 当前正在搜索最后一个参数
			if (searchParamIdx >= ParamNum - 1)
			{
				moveCurrIdx = true;
				searchParamIdx = 0;
				return E_ReturnState::SUCCESS;
			}

			// 搜索下一组参数
			searchParamIdx++;
			moveCurrIdx = moveNextIdx;
			GetNexComb();
		}

		/************************************************************************/
		/* 记录当前参数组合														*/
		/************************************************************************/
		E_ReturnState RecordBestComb()
		{
			for (int i = 0; i < ParamNum; i++)
			{
				(*searchParams)[i].BestIdx = (*searchParams)[i].CurrIdx;
				(*searchParams)[i].BestValue = (*searchParams)[i].CurrValue;
			}
		}

		/************************************************************************/
		/* 添加一组新的参数列表													*/
		/************************************************************************/
		E_ReturnState AddOneParam(T_SearchParam * param)
		{
			T_SearchParam *newParam = new T_SearchParam();
			*newParam = *param;

			if (newParam->ValueArray.size() == 0)
			{
				if (newParam->Step == 0)
				{
					return E_ReturnState::FAIL;
				}

				int len = (int)ceil((newParam->MaxValue - newParam->MinValue) / newParam->Step);

				if (len <= 0)
				{
					return E_ReturnState::FAIL;
				}

				int val = newParam->MinValue;
				for (int i = 0; i < len; i++)
				{
					newParam->ValueArray.push_back(val);
					val + newParam->Step;
				}
			}

			newParam->CurrIdx = 0;
			newParam->CurrValue = newParam->ValueArray[0];
			newParam->ValueNum = newParam->ValueArray.size();

			searchParams->push_back(*newParam);
			ParamNum++;

			return E_ReturnState::SUCCESS;
		}

		/************************************************************************/
		/* 获取下一个参数															*/
		/************************************************************************/
		T_SearchParam * GetOneParam()
		{
			if (searchParams == NULL)
			{
				getParamIdx = 0;
				return NULL;
			}

			if (getParamIdx >= searchParams->size())
			{
				getParamIdx = 0;
				return NULL;
			}

			getParamIdx++;
			return &(*searchParams)[getParamIdx - 1];
		}
	};
}
