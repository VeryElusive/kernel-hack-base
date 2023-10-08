#pragma once
#include "../ext/intel_driver.h"
#include "../ext/portable_executable.h"

namespace Mapper {
	void MapWorkerDriver( HANDLE iqvw64e_device_handle, uint8_t* data, void* comms );


	void RelocateImageByDelta( portable_executable::vec_relocs relocs, const uint64_t delta );
	bool FixSecurityCookie( void* local_image, uint64_t kernel_image_base );
	bool ResolveImports( HANDLE iqvw64e_device_handle, portable_executable::vec_imports imports );
}