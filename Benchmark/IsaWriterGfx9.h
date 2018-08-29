#pragma once
#include "IsaWriter.h" 

class IsaWriterGfx9:public IsaWriterBase
{
public:
	/*std::string SMemDword(
		E_LoadStore loadStore,
		int dNum,
		T_Gpr sDst,
		T_Gpr sAddr,
		T_Gpr offset,
		char *common)
	{
		int idx = 0;
		int tmpIdx;

		memset(tmpInsr, INSTR_BUFF_SIZE, sizeof(char));

		idx += sprintf(tmpInsr+idx, "s");

		switch (loadStore)
		{
		case E_LoadStore::LOAD:
			idx += sprintf(tmpInsr+idx, "_load");
			break;
		case E_LoadStore::STORE:
			idx += sprintf(tmpInsr+idx, "_store");
			break;
		}

		switch (dNum)
		{
		case 2:
			idx += sprintf(tmpInsr+idx, "dwordx2");
			break;
		case 4:
			idx += sprintf(tmpInsr+idx, "dwordx4");
			break;
		case 8:
			idx += sprintf(tmpInsr+idx, "dwordx8");
			break;
		case 16:
			idx += sprintf(tmpInsr+idx, "dwordx16");
			break;
		default:
			idx += sprintf(tmpInsr+idx, "dword");
			break;
		}

		tmpIdx = INSTR_STOP_COL - idx;
		for (int i = 0; i < tmpIdx; i++)
			idx += sprintf(tmpInsr+idx, " ");

		switch (dNum)
		{
		case 2:
			idx += sprintf(tmpInsr+idx, "s[%d:%d], ", sDst.GprIdx, sDst.GprIdx + 1);
			break;
		case 4:
			idx += sprintf(tmpInsr+idx, "s[%d:%d], ", sDst.GprIdx, sDst.GprIdx + 3);
			break;
		case 8:
			idx += sprintf(tmpInsr+idx, "s[%d:%d], ", sDst.GprIdx, sDst.GprIdx + 7);
			break;
		case 16:
			idx += sprintf(tmpInsr+idx, "s[%d:%d], ", sDst.GprIdx, sDst.GprIdx + 15);
			break;
		default:
			idx += sprintf(tmpInsr+idx, "s[%d], ", sDst.GprIdx);
			break;
		}

		if (offset.IsImm == true)
		{
			idx += sprintf(tmpInsr+idx, "0x0+%d", offset.Imm);
		}
		else
		{
			idx += sprintf(tmpInsr+idx, "s[%d]", offset.GprIdx);
		}

		tmpIdx = COMMON_START_COL - idx;
		for (int i = 0; i < tmpIdx; i++)
			idx += sprintf(tmpInsr+idx, " ");

		tmpInsr[idx++] = '\\';
		tmpInsr[idx++] = '\\';
		tmpIdx = INSTR_BUFF_SIZE - idx;
		for (int i = 0; i < tmpIdx; i++)
		{
			tmpInsr[idx++] = common[i];

			if (common[i] == '\0')
				break;
		}

		tmpInsr[idx] = '\n';

		return std::string(tmpInsr);
	}*/

private:
};

