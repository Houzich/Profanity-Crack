/*****************************************************************************
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <memory.h>
#include <windows.h>
#include <thread>
#include <vector>
#include <array>
 // Suppress a compiler warning about undefined CL_TARGET_OPENCL_VERSION
 // Khronos ICD supports only latest OpenCL version
 //#define CL_TARGET_OPENCL_VERSION 220

#include "CL\cl.h"
#include "CL\cl_ext.h"

#include "utils.hpp"
#include "CrackPublicKey/Crack.hpp"
#include "Data/Buffer.h"
#include "Config/Config.hpp"


int main(int argc, char** argv) {
	int ret = 0;

	std::vector <result> crack_res(1);
	HostBuffersClass Data = new HostBuffersClass(true);

	setlocale(LC_ALL, "Russian");
	system("chcp 1251");


	ConfigClass config;
	parse_gonfig(&config, "config.cfg");

	std::vector<cl_device_id> vFoundDevices = getAllDevices();
	cl_device_id Device = NULL;

	std::wcout << L"Найденные видеокарты:" << std::endl;
	for (size_t i = 0; i < vFoundDevices.size(); ++i) {

		if (vFoundDevices[i] == NULL) continue;
		cl_device_id& deviceId = vFoundDevices[i];

		const auto strName = clGetWrapperString(clGetDeviceInfo, deviceId, CL_DEVICE_NAME);
		const auto computeUnits = clGetWrapper<cl_uint>(clGetDeviceInfo, deviceId, CL_DEVICE_MAX_COMPUTE_UNITS);
		const auto globalMemSize = clGetWrapper<cl_ulong>(clGetDeviceInfo, deviceId, CL_DEVICE_GLOBAL_MEM_SIZE);

		std::cout << "  [" << i << "]" << " GPU" << ": " << strName << ", " << globalMemSize << " bytes available, " << computeUnits << std::endl;

	}
	size_t number_device = 0;
	std::string pubKeyIn = "";

	std::wcout << L"Введите номер используемой видеокарты: ";
	std::cin >> number_device;
	if (number_device >= vFoundDevices.size()) {
		std::wcout << L"ERROR: неверный номер устройства: " << "  [" << number_device << "]\n";
		std::wcout << L"Всего найденных устройств: " << vFoundDevices.size() << "\n";
		goto exit;
	}


	std::wcout << L"Введите искомый публичный ключ: ";
	std::cin >> pubKeyIn;
	if (pubKeyIn.size() != 128) {
		std::wcout << L"ERROR: неверная длина публичного ключа: " << "  [" << pubKeyIn.size() << "]\n";
		goto exit;
	}


	Device = vFoundDevices[number_device];
	if (Device == NULL) {
		goto exit;
	}


	ret = ReadTablesToMemory(config, Data);
	if (ret != 0) {
		std::cout << "ERROR: read table files" << std::endl;
		goto exit;
	}

	std::cout << "\n*************** Crack START! ********************\n" << std::endl;
	point pubKey;
	stringToPoint(pubKeyIn, &pubKey);

	if (crack_public_key(&Device, Data, pubKey, crack_res, config)) {
		for (;;)
			std::this_thread::sleep_for(std::chrono::seconds(30));
	}

	std::cout << "\n*************** Crack END! **********************\n" << std::endl;
	std::cout << "\n\n";
	std::cout << "FINISH!!!!!\n";
	std::cout << "FINISH!!!!!\n";
	std::cout << "FINISH!!!!!\n";
exit:
	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::seconds(30000));
	}
		
}

