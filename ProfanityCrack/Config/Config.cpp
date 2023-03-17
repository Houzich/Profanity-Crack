
#include "Config.hpp"


int parse_gonfig(ConfigClass* config, std::string path)
{
	try {
		const tao::config::value v = tao::config::from_file(path);
		config->gpu_local_size = access(v, tao::config::key("gpu_local_size")).get_unsigned();
		config->folder_keys = access(v, tao::config::key("folder_keys")).get_string();
		config->folder_8_bytes_keys = access(v, tao::config::key("folder_8_bytes_keys")).get_string();
	}
	catch (std::runtime_error& e) {
		std::cerr << "Error parse config.cfg file " << path << " : " << e.what() << '\n';
		throw;
	}
	catch (...) {
		std::cerr << "Error parse config.cfg file, unknown exception occured" << std::endl;
		throw;
	}
	return 0;
}


