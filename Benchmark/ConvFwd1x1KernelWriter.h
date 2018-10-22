#pragma once

#include "ConvFwd1x1Config.h"
#include "KernelWriter.h"
#include "ProblemControl.h"

class KernelWriterConv1x1 :public krnelWriter::KernelWriter
{
public:
	KernelWriterConv1x1(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg);

protected:
	T_ExtConvFwd1x1ProblemConfig * extProbCfg;	// 当前正在处理的问题扩展配置
	T_ExtConvFwd1x1SolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置

	typedef enum
	{
		PING_FLAG = 1,
		PANG_FLAG = 2
	}E_PingPang;
	
	void writeProgram();

private:
	int c_in_maps_once;
	int k_out_maps;
	int k_out_group;		// 一个像素的所有输出特征值分给几个CU计算
	
	int blk_per_group;		// PIX_BLK_PER_GROUP
	int in_chan_stride;		// IN_CHANNEL_STRIDE
	int in_batch_stride;	// IN_BATCH_STRIDE
	int wei_chan_stride;	// WEI_CHANNEL_STRIDE
	int out_chan_stride;	// OUT_CHANNEL_STRIDE
	int out_batch_stride;	// OUT_BATCH_STRIDE
	int conv_loop;			// LOOP

	bool en_input_offset;
	bool en_wei_addr_offset;
	int offset_grp_num;
	krnelWriter::Var * v_global_offset;
	int wei_offset;

	krnelWriter::Var * s_ptr_in;
	krnelWriter::Var * s_ptr_wei;
	krnelWriter::Var * s_ptr_out;

	krnelWriter::Var * v_weiBlkId;
	krnelWriter::Var * v_pixBlkId;
	krnelWriter::Var * v_posId;
	krnelWriter::Var * v_batchId;
	krnelWriter::Var * v_outId;

	krnelWriter::Var * v_addr_in;
	krnelWriter::Var * s_addr_wei;
	krnelWriter::Var * v_addr_out;

	krnelWriter::Var * v_in_buff_a;
	krnelWriter::Var * v_in_buff_b;
	krnelWriter::Var * s_wei_buff_a;
	krnelWriter::Var * s_wei_buff_b;
	krnelWriter::Var * v_acc_buff;
	krnelWriter::Var * s_prefetch;

	void main_conv();
	void conv_one_loop(krnelWriter::Var * in_buff, bool is_pang_buff);
	void conv_last_loop(krnelWriter::Var * in_buff);
	void conv_one_accum(krnelWriter::Var * in_buff, krnelWriter::Var * wei_buff, krnelWriter::Var * accum);

	void calcuIndex();
	void calcuBlkIndex();
	void calcuPosIndex();
	void calcuOffset();

	void load_input(krnelWriter::Var * in_buff);
	void load_weight(krnelWriter::Var * wei_buff);
	void prefetch_weight();
	void save_output();
};
