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
		if (opter->type == E_OpterType::OPTER_SGPR)
		{
			if (len == 1)
				return std::string("s[" + d2s(opter->sgpr.gprIdx) + "]");
			else
				return std::string("s[" + d2s(opter->sgpr.gprIdx) + ":" + d2s(opter->sgpr.gprIdx + len - 1) + "]");
		}
		else if (opter->type == E_OpterType::OPTER_VGPR)
		{
			if (len == 1)
				return std::string("v[" + d2s(opter->vgpr.gprIdx) + "]");
			else
				return std::string("v[" + d2s(opter->vgpr.gprIdx) + ":" + d2s(opter->vgpr.gprIdx + len - 1) + "]");
		}
		else if (opter->type == E_OpterType::OPTER_IMM)
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

	/************************************************************************************/
	/* SMEM																				*/
	/************************************************************************************/
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

	/************************************************************************************/
	/* MUBUF																			*/
	/************************************************************************************/
	void setBufferDsc(
		t_operator * des, 
		t_operator * base,
		uint stride,
		uint record_num,
		uint num_fmt, uint dat_fmt,
		bool add_tid_en)
	{
		// desc0
		op1("s_mov_b64", des, base);

		// desc1
		uint dsc1_tmp = stride & 0x3FFF;
		dsc1_tmp = dsc1_tmp << 16;
		op2("s_or_b32", *des + 1, *des + 1, dsc1_tmp);

		// desc2
		op1("s_mov_b32", *des + 2, record_num);

		// desc3
		uint dsc3_tmp = (num_fmt & 0x7) << 12;
		if (add_tid_en == true)
		{
			dsc3_tmp |= ((stride & 0x3C000) >> 14) << 15;
			dsc3_tmp |= 1 << 23;
		}
		else
		{
			dsc3_tmp |= (dat_fmt & 0xF) << 15;
		}
		op1("s_mov_b32", *des + 3, dsc3_tmp);
	}

	template <typename T1, typename T2, typename T3, typename T4>
	void buffer_load_dword(int num, T1 v_dst, T2 v_off_idx, T3 s_dsc, T4 s_base_off,
		bool idx_en, bool off_en, uint i_offset)
	{
		int tmpIdx;
		std::string str = sblk();
		str.append("buffer_load_dword");
		if (num > 1)
		{
			str.append("x" + d2s(num));
		}
		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");
		
		// dest_data
		if (num == 1)
		{
			str.append(getOpter(v_dst));
		}
		else
		{
			str.append(getOpter(v_dst, num));
		}

		// v_index & v_offset
		str.append(", ");
		if ((idx_en == false) && (off_en == false))
		{
			str.append(getOpter("off"));
		}
		else if ((idx_en == false) && (off_en == true))
		{
			str.append(getOpter(v_off_idx));
		}
		else if ((idx_en == true) && (off_en == false))
		{
			str.append(getOpter(v_off_idx));
		}
		else if ((idx_en == true) && (off_en == true))
		{
			str.append(getOpter(v_off_idx, 2));
		}

		// s_buffer_descripter
		str.append(", ");
		str.append(getOpter(s_dsc, 4));

		// s_base_offset
		str.append(", ");
		str.append(getOpter(s_base_off));

		if (idx_en == true)
		{
			str.append(" ");
			str.append("idxen");
		}
		if (off_en == true)
		{
			str.append(" ");
			str.append("offen");
		}

		// inst_offset
		str.append(" ");
		str.append("offset:" + d2s(i_offset));

		str.append("\n");
		kernelStr->append(str);
	}

	/************************************************************************************/
	/* sopÍ¨ÓÃ²Ù×÷																		*/
	/************************************************************************************/
	template <typename T1, typename T2>
	void op1(std::string op, T1 dst, T2 src)
	{
		int tmpIdx;
		std::string str = sblk();
		str.append(op);

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");

		if ((op[op.length() - 2] == '6') && (op[op.length() - 1] == '4'))
			str.append(getOpter(dst,2));
		else
			str.append(getOpter(dst));

		str.append(", ");
		if ((op[op.length() - 2] == '6') && (op[op.length() - 1] == '4'))
			str.append(getOpter(src, 2));
		else
			str.append(getOpter(src));

		str.append("\n");
		kernelStr->append(str);
	}
	template <typename T1, typename T2, typename T3>
	void op2(std::string op, T1 dst, T2 src0, T3 src1)
	{
		int tmpIdx;
		std::string str = sblk();
		str.append(op);

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");

		str.append(getOpter(dst));

		str.append(", ");
		str.append(getOpter(src0));
		str.append(", ");
		str.append(getOpter(src1));

		str.append("\n");
		kernelStr->append(str);
	}
	template <typename T1, typename T2, typename T3, typename T4>
	void op3(std::string op, T1 dst, T2 src0, T3 src1, T4 src2)
	{
		int tmpIdx;
		std::string str = sblk();
		str.append(op);

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");

		str.append(getOpter(dst));

		str.append(", ");
		str.append(getOpter(src0));
		str.append(", ");
		str.append(getOpter(src1));
		str.append(", ");
		str.append(getOpter(src2));

		str.append("\n");
		kernelStr->append(str);
	}
	template <typename T1, typename T2, typename T3, typename T4, typename T5>
	void op4(std::string op, T1 dst, T2 src0, T3 src1, T4 src2, T5 src3)
	{
		int tmpIdx;
		std::string str = sblk();
		str.append(op);

		tmpIdx = PARAM_START_COL - str.length();
		for (int i = 0; i < tmpIdx; i++)
			str.append(" ");

		str.append(getOpter(dst));

		str.append(", ");
		str.append(getOpter(src0));
		str.append(", ");
		str.append(getOpter(src1));
		str.append(", ");
		str.append(getOpter(src2));
		str.append(", ");
		str.append(getOpter(src3));

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

	/************************************************************************************/
	/* ¹Å¶­																				*/
	/************************************************************************************/
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
