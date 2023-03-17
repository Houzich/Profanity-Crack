
#include "Buffer.h"
#include "Config/Config.hpp"
#include "utils.hpp"

HostBuffersClass* Data;



int ReadTablesToMemory(ConfigClass& config, HostBuffersClass& Data)
{
	int ret = 0;
	openBinaryFilesForRead(Data.instream, config.folder_8_bytes_keys);
//#pragma omp parallel for
	for (int num_file = 0; num_file < config.num_files; num_file++) {
		double percent = (num_file == 0) ? 0 : (100.0 / ((double)config.num_files / (double)num_file));
//#pragma omp critical (ReadTables)
		{
			printPercent("READ TABLES PROCESS: ", percent);
		}
		std::string filename = getFileName(config.folder_8_bytes_keys, num_file, "bin");
		size_t filesize = getBinaryFileSize(filename);
		if (filesize % (8) != 0) {
			std::cout << "Error file (" << filename << ") size (" << filesize << ")" << std::endl;
			return -1;
		}
		size_t numKeys = filesize / (8);
		ret = Data.MallocTable(num_file, numKeys * 8);
		if (ret != 0) return ret;

		uint8_t* data = Data.getTable(num_file);
		getFromBinaryFile(filename, data, 8 * numKeys, 0);
	}
	closeInStreams(Data.instream);
	printPercent("\nREAD TABLES PROCESS: ", 100.0);

	return 0;
}
















