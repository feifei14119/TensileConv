#pragma once

#include "BasicClass.h"
#include "ProblemControl.h"

#include "GasWriter.h"
#include "IsaWriter.h"

class KernelWriterBase
{
public:	
	KernelWriterBase(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg)
	{
		variableMap = new std::map<std::string, int>();
		OperatorMap = new std::map<std::string, t_operator*>();
		sgprCount = 0;
		vgprCount = 0;
		memset(SgprState, 0, sizeof(int)*MAX_SGPR_COUNT);
		memset(VgprState, 0, sizeof(int)*MAX_VGPR_COUNT);
		
		gas = new GasWriter();
		gas->tableCnt = &tableCnt;
		gas->kernelStr = &KernelStr;

		isa = new IsaWriterBase();
		isa->tableCnt = &tableCnt;
		isa->kernelStr = &KernelStr;
		isa->OperatorMap = OperatorMap;

		problemConfig = probCfg;
		solutionConfig = solCfg;

		kernelName = solutionConfig->KernelName;
		kernelFile = solutionConfig->KernelFile;
		group_size = solutionConfig->l_wk0;
	}

public:

	void GenKernelString()
	{
		KernelStr.clear();

		// 用来确定gpr使用的部分
		generateParam();
		writeContent();
		KernelStr.clear();

		generateParam();
		writeSignature();
		writeContent();
		writeMetadata();
	}

	void SaveKernelStr2File()
	{
		std::string SrcFileName = "../../../Kernels/" + kernelFile;
		std::ofstream fout(SrcFileName, std::ios::out);
		if (!fout.is_open())
		{
			FATAL("can't open save file");
		}
		fout.write(KernelStr.c_str(), KernelStr.length());
		fout.close();
	}

protected:
	std::string KernelStr;
	GasWriter *gas;
	IsaWriterBase *isa;
	T_ProblemConfig * problemConfig;	// 当前正在处理的问题配置
	T_SolutionConfig * solutionConfig;	// 当前正在处理的解决方案配置
	std::string kernelName;
	std::string kernelFile;
	int SgprState[MAX_SGPR_COUNT];
	int VgprState[MAX_VGPR_COUNT];
	
	
	std::map<std::string, t_operator*> *OperatorMap;
	t_operator * newSgpr(std::string name, int len = 1, int align = 1)
	{
		t_operator *opt;

		if ((name == "off") || (name == "OFF"))
		{
			opt = new t_operator;
			opt->name = name;
			opt->type = E_OpterType::OPTER_OFF;
		}
		else
		{
			int idleIdx;
			for (idleIdx = 0; idleIdx < MAX_SGPR_COUNT; idleIdx++)
			{
				if (idleIdx % align != 0)
					continue;

				if (SgprState[idleIdx] != 0)
					continue;

				int idleChk;
				for (idleChk = 0; idleChk < len; idleChk++)
				{
					if (SgprState[idleChk + idleIdx] != 0)
						break;
				}
				if (idleChk != len)
					continue;
				break;
			}
			if (idleIdx == MAX_SGPR_COUNT)
				return nullptr;

			for (int i = 0; i < len; i++)
			{
				SgprState[idleIdx + i] = 1;
			}

			opt = new t_operator;
			opt->name = name;
			opt->type = E_OpterType::OPTER_SGPR;
			opt->sgpr.name = name;
			opt->sgpr.perGprIdx = sgprCount;
			opt->sgpr.gprIdx = idleIdx;
			opt->sgpr.len = len;
			opt->sgpr.align = align;

			if (idleIdx + len > sgprCountMax)
				sgprCountMax = idleIdx + len;
		}

		OperatorMap->insert(std::pair<std::string, t_operator*>(name, opt));
		return opt;
	}
	t_operator * newVgpr(std::string name, int len = 1, int align = 1)
	{
		t_operator *opt;

		if ((name == "off") || (name == "OFF"))
		{
			opt = new t_operator;
			opt->name = name;
			opt->type = E_OpterType::OPTER_OFF;
		}
		else
		{
			int idleIdx;
			for (idleIdx = 0; idleIdx < MAX_VGPR_COUNT; idleIdx++)
			{
				if (idleIdx % align != 0)
					continue;

				if (VgprState[idleIdx] != 0)
					continue;

				int idleChk;
				for (idleChk = 0; idleChk < len; idleChk++)
				{
					if (VgprState[idleChk + idleIdx] != 0)
						break;
				}
				if (idleChk != len)
					continue;
				break;
			}
			if (idleIdx == MAX_VGPR_COUNT)
				return nullptr;

			for (int i = 0; i < len; i++)
			{
				VgprState[idleIdx + i] = 1;
			}

			opt = new t_operator;
			opt->name = name;
			opt->type = E_OpterType::OPTER_VGPR;
			opt->vgpr.name = name;
			opt->vgpr.perGprIdx = sgprCount;
			opt->vgpr.gprIdx = idleIdx;
			opt->vgpr.len = len;
			opt->vgpr.align = align;

			if (idleIdx + len > vgprCountMax)
				vgprCountMax = idleIdx + len;
		}

		OperatorMap->insert(std::pair<std::string, t_operator*>(name, opt));
		return opt;
	}
	t_operator * newImm(std::string name, int val = 0)
	{
		t_operator *opt = new t_operator;

		opt->name = name;
		opt->type = E_OpterType::OPTER_IMM;
		opt->imm.name = name;
		opt->imm.value = val;

		OperatorMap->insert(std::pair<std::string, t_operator*>(name, opt));
		return opt;
	}
	void delOpter(t_operator * opter)
	{
		if (opter->type == E_OpterType::OPTER_SGPR)
		{
			int gprIdx = opter->sgpr.gprIdx;
			for (int i = 0; i < opter->sgpr.len; i++)
			{
				SgprState[gprIdx + i] = 0;
			}
		}
		else if (opter->type == E_OpterType::OPTER_VGPR)
		{
			int gprIdx = opter->vgpr.gprIdx;
			for (int i = 0; i < opter->vgpr.len; i++)
			{
				VgprState[gprIdx + i] = 0;
			}
		}
		OperatorMap->erase(opter->name);
		delete(opter);
	}
	void clrOpter()
	{
		memset(SgprState, 0, sizeof(int)*MAX_SGPR_COUNT);
		memset(VgprState, 0, sizeof(int)*MAX_VGPR_COUNT);
		OperatorMap->clear();
	}

