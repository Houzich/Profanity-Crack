#ifndef HPP_CRACK_PUBLIC_KEY
#define HPP_CRACK_PUBLIC_KEY

#include <string>
#include "Data/Buffer.h"

int crack_public_key(cl_device_id* device, HostBuffersClass& Data, point public_key, std::vector< result>& result, ConfigClass& config);

#endif /* HPP_CRACK_PUBLIC_KEY */
