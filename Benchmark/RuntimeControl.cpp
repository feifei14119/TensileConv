#include "RuntimeControl.h"
#include "ProblemControl.h"

int RuntimeCtrlBase::DeviceNum = 0;
int RuntimeCtrlBase::SpecifyDeviceIdx = DEFAULT_DEVICE;
int RuntimeCtrlBase::DeviceIdx = 0;
size_t RuntimeCtrlBase::Compute = 0;
size_t RuntimeCtrlBase::Bandwidth = 0;
T_DeviceInfo RuntimeCtrlBase::DeviceInfo;

#if RUNTIME_CUDA
cudaDeviceProp RuntimeControlCuda::deviceProperties;
#endif

#if RUNTIME_HIP
hipStream_t SolutionCtrlHip::stream;
hipDeviceProp_t SolutionCtrlHip::deviceProperties;
#endif

#if RUNTIME_OCL
CLCommandArgs * RuntimeCtrl::clArgs;

cl_platform_id RuntimeCtrl::platform;
cl_context RuntimeCtrl::context;
cl_device_id RuntimeCtrl::device;
SDKDeviceInfo RuntimeCtrl::deviceInfo;
std::vector<cl_command_queue> * RuntimeCtrl::streams;
#endif

std::ofstream *performance_log_file;
char log_char_buffer[1024];

void init_log_file()
{
	std::string SrcFileName = "./performence log.log";
	performance_log_file = new std::ofstream(SrcFileName, std::ios::out);
	if (!performance_log_file->is_open())
	{
		FATAL("can't open log file");
	}
}
void DeLog()
{
	performance_log_file->close();
}
void write_string_to_file(std::string log_str)
{
	if (performance_log_file == nullptr)
	{
		init_log_file();
	}
	performance_log_file->write(log_str.c_str(), log_str.length());
	performance_log_file->flush();
}
void write_format_to_file(const char * format,...)
{
	va_list args;
	va_start(args, format);
	printf(format, args);
	memset(log_char_buffer, 0, 1024);
	vsprintf(log_char_buffer, format, args);
	write_string_to_file(std::string(log_char_buffer));
	va_end(args);
}