	/************************************************************************/
	/* sgpr操作		                                                        */
	/************************************************************************/
	int sgprCount = 0, sgprCountMax = 0;
	void newSgpr(int *gpr, int num = 1, int align = 1)
	{
		while (true)
		{
			if (sgprCount % align == 0)
				break;
			sgprCount++;
		}
		*gpr = sgprCount;
		sgprCount += num;
		if (sgprCount > sgprCountMax)
			sgprCountMax = sgprCount;
		if (sgprCount >= MAX_SGPR_COUNT)
			FATAL("malloc sgpr overflow");
	}
	void delSgpr(int *gpr)
	{
		sgprCount = *gpr;
		if (sgprCount <= 0)
			FATAL("free sgpr overflow");
	}
	std::string sgpr(std::string name, int num = 1)
	{
		if (num == 1)
			return std::string("s[" + name + "]");
		else
			return std::string("s[" + name + ":" + name + "+" + d2s(num-1) + "]");
	}
	std::string sgpr(int idx, int num = 1)
	{
		if (num == 1)
			return std::string("s[" + d2s(idx) + "]");
		else
			return std::string("s[" + d2s(idx) + ":" + d2s(idx + num - 1) + "]");
	}
	std::string sgpr_h(std::string name)
	{
		return std::string("s[" + name + "+1]");
	}
	std::string sgpr_h(int idx)
	{
		return std::string("s[" + d2s(idx + 1) + "]");
	}
	
	/************************************************************************/
	/* vgpr操作		                                                        */
	/************************************************************************/
	int vgprCount = 0, vgprCountMax = 0;
	void newVgpr(int *gpr, int num = 1, int align = 1)
	{
		while (true)
		{
			if (vgprCount % align == 0)
				break;
			vgprCount++;
		}
		*gpr = vgprCount;
		vgprCount += num;
		if (vgprCount > vgprCountMax)
			vgprCountMax = vgprCount;
		if (vgprCount >= MAX_VGPR_COUNT)
			FATAL("malloc vgpr overflow");
	}
	void delVgpr(int *gpr)
	{
		vgprCount = *gpr;
		if (vgprCount <= 0)
			FATAL("free sgpr overflow");
	}
	std::string vgpr(std::string name, int num = 1)
	{
		if (num == 1)
			return std::string("v[" + name + "]");
		else
			return std::string("v[" + name + ":" + name + "+" + d2s(num-1) + "]");
	}
	std::string vgpr(int idx, int num = 1)
	{
		if (num == 1)
			return std::string("v[" + d2s(idx) + "]");
		else
			return std::string("v[" + d2s(idx) + ":" + d2s(idx + num - 1) + "]");
	}
	std::string vgpr_h(std::string name)
	{
		return std::string("v[" + name + "+1]");
	}
	std::string vgpr_h(int idx)
	{
		return std::string("v[" + d2s(idx + 1) + "]");
	}

