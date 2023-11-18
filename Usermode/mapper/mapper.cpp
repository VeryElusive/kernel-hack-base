#include "mapper.h"
#include "../../shared_structs.h"

void CMapper::RelocateImageByDelta( portable_executable::vec_relocs relocs, const uint64_t delta ) {
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
bool CMapper::FixSecurityCookie( void* local_image, uint64_t kernel_image_base ) {
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

bool CMapper::ResolveImports( HANDLE iqvw64e_device_handle, portable_executable::vec_imports imports ) {
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

uint64_t CMapper::AllocIndependentPages( HANDLE device_handle, uint32_t size )
{
	const auto base = intel_driver::MmAllocateIndependentPagesEx( device_handle, size );
	if ( !base )
	{
		//Log( L"[-] Error allocating independent pages" << std::endl );
		return 0;
	}

	if ( !intel_driver::MmSetPageProtection( device_handle, base, size, PAGE_EXECUTE_READWRITE ) )
	{
		//Log( L"[-] Failed to change page protections" << std::endl );
		intel_driver::MmFreeIndependentPages( device_handle, base, size );
		return 0;
	}

	return base;
}

//????
#define PAGE_SIZE 0x1000
uint64_t AllocMdlMemory( HANDLE iqvw64e_device_handle, uint64_t size, uint64_t* mdlPtr ) {
	/*added by psec*/
	LARGE_INTEGER LowAddress, HighAddress;
	LowAddress.QuadPart = 0;
	HighAddress.QuadPart = 0xffff'ffff'ffff'ffffULL;

	uint64_t pages = ( size / PAGE_SIZE ) + 1;
	auto mdl = intel_driver::MmAllocatePagesForMdl( iqvw64e_device_handle, LowAddress, HighAddress, LowAddress, pages * ( uint64_t ) PAGE_SIZE );
	if ( !mdl ) {
		//Log( L"[-] Can't allocate pages for mdl" << std::endl );
		return { 0 };
	}

	uint32_t byteCount = 0;
	if ( !intel_driver::ReadMemory( iqvw64e_device_handle, mdl + 0x028 /*_MDL : byteCount*/, &byteCount, sizeof( uint32_t ) ) ) {
		//Log( L"[-] Can't read the _MDL : byteCount" << std::endl );
		return { 0 };
	}

	if ( byteCount < size ) {
		//Log( L"[-] Couldn't allocate enough memory, cleaning up" << std::endl );
		intel_driver::MmFreePagesFromMdl( iqvw64e_device_handle, mdl );
		intel_driver::FreePool( iqvw64e_device_handle, mdl );
		return { 0 };
	}

	auto mappingStartAddress = intel_driver::MmMapLockedPagesSpecifyCache( iqvw64e_device_handle, mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority );
	if ( !mappingStartAddress ) {
		//Log( L"[-] Can't set mdl pages cache, cleaning up." << std::endl );
		intel_driver::MmFreePagesFromMdl( iqvw64e_device_handle, mdl );
		intel_driver::FreePool( iqvw64e_device_handle, mdl );
		return { 0 };
	}

	const auto result = intel_driver::MmProtectMdlSystemAddress( iqvw64e_device_handle, mdl, PAGE_EXECUTE_READWRITE );
	if ( !result ) {
		//Log( L"[-] Can't change protection for mdl pages, cleaning up" << std::endl );
		intel_driver::MmUnmapLockedPages( iqvw64e_device_handle, mappingStartAddress, mdl );
		intel_driver::MmFreePagesFromMdl( iqvw64e_device_handle, mdl );
		intel_driver::FreePool( iqvw64e_device_handle, mdl );
		return { 0 };
	}
	//Log( L"[+] Allocated pages for mdl" << std::endl );

	if ( mdlPtr )
		*mdlPtr = mdl;

	return mappingStartAddress;
}

void CMapper::MapWorkerDriver( HANDLE iqvw64e_device_handle, uint8_t* data, void* comms ) {
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

	uint64_t kernel_image_base = AllocIndependentPages( iqvw64e_device_handle, image_size );

	if ( !kernel_image_base ) {
		VirtualFree( local_image_base, 0, MEM_RELEASE );
		return;
	}

	// TODO: remove everything that is unnecessary!
	do {
		// Copy image headers
		memcpy( local_image_base, data, nt_headers->OptionalHeader.SizeOfHeaders );

		// Copy image sections
		const PIMAGE_SECTION_HEADER current_image_section = IMAGE_FIRST_SECTION( nt_headers );
		for ( auto i = 0; i < nt_headers->FileHeader.NumberOfSections; ++i ) {
			if ( ( current_image_section[ i ].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ) > 0 )
				continue;

			const char* sectionName = reinterpret_cast< const char* >( current_image_section[ i ].Name );
			char sectionNameStr[ 9 ];
			memcpy( sectionNameStr, sectionName, 8 );
			sectionNameStr[ 8 ] = '\0';

			auto local_section = reinterpret_cast< void* >( reinterpret_cast< uint64_t >( local_image_base ) + current_image_section[ i ].VirtualAddress );

			if ( strcmp( sectionNameStr, ".reloc" ) == 0 ||
				 strcmp( sectionNameStr, ".idata" ) == 0 ||
				 strcmp( sectionNameStr, ".rsrc" ) == 0 )
				continue;

			memcpy( local_section, reinterpret_cast< void* >( reinterpret_cast< uint64_t >( data ) + current_image_section[ i ].PointerToRawData ), current_image_section[ i ].SizeOfRawData );
		}

		// TODO: cleanup!!!
		uint64_t realBase = kernel_image_base;
		kernel_image_base -= TotalVirtualHeaderSize;

		// Resolve relocs and imports
		RelocateImageByDelta( portable_executable::GetRelocs( local_image_base ), kernel_image_base - nt_headers->OptionalHeader.ImageBase );

		//if ( !FixSecurityCookie( local_image_base, kernel_image_base ) ) {
		//	break;
		//}

		if ( !ResolveImports( iqvw64e_device_handle, portable_executable::GetImports( local_image_base ) ) ) {
			kernel_image_base = realBase;
			break;
		}

		const uint64_t kernel_entry = kernel_image_base + nt_headers->OptionalHeader.AddressOfEntryPoint;
		//void* local_entry = ( void* ) ( ( uintptr_t ) local_image_base + nt_headers->OptionalHeader.AddressOfEntryPoint );

		if ( !intel_driver::WriteMemory( iqvw64e_device_handle, realBase, ( PVOID ) ( ( uintptr_t ) local_image_base + TotalVirtualHeaderSize ), image_size ) ) {
			kernel_image_base = realBase;
			break;
		}

		VirtualFree( local_image_base, 0, MEM_RELEASE );

		// Call driver entry point
		NTSTATUS status = 0;
		intel_driver::CallKernelFunction( iqvw64e_device_handle, &status, kernel_entry, comms );

		auto handle{ intel_driver::Load( ) };
		if ( handle == INVALID_HANDLE_VALUE )
			handle = iqvw64e_device_handle;

		if ( handle != INVALID_HANDLE_VALUE ) {
			intel_driver::MmFreeIndependentPages( iqvw64e_device_handle, realBase, image_size );

			if ( !intel_driver::Unload( handle ) )
				printf( "failed to unload driver. \n" );
		}

		return;

	} while ( false );

	VirtualFree( local_image_base, 0, MEM_RELEASE );
	intel_driver::MmFreeIndependentPages( iqvw64e_device_handle, kernel_image_base, image_size );
}