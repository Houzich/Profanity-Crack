#ifndef HPP_CRACK_PUB_KEY
#define HPP_CRACK_PUB_KEY

#include <stdexcept>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#define clCreateCommandQueueWithProperties clCreateCommandQueue
#else
#include <CL/cl.h>
#endif

#include "SpeedSample.hpp"
#include "CLMemory.hpp"
#include "types.hpp"
#include "Data/Buffer.h"



class Dispatcher {
private:
	class OpenCLException : public std::runtime_error {
	public:
		OpenCLException(const std::string s, const cl_int res);

		static void throwIfError(const std::string s, const cl_int res);

		const cl_int m_res;
	};

	struct Device {
		static cl_command_queue createQueue(cl_context& clContext, cl_device_id& clDeviceId);
		static cl_kernel createKernel(cl_program& clProgram, const std::string s);
		static cl_ulong4 genPrivateKeys(private_key* priv_keys, size_t size, cl_ulong4 init_value);

		Device(Dispatcher& parent, 
			cl_context& clContext, 
			cl_program& clProgram, 
			cl_device_id clDeviceId, 
			const size_t worksizeLocal, 
			const size_t size, 
			HostBuffersClass& Data,
			point public_key,
			point* result);
		~Device();

		
		HostBuffersClass& data;
		point public_key;
		point* result;

		Dispatcher& m_parent;

		cl_device_id m_clDeviceId;
		size_t m_worksizeLocal;

		cl_command_queue m_clQueue;
		cl_kernel m_kernelInit;
		cl_kernel m_kernelCrack;

		CLMemory<point> m_memPrecomp;
		CLMemory<point> m_memExtensionPublicKey;
		CLMemory<point> m_memPublicKey;
		CLMemory<point> m_memResult;

		// Seed and round information
		cl_ulong m_round;
		// Speed sampling
		SpeedSample m_speed;

		// Initialization
		size_t m_sizeInitialized;
		cl_event m_eventFinished;
	};

public:
	Dispatcher::Dispatcher(cl_context& clContext, cl_program& clProgram, const size_t worksize, ConfigClass& config);
	~Dispatcher();

	void addDevice(cl_device_id clDeviceId, 
		const size_t worksizeLocal, 
		HostBuffersClass& Data,
		point public_key,
		point* result);
	void run();

private:
	void init();
	void initBegin();
	void initContinue();

	void dispatch(Device& d);
	void enqueueKernel(cl_command_queue& clQueue, cl_kernel& clKernel, size_t worksizeGlobal, const size_t worksizeLocal, cl_event* pEvent);
	void enqueueKernelDevice(cl_kernel& clKernel, size_t worksizeGlobal, cl_event* pEvent);
	void flushKernelCrack(bool bBlock);
	void handleResult();

	void onEvent(cl_event event, cl_int status, Device& d);
	void onEventInit(cl_event event, cl_int status, Device& d);
	void printSpeed(size_t round);

private:
	static void CL_CALLBACK staticCallback(cl_event event, cl_int event_command_exec_status, void* user_data);
	static void CL_CALLBACK initCallback(cl_event event, cl_int event_command_exec_status, void* user_data);
	static std::string formatSpeed(double s);

private: /* Instance variables */
	cl_context& m_clContext;
	cl_program& m_clProgram;
	const size_t m_size;

	Device* Dev;

	cl_event m_eventFinished;

	// Run information
	std::mutex mtx;
	std::chrono::time_point<std::chrono::steady_clock> timeStart;
	size_t m_sizeInitTotal;
	size_t m_sizeInitDone;
	bool m_quit;
	ConfigClass& config;

};

#endif /* HPP_CRACK_PUB_KEY */