	/************************************************************************/
	/* 全局变量操作	                                                        */
	/************************************************************************/
	std::map<std::string, int> *variableMap;
	void newVar(std::string name, int val)
	{
		variableMap->insert(std::pair<std::string, int>(name, val));
	}
	std::string var(std::string name)
	{
		return d2s((*variableMap)[name]);
	}
	int varVal(std::string name)
	{
		return (*variableMap)[name];
	}
	
	/************************************************************************/
	/* 通用函数		                                                        */
	/************************************************************************/
	int tableCnt = 0;
	std::string sblk()
	{
		std::string str = "";
		for (int i = 0; i < tableCnt; i++)
		{
			str.append("    ");
		}
		return str;
	}
	void sline(std::string line)
	{
		std::string str = sblk();
		str.append(line);
		str.append("\n");
		KernelStr.append(str);
	}
	void sline(std::string * srcString, std::string line)
	{
		std::string str = sblk();
		str.append(line);
		str.append("\n");
		srcString->append(str);
	}

	void wirteCommom1(std::string common)
	{
		KernelStr.append("/************************************************************************************/\n");
		KernelStr.append("/* " + common + " */\n");
		KernelStr.append("/************************************************************************************/\n");
	}
	void wirteCommom2(std::string common)
	{
		KernelStr.append("// ==================================================================================\n");
		KernelStr.append("// " + common + " \n");
		KernelStr.append("// ==================================================================================\n");
	}	
	void wirteCommom3(std::string common)
	{
		KernelStr.append("// ----------------------------------------------------------------------------------\n");
		KernelStr.append("// " + common + " \n");
		KernelStr.append("// ----------------------------------------------------------------------------------\n");
	}

	int log2(int value)
	{
		int log2 = 0;
		while (value > 1)
		{
			value = value / 2;
			log2++;
		}
		return log2;
	}
	int modMask(int value)
	{
		return value - 1;
	}

	/************************************************************************/
	/* kernel文件生成函数                                                    */
	/************************************************************************/
	virtual void generateParam() = 0;
	void writeSignature()
	{
		KernelStr.append(".hsa_code_object_version 2, 1\n");
		KernelStr.append(".hsa_code_object_isa 9, 0, 0, \"AMD\", \"AMDGPU\"\n");
		KernelStr.append("\n");
		KernelStr.append(".text\n");
		KernelStr.append(".globl " + kernelName + "\n");
		KernelStr.append(".p2align 8\n");
		KernelStr.append(".type " + kernelName + ",@function\n");
		KernelStr.append(".amdgpu_hsa_kernel " + kernelName + "\n");
		KernelStr.append("\n");
		KernelStr.append(".include \"gpr_alloc.inc\"\n");
		KernelStr.append(".include \"common.inc\"\n");
		KernelStr.append(".include \"inst_wrappers.inc\"\n");
		KernelStr.append("\n");
	}

	void writeContent0()
	{
		int objPos;
		startProgram();
		objPos = KernelStr.length();
		writeProgram();
		endProgram();

		std::string objStr = "";
		generateCodeObj(&objStr);
		KernelStr.insert(objPos, s2c(objStr));
	}

	void writeContent()
	{
		startProgram();
		generateCodeObj();
		writeProgram();
		endProgram();
	}

