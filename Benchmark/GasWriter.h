#pragma once

#include "BasicClass.h"

class GasWriter
{
private:
#define	PARAM_START_COL		(44)
#define	FLAG_START_COL		(85)
#define	COMMON_START_COL	(109)

public:
	int *tableCnt;
	std::string *kernelStr;

	/************************************************************************/
	/* if...else...															*/
	/************************************************************************/
	void sIF(std::string param1, std::string op, std::string param2)
	{
		std::string str = sblk();
		str.append(".if (");
		str.append(param1);
		str.append(" " + op + " ");
		str.append(param2);
		str.append(")\n");

		(*tableCnt)++;
		kernelStr->append(str);
	}
	void sELSE()
	{
		(*tableCnt)--;
		std::string str = sblk();
		str.append(".else\n");
		(*tableCnt)++;
		kernelStr->append(str);
	}
	void eIF()
	{
		(*tableCnt)--;
		std::string str = sblk();
		str.append(".endif\n");
		kernelStr->append(str);
	}

	/************************************************************************/
	/* for 循环																*/
	/************************************************************************/
	void sFOR(int loop)
	{
		std::string str = sblk();
		str.append(".rept(");
		str.append(d2s(loop));
		str.append(")\n");

		(*tableCnt)++;
		kernelStr->append(str);
	}
	void eFOR()
	{
		(*tableCnt)--;
		std::string str = sblk();
		str.append(".endr\n");
		kernelStr->append(str);
	}

	/************************************************************************/
	/* 定义函数																*/
	/************************************************************************/
	void sFUNC(std::string name, int parCnt, ...)
	{
		int tmpIdx;
		std::string str = sblk();
		str.append(".macro ");
		str.append(name);

		if (parCnt > 0)
		{
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			va_list args;
			char * arg;
			va_start(args, parCnt);

			for (int i = 0; i < parCnt - 1; i++)
			{
				arg = va_arg(args, char*);
				str.append(arg);
				str.append(", ");
			}
			arg = va_arg(args, char*);
			str.append(arg);

			va_end(args);
		}

		str.append("\n");

		(*tableCnt)++;
		kernelStr->append(str);
	}
	void eFUNC()
	{
		(*tableCnt)--;
		std::string str = sblk();
		str.append(".endm\n\n");
		kernelStr->append(str);
	}
	void FUNC(std::string func, int parCnt, ...)
	{
		int tmpIdx;

		std::string str = sblk();
		str.append(func);

		if (parCnt > 0)
		{
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			va_list args;
			char * arg;
			va_start(args, parCnt);

			for (int i = 0; i < parCnt - 1; i++)
			{
				arg = va_arg(args, char*);
				str.append(arg);
				str.append(", ");
			}
			arg = va_arg(args, char*);
			str.append(arg);

			va_end(args);
		}

		str.append("\n");

		kernelStr->append(str);
	}

	/************************************************************************/
	/* 变量设置																*/
	/************************************************************************/
	void refGPR(std::string tgpr, std::string gpr)
	{
		std::string str = sblk();
		str.append(tgpr);
		str.append(" = \\");
		str.append(gpr);
		str.append("\n");
		kernelStr->append(str);
	}
	void setGPR(std::string tgpr, int idx)
	{
		std::string str = sblk();
		str.append(tgpr);
		str.append(" = ");
		str.append(d2s(idx));
		str.append("\n");
		kernelStr->append(str);
	}

protected:
	std::string sblk()
	{
		std::string str = "";
		for (int i = 0; i < *tableCnt; i++)
		{
			str.append("    ");
		}
		return str;
	}
};
