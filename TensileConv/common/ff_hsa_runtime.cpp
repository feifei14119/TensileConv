#include <iostream> 
#include <fstream>
#include <sstream>

#include "ff_hsa_runtime.h"

using namespace feifei;
RuntimeHSA * RuntimeHSA::pInstance = nullptr;
RuntimeHSA * RuntimeHSA::GetInstance()
{
	if (pInstance == nullptr)
	{
		pInstance = new RuntimeHSA();

		if (pInstance->initPlatform() != E_ReturnState::SUCCESS)
		{
			pInstance = nullptr;
		}
	}

	return pInstance;
}

hsa_status_t find_gpu_device(hsa_agent_t agent, void *data)
{
	hsa_device_type_t hsa_device_type;
	hsa_status_t errNum;
	
	errNum = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &hsa_device_type);

	if (hsa_device_type == HSA_DEVICE_TYPE_GPU) 
	{
		RuntimeHSA* hsaRt = static_cast<RuntimeHSA*>(data);
		AgentHSA * newAgt = new AgentHSA(agent);
		hsaRt->Devices()->push_back(newAgt);
	}

	return HSA_STATUS_SUCCESS;
}
E_ReturnState RuntimeHSA::initPlatform()
{
	hsa_status_t errNum;

	// init hsa system
	errNum = hsa_init();
	hsaCheckErr(errNum);
	getHsaSystemInfo();

	// Iterate every agent
	errNum = hsa_iterate_agents(find_gpu_device, this);


	/*errNum = hsa_queue_create(agent, queue_size, HSA_QUEUE_TYPE_MULTI, NULL, NULL, UINT32_MAX, UINT32_MAX, &queue);
	if (errNum != HSA_STATUS_SUCCESS)
		return E_ReturnState::FAIL;*/


	return E_ReturnState::SUCCESS;
}

void RuntimeHSA::getHsaSystemInfo()
{
	hsa_status_t errNum;

	errNum = hsa_system_get_info(HSA_SYSTEM_INFO_VERSION_MAJOR, &hsaSystemInfo.major);
	errNum = hsa_system_get_info(HSA_SYSTEM_INFO_VERSION_MINOR, &hsaSystemInfo.minor);
}
void RuntimeHSA::PrintRuntimeInfo(bool isFullInfo)
{
	PrintSeperator('*');
	INFO("* HSA Runtime Info:");
	PrintHsaSystemInfo();
	PrintSeperator('*');

	for (int i = 0; i < DevicesCnt(); i++)
	{
		if (isFullInfo)
		{
			agents[i]->PrintAgentInfoFull();
		}
		else
		{
			agents[i]->PrintAgentInfoShort();
		}
	}
}
void RuntimeHSA::PrintHsaSystemInfo()
{
	INFO("* Runtime Version: %d.%d", hsaSystemInfo.major, hsaSystemInfo.minor);
}

