#pragma once

#include "ntstructs.h"

extern "C"
NTKERNELAPI
PVOID
NTAPI
RtlFindExportedRoutineByName(
	PVOID ImageBase,
	PCCH RoutineName
);

extern "C"
NTKERNELAPI
NTSTATUS
ZwQuerySystemInformation(
	ULONG InfoClass,
	PVOID Buffer,
	ULONG Length,
	PULONG ReturnLength
);

extern "C"
NTKERNELAPI
PVOID
NTAPI
PsGetProcessWow64Process( _In_ PEPROCESS Process );

extern "C"
NTKERNELAPI
NTSTATUS
NTAPI
MmCopyVirtualMemory(
	_In_ PEPROCESS FromProcess,
	_In_ PVOID FromAddress,
	_In_ PEPROCESS ToProcess,
	_Out_ PVOID ToAddress,
	_In_ SIZE_T BufferSize,
	_In_ KPROCESSOR_MODE PreviousMode,
	_Out_ PSIZE_T NumberOfBytesCopied
);

extern "C"
NTKERNELAPI
PVOID
PsGetProcessSectionBaseAddress(
	__in PEPROCESS Process
);

extern "C"
NTKERNELAPI
PPEB
PsGetProcessPeb(
	IN PEPROCESS Process
);

extern "C"
NTKERNELAPI
PEPROCESS
NTAPI
PsGetThreadProcess(
	_In_ PETHREAD Thread
);

extern "C"
NTKERNELAPI
ULONG
NTAPI
KeCapturePersistentThreadState(
	IN PCONTEXT Context,
	IN PKTHREAD Thread,
	IN ULONG BugCheckCode,
	IN ULONG BugCheckParameter1,
	IN ULONG BugCheckParameter2,
	IN ULONG BugCheckParameter3,
	IN ULONG BugCheckParameter4,
	OUT PVOID VirtualAddress
);