#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../include/TensileConv.h"

#include "../common/ff_utils.h"

int main(int argc, char *argv[])
{
	// init your own runtime 
	RuntimeOCL * rtOcl = RuntimeOCL::GetInstance();
	rtOcl->SellectDevice(0);

	// get your runtime param
	cl_platform_id platformId = rtOcl->PlatformId();
	cl_context context = rtOcl->Context();
	cl_device_id deviceId = rtOcl->Device()->DeviceId();

	// set runtime for TensileConv
	TensileConv::SetRuntime(platformId, context, deviceId);

	// run TensileConv
	TensileConv::DirConv1x1Fwd(14, 512, 64, 1, 1, true, true);

	return 0;
}