void AgentHSA::getAgentInfo()
{
	hsa_status_t errNum;

	// Get agent name and vendor
	char * tmpChar = (char*)alloca(64);
	hsa_agent_get_info(agent, HSA_AGENT_INFO_NAME, tmpChar);
	agentInfo.name = std::string(tmpChar);
	hsa_agent_get_info(agent, HSA_AGENT_INFO_VENDOR_NAME, tmpChar);
	agentInfo.vendor_name = std::string(tmpChar);

	// Get agent feature
	hsa_agent_get_info(agent, HSA_AGENT_INFO_FEATURE, &agentInfo.agent_feature);

	// Get profile supported by the agent
	hsa_agent_get_info(agent, HSA_AGENT_INFO_PROFILE, &agentInfo.agent_profile);

	// Get floating-point rounding mode
	 hsa_agent_get_info(agent, HSA_AGENT_INFO_DEFAULT_FLOAT_ROUNDING_MODE, &agentInfo.float_rounding_mode);

	// Get max number of queue
	hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUES_MAX, &agentInfo.max_queue);

	// Get queue min size
	hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUE_MIN_SIZE, &agentInfo.queue_min_size);

	// Get queue max size
	hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &agentInfo.queue_max_size);

	// Get queue type
	hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUE_TYPE, &agentInfo.queue_type);

	// Get agent node
	hsa_agent_get_info(agent, HSA_AGENT_INFO_NODE, &agentInfo.node);

	// Get device type
	hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &agentInfo.device_type);

	if (HSA_DEVICE_TYPE_GPU == agentInfo.device_type) 
	{
		hsa_agent_get_info(agent, HSA_AGENT_INFO_ISA, &agentInfo.agent_isa);
	}

	// Get cache size
	hsa_agent_get_info(agent, HSA_AGENT_INFO_CACHE_SIZE, agentInfo.cache_size);

	// Get chip id
	hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_CHIP_ID, &agentInfo.chip_id);

	// Get cacheline size
	hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_CACHELINE_SIZE, &agentInfo.cacheline_size);

	// Get Max clock frequency
	hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_MAX_CLOCK_FREQUENCY, &agentInfo.max_clock_freq);

	// Get Agent BDFID
	hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_BDFID, &agentInfo.bdf_id);

	// Get number of Compute Unit
	hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_COMPUTE_UNIT_COUNT, &agentInfo.compute_unit);

	// Check if the agent is kernel agent
	if (agentInfo.agent_feature & HSA_AGENT_FEATURE_KERNEL_DISPATCH)
	{
		// Get flaf of fast_f16 operation
		hsa_agent_get_info(agent, HSA_AGENT_INFO_FAST_F16_OPERATION, &agentInfo.fast_f16);

		// Get wavefront size
		hsa_agent_get_info(agent, HSA_AGENT_INFO_WAVEFRONT_SIZE, &agentInfo.wavefront_size);

		// Get max total number of work-items in a workgroup
		hsa_agent_get_info(agent, HSA_AGENT_INFO_WORKGROUP_MAX_SIZE, &agentInfo.workgroup_max_size);

		// Get max number of work-items of each dimension of a work-group
		hsa_agent_get_info(agent, HSA_AGENT_INFO_WORKGROUP_MAX_DIM, &agentInfo.workgroup_max_dim);

		// Get max number of a grid per dimension
		errNum = hsa_agent_get_info(agent, HSA_AGENT_INFO_GRID_MAX_DIM, &agentInfo.grid_max_dim);
		hsaCheckErr(errNum);

		// Get max total number of work-items in a grid
		errNum = hsa_agent_get_info(agent, HSA_AGENT_INFO_GRID_MAX_SIZE, &agentInfo.grid_max_size);
		hsaCheckErr(errNum);

		// Get max number of fbarriers per work group
		hsa_agent_get_info(agent, HSA_AGENT_INFO_FBARRIER_MAX_SIZE, &agentInfo.fbarrier_max_size);

		hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_MAX_WAVES_PER_CU, &agentInfo.waves_per_cu);
	}
}
void AgentHSA::PrintAgentInfoFull()
{
	PrintSeperator('+');

	INFO("Name: " + agentInfo.name);
	INFO("Vendor Name: " + agentInfo.vendor_name);

	if (agentInfo.agent_feature & HSA_AGENT_FEATURE_KERNEL_DISPATCH	&& agentInfo.agent_feature & HSA_AGENT_FEATURE_AGENT_DISPATCH)
	{
		INFO("Feature: KERNEL_DISPATCH & AGENT_DISPATCH");
	}
	else if (agentInfo.agent_feature & HSA_AGENT_FEATURE_KERNEL_DISPATCH) 
	{
		INFO("Feature: KERNEL_DISPATCH");
	}
	else if (agentInfo.agent_feature & HSA_AGENT_FEATURE_AGENT_DISPATCH)
	{
		INFO("Feature: AGENT_DISPATCH");
	}
	else 
	{
		INFO("Feature: None specified");
	}

	if (HSA_PROFILE_BASE == agentInfo.agent_profile) 
	{
		INFO("Profile: BASE_PROFILE");
	}
	else if (HSA_PROFILE_FULL == agentInfo.agent_profile) 
	{
		INFO("Profile: FULL_PROFILE");
	}
	else 
	{
		INFO("Profile: Unknown");
	}

	if (HSA_DEFAULT_FLOAT_ROUNDING_MODE_ZERO == agentInfo.float_rounding_mode) 
	{
		INFO("Float Round Mode: ZERO");
	}
	else if (HSA_DEFAULT_FLOAT_ROUNDING_MODE_NEAR == agentInfo.float_rounding_mode) 
	{
		INFO("Float Round Mode: NEAR");
	}
	else 
	{
		INFO("Float Round Mode: Not Supported");
	}

	INFO("Max Queue Number: %d", agentInfo.max_queue);

	INFO("Queue Min Size: %d", agentInfo.queue_min_size);

	INFO("Queue Max Size: %d", agentInfo.queue_max_size);

	if (HSA_QUEUE_TYPE_MULTI == agentInfo.queue_type)
	{
		INFO("Queue Type: MULTI");
	}
	else if (HSA_QUEUE_TYPE_SINGLE == agentInfo.queue_type) 
	{
		INFO("Queue Type: SINGLE");
	}
	else
	{
		INFO("Queue Type: Unknown");
	}

	if (HSA_DEVICE_TYPE_CPU == agentInfo.device_type) 
	{
		INFO("Device Type: CPU");
	}
	else if (HSA_DEVICE_TYPE_GPU == agentInfo.device_type) 
	{
		INFO("Device Type: GPU");
	}
	else 
	{
		INFO("Device Type: DSP");
	}

	INFO("Cache Info:");
	for (int i = 0; i < 4; i++) 
	{
		if (agentInfo.cache_size[i] > 0) 
		{
			INFO("L%d: %d KB", i + 1, agentInfo.cache_size[i] / 1024);
		}
	}

	INFO("Chip ID: %d", agentInfo.chip_id);
	INFO("Cacheline Size: %d", agentInfo.cacheline_size);
	INFO("Max Clock Frequency: %d(MHz):", agentInfo.max_clock_freq);
	INFO("BDFID: %d", agentInfo.bdf_id);
	INFO("Compute Unit: %d", agentInfo.compute_unit);

	if (agentInfo.agent_feature & HSA_AGENT_FEATURE_KERNEL_DISPATCH) 
	{
		INFO("Features: KERNEL_DISPATCH");
	}
	if (agentInfo.agent_feature & HSA_AGENT_FEATURE_AGENT_DISPATCH)
	{
		INFO("Features: AGENT_DISPATCH");
	}
	if (agentInfo.agent_feature == 0)
	{
		INFO("Features: None");
	}

	if (agentInfo.agent_feature & HSA_AGENT_FEATURE_KERNEL_DISPATCH)
	{
		if (agentInfo.fast_f16)
		{
			INFO("Fast F16 Operation: TRUE");
		}
		else
		{
			INFO("Fast F16 Operation: FALSE");
		}

		INFO("Wavefront Size: %d", agentInfo.wavefront_size);

		INFO("Workgroup Max Size: %d", agentInfo.workgroup_max_size);
		INFO("Workgroup Max Size per Dimension:");
		INFO("x: %d",	static_cast<uint32_t>(agentInfo.workgroup_max_dim[0]));
		INFO("y: %d", static_cast<uint32_t>(agentInfo.workgroup_max_dim[1]));
		INFO("z: %d", static_cast<uint32_t>(agentInfo.workgroup_max_dim[2]));

		INFO("Waves Per CU: %d", agentInfo.waves_per_cu);
		INFO("Max Work-item Per CU: %d", agentInfo.wavefront_size*agentInfo.waves_per_cu);

		INFO("Grid Max Size: %ld", agentInfo.grid_max_size);
		INFO("Grid Max Size per Dimension:");
		INFO("x: %ld", agentInfo.grid_max_dim.x);
		INFO("y: %ld", agentInfo.grid_max_dim.y);
		INFO("z: %ld", agentInfo.grid_max_dim.z);

		INFO("Max number Of fbarriers Per Workgroup: %d", agentInfo.fbarrier_max_size);
	}
	PrintSeperator('+');
}
void AgentHSA::PrintAgentInfoShort()
{
	PrintSeperator('+');

	int cuNum = agentInfo.compute_unit;
	double freq = agentInfo.max_clock_freq * 1e6;
	double perf = SIMD_PER_CU * cuNum * freq * 2;	// 2 opts(mult & add) in one cycle

	INFO("+ Agent Name: " + agentInfo.name);
	INFO("+ Vendor Name: " + agentInfo.vendor_name);
	INFO("+ CU Number = %d", cuNum);
	INFO("+ Core Frequency = %.3f(GHz)", freq * 1e-9);
	INFO("+ Performance(fp32) = %.3f(TFlops)", perf * 1e-12);

	PrintSeperator('+');
}
