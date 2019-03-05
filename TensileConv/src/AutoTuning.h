#pragma once

#include <math.h>
#include <algorithm>
#include "../common/ff_utils.h"

namespace TensileConv{
namespace AutoTune{

typedef enum SearchMethordEnum
{
	SEARCH_NONE = 0,
	SEARCH_AUTO = 1,	// 如果有记录,则不进行搜索
	SEARCH_BRUTE = 2,
	SEARCH_GENETIC = 3
} E_SearchMethord;
/************************************************************************/
/* 搜索参数					                                            */
/************************************************************************/
typedef struct SearchParamType
{
	SearchParamType(std::string name = "")
	{
		Name = name;
	}

	std::string Name;

	int MinValue;
	int MaxValue;
	int Step;
	int ValueNum;
	std::vector<int> ValueArray;

	int CurrIdx;
	int CurrValue;
	int BestIdx;
	int BestValue;

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

/************************************************************************/
/* 搜索基类																*/
/************************************************************************/
class SearchSpaceBase
{
protected:
	E_SearchMethord searchMethod;
	std::vector<T_SearchParam*> * searchParams;	// 搜索参数数组列表
	int paramNumber = 0;
	int paramCombNum = 1;
	int searchedCombNum = 1;
	int checkedCombNum = 1;

public:
	SearchSpaceBase() { searchParams = new std::vector<T_SearchParam*>; }
	~SearchSpaceBase() { delete searchParams; }
	int ParamNum() { return paramNumber; }
	int ParamCombNum() { return paramCombNum; }
	int SearchedCombNum() { return searchedCombNum; }
	int CheckedCombNum() { return checkedCombNum; }
	E_SearchMethord SearchMethod() { return searchMethod; }
	std::vector<T_SearchParam*> * SearchParams() { searchedCombNum++; return searchParams; }

	// 初始化搜索
	virtual void InitSearching() 
	{
		searchedCombNum = 0;
		checkedCombNum = 0; 
	}
	// 添加一组新的参数列表
	virtual E_ReturnState AddOneSearchParam(T_SearchParam * param) = 0;
	// 产生一组新的参数组合
	virtual E_ReturnState GenerateNextComb() = 0;
	// 记录当前参数组合
	void RecordCurrComb()
	{
		for (T_SearchParam * param : *searchParams)
		{
			param->BestIdx = param->CurrIdx;
			param->BestValue = param->CurrValue;
		}
	}
	// 设置当前组合的分数
	virtual void SetOneCombScore(double value) { checkedCombNum++; };
};

/************************************************************************/
/* 暴力搜索																*/
/************************************************************************/
class BruteSearch : public SearchSpaceBase
{
private:
	int searchParamIdx;
	bool moveCurrIdx;
	int getParamIdx;

public:
	BruteSearch() : SearchSpaceBase() { searchMethod = E_SearchMethord::SEARCH_BRUTE; };
	
	void InitSearching()
	{
		searchParamIdx = 0;
		moveCurrIdx = true;
		getParamIdx = 0;
	}
	E_ReturnState AddOneSearchParam(T_SearchParam * param)
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

		searchParams->push_back(newParam);
		paramNumber++;
		paramCombNum *= newParam->ValueNum;

		return E_ReturnState::SUCCESS;
	}
	E_ReturnState GenerateNextComb()
	{
		T_SearchParam * currParam;
		currParam = (*searchParams)[searchParamIdx];

		// 遍历完成: 如果已经指向最后一个参数且仍需调整指针,则搜索完成
		if ((searchParamIdx >= paramNumber - 1) && (currParam->CurrIdx >= currParam->ValueNum - 1) && moveCurrIdx)
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
		if (searchParamIdx >= paramNumber - 1)
		{
			moveCurrIdx = true;
			searchParamIdx = 0;
			return E_ReturnState::SUCCESS;
		}

		// 搜索下一组参数
		searchParamIdx++;
		moveCurrIdx = moveNextIdx;
		GenerateNextComb();
	}
};

/************************************************************************/
/* 遗传算法搜索															*/
/************************************************************************/
class GeneticSearch : SearchSpaceBase
{
private:
#define SURV_CHANCE		(0.7) // 10个里面存活7个
#define CORSS_CHANCE	(0.8)
#define MUTATE_CHANCE	(0.1)
#define POP_SIZE		(10) // 每代种群10个个体
#define MAX_GENERATION	(50) // 50代种群

	// 一个基因位点
	typedef struct GeneLocusType
	{
		std::string Name;
		int Idx;
		int Val;
	} T_GeneLocus;
	// 一条染色体即一个个体,包含多个基因位点
	typedef struct ChromosomeType
	{
		std::vector<T_GeneLocus> Chrom;
	} T_Chromosome;
	std::vector<T_SearchParam*> * genePool;	// 基因库,即搜索参数数组列表
	std::vector<T_Chromosome> population;
	int currObjChromIdx = 0;

	// 为一个基因位点随机生成一个值
	void genRandGene(T_SearchParam *pGene)
	{
		pGene->CurrIdx = randInt10(0, pGene->ValueNum);
		pGene->CurrValue = pGene->ValueArray[pGene->CurrIdx];
	}
	// 为一个个体随机生成一条染色体
	void genRandChrom()
	{
		for (T_SearchParam *pGene : *genePool)
		{
			genRandGene(pGene);
		}
	}
	
