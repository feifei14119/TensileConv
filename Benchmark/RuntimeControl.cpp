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


