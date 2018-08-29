#pragma once
#include "BasicClass.h" 


class IsaWriterBase
{
private:
#define	PARAM_START_COL		(44)
#define	FLAG_START_COL		(85)
#define	COMMON_START_COL	(109)

public:
	int *tableCnt;
	std::string *kernelStr;

	/*void global_load(
		int dNum,
		std::string vDat,
		std::string vAddr,
		std::string sAddr,
		int offset,
		std::string common)
	{
		int tmpIdx;

		std::string str = sblk();
		str.append("global_load_dword");
		if (dNum > 1)
		{
			str.append("x");
			str.append(d2s(dNum));
		}

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");
		str.append(vDat);
		str.append(", ");
		str.append(vAddr);
		str.append(", ");
		str.append(sAddr);

		if (offset > 0)
		{
			tmpIdx = FLAG_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");
			str.append("offset:0x0+");
			str.append(d2s(offset));
		}

		if (common != "")
		{
			tmpIdx = COMMON_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");
			str.append("\\\\ ");
			str.append(common);
		}

		str.append("\n");
		kernelStr->append(str);
	}*/
	
	void s_waitcnt(std::string type, std::string cnt)
	{
		int tmpIdx;

		std::string str = sblk();
		str.append("s_waitcnt");

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");

		str.append(type);
		str.append("(" + cnt + ")");

		str.append("\n");
		kernelStr->append(str);
	}
	void s_waitcnt(std::string type, int cnt)
	{
		int tmpIdx;

		std::string str = sblk();
		str.append("s_waitcnt");

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");

		str.append(type);
		str.append("(" + d2s(cnt) + ")");

		str.append("\n");
		kernelStr->append(str);
	}

	void inst1(
		std::string op,
		std::string src0,
		std::string common)
	{
		int tmpIdx;

		std::string str = sblk();
		str.append(op);

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");
		str.append(src0);

		if (common != "")
		{
			tmpIdx = COMMON_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");
			str.append("\\\\ ");
			str.append(common);
		}

		str.append("\n");
		kernelStr->append(str);
	}

	void inst2(
		std::string op,
		std::string dst,
		std::string src0,
		std::string common)
	{
		int tmpIdx;

		std::string str = sblk();
		str.append(op);

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");
		str.append(dst);
		str.append(", ");
		str.append(src0);

		if (common != "")
		{
			tmpIdx = COMMON_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");
			str.append("\\\\ ");
			str.append(common);
		}

		str.append("\n");
		kernelStr->append(str);
	}

	void inst3(
		std::string op,
		std::string dst,
		std::string src0,
		std::string src1,
		std::string common,
		int offset = 0)
	{
		int tmpIdx;

		std::string str = sblk();
		str.append(op);

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");
		str.append(dst);
		str.append(", ");
		str.append(src0);
		str.append(", ");
		str.append(src1);

		if (offset > 0)
		{
			tmpIdx = FLAG_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");
			str.append("offset:");
			str.append(d2s(offset));
		}

		if (common != "")
		{
			tmpIdx = COMMON_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");
			str.append("\\\\ ");
			str.append(common);
		}

		str.append("\n");
		kernelStr->append(str);
	}

	void inst4(
		std::string op,
		std::string dst,
		std::string src0,
		std::string src1,
		std::string src2,
		std::string common)
	{
		int tmpIdx;

		std::string str = sblk();
		str.append(op);

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");
		str.append(dst);
		str.append(", ");
		str.append(src0);
		str.append(", ");
		str.append(src1);
		str.append(", ");
		str.append(src2);

		if (common != "")
		{
			tmpIdx = COMMON_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");
			str.append("\\\\ ");
			str.append(common);
		}

		str.append("\n");
		kernelStr->append(str);
	}
	
	void inst5(
		std::string op,
		std::string dst,
		std::string src0,
		std::string src1,
		std::string src2,
		std::string src3,
		std::string common)
	{
		int tmpIdx;

		std::string str = sblk();
		str.append(op);

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");
		str.append(dst);
		str.append(", ");
		str.append(src0);
		str.append(", ");
		str.append(src1);
		str.append(", ");
		str.append(src2);
		str.append(", ");
		str.append(src3);

		if (common != "")
		{
			tmpIdx = COMMON_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");
			str.append("\\\\ ");
			str.append(common);
		}

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
