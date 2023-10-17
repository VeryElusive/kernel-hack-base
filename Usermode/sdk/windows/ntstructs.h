#pragma once
#include <windef.h>
#include <winternl.h>

typedef NTSTATUS( *NtLoadDriver_t )( PUNICODE_STRING DriverServiceName );
typedef NTSTATUS( *NtUnloadDriver_t )( PUNICODE_STRING DriverServiceName );
typedef NTSTATUS( *RtlAdjustPrivilege_t )( _In_ ULONG Privilege, _In_ BOOLEAN Enable, _In_ BOOLEAN Client, _Out_ PBOOLEAN WasEnabled );

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName[ 256 ];
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[ 1 ];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

typedef struct _SYSTEM_HANDLE
{
	PVOID Object;
	HANDLE UniqueProcessId;
	HANDLE HandleValue;
	ULONG GrantedAccess;
	USHORT CreatorBackTraceIndex;
	USHORT ObjectTypeIndex;
	ULONG HandleAttributes;
	ULONG Reserved;
} SYSTEM_HANDLE, * PSYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX
{
	ULONG_PTR HandleCount;
	ULONG_PTR Reserved;
	SYSTEM_HANDLE Handles[ 1 ];
} SYSTEM_HANDLE_INFORMATION_EX, * PSYSTEM_HANDLE_INFORMATION_EX;


typedef enum _MODE {
	KernelMode,
	UserMode,
	MaximumMode
} MODE;

typedef enum _MM_PAGE_PRIORITY {
	LowPagePriority,
	NormalPagePriority = 16,
	HighPagePriority = 32
} MM_PAGE_PRIORITY;