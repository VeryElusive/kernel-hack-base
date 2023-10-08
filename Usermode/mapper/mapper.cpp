#include "mapper.h"

void Mapper::RelocateImageByDelta( portable_executable::vec_relocs relocs, const uint64_t delta ) {
	for ( const auto& current_reloc : relocs ) {
		for ( auto i = 0u; i < current_reloc.count; ++i ) {
			const uint16_t type = current_reloc.item[ i ] >> 12;
			const uint16_t offset = current_reloc.item[ i ] & 0xFFF;

			if ( type == IMAGE_REL_BASED_DIR64 )
				*reinterpret_cast< uint64_t* >( current_reloc.address + offset ) += delta;
		}
	}
}

// Fix cookie by @Jerem584
bool Mapper::FixSecurityCookie( void* local_image, uint64_t kernel_image_base ) {
	auto headers = portable_executable::GetNtHeaders( local_image );
	if ( !headers )
		return false;

	auto load_config_directory = headers->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG ].VirtualAddress;
	if ( !load_config_directory )
		return true;

	auto load_config_struct = ( PIMAGE_LOAD_CONFIG_DIRECTORY ) ( ( uintptr_t ) local_image + load_config_directory );
	auto stack_cookie = load_config_struct->SecurityCookie;
	if ( !stack_cookie )
		return true; // as I said, it is not an error and we should allow that behavior

	stack_cookie = stack_cookie - ( uintptr_t ) kernel_image_base + ( uintptr_t ) local_image; //since our local image is already relocated the base returned will be kernel address

	if ( *( uintptr_t* ) ( stack_cookie ) != 0x2B992DDFA232 )
		return false;

	auto new_cookie = 0x2B992DDFA232 ^ GetCurrentProcessId( ) ^ GetCurrentThreadId( ); // here we don't really care about the value of stack cookie, it will still works and produce nice result
	if ( new_cookie == 0x2B992DDFA232 )
		new_cookie = 0x2B992DDFA233;

	*( uintptr_t* ) ( stack_cookie ) = new_cookie; // the _security_cookie_complement will be init by the driver itself if they use crt
	return true;
}

bool Mapper::ResolveImports( HANDLE iqvw64e_device_handle, portable_executable::vec_imports imports ) {
	for ( const auto& current_import : imports ) {
		ULONG64 Module = Utils::GetKernelModuleAddress( current_import.module_name );
		if ( !Module ) {
#if !defined(DISABLE_OUTPUT)
			std::cout << "[-] Dependency " << current_import.module_name << " wasn't found" << std::endl;
#endif
			return false;
		}

		for ( auto& current_function_data : current_import.function_datas ) {
			uint64_t function_address = intel_driver::GetKernelModuleExport( iqvw64e_device_handle, Module, current_function_data.name );

			if ( !function_address ) {
				//Lets try with ntoskrnl
				if ( Module != intel_driver::ntoskrnlAddr ) {
					function_address = intel_driver::GetKernelModuleExport( iqvw64e_device_handle, intel_driver::ntoskrnlAddr, current_function_data.name );
					if ( !function_address ) {
#if !defined(DISABLE_OUTPUT)
						std::cout << "[-] Failed to resolve import " << current_function_data.name << " (" << current_import.module_name << ")" << std::endl;
#endif
						return false;
					}
				}
			}

			*current_function_data.address = function_address;
		}
	}

	return true;
}

void Mapper::MapWorkerDriver( HANDLE iqvw64e_device_handle, uint8_t* data, void* comms ) {
	const PIMAGE_NT_HEADERS64 nt_headers = portable_executable::GetNtHeaders( data );

	if ( !nt_headers )
		return;

	if ( nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC )
		return;

	uint32_t image_size = nt_headers->OptionalHeader.SizeOfImage;

	void* local_image_base = VirtualAlloc( nullptr, image_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
	if ( !local_image_base )
		return;

	DWORD TotalVirtualHeaderSize = ( IMAGE_FIRST_SECTION( nt_headers ) )->VirtualAddress;
	image_size = image_size - TotalVirtualHeaderSize;

	uint64_t kernel_image_base = intel_driver::AllocatePool( iqvw64e_device_handle, POOL_TYPE::NonPagedPool, image_size );

	if ( !kernel_image_base ) {
		VirtualFree( local_image_base, 0, MEM_RELEASE );
		return;
	}

	do {
		// Copy image headers
		memcpy( local_image_base, data, nt_headers->OptionalHeader.SizeOfHeaders );

		// Copy image sections
		const PIMAGE_SECTION_HEADER current_image_section = IMAGE_FIRST_SECTION( nt_headers );

		for ( auto i = 0; i < nt_headers->FileHeader.NumberOfSections; ++i ) {
			if ( ( current_image_section[ i ].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ) > 0 )
				continue;

			auto local_section = reinterpret_cast< void* >( reinterpret_cast< uint64_t >( local_image_base ) + current_image_section[ i ].VirtualAddress );
			memcpy( local_section, reinterpret_cast< void* >( reinterpret_cast< uint64_t >( data ) + current_image_section[ i ].PointerToRawData ), current_image_section[ i ].SizeOfRawData );
		}

		uint64_t realBase = kernel_image_base;
		kernel_image_base -= TotalVirtualHeaderSize;

		// Resolve relocs and imports
		RelocateImageByDelta( portable_executable::GetRelocs( local_image_base ), kernel_image_base - nt_headers->OptionalHeader.ImageBase );

		if ( !FixSecurityCookie( local_image_base, kernel_image_base ) )
			break;

		if ( !ResolveImports( iqvw64e_device_handle, portable_executable::GetImports( local_image_base ) ) ) {
			kernel_image_base = realBase;
			break;
		}

		if ( !intel_driver::WriteMemory( iqvw64e_device_handle, realBase, ( PVOID ) ( ( uintptr_t ) local_image_base + TotalVirtualHeaderSize ), image_size ) ) {
			kernel_image_base = realBase;
			break;
		}

		// Call driver entry point
		const uint64_t address_of_entry_point = kernel_image_base + nt_headers->OptionalHeader.AddressOfEntryPoint;
		NTSTATUS status = 0;

		//void* base, char* communicationBuffer, int gamePID, int clientPID
		if ( !intel_driver::CallKernelFunction( iqvw64e_device_handle, &status, address_of_entry_point, realBase, comms ) ) {
			kernel_image_base = realBase;
			break;
		}

		kernel_image_base = realBase;

	} while ( false );

	VirtualFree( local_image_base, 0, MEM_RELEASE );

	// even tho most memory will be freed with the worker thread
	intel_driver::FreePool( iqvw64e_device_handle, kernel_image_base );
}