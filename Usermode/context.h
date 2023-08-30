#pragma once
#include "../shared_structs.h"
#include <mutex>
#include <condition_variable>

namespace Context {
	inline DataRequest_t CommunicationBuffer;
	inline CommsParse_t Comms{ };

	inline std::mutex mtx;
	inline std::condition_variable cv;
}