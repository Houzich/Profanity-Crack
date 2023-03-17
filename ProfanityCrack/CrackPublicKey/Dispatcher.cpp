

// Includes
#include <stdexcept>
#include <iostream>
#include <thread>
#include <sstream>
#include <iomanip>
#include <random>
#include <thread>
#include <algorithm>

#include "precomp.hpp"
#include "utils.hpp"
#include "Dispatcher.hpp"
#include "Data/Buffer.h"
#include "Data/Find.h"

Dispatcher::OpenCLException::OpenCLException(const std::string s, const cl_int res) :
	std::runtime_error(s + " (res = " + toString(res) + ")"),
	m_res(res)
{

}

void Dispatcher::OpenCLException::OpenCLException::throwIfError(const std::string s, const cl_int res) {
	if (res != CL_SUCCESS) {
		throw OpenCLException(s, res);
	}
}

cl_command_queue Dispatcher::Device::createQueue(cl_context& clContext, cl_device_id& clDeviceId) {
	// nVidia CUDA Toolkit 10.1 only supports OpenCL 1.2 so we revert back to older functions for compatability
#ifdef PROFANITY_DEBUG
	//cl_command_queue_properties p = CL_QUEUE_PROFILING_ENABLE;
	cl_command_queue_properties p = NULL;
#else
	cl_command_queue_properties p = NULL;
#endif

#ifdef CL_VERSION_2_0
	const cl_command_queue ret = clCreateCommandQueueWithProperties(clContext, clDeviceId, &p, NULL);
#else
	const cl_command_queue ret = clCreateCommandQueue(clContext, clDeviceId, p, NULL);
#endif
	return ret == NULL ? throw std::runtime_error("failed to create command queue") : ret;
}

cl_kernel Dispatcher::Device::createKernel(cl_program& clProgram, const std::string s) {
	cl_kernel ret = clCreateKernel(clProgram, s.c_str(), NULL);
	return ret == NULL ? throw std::runtime_error("failed to create kernel \"" + s + "\"") : ret;
}



//(any random 256-bit number from 0x1 to 0xFFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFE BAAE DCE6 AF48 A03B BFD2 5E8C D036 4140)
cl_ulong4 Dispatcher::Device::genPrivateKeys(private_key* priv_keys, size_t size, cl_ulong4 init_value) {

	cl_ulong4 value;
	value.s[0] = init_value.s[0];
	value.s[1] = init_value.s[1];
	value.s[2] = init_value.s[2];
	value.s[3] = init_value.s[3];

	for (size_t i = 0; i < size; i++)
	{
		priv_keys[i].key.s[0] = value.s[0];
		priv_keys[i].key.s[1] = value.s[1];
		priv_keys[i].key.s[2] = value.s[2];
		priv_keys[i].key.s[3] = value.s[3];
		incUlong4(&value);
	}
	return value;
}


Dispatcher::Device::Device(
	Dispatcher& parent,
	cl_context& clContext,
	cl_program& clProgram,
	cl_device_id clDeviceId,
	const size_t worksizeLocal,
	const size_t size,
	HostBuffersClass& Data,
	point public_key,
	point* result) :
	m_parent(parent),
	m_clDeviceId(clDeviceId),
	m_worksizeLocal(worksizeLocal),
	m_clQueue(createQueue(clContext, clDeviceId)),
	m_kernelInit(createKernel(clProgram, "crack_init")),
	m_kernelCrack(createKernel(clProgram, "crack")),
	m_memPrecomp(clContext, m_clQueue, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(g_precomp), g_precomp),
	//m_memTable(clContext, m_clQueue, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, table.size()),
	m_memExtensionPublicKey(clContext, m_clQueue, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, size, true),
	m_memPublicKey(clContext, m_clQueue, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, 1),
	m_memResult(clContext, m_clQueue, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, size),
	//m_memSizeTable((cl_uint)table.size()),
	m_round(0),
	m_speed(SPEED_SAMPLES),
	m_sizeInitialized(0),
	data(Data),
	public_key(public_key),
	result(result),
	m_eventFinished(NULL)
{

		for (size_t i = 0; i < 8; i++)
		{
			m_memPublicKey.data()[0].x.d[i] = public_key.x.d[i];
			m_memPublicKey.data()[0].y.d[i] = public_key.y.d[i];
		}
}

