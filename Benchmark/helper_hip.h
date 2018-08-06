#ifndef __HELPER_HIP_H__
#define __HELPER_HIP_H__
#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <hip/hip_runtime.h>

static const char *hipGetErrorInfo(hipError_t error)
{
	switch (error)
	{
	case hipSuccess:
		return "hipSuccess";
	case hipErrorOutOfMemory:
		return "hipErrorOutOfMemory";
	case hipErrorNotInitialized:
		return "hipErrorNotInitialized";
	case hipErrorDeinitialized:
		return "hipErrorDeinitialized";
	case hipErrorProfilerDisabled:
		return "hipErrorProfilerDisabled";
	case hipErrorProfilerNotInitialized:
		return "hipErrorProfilerNotInitialized";
	case hipErrorProfilerAlreadyStarted:
		return "hipErrorProfilerAlreadyStarted";
	case hipErrorProfilerAlreadyStopped:
		return "hipErrorProfilerAlreadyStopped";
	case hipErrorInsufficientDriver:
		return "hipErrorInsufficientDriver";

	case hipErrorInvalidImage:
		return "hipErrorInvalidImage";
	case hipErrorInvalidContext:
		return "hipErrorInvalidContext";
	case hipErrorContextAlreadyCurrent:
		return "hipErrorContextAlreadyCurrent";
	case hipErrorMapFailed:
		return "hipErrorMapFailed";
	case hipErrorUnmapFailed:
		return "hipErrorUnmapFailed";
	case hipErrorArrayIsMapped:
		return "hipErrorArrayIsMapped";
	case hipErrorAlreadyMapped:
		return "hipErrorAlreadyMapped";
	case hipErrorNoBinaryForGpu:
		return "hipErrorNoBinaryForGpu";
	case hipErrorAlreadyAcquired:
		return "hipErrorAlreadyAcquired";
	case hipErrorNotMapped:
		return "hipErrorNotMapped";
	case hipErrorNotMappedAsArray:
		return "hipErrorNotMappedAsArray";
	case hipErrorNotMappedAsPointer:
		return "hipErrorNotMappedAsPointer";
	case hipErrorECCNotCorrectable:
		return "hipErrorECCNotCorrectable";
	case hipErrorUnsupportedLimit:
		return "hipErrorUnsupportedLimit";
	case hipErrorContextAlreadyInUse:
		return "hipErrorContextAlreadyInUse";
	case hipErrorPeerAccessUnsupported:
		return "hipErrorPeerAccessUnsupported";
	case hipErrorInvalidKernelFile:
		return "hipErrorInvalidKernelFile";
	case hipErrorInvalidGraphicsContext:
		return "hipErrorInvalidGraphicsContext";

	case hipErrorInvalidSource:
		return "hipErrorInvalidSource";
	case hipErrorFileNotFound:
		return "hipErrorFileNotFound";
	case hipErrorSharedObjectSymbolNotFound:
		return "hipErrorSharedObjectSymbolNotFound";
	case hipErrorSharedObjectInitFailed:
		return "hipErrorSharedObjectInitFailed";
	case hipErrorOperatingSystem:
		return "hipErrorOperatingSystem";
	case hipErrorSetOnActiveProcess:
		return "hipErrorSetOnActiveProcess";

	case hipErrorInvalidHandle:
		return "hipErrorInvalidHandle";
	case hipErrorNotFound:
		return "hipErrorNotFound";
	case hipErrorIllegalAddress:
		return "hipErrorIllegalAddress";
	case hipErrorInvalidSymbol:
		return "hipErrorInvalidSymbol";
	
	// Runtime Error Codes start here.
	case hipErrorMissingConfiguration:
		return "hipErrorMissingConfiguration";
	case hipErrorMemoryAllocation:
		return "hipErrorMemoryAllocation";
	case hipErrorInitializationError:
		return "hipErrorInitializationError";
	case hipErrorLaunchFailure:
		return "hipErrorLaunchFailure";
	case hipErrorPriorLaunchFailure:
		return "hipErrorPriorLaunchFailure";
	case hipErrorLaunchTimeOut:
		return "hipErrorLaunchTimeOut";
	case hipErrorLaunchOutOfResources:
		return "hipErrorLaunchOutOfResources";
	case hipErrorInvalidDeviceFunction:
		return "hipErrorInvalidDeviceFunction";
	case hipErrorInvalidConfiguration:
		return "hipErrorInvalidConfiguration";
	case hipErrorInvalidDevice:
		return "hipErrorInvalidDevice";
	case hipErrorInvalidValue:
		return "hipErrorInvalidValue";
	case hipErrorInvalidDevicePointer:
		return "hipErrorInvalidDevicePointer";
	case hipErrorInvalidMemcpyDirection:
		return "hipErrorInvalidMemcpyDirection";
	case hipErrorUnknown:
		return "hipErrorUnknown";
	case hipErrorInvalidResourceHandle:
		return "hipErrorInvalidResourceHandle";
	case hipErrorNotReady:
		return "hipErrorNotReady";
	case hipErrorNoDevice:
		return "hipErrorNoDevice";

	case hipErrorPeerAccessAlreadyEnabled:
		return "hipErrorPeerAccessAlreadyEnabled";
	case hipErrorPeerAccessNotEnabled:
		return "hipErrorPeerAccessNotEnabled";
	case hipErrorRuntimeMemory:
		return "hipErrorRuntimeMemory";
	case hipErrorRuntimeOther:
		return "hipErrorRuntimeOther";
	case hipErrorHostMemoryAlreadyRegistered:
		return "hipErrorHostMemoryAlreadyRegistered";
	case hipErrorHostMemoryNotRegistered:
		return "hipErrorHostMemoryNotRegistered";
	case hipErrorMapBufferObjectFailed:
		return "hipErrorMapBufferObjectFailed";
	}

	return "<unknown>";
}

static void hip_checkFuncRet(hipError_t errNum, char const *const func, const char *const file, int const line)
{
	if (errNum)
	{
		fprintf(stderr, "HIP error at %s:%d code=%d(%s) \"%s\" \n", file, line, static_cast<unsigned int>(errNum), hipGetErrorInfo(errNum), func);

		exit(EXIT_FAILURE);
	}
}

static void hip_checkErrNum(hipError_t errNum, const char *const file, int const line)
{
	if (errNum)
	{
		fprintf(stderr, "HIP error at %s:%d code=%d(%s) \n", file, line, static_cast<unsigned int>(errNum), hipGetErrorInfo(errNum));

		exit(EXIT_FAILURE);
	}
}


#endif
#pragma once
