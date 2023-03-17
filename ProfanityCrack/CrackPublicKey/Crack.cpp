#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <map>
#include <set>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h> // Included to get topology to get an actual unique identifier per device
#else
#include <CL/cl.h>
#include <CL/cl_ext.h> // Included to get topology to get an actual unique identifier per device
#endif

#include "utils.hpp"
#include "Dispatcher.hpp"
#include "Data/Buffer.h"

#define SOURCE_FILE_CODE "crack.cl"


int crack_public_key(cl_device_id* device, HostBuffersClass& Data, point public_key, std::vector< result>& result, ConfigClass& config) {
		size_t worksizeLocal = config.gpu_local_size;
		size_t worksizeGlobal = PROFANITY_GLOGAL_SIZE; // Will be automatically determined later if not overriden by user
		bool bNoCache = false;
		cl_int errorCode;
		bool bUsedCache = false;


		std::cout << std::endl;
		std::cout << "Initializing OpenCL..." << std::endl;
		std::cout << "  Creating context..." << std::flush;
		auto clContext = clCreateContext( NULL, 1, device, NULL, NULL, &errorCode);
		if (printResult(clContext, errorCode)) {
			return -1;
		}

		cl_program clProgram;

		// Create a program from the kernel source
		std::cout << "  Compiling kernel..." << std::flush;
		const std::string strVanity = readFile(SOURCE_FILE_CODE);
		const char * szKernels[] = { strVanity.c_str() };

		clProgram = clCreateProgramWithSource(clContext, sizeof(szKernels) / sizeof(char *), szKernels, NULL, &errorCode);
		if (printResult(clProgram, errorCode)) {
			return -1;
		}


		// Build the program
		std::cout << "  Building program..." << std::flush;
		if (printProgramBuildInfo(clBuildProgram(clProgram, (cl_uint)1, device, NULL, NULL, NULL), clProgram, *device)) {
			return -1;
		}

		std::cout << std::endl;
		int ret = Data.MallocResult(worksizeGlobal);
		if (ret != 0)
		{
			std::cout << "Error MallocResultBuffer()" << std::endl;
			return -1;
		}
		
		Dispatcher d(clContext, clProgram, worksizeGlobal, config);
		d.addDevice(*device, worksizeLocal, Data, public_key, Data.getResultBuffer());
		d.run();
		clReleaseContext(clContext);
		return 0;
}