Dispatcher::Device::~Device() {

}
Dispatcher::Dispatcher(cl_context& clContext, cl_program& clProgram, const size_t worksize, ConfigClass& config) :
	m_clContext(clContext),
	m_clProgram(clProgram),
	m_size(worksize),
	m_sizeInitTotal(worksize),
	m_sizeInitDone(0),
	m_quit(false),
	Dev(NULL),
	m_eventFinished(NULL),
	config(config)
{

}

Dispatcher::~Dispatcher() {

}

void Dispatcher::addDevice(cl_device_id clDeviceId, const size_t worksizeLocal, HostBuffersClass& Data, point public_key, point *result) {
	Device* pDevice = new Device(*this, m_clContext, m_clProgram, clDeviceId, worksizeLocal, m_size, Data, public_key, result);
	Dev = pDevice;
}

void Dispatcher::run() {
	m_eventFinished = clCreateUserEvent(m_clContext, NULL);

	init();
	flushKernelCrack(false);

	clWaitForEvents(1, &m_eventFinished);
	clReleaseEvent(m_eventFinished);
	m_eventFinished = NULL;
}

void Dispatcher::init() {
	std::wcout << L"Инициализация..." << std::endl;
	std::wcout << L"Может занять несколько минут..." << std::endl;
	m_sizeInitTotal = m_size;
	m_sizeInitDone = 0;

	cl_event const InitEvent = clCreateUserEvent(m_clContext, NULL);
	Dev->m_eventFinished = InitEvent;
	initBegin();

	clWaitForEvents((cl_uint)1, &InitEvent);
	Dev->m_eventFinished = NULL;
	clReleaseEvent(InitEvent);

	std::cout << std::endl;
}

void Dispatcher::initBegin() {

	// Write precompute table
	Dev->m_memPrecomp.write(true);
	Dev->m_memPublicKey.write(true);
	// Kernel arguments - init
	Dev->m_memPrecomp.setKernelArg(Dev->m_kernelInit, 0);
	Dev->m_memExtensionPublicKey.setKernelArg(Dev->m_kernelInit, 1);
	Dev->m_memPublicKey.setKernelArg(Dev->m_kernelInit, 2);

	// Kernel arguments - crack
	Dev->m_memExtensionPublicKey.setKernelArg(Dev->m_kernelCrack, 0);
	Dev->m_memResult.setKernelArg(Dev->m_kernelCrack, 1);
	// Seed device
	initContinue();
}

void Dispatcher::initContinue() {
	size_t sizeLeft = m_size - Dev->m_sizeInitialized;
	size_t sizeInitLimit = m_size / SPEED_SAMPLES;
	if (m_size < SPEED_SAMPLES) sizeInitLimit = m_size;

	// Print progress
	const size_t percentDone = m_sizeInitDone * 100 / m_sizeInitTotal;
	std::cout << "  " << percentDone << "%\r" << std::flush;
	if (sizeLeft) {
		cl_event event;
		const size_t sizeRun = std::min(sizeInitLimit, sizeLeft);
		const auto resEnqueue = clEnqueueNDRangeKernel(Dev->m_clQueue, Dev->m_kernelInit, 1, &Dev->m_sizeInitialized, &sizeRun, NULL, 0, NULL, &event);
		OpenCLException::throwIfError("kernel queueing failed during initilization", resEnqueue);

		const cl_int ret = clFlush(Dev->m_clQueue);
		if (ret != CL_SUCCESS) {
			throw std::runtime_error("initContinue() Error clFlush - " + toString(ret));
		}

		Dev->m_sizeInitialized += sizeRun;
		m_sizeInitDone += sizeRun;

		const auto resCallback = clSetEventCallback(event, CL_COMPLETE, initCallback, Dev);
		OpenCLException::throwIfError("failed to set custom callback during initialization", resCallback);

	}
	else {
		// Printing one whole string at once helps in avoiding garbled output when executed in parallell
		const std::string strOutput = "  Crack programm initialized";
		std::cout << strOutput << std::endl;
		clSetUserEventStatus(Dev->m_eventFinished, CL_COMPLETE);
	}

}

void Dispatcher::enqueueKernel(cl_command_queue& clQueue, cl_kernel& clKernel, size_t worksizeGlobal, const size_t worksizeLocal, cl_event* pEvent = NULL) {
	size_t worksizeOffset = 0;
	while (worksizeGlobal) {
		const size_t worksizeRun = worksizeGlobal;
		const size_t* const pWorksizeLocal = (worksizeLocal == 0 ? NULL : &worksizeLocal);
		const auto res = clEnqueueNDRangeKernel(clQueue, clKernel, 1, &worksizeOffset, &worksizeRun, pWorksizeLocal, 0, NULL, pEvent);
		OpenCLException::throwIfError("kernel queueing failed", res);

		worksizeGlobal -= worksizeRun;
		worksizeOffset += worksizeRun;
	}
}

