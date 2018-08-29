#pragma once
#include "BasicClass.h" 
#include "IsaWriter.h"

class KernelWriterBase
{
public:

#define	MAX_VGPR_COUNT		(256)
#define	MAX_SGPR_COUNT		(800)

	KernelWriterBase()
	{
		variableMap = new std::map<std::string, int>();
		sgprCount = 0;
		vgprCount = 0;
		
		gas = new GasWriter();
		gas->tableCnt = &tableCnt;
		gas->kernelStr = &KernelStr;

		isa = new IsaWriterBase();
		isa->tableCnt = &tableCnt;
		isa->kernelStr = &KernelStr;
	}

	void GenKernel()
	{
		KernelStr.clear();

		writeSignature();
		writeProgram();
		writeMetadata();
	}

	void SaveKernelStr2File()
	{
		std::string SrcFileName = "../../../Kernels/" + KernelFile;
		std::ofstream fout(SrcFileName, std::ios::out);
		if (!fout.is_open())
		{
			FATAL("can't open save file");
		}
		fout.write(KernelStr.c_str(), KernelStr.length());
		fout.close();
	}

	std::string KernelName;
	std::string KernelFile;
	std::string KernelStr;

protected:
	GasWriter *gas;
	IsaWriterBase *isa;
	
	/************************************************************************/
	/* sgpr操作		                                                        */
	/************************************************************************/
	int sgprCount, sgprCountMax;
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
	int vgprCount, vgprCountMax;
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
	void writeSignature()
	{
		KernelStr.append(".hsa_code_object_version 2, 1\n");
		KernelStr.append(".hsa_code_object_isa 9, 0, 0, \"AMD\", \"AMDGPU\"\n");
		KernelStr.append("\n");
		KernelStr.append(".text\n");
		KernelStr.append(".globl " + KernelName + "\n");
		KernelStr.append(".p2align 8\n");
		KernelStr.append(".type " + KernelName + ",@function\n");
		KernelStr.append(".amdgpu_hsa_kernel " + KernelName + "\n");
		KernelStr.append("\n");
		KernelStr.append(".include \"gpr_alloc.inc\"\n");
		KernelStr.append(".include \"common.inc\"\n");
		KernelStr.append(".include \"inst_wrappers.inc\"\n");
		KernelStr.append("\n");
	}

	void writeProgram()
	{
		int objPos;
		startProgram();
		objPos = KernelStr.length();
		writeKernel();
		endProgram();

		std::string objStr = "";
		generateCodeObj(&objStr);
		KernelStr.insert(objPos, s2c(objStr));
	}

	void writeMetadata()
	{
		KernelStr.append(".amd_amdgpu_hsa_metadata\n");
		KernelStr.append("{ Version: [1, 0],\n");
		KernelStr.append("  Kernels :\n");
		KernelStr.append("    - { Name: " + KernelName + ",\n");
		KernelStr.append("        SymbolName: " + KernelName + ",\n");
		KernelStr.append("        Language: OpenCL C, LanguageVersion: [ 1, 2 ],\n");
		KernelStr.append("        Attrs: { ReqdWorkGroupSize: [ " + d2s(64) + ", 1, 1 ] }\n");
		KernelStr.append("        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize : 0, PrivateSegmentFixedSize : 0, KernargSegmentAlign : 8, WavefrontSize : 64, MaxFlatWorkGroupSize : 256 }\n");
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
	virtual void startProgram() = 0;
	virtual void generateCodeObj(std::string * objStr) = 0;
	virtual void writeKernel() = 0;
	virtual void endProgram() = 0;
};
