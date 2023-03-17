#pragma once
#include <stdint.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <memory>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <omp.h>
#include "Data/Buffer.h"
#include "Config/Config.hpp"
#include "types.hpp"

class HostBuffersClass
{
private:
	uint8_t* key_table[256] = { NULL };
	uint8_t* tempBuffer = NULL;
	size_t key_table_size[256] = { 0 };
	size_t tempBufferSize = 0;
	uint64_t memory_size = 0;
	point* resultBuffer;
	uint64_t result_size = 0;
public:
	std::ofstream outstream[256];
	std::ifstream instream[256];

	HostBuffersClass()
	{
	}
	HostBuffersClass(bool malloc)
	{
		if (malloc) Malloc();
	}
	std::string FormatWithCommas(uint64_t value)
	{
		std::stringstream ss;
		ss.imbue(std::locale(""));
		ss << std::fixed << value;
		return ss.str();
	}
	int alignedMalloc8(uint8_t** point, uint64_t size, uint64_t* all_ram_memory_size, std::string buff_name) {
		void* ptr = _aligned_malloc(size, 4096);
		if (ptr == NULL) {
			printf("\n!!!!ERROR!!!! MALLOC: _aligned_malloc (%s) failed! Size: %s\n\n", buff_name.c_str(), FormatWithCommas(size).data()); 
			return -1; 
		}
		*point = (uint8_t*)ptr;
		*all_ram_memory_size += size;
		return 0;
	}
	int alignedMalloc32(uint32_t** point, uint64_t size, uint64_t* all_ram_memory_size, std::string buff_name) {
		return alignedMalloc8((uint8_t**)point, size, all_ram_memory_size, buff_name);
	}
	int alignedMalloc64(uint64_t** point, uint64_t size, uint64_t* all_ram_memory_size, std::string buff_name) {
		return alignedMalloc8((uint8_t**)point, size, all_ram_memory_size, buff_name);
	}

	int Malloc()
	{
		memory_size = 0;
		return 0;
	}

	int MallocTable(uint32_t num, size_t size)
	{
		int ret = alignedMalloc8(&key_table[num], size, &memory_size, "key_table[" + std::to_string(num) + "]");
		if (ret == 0) {
			key_table_size[num] = size;
			std::cout << " MALLOC TABLE[" + std::to_string(num) + "]  " << std::to_string((float)size / (1024.0f * 1024.0f)) << " MB, ALL RAM MEMORY SIZE (ALL): " << std::to_string((float)memory_size / (1024.0f * 1024.0f * 1024.0f)) << " GB\r";
			return 0;
		}
		else
		{
			return ret;
		}
	}

	int MallocTempBuffer(size_t size)
	{
		int ret = alignedMalloc8(&tempBuffer, size, &memory_size, "tempBuffer");
		if (ret == 0) {
			tempBufferSize = size;
			return 0;
		}
		else
		{
			return ret;
		}
	}

	int MallocResult(size_t size)
	{
		int ret = alignedMalloc8((uint8_t**)&resultBuffer, size * sizeof(point), &memory_size, "resultBuffer");
		if (ret == 0) {
			result_size = size;
			return 0;
		}
		else
		{
			return ret;
		}
	}
	void FreeResult()
	{
		if (resultBuffer != NULL) {
			_aligned_free(resultBuffer);
			memory_size -= result_size;
		}
	}



	uint8_t* getTempBuffer()
	{
		return tempBuffer;
	}
	size_t getSizeTempBuffer()
	{
		return tempBufferSize;
	}
	uint8_t* getTable(uint32_t num)
	{
		return key_table[num];
	}
	size_t getSizeTable(uint32_t num)
	{
		return key_table_size[num];
	}
	size_t addSizeTable(uint32_t num, size_t size)
	{
		key_table_size[num] += size;
	}

	point* getResultBuffer()
	{
		return resultBuffer;
	}


	void free_tale_buffers(void) {
		for (int x = 0; x < 256; x++) {
			if (key_table[x] != NULL) {
				_aligned_free(key_table[x]);
				memory_size -= key_table_size[x];
			}
				
		}
	}
	void FreeTempBuffer()
	{
		if (tempBuffer != NULL) {
			_aligned_free(tempBuffer);
			memory_size -= tempBufferSize;
		}
	}
	~HostBuffersClass()
	{
		free_tale_buffers();
		FreeTempBuffer();
		FreeResult();
		//for CPU
	}

};


extern HostBuffersClass* Data;
int ReadTablesToMemory(ConfigClass& config, HostBuffersClass& Data);
