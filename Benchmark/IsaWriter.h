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
	std::map<std::string, t_operator*> *OperatorMap;
	
	std::string getOpter(t_operator * opter, int len = 1)
	{
		if (opter->type == e_opType::OP_SGPR)
		{
			if (len == 1)
				return std::string("s[" + d2s(opter->sgpr.gprIdx) + "]");
			else
				return std::string("s[" + d2s(opter->sgpr.gprIdx) + ":" + d2s(opter->sgpr.gprIdx + len - 1) + "]");
		}
		else if (opter->type == e_opType::OP_VGPR)
		{
			if (len == 1)
				return std::string("v[" + d2s(opter->vgpr.gprIdx) + "]");
			else
				return std::string("v[" + d2s(opter->vgpr.gprIdx) + ":" + d2s(opter->vgpr.gprIdx + len - 1) + "]");
		}
		else if (opter->type == e_opType::OP_IMM)
		{
			return std::string(d2s(opter->imm.value));
		}
	}
	std::string getOpter(std::string name, int len = 1)
	{
		if ((name == "vcc") || (name == "vcc_hi") || (name == "vcc_lo") ||
			(name == "exec") || (name == "exec_hi") || (name == "exec_lo")||
			(name == "off"))
		{
			return name;
		}
	}
	std::string getOpter(int immVal, int len = 1)
	{
		return d2s(immVal);
	}

	template <typename T1, typename T2, typename T3>
	void s_load_dword(int num, T1 s_dst, T2 s_base, T3 offset)
	{
		int tmpIdx;
		std::string str = sblk();
		str.append("s_load_dword");
		if (num > 1)
		{
			str.append("x" + d2s(num));
		}
		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");

		if (num == 1)
		{
			str.append(getOpter(s_dst));
		}
		else
		{
			str.append(getOpter(s_dst, num));
		}
		str.append(", ");
		str.append(getOpter(s_base,2));
		str.append(", ");
		str.append(getOpter(offset));

		str.append("\n");
		kernelStr->append(str);
	}
	template <typename T1, typename T2, typename T3>
	void s_store_dword(int num, T1 s_dst, T2 s_base, T3 offset)
	{
		int tmpIdx;
		std::string str = sblk();
		str.append("s_store_dword");
		if (num > 1)
		{
			str.append("x" + d2s(num));
		}
		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");

		if (num == 1)
		{
			str.append(getOpter(s_dst));
		}
		else
		{
			str.append(getOpter(s_dst, num));
		}
		str.append(", ");
		str.append(getOpter(s_base, 2));
		str.append(", ");
		str.append(getOpter(offset));

		str.append("\n");
		kernelStr->append(str);
	}
	
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

	void inst0(
		std::string op,
		std::string common)
	{
		int tmpIdx;

		std::string str = sblk();
		str.append(op);

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
