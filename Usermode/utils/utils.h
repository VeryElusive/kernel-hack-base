#pragma once
#include "../sdk/windows/ntstructs.h"
#include "../sdk/vector.h"

#include <string>
#include <fstream>
#include <vector>

namespace Utils {
	__forceinline std::wstring GetFullTempPath( ) {
		wchar_t temp_directory[ MAX_PATH + 1 ] = { 0 };
		const uint32_t get_temp_path_ret = GetTempPathW( sizeof( temp_directory ) / 2, temp_directory );
		if ( !get_temp_path_ret || get_temp_path_ret > MAX_PATH + 1 )
			return L"";

		if ( temp_directory[ wcslen( temp_directory ) - 1 ] == L'\\' )
			temp_directory[ wcslen( temp_directory ) - 1 ] = 0x0;

		return std::wstring( temp_directory );
	}

	__forceinline bool ReadFileToMemory( const std::wstring& file_path, std::vector<uint8_t>* out_buffer ) {
		std::ifstream file_ifstream( file_path, std::ios::binary );

		if ( !file_ifstream )
			return false;

		out_buffer->assign( ( std::istreambuf_iterator<char>( file_ifstream ) ), std::istreambuf_iterator<char>( ) );
		file_ifstream.close( );

		return true;
	}

	__forceinline bool CreateFileFromMemory( const std::wstring& desired_file_path, const char* address, size_t size ) {
		std::ofstream file_ofstream( desired_file_path.c_str( ), std::ios_base::out | std::ios_base::binary );

		if ( !file_ofstream.write( address, size ) ) {
			file_ofstream.close( );
			return false;
		}

		file_ofstream.close( );
		return true;
	}

#define SYSTEM_MODULE_INFORMATION 11
#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004

	__forceinline uint64_t GetKernelModuleAddress( const std::string& module_name ) {
		void* buffer = nullptr;
		DWORD buffer_size = 0;

		NTSTATUS status = NtQuerySystemInformation( static_cast< SYSTEM_INFORMATION_CLASS >( SYSTEM_MODULE_INFORMATION ), buffer, buffer_size, &buffer_size );

		while ( status == STATUS_INFO_LENGTH_MISMATCH ) {
			if ( buffer != nullptr )
				VirtualFree( buffer, 0, MEM_RELEASE );

			buffer = VirtualAlloc( nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
			status = NtQuerySystemInformation( static_cast< SYSTEM_INFORMATION_CLASS >( SYSTEM_MODULE_INFORMATION ), buffer, buffer_size, &buffer_size );
		}

		if ( !NT_SUCCESS( status ) ) {
			if ( buffer != nullptr )
				VirtualFree( buffer, 0, MEM_RELEASE );
			return 0;
		}

		const auto modules = static_cast< PRTL_PROCESS_MODULES >( buffer );
		if ( !modules )
			return 0;

		for ( auto i = 0u; i < modules->NumberOfModules; ++i ) {
			const std::string current_module_name = std::string( reinterpret_cast< char* >( modules->Modules[ i ].FullPathName ) + modules->Modules[ i ].OffsetToFileName );

			if ( !_stricmp( current_module_name.c_str( ), module_name.c_str( ) ) )
			{
				const uint64_t result = reinterpret_cast< uint64_t >( modules->Modules[ i ].ImageBase );

				VirtualFree( buffer, 0, MEM_RELEASE );
				return result;
			}
		}

		VirtualFree( buffer, 0, MEM_RELEASE );
		return 0;
	}

	__forceinline BOOLEAN bDataCompare( const BYTE* pData, const BYTE* bMask, const char* szMask ) {
		for ( ; *szMask; ++szMask, ++pData, ++bMask )
			if ( *szMask == 'x' && *pData != *bMask )
				return 0;
		return ( *szMask ) == 0;
	}

	__forceinline uintptr_t FindPattern( uintptr_t dwAddress, uintptr_t dwLen, BYTE* bMask, const char* szMask ) {
		size_t max_len = dwLen - strlen( szMask );
		for ( uintptr_t i = 0; i < max_len; i++ )
			if ( bDataCompare( ( BYTE* ) ( dwAddress + i ), bMask, szMask ) )
				return ( uintptr_t ) ( dwAddress + i );
		return 0;
	}

	__forceinline PVOID FindSection( const char* sectionName, uintptr_t modulePtr, PULONG size ) {
		size_t namelength = strlen( sectionName );
		PIMAGE_NT_HEADERS headers = ( PIMAGE_NT_HEADERS ) ( modulePtr + ( ( PIMAGE_DOS_HEADER ) modulePtr )->e_lfanew );
		PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION( headers );
		for ( DWORD i = 0; i < headers->FileHeader.NumberOfSections; ++i ) {
			PIMAGE_SECTION_HEADER section = &sections[ i ];
			if ( memcmp( section->Name, sectionName, namelength ) == 0 &&
				namelength == strlen( ( char* ) section->Name ) ) {
				if ( !section->VirtualAddress ) {
					return 0;
				}
				if ( size ) {
					*size = section->Misc.VirtualSize;
				}
				return ( PVOID ) ( modulePtr + section->VirtualAddress );
			}
		}
		return 0;
	}

	__forceinline std::string WideToMultiByte( const std::wstring& wstr ) {
		const int sizeNeeded{ WideCharToMultiByte( CP_UTF8, 0, wstr.c_str( ), -1, nullptr, 0, nullptr, nullptr ) };
		std::string result( sizeNeeded, 0 );
		WideCharToMultiByte( CP_UTF8, 0, wstr.c_str( ), -1, &result[ 0 ], sizeNeeded, nullptr, nullptr );
		return result;
	}

	__forceinline std::wstring MultiByteToWide( const std::string& str ) {
		const int sizeNeeded{ MultiByteToWideChar( CP_UTF8, 0, str.c_str( ), -1, nullptr, 0 ) };
		std::wstring result( sizeNeeded, 0 );
		MultiByteToWideChar( CP_UTF8, 0, str.c_str( ), -1, &result[ 0 ], sizeNeeded );
		return result;
	}
}