	int group_size;
	void writeMetadata()
	{
		KernelStr.append(".amd_amdgpu_hsa_metadata\n");
		KernelStr.append("{ Version: [1, 0],\n");
		KernelStr.append("  Kernels :\n");
		KernelStr.append("    - { Name: " + kernelName + ",\n");
		KernelStr.append("        SymbolName: " + kernelName + ",\n");
		KernelStr.append("        Language: OpenCL C, LanguageVersion: [ 1, 2 ],\n");
		KernelStr.append("        Attrs: { ReqdWorkGroupSize: [ " + d2s(group_size) + ", 1, 1 ] }\n");
		KernelStr.append("        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize : 0, PrivateSegmentFixedSize : 0, KernargSegmentAlign : 8, WavefrontSize : 64, MaxFlatWorkGroupSize : 512 }\n");
		KernelStr.append("        Args:\n");
		KernelStr.append("        - { Name: d_in  , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global, IsConst : true }\n");
		KernelStr.append("        - { Name: d_wei , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global, IsConst : true }\n");
		KernelStr.append("        - { Name: d_out , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global  }\n");
		KernelStr.append("      }\n");
		KernelStr.append("}\n");
		KernelStr.append(".end_amd_amdgpu_hsa_metadata\n");
		KernelStr.append("\n");
	}

	/************************************************************************/
	/* kernel 函数内容生成函数                                                */
	/************************************************************************/
	char * END_PROG = "END_PROG";
	void startProgram()
	{
		tableCnt = 0;
		sline(kernelName + ":");
		tableCnt = 1;
	}
	void generateCodeObj(std::string * objString)
	{
		tableCnt = 1;
		std::string tmpStr = sblk();
		sline(&tmpStr, ".amd_kernel_code_t");
		tableCnt++;
		sline(&tmpStr, "enable_sgpr_private_segment_buffer = 1");
		sline(&tmpStr, "enable_sgpr_kernarg_segment_ptr = 1");
		sline(&tmpStr, "enable_sgpr_workgroup_id_x = 1");
		sline(&tmpStr, "enable_sgpr_workgroup_id_y = 1");
		sline(&tmpStr, "enable_sgpr_workgroup_id_z = 1");
		sline(&tmpStr, "enable_vgpr_workitem_id = 0");
		sline(&tmpStr, "is_ptr64 = 1");
		sline(&tmpStr, "float_mode = 240");
		sline(&tmpStr, "granulated_wavefront_sgpr_count = " + d2s((sgprCountMax - 1) / 8));
		sline(&tmpStr, "granulated_workitem_vgpr_count = " + d2s((vgprCountMax - 1) / 4));
		sline(&tmpStr, "user_sgpr_count = 6");
		sline(&tmpStr, "wavefront_sgpr_count = " + d2s(sgprCountMax));
		sline(&tmpStr, "workitem_vgpr_count = " + d2s(vgprCountMax));
		sline(&tmpStr, "kernarg_segment_byte_size = 56");
		sline(&tmpStr, "workgroup_group_segment_byte_size = 0");
		tableCnt--;
		sline(&tmpStr, ".end_amd_kernel_code_t");
		sline(&tmpStr, "");

		objString->append(tmpStr);
	}
	void generateCodeObj()
	{
		tableCnt = 1;
		sline(".amd_kernel_code_t");
		tableCnt++;
		sline("enable_sgpr_private_segment_buffer = 1");
		sline("enable_sgpr_kernarg_segment_ptr = 1");
		sline("enable_sgpr_workgroup_id_x = 1");
		sline("enable_sgpr_workgroup_id_y = 1");
		sline("enable_sgpr_workgroup_id_z = 1");
		sline("enable_vgpr_workitem_id = 0");
		sline("is_ptr64 = 1");
		sline("float_mode = 240");
		sline("granulated_wavefront_sgpr_count = " + d2s((sgprCountMax - 1) / 8));
		sline("granulated_workitem_vgpr_count = " + d2s((vgprCountMax - 1) / 4));
		sline("user_sgpr_count = 6");
		sline("wavefront_sgpr_count = " + d2s(sgprCountMax));
		sline("workitem_vgpr_count = " + d2s(vgprCountMax));
		sline("kernarg_segment_byte_size = 56");
		sline("workgroup_group_segment_byte_size = 0");
		tableCnt--;
		sline(".end_amd_kernel_code_t");
		sline("");
	}
	virtual void writeProgram() = 0;
	void endProgram()
	{
		tableCnt = 0;
		sline(c2s(END_PROG) + ":");
		tableCnt = 1;
		sline("s_endpgm\n");
		tableCnt = 0;
	}
};