void Dispatcher::enqueueKernelDevice(cl_kernel& clKernel, size_t worksizeGlobal, cl_event* pEvent = NULL) {
	try {
		enqueueKernel(Dev->m_clQueue, clKernel, worksizeGlobal, Dev->m_worksizeLocal, pEvent);
	}
	catch (OpenCLException& e) {
		// If local work size is invalid, abandon it and let implementation decide
		if ((e.m_res == CL_INVALID_WORK_GROUP_SIZE || e.m_res == CL_INVALID_WORK_ITEM_SIZE) && Dev->m_worksizeLocal != 0) {
			std::cout << std::endl << "warning: local work size abandoned on GPU" << std::endl << std::endl;
			Dev->m_worksizeLocal = 0;
			enqueueKernel(Dev->m_clQueue, clKernel, worksizeGlobal, Dev->m_worksizeLocal, pEvent);
		}
		else {
			throw;
		}
	}
}

void Dispatcher::flushKernelCrack(bool bBlock) {
	cl_event evRead;

	Dev->m_memResult.read(bBlock, &evRead);
	enqueueKernelDevice(Dev->m_kernelCrack, m_size);
	clFlush(Dev->m_clQueue);

	const auto res = clSetEventCallback(evRead, CL_COMPLETE, staticCallback, Dev);
	OpenCLException::throwIfError("failed to set custom callback", res);
}


void Dispatcher::dispatch(Device& d) {
	//if overflow
	flushKernelCrack(false);
}

void Dispatcher::handleResult() {
	point* res = Dev->result;
	if (Search_Keys_In_Memory(Dev->data, res, m_size, Dev->m_round, config) == 1) {
		m_quit = true;
	}
}

void Dispatcher::onEvent(cl_event event, cl_int status, Device& d) {
	if (status != CL_COMPLETE) {
		std::cout << "DispatcherCrack::onEvent - Got bad status: " << status << std::endl;
	}
	else 
	{
		std::lock_guard<std::mutex> lock(mtx);
		++d.m_round;
		d.m_speed.sample((double)m_size);
		printSpeed(d.m_round);

		if (m_quit) {
			clSetUserEventStatus(m_eventFinished, CL_COMPLETE);
		}

		if (!m_quit) {
			memcpy(Dev->result, Dev->m_memResult.data(), m_size*sizeof(point));
			dispatch(d);
			handleResult();
		}
	}
}

void Dispatcher::onEventInit(cl_event event, cl_int status, Device& d) {
	if (status != CL_COMPLETE) {
		std::cout << "DispatcherCrack::onEventInit - Got bad status: " << status << std::endl;
	}
	else if (d.m_eventFinished != NULL) {
		initContinue();
	}

}



// This is run when m_mutex is held.
void Dispatcher::printSpeed(size_t round) {
	std::string strGPUs;
	double speedTotal = 0;
	unsigned int i = 0;

	const auto curSpeed = Dev->m_speed.getSpeed();
	speedTotal += curSpeed;
	strGPUs += " GPU: " + formatSpeed(curSpeed);


	const std::string strVT100ClearLine = "\33[2K\r";
	std::cerr << strVT100ClearLine << "Total: " << formatSpeed(speedTotal) << " -" << strGPUs << ", Round: " << round << '\r' << std::flush;
}

void CL_CALLBACK Dispatcher::staticCallback(cl_event event, cl_int event_command_exec_status, void* user_data) {
	Device* const pDevice = static_cast<Device*>(user_data);
	pDevice->m_parent.onEvent(event, event_command_exec_status, *pDevice);
	clReleaseEvent(event);
}

void CL_CALLBACK Dispatcher::initCallback(cl_event event, cl_int event_command_exec_status, void* user_data) {
	Device* const pDevice = static_cast<Device*>(user_data);
	pDevice->m_parent.onEventInit(event, event_command_exec_status, *pDevice);
	clReleaseEvent(event);
}


std::string Dispatcher::formatSpeed(double f) {
	const std::string S = " KMGT";

	unsigned int index = 0;
	while (f > 1000.0f && index < S.size()) {
		f /= 1000.0f;
		++index;
	}

	std::ostringstream ss;
	ss << std::fixed << std::setprecision(5) << (double)f << " " << S[index] << "H/s";
	return ss.str();
}
