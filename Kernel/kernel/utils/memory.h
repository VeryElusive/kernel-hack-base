#pragma once
#include "../sdk/windows/ntapi.h"
#include "../communication/communication.h"

namespace Memory {
	bool Read( void* address, void* buffer, int size ) {
		size_t bytes{ };
		const auto status = MmCopyVirtualMemory( Communication::Process, address, IoGetCurrentProcess( ), buffer, size, KernelMode, &bytes );

		return status >= 0;
	}

	bool Write( void* address, void* buffer, int size ) {
		size_t bytes{ };
		const auto status = MmCopyVirtualMemory( IoGetCurrentProcess( ), buffer, Communication::Process, address, size, KernelMode, &bytes );

		return status >= 0;
	}
}