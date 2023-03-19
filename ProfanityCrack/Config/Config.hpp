#ifndef HPP_CONFIG
#define HPP_CONFIG

#include <string>
#include <tao/config.hpp>


struct ConfigClass
{
public:
	size_t gpu_local_size = 64;
	std::string folder_keys = "";
	std::string folder_8_bytes_keys = "";
	size_t num_files = 256;
public:
	ConfigClass()
	{
	}
	~ConfigClass()
	{
	}
};


int parse_gonfig(ConfigClass* config, std::string path);
#endif /* HPP_CONFIG */