	// 种群评价
	std::vector<double> populationValue;
	void setObjChromValue(double value)
	{
		populationValue[currObjChromIdx] = value;
		for (int loci = 0; loci < genePool->size(); loci++)
		{
			population[currObjChromIdx].Chrom[loci].Name = (*genePool)[loci]->Name;
			population[currObjChromIdx].Chrom[loci].Idx = (*genePool)[loci]->CurrIdx;
			population[currObjChromIdx].Chrom[loci].Val = (*genePool)[loci]->CurrValue;
		}
	}

	// 种群的个体存活
	void populationFit()
	{
		int thresholdPos;
		double thresholdValue;

		std::sort(populationValue.begin(), populationValue.end());
		thresholdPos = int(POP_SIZE * SURV_CHANCE);
		thresholdValue = populationValue[thresholdPos];

		for (int i = 0; i < POP_SIZE; i++)
		{
			if (populationValue[i] >= thresholdValue)
			{
				populationValue[i] = 0;
			}
		}
	}

	// 种群存活概率	
	std::vector<double> surviveChance;
	std::vector<double> rouletteChance;
	void populationChance()
	{
		double totalValue = 0;
		for (int i = 0; i < POP_SIZE; i++)
		{
			totalValue += populationValue[i];
		}

		// 存活概率
		for (int i = 0; i < POP_SIZE; i++)
		{
			surviveChance[i] = populationValue[i] / totalValue;
		}

		// 转盘概率
		for (int objIdx = 0; objIdx < POP_SIZE; objIdx++)
		{
			double sum_chance = 0;
			for (int i = 0; i <= objIdx; i++)
			{
				sum_chance += surviveChance[i];
			}
			rouletteChance[objIdx] = sum_chance;
		}
	}

	// 种群选择
	std::vector<T_Chromosome> newPopulation;
	void sellectPopulation()
	{
		double chance = 0;
		for (int newIdx = 0; newIdx < POP_SIZE; newIdx++)
		{
			chance = 1.0 * rand() / RAND_MAX;

			for (int oldIdx = 0; oldIdx < POP_SIZE; oldIdx++)
			{
				if (chance < rouletteChance[oldIdx])
				{
					newPopulation[newIdx] = population[oldIdx];
					break;
				}
			}
		}
	}

	// 基因杂交
	void crossover()
	{
		double chance = 0;
		int cpyLen = 0;
		T_GeneLocus tempGene;

		for (int objIdx = 0; objIdx < POP_SIZE - 1; objIdx++)
		{
			chance = 1.0 * rand() / RAND_MAX;
			if (chance < CORSS_CHANCE)
			{
				cpyLen = (int)randInt10(0, genePool->size());
				for (int geneIdx = cpyLen; geneIdx < genePool->size(); geneIdx++)
				{
					tempGene = newPopulation[objIdx].Chrom[geneIdx];
					newPopulation[objIdx].Chrom[geneIdx] = newPopulation[objIdx + 1].Chrom[geneIdx];
					newPopulation[objIdx + 1].Chrom[geneIdx] = tempGene;
				}
			}
		}
	}

	// 基因突变
	void mutation()
	{
		double chance = 0;
		int mutPos = 0;

		for (int objIdx = 0; objIdx < POP_SIZE - 1; objIdx++)
		{
			chance = 1.0 * rand() / RAND_MAX;
			if (chance < MUTATE_CHANCE)
			{
				mutPos = (int)randInt10(0, genePool->size());
				genRandGene((*genePool)[mutPos]);
				newPopulation[objIdx].Chrom[mutPos].Idx = (*genePool)[mutPos]->CurrIdx;
				newPopulation[objIdx].Chrom[mutPos].Val = (*genePool)[mutPos]->CurrValue;
			}
		}
	}
	
public:
	GeneticSearch() 
	{ 
		genePool = searchParams;
		searchMethod = E_SearchMethord::SEARCH_GENETIC; 
		paramCombNum = POP_SIZE * MAX_GENERATION;
	}
	~GeneticSearch() 
	{
		delete &population;
		delete &newPopulation;
		delete &populationValue;
		delete &surviveChance;
		delete &rouletteChance;
	}

	void InitSearching()
	{
		// 初始化种群数量
		population.resize(POP_SIZE);
		newPopulation.resize(POP_SIZE);
		populationValue.resize(POP_SIZE);
		surviveChance.resize(POP_SIZE);
		rouletteChance.resize(POP_SIZE);
		currObjChromIdx = 0;

		// 初始化染色体长度
		for (int objIdx = 0; objIdx < population.size(); objIdx++)
		{
			population[objIdx].Chrom.resize(genePool->size());
			newPopulation[objIdx].Chrom.resize(genePool->size());
		}
	}

	E_ReturnState AddOneSearchParam(T_SearchParam * param)
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

		genePool->push_back(newParam);

		return E_ReturnState::SUCCESS;
	}
	E_ReturnState GenerateNextComb()
	{
		if (checkedCombNum >= POP_SIZE * MAX_GENERATION)
			return E_ReturnState::FAIL;

		genRandChrom();
		return E_ReturnState::SUCCESS;
	}
	void SetOneCombScore(double value)
	{
		SearchSpaceBase::SetOneCombScore(value);

		setObjChromValue(value);

		// 指向下一个种群个体
		currObjChromIdx++;

		// 如果当代种群个体遍历完成，则进行种群进化
		if (currObjChromIdx == POP_SIZE)
		{
			currObjChromIdx = 0;
			populationFit();
			populationChance();
			sellectPopulation();
			crossover();
			mutation();
			population = newPopulation;
		}
	}
};
}
}
