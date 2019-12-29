// Copyright (c) 2009-2011 Ignacio Castano <castano@gmail.com>
// Copyright (c) 2007-2009 NVIDIA Corporation -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "nvcore/Debug.h"
#include "CudaUtils.h"

#if defined HAVE_CUDA
#include <cuda.h>
#include <cuda_runtime_api.h>
#endif

using namespace nv;
using namespace cuda;

/* @@ Move this to win32 utils or somewhere else.
#if NV_OS_WIN32

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

static bool isWindowsVista()
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	::GetVersionEx(&osvi);
	return osvi.dwMajorVersion >= 6;
}


typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

static bool isWow32()
{
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle("kernel32"), "IsWow64Process");

	BOOL bIsWow64 = FALSE;
 
	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			// Assume 32 bits.
			return true;
		}
	}

	return !bIsWow64;
}

#endif
*/


static bool isCudaDriverAvailable(int version)
{
#if defined HAVE_CUDA
#if NV_OS_WIN32
	Library nvcuda("nvcuda.dll");
#else
	Library nvcuda(NV_LIBRARY_NAME(cuda));
#endif
	
	if (!nvcuda.isValid())
	{
		nvDebug("*** CUDA driver not found.\n");
		return false;
	}
	
	if (version >= 2000)
	{
		void * address = nvcuda.bindSymbol("cuStreamCreate");
		if (address == NULL) {
			nvDebug("*** CUDA driver version < 2.0.\n");
			return false;
		}
	}

	if (version >= 2010)
	{
		void * address = nvcuda.bindSymbol("cuModuleLoadDataEx");
		if (address == NULL) {
			nvDebug("*** CUDA driver version < 2.1.\n");
			return false;
		}
	}
	
	if (version >= 2020)
	{
		typedef CUresult (CUDAAPI * PFCU_DRIVERGETVERSION)(int * version);

		PFCU_DRIVERGETVERSION driverGetVersion = (PFCU_DRIVERGETVERSION)nvcuda.bindSymbol("cuDriverGetVersion");
		if (driverGetVersion == NULL) {
			nvDebug("*** CUDA driver version < 2.2.\n");
			return false;
		}

		int driverVersion;
		CUresult err = driverGetVersion(&driverVersion);
		if (err != CUDA_SUCCESS) {
			nvDebug("*** Error querying driver version: '%s'.\n", cudaGetErrorString((cudaError_t)err));
			return false;
		}

		return driverVersion >= version;
	}
#endif // HAVE_CUDA

	return true;
}


/// Determine if CUDA is available.
bool nv::cuda::isHardwarePresent()
{
#if defined HAVE_CUDA
	// Make sure that CUDA driver matches CUDA runtime.
	if (!isCudaDriverAvailable(CUDART_VERSION))
	{
		nvDebug("CUDA driver not available for CUDA runtime %d\n", CUDART_VERSION);
		return false;
	}

	int count = deviceCount();
	if (count == 1)
	{
		// Make sure it's not an emulation device.
		cudaDeviceProp deviceProp;
		cudaGetDeviceProperties(&deviceProp, 0);

		// deviceProp.name != Device Emulation (CPU)
		if (deviceProp.major == -1 || deviceProp.minor == -1)
		{
			return false;
		}
	}

	// @@ Make sure that warp size == 32

	return count > 0;
#else
	return false;
#endif
}

/// Get number of CUDA enabled devices.
int nv::cuda::deviceCount()
{
#if defined HAVE_CUDA
	int gpuCount = 0;

	cudaError_t result = cudaGetDeviceCount(&gpuCount);

	if (result == cudaSuccess)
	{
		return gpuCount;
	}
#endif
	return 0;
}

int nv::cuda::getFastestDevice()
{
	int max_gflops_device = 0;
#if defined HAVE_CUDA
	int max_gflops = 0;

	const int device_count = deviceCount();
	int current_device = 0;
	while (current_device < device_count)
	{
		cudaDeviceProp device_properties;
		cudaGetDeviceProperties(&device_properties, current_device);
		int gflops = device_properties.multiProcessorCount * device_properties.clockRate;

		if (device_properties.major != -1 && device_properties.minor != -1)
		{
			if( gflops > max_gflops )
			{
				max_gflops = gflops;
				max_gflops_device = current_device;
			}
		}
		
		current_device++;
	}
#endif
	return max_gflops_device;
}


/// Activate the given devices.
bool nv::cuda::setDevice(int i)
{
	nvCheck(i < deviceCount());
#if defined HAVE_CUDA
	cudaError_t result = cudaSetDevice(i);

	if (result != cudaSuccess) {
		nvDebug("*** CUDA Error: %s\n", cudaGetErrorString(result));
	}

	return result == cudaSuccess;
#else
	return false;
#endif
}

void nv::cuda::exit()
{
#if defined HAVE_CUDA
	cudaError_t result = cudaThreadExit();

	if (result != cudaSuccess) {
		nvDebug("*** CUDA Error: %s\n", cudaGetErrorString(result));
	}
#endif
}
