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

typedef struct WIN__PEB {
	BYTE Reserved1[ 2 ];
	BYTE BeingDebugged;
	BYTE Reserved2[ 1 ];
	PVOID Reserved3[ 2 ];
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID Reserved4[ 3 ];
	PVOID AtlThunkSListPtr;
	PVOID Reserved5;
	ULONG Reserved6;
	PVOID Reserved7;
	ULONG Reserved8;
	ULONG AtlThunkSListPtr32;
	PVOID Reserved9[ 45 ];
	BYTE Reserved10[ 96 ];
	PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE Reserved11[ 128 ];
	PVOID Reserved12[ 1 ];
	ULONG SessionId;
} WIN_PEB, * WIN_PPEB;