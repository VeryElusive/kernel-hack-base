#pragma once
#pragma warning(disable: 4201)

#include <ntifs.h>
#include <windef.h>

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemBasicInformation,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,
	SystemModuleInformation,
} SYSTEM_INFORMATION_CLASS, * PSYSTEM_INFORMATION_CLASS;

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

typedef struct _PEB_LDR_DATA32
{
	ULONG Length;
	UCHAR Initialized;
	ULONG SsHandle;
	LIST_ENTRY32 InLoadOrderModuleList;
	LIST_ENTRY32 InMemoryOrderModuleList;
	LIST_ENTRY32 InInitializationOrderModuleList;
} PEB_LDR_DATA32, * PPEB_LDR_DATA32;

typedef struct _LDR_DATA_TABLE_ENTRY32
{
	LIST_ENTRY32 InLoadOrderLinks;
	LIST_ENTRY32 InMemoryOrderLinks;
	LIST_ENTRY32 InInitializationOrderLinks;
	ULONG DllBase;
	ULONG EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING32 FullDllName;
	UNICODE_STRING32 BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	LIST_ENTRY32 HashLinks;
	ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY32, * PLDR_DATA_TABLE_ENTRY32;

typedef struct _PEB32
{
	UCHAR InheritedAddressSpace;
	UCHAR ReadImageFileExecOptions;
	UCHAR BeingDebugged;
	UCHAR BitField;
	ULONG Mutant;
	ULONG ImageBaseAddress;
	ULONG Ldr;
	ULONG ProcessParameters;
	ULONG SubSystemData;
	ULONG ProcessHeap;
	ULONG FastPebLock;
	ULONG AtlThunkSListPtr;
	ULONG IFEOKey;
	ULONG CrossProcessFlags;
	ULONG UserSharedInfoPtr;
	ULONG SystemReserved;
	ULONG AtlThunkSListPtr32;
	ULONG ApiSetMap;
} PEB32, * PPEB32;

typedef struct _SYSTEM_MODULE
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
	UCHAR  FullPathName[ MAXIMUM_FILENAME_LENGTH ];
} SYSTEM_MODULE, * PSYSTEM_MODULE;

typedef struct _SYSTEM_MODULE_INFORMATION
{
	ULONG NumberOfModules;
	SYSTEM_MODULE Modules[ 1 ];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

typedef struct _PEB_LDR_DATA {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY ModuleListLoadOrder;
	LIST_ENTRY ModuleListMemoryOrder;
	LIST_ENTRY ModuleListInitOrder;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE Reserved1[ 16 ];
	PVOID Reserved2[ 10 ];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

typedef void( __stdcall* PPS_POST_PROCESS_INIT_ROUTINE )( void ); // not exported

typedef struct _PEB {
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
} PEB, * PPEB;

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;  // in bytes
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;  // LDR_*
	USHORT LoadCount;
	USHORT TlsIndex;
	LIST_ENTRY HashLinks;
	PVOID SectionPointer;
	ULONG CheckSum;
	ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;


//0x8 bytes (sizeof)
struct _MI_ACTIVE_PFN
{
	union
	{
		struct
		{
			ULONGLONG Tradable : 1;                                           //0x0
			ULONGLONG NonPagedBuddy : 43;                                     //0x0
		} Leaf;                                                             //0x0
		struct
		{
			ULONGLONG Tradable : 1;                                           //0x0
			ULONGLONG WsleAge : 3;                                            //0x0
			ULONGLONG OldestWsleLeafEntries : 10;                             //0x0
			ULONGLONG OldestWsleLeafAge : 3;                                  //0x0
			ULONGLONG NonPagedBuddy : 43;                                     //0x0
		} PageTable;                                                        //0x0
		ULONGLONG EntireActiveField;                                        //0x0
	};
};

typedef union _virt_addr_t
{
	void* value;
	struct
	{
		ULONG64 offset : 12;
		ULONG64 pt_index : 9;
		ULONG64 pd_index : 9;
		ULONG64 pdpt_index : 9;
		ULONG64 pml4_index : 9;
		ULONG64 reserved : 16;
	};
} VirtAddr_t, * PVirtAddr_t;

//0x8 bytes (sizeof)
struct _MMPTE_HARDWARE
{
	ULONGLONG Valid : 1;                                                      //0x0
	ULONGLONG Dirty1 : 1;                                                     //0x0
	ULONGLONG Owner : 1;                                                      //0x0
	ULONGLONG WriteThrough : 1;                                               //0x0
	ULONGLONG CacheDisable : 1;                                               //0x0
	ULONGLONG Accessed : 1;                                                   //0x0
	ULONGLONG Dirty : 1;                                                      //0x0
	ULONGLONG LargePage : 1;                                                  //0x0
	ULONGLONG Global : 1;                                                     //0x0
	ULONGLONG CopyOnWrite : 1;                                                //0x0
	ULONGLONG Unused : 1;                                                     //0x0
	ULONGLONG Write : 1;                                                      //0x0
	ULONGLONG PageFrameNumber : 40;                                           //0x0
	ULONGLONG ReservedForSoftware : 4;                                        //0x0
	ULONGLONG WsleAge : 4;                                                    //0x0
	ULONGLONG WsleProtection : 3;                                             //0x0
	ULONGLONG NoExecute : 1;                                                  //0x0
};

//0x8 bytes (sizeof)
struct _MMPTE_PROTOTYPE
{
	ULONGLONG Valid : 1;                                                      //0x0
	ULONGLONG DemandFillProto : 1;                                            //0x0
	ULONGLONG HiberVerifyConverted : 1;                                       //0x0
	ULONGLONG ReadOnly : 1;                                                   //0x0
	ULONGLONG SwizzleBit : 1;                                                 //0x0
	ULONGLONG Protection : 5;                                                 //0x0
	ULONGLONG Prototype : 1;                                                  //0x0
	ULONGLONG Combined : 1;                                                   //0x0
	ULONGLONG Unused1 : 4;                                                    //0x0
	LONGLONG ProtoAddress : 48;                                               //0x0
};

//0x8 bytes (sizeof)
struct _MMPTE_SOFTWARE
{
	ULONGLONG Valid : 1;                                                      //0x0
	ULONGLONG PageFileReserved : 1;                                           //0x0
	ULONGLONG PageFileAllocated : 1;                                          //0x0
	ULONGLONG ColdPage : 1;                                                   //0x0
	ULONGLONG SwizzleBit : 1;                                                 //0x0
	ULONGLONG Protection : 5;                                                 //0x0
	ULONGLONG Prototype : 1;                                                  //0x0
	ULONGLONG Transition : 1;                                                 //0x0
	ULONGLONG PageFileLow : 4;                                                //0x0
	ULONGLONG UsedPageTableEntries : 10;                                      //0x0
	ULONGLONG ShadowStack : 1;                                                //0x0
	ULONGLONG OnStandbyLookaside : 1;                                         //0x0
	ULONGLONG Unused : 4;                                                     //0x0
	ULONGLONG PageFileHigh : 32;                                              //0x0
};

//0x8 bytes (sizeof)
struct _MMPTE_TIMESTAMP
{
	ULONGLONG MustBeZero : 1;                                                 //0x0
	ULONGLONG Unused : 3;                                                     //0x0
	ULONGLONG SwizzleBit : 1;                                                 //0x0
	ULONGLONG Protection : 5;                                                 //0x0
	ULONGLONG Prototype : 1;                                                  //0x0
	ULONGLONG Transition : 1;                                                 //0x0
	ULONGLONG PageFileLow : 4;                                                //0x0
	ULONGLONG Reserved : 16;                                                  //0x0
	ULONGLONG GlobalTimeStamp : 32;                                           //0x0
};

//0x8 bytes (sizeof)
struct _MMPTE_TRANSITION
{
	ULONGLONG Valid : 1;                                                      //0x0
	ULONGLONG Write : 1;                                                      //0x0
	ULONGLONG OnStandbyLookaside : 1;                                         //0x0
	ULONGLONG IoTracker : 1;                                                  //0x0
	ULONGLONG SwizzleBit : 1;                                                 //0x0
	ULONGLONG Protection : 5;                                                 //0x0
	ULONGLONG Prototype : 1;                                                  //0x0
	ULONGLONG Transition : 1;                                                 //0x0
	ULONGLONG PageFrameNumber : 40;                                           //0x0
	ULONGLONG Unused : 12;                                                    //0x0
};

//0x8 bytes (sizeof)
struct _MMPTE_SUBSECTION
{
	ULONGLONG Valid : 1;                                                      //0x0
	ULONGLONG Unused0 : 2;                                                    //0x0
	ULONGLONG OnStandbyLookaside : 1;                                         //0x0
	ULONGLONG SwizzleBit : 1;                                                 //0x0
	ULONGLONG Protection : 5;                                                 //0x0
	ULONGLONG Prototype : 1;                                                  //0x0
	ULONGLONG ColdPage : 1;                                                   //0x0
	ULONGLONG Unused2 : 3;                                                    //0x0
	ULONGLONG ExecutePrivilege : 1;                                           //0x0
	LONGLONG SubsectionAddress : 48;                                          //0x0
};

//0x8 bytes (sizeof)
struct _MMPTE_LIST
{
	ULONGLONG Valid : 1;                                                      //0x0
	ULONGLONG OneEntry : 1;                                                   //0x0
	ULONGLONG filler0 : 2;                                                    //0x0
	ULONGLONG SwizzleBit : 1;                                                 //0x0
	ULONGLONG Protection : 5;                                                 //0x0
	ULONGLONG Prototype : 1;                                                  //0x0
	ULONGLONG Transition : 1;                                                 //0x0
	ULONGLONG filler1 : 16;                                                   //0x0
	ULONGLONG NextEntry : 36;                                                 //0x0
};

//0x8 bytes (sizeof)
struct _MMPTE
{
	union
	{
		ULONGLONG Long;                                                     //0x0
		volatile ULONGLONG VolatileLong;                                    //0x0
		struct _MMPTE_HARDWARE Hard;                                        //0x0
		struct _MMPTE_PROTOTYPE Proto;                                      //0x0
		struct _MMPTE_SOFTWARE Soft;                                        //0x0
		struct _MMPTE_TIMESTAMP TimeStamp;                                  //0x0
		struct _MMPTE_TRANSITION Trans;                                     //0x0
		struct _MMPTE_SUBSECTION Subsect;                                   //0x0
		struct _MMPTE_LIST List;                                            //0x0
	} u;                                                                    //0x0
};

//0x8 bytes (sizeof)
struct _MIPFNBLINK
{
	union
	{
		struct
		{
			ULONGLONG Blink : 40;                                             //0x0
			ULONGLONG NodeBlinkLow : 19;                                      //0x0
			ULONGLONG TbFlushStamp : 3;                                       //0x0
			ULONGLONG PageBlinkDeleteBit : 1;                                 //0x0
			ULONGLONG PageBlinkLockBit : 1;                                   //0x0
			ULONGLONG ShareCount : 62;                                        //0x0
			ULONGLONG PageShareCountDeleteBit : 1;                            //0x0
			ULONGLONG PageShareCountLockBit : 1;                              //0x0
		};
		ULONGLONG EntireField;                                              //0x0
		volatile LONGLONG Lock;                                             //0x0
		struct
		{
			ULONGLONG LockNotUsed : 62;                                       //0x0
			ULONGLONG DeleteBit : 1;                                          //0x0
			ULONGLONG LockBit : 1;                                            //0x0
		};
	};
};

//0x1 bytes (sizeof)
struct _MMPFNENTRY1
{
	UCHAR PageLocation : 3;                                                   //0x0
	UCHAR WriteInProgress : 1;                                                //0x0
	UCHAR Modified : 1;                                                       //0x0
	UCHAR ReadInProgress : 1;                                                 //0x0
	UCHAR CacheAttribute : 2;                                                 //0x0
};

//0x1 bytes (sizeof)
struct _MMPFNENTRY3
{
	UCHAR Priority : 3;                                                       //0x0
	UCHAR OnProtectedStandby : 1;                                             //0x0
	UCHAR InPageError : 1;                                                    //0x0
	UCHAR SystemChargedPage : 1;                                              //0x0
	UCHAR RemovalRequested : 1;                                               //0x0
	UCHAR ParityError : 1;                                                    //0x0
};

//0x4 bytes (sizeof)
struct _MI_PFN_ULONG5
{
	union
	{
		ULONG EntireField;                                                  //0x0
		struct
		{
			ULONG NodeBlinkHigh : 21;                                         //0x0
			ULONG NodeFlinkMiddle : 11;                                       //0x0
		} StandbyList;                                                      //0x0
		struct
		{
			UCHAR ModifiedListBucketIndex : 4;                                //0x0
		} MappedPageList;                                                   //0x0
		struct
		{
			UCHAR AnchorLargePageSize : 2;                                    //0x0
			UCHAR Spare0 : 6;                                                 //0x0
			UCHAR Spare1 : 8;                                                 //0x1
			USHORT Spare2;                                                  //0x2
		} Active;                                                           //0x0
	};
};

//0x30 bytes (sizeof)
typedef struct _MMPFN
{
	union
	{
		struct _LIST_ENTRY ListEntry;                                       //0x0
		struct _RTL_BALANCED_NODE TreeNode;                                 //0x0
		struct
		{
			union
			{
				struct _SINGLE_LIST_ENTRY NextSlistPfn;                     //0x0
				VOID* Next;                                                 //0x0
				ULONGLONG Flink : 40;                                         //0x0
				ULONGLONG NodeFlinkLow : 24;                                  //0x0
				struct _MI_ACTIVE_PFN Active;                               //0x0
			} u1;                                                           //0x0
			union
			{
				struct _MMPTE* PteAddress;                                  //0x8
				ULONGLONG PteLong;                                          //0x8
			};
			struct _MMPTE OriginalPte;                                      //0x10
		};
	};
	struct _MIPFNBLINK u2;                                                  //0x18
	union
	{
		struct
		{
			USHORT ReferenceCount;                                          //0x20
			struct _MMPFNENTRY1 e1;                                         //0x22
		};
		struct
		{
			struct _MMPFNENTRY3 e3;                                         //0x23
			struct
			{
				USHORT ReferenceCount;                                          //0x20
			} e2;                                                               //0x20
		};
		struct
		{
			ULONG EntireField;                                              //0x20
		} e4;                                                               //0x20
	} u3;                                                                   //0x20
	struct _MI_PFN_ULONG5 u5;                                               //0x24
	union
	{
		ULONGLONG PteFrame : 40;                                              //0x28
		ULONGLONG ResidentPage : 1;                                           //0x28
		ULONGLONG Unused1 : 1;                                                //0x28
		ULONGLONG Unused2 : 1;                                                //0x28
		ULONGLONG Partition : 10;                                             //0x28
		ULONGLONG FileOnly : 1;                                               //0x28
		ULONGLONG PfnExists : 1;                                              //0x28
		ULONGLONG NodeFlinkHigh : 5;                                          //0x28
		ULONGLONG PageIdentity : 3;                                           //0x28
		ULONGLONG PrototypePte : 1;                                           //0x28
		ULONGLONG EntireField;                                              //0x28
	} u4;                                                                   //0x28
} MMPFN, * PMMPFN;

typedef struct DBGKD_DEBUG_DATA_HEADER64 {
	LIST_ENTRY64    List;
	unsigned int           OwnerTag;
	unsigned int           Size;
} DBGKD_DEBUG_DATA_HEADER64;

typedef struct KDDEBUGGER_DATA64 {
	DBGKD_DEBUG_DATA_HEADER64 Header;

	unsigned __int64 KernBase;
	unsigned __int64 BreakpointWithStatus;
	unsigned __int64 SavedContext;
	unsigned short ThCallbackStack;
	unsigned short NextCallback;
	unsigned short FramePointer;
	unsigned short PaeEnabled : 1;
	unsigned __int64 KiCallUserMode;
	unsigned __int64 KeUserCallbackDispatcher;
	unsigned __int64 PsLoadedModuleList;
	unsigned __int64 PsActiveProcessHead;
	unsigned __int64 PspCidTable;
	unsigned __int64 ExpSystemResourcesList;
	unsigned __int64 ExpPagedPoolDescriptor;
	unsigned __int64 ExpNumberOfPagedPools;
	unsigned __int64 KeTimeIncrement;
	unsigned __int64 KeBugCheckCallbackListHead;
	unsigned __int64 KiBugcheckData;
	unsigned __int64 IopErrorLogListHead;
	unsigned __int64 ObpRootDirectoryObject;
	unsigned __int64 ObpTypeObjectType;
	unsigned __int64 MmSystemCacheStart;
	unsigned __int64 MmSystemCacheEnd;
	unsigned __int64 MmSystemCacheWs;
	unsigned __int64 MmPfnDatabase;
	unsigned __int64 MmSystemPtesStart;
	unsigned __int64 MmSystemPtesEnd;
	unsigned __int64 MmSubsectionBase;
	unsigned __int64 MmNumberOfPagingFiles;
	unsigned __int64 MmLowestPhysicalPage;
	unsigned __int64 MmHighestPhysicalPage;
	unsigned __int64 MmNumberOfPhysicalPages;
	unsigned __int64 MmMaximumNonPagedPoolInBytes;
	unsigned __int64 MmNonPagedSystemStart;
	unsigned __int64 MmNonPagedPoolStart;
	unsigned __int64 MmNonPagedPoolEnd;
	unsigned __int64 MmPagedPoolStart;
	unsigned __int64 MmPagedPoolEnd;
	unsigned __int64 MmPagedPoolInformation;
	unsigned __int64 MmPageSize;
	unsigned __int64 MmSizeOfPagedPoolInBytes;
	unsigned __int64 MmTotalCommitLimit;
	unsigned __int64 MmTotalCommittedPages;
	unsigned __int64 MmSharedCommit;
	unsigned __int64 MmDriverCommit;
	unsigned __int64 MmProcessCommit;
	unsigned __int64 MmPagedPoolCommit;
	unsigned __int64 MmExtendedCommit;
	unsigned __int64 MmZeroedPageListHead;
	unsigned __int64 MmFreePageListHead;
	unsigned __int64 MmStandbyPageListHead;
	unsigned __int64 MmModifiedPageListHead;
	unsigned __int64 MmModifiedNoWritePageListHead;
	unsigned __int64 MmAvailablePages;
	unsigned __int64 MmResidentAvailablePages;
	unsigned __int64 PoolTrackTable;
	unsigned __int64 NonPagedPoolDescriptor;
	unsigned __int64 MmHighestUserAddress;
	unsigned __int64 MmSystemRangeStart;
	unsigned __int64 MmUserProbeAddress;
	unsigned __int64 KdPrintCircularBuffer;
	unsigned __int64 KdPrintCircularBufferEnd;
	unsigned __int64 KdPrintWritePointer;
	unsigned __int64 KdPrintRolloverCount;
	unsigned __int64 MmLoadedUserImageList;

	/* NT 5.1 Addition */

	unsigned __int64 NtBuildLab;
	unsigned __int64 KiNormalSystemCall;

	/* NT 5.0 hotfix addition */

	unsigned __int64 KiProcessorBlock;
	unsigned __int64 MmUnloadedDrivers;
	unsigned __int64 MmLastUnloadedDriver;
	unsigned __int64 MmTriageActionTaken;
	unsigned __int64 MmSpecialPoolTag;
	unsigned __int64 KernelVerifier;
	unsigned __int64 MmVerifierData;
	unsigned __int64 MmAllocatedNonPagedPool;
	unsigned __int64 MmPeakCommitment;
	unsigned __int64 MmTotalCommitLimitMaximum;
	unsigned __int64 CmNtCSDVersion;

	/* NT 5.1 Addition */

	unsigned __int64 MmPhysicalMemoryBlock;
	unsigned __int64 MmSessionBase;
	unsigned __int64 MmSessionSize;
	unsigned __int64 MmSystemParentTablePage;

	/* Server 2003 addition */

	unsigned __int64 MmVirtualTranslationBase;
	unsigned short OffsetKThreadNextProcessor;
	unsigned short OffsetKThreadTeb;
	unsigned short OffsetKThreadKernelStack;
	unsigned short OffsetKThreadInitialStack;
	unsigned short OffsetKThreadApcProcess;
	unsigned short OffsetKThreadState;
	unsigned short OffsetKThreadBStore;
	unsigned short OffsetKThreadBStoreLimit;
	unsigned short SizeEProcess;
	unsigned short OffsetEprocessPeb;
	unsigned short OffsetEprocessParentCID;
	unsigned short OffsetEprocessDirectoryTableBase;
	unsigned short SizePrcb;
	unsigned short OffsetPrcbDpcRoutine;
	unsigned short OffsetPrcbCurrentThread;
	unsigned short OffsetPrcbMhz;
	unsigned short OffsetPrcbCpuType;
	unsigned short OffsetPrcbVendorString;
	unsigned short OffsetPrcbProcStateContext;
	unsigned short OffsetPrcbNumber;
	unsigned short SizeEThread;
	unsigned __int64 KdPrintCircularBufferPtr;
	unsigned __int64 KdPrintBufferSize;
	unsigned __int64 KeLoaderBlock;
	unsigned short SizePcr;
	unsigned short OffsetPcrSelfPcr;
	unsigned short OffsetPcrCurrentPrcb;
	unsigned short OffsetPcrContainedPrcb;
	unsigned short OffsetPcrInitialBStore;
	unsigned short OffsetPcrBStoreLimit;
	unsigned short OffsetPcrInitialStack;
	unsigned short OffsetPcrStackLimit;
	unsigned short OffsetPrcbPcrPage;
	unsigned short OffsetPrcbProcStateSpecialReg;
	unsigned short GdtR0Code;
	unsigned short GdtR0Data;
	unsigned short GdtR0Pcr;
	unsigned short GdtR3Code;
	unsigned short GdtR3Data;
	unsigned short GdtR3Teb;
	unsigned short GdtLdt;
	unsigned short GdtTss;
	unsigned short Gdt64R3CmCode;
	unsigned short Gdt64R3CmTeb;
	unsigned __int64 IopNumTriageDumpDataBlocks;
	unsigned __int64 IopTriageDumpDataBlocks;

	/* Longhorn addition */

	unsigned __int64 VfCrashDataBlock;
	unsigned __int64 MmBadPagesDetected;
	unsigned __int64 MmZeroedPageSingleBitErrorsDetected;

	/* Windows 7 addition */

	unsigned __int64 EtwpDebuggerData;
	unsigned short OffsetPrcbContext;
} KDDEBUGGER_DATA64;


typedef struct _DUMP_HEADER
{
	ULONG Signature;
	ULONG ValidDump;
	ULONG MajorVersion;
	ULONG MinorVersion;
	ULONG_PTR DirectoryTableBase;
	ULONG_PTR PfnDataBase;
	PLIST_ENTRY PsLoadedModuleList;
	PLIST_ENTRY PsActiveProcessHead;
	ULONG MachineImageType;
	ULONG NumberProcessors;
	ULONG BugCheckCode;
	ULONG_PTR BugCheckParameter1;
	ULONG_PTR BugCheckParameter2;
	ULONG_PTR BugCheckParameter3;
	ULONG_PTR BugCheckParameter4;
	CHAR VersionUser[ 32 ];
	struct _KDDEBUGGER_DATA64* KdDebuggerDataBlock;
} DUMP_HEADER, * PDUMP_HEADER;


//0xa0 bytes (sizeof)
typedef struct _KLDR_DATA_TABLE_ENTRY
{
	struct _LIST_ENTRY InLoadOrderLinks;                                    //0x0
	VOID* ExceptionTable;                                                   //0x10
	ULONG ExceptionTableSize;                                               //0x18
	VOID* GpValue;                                                          //0x20
	struct _NON_PAGED_DEBUG_INFO* NonPagedDebugInfo;                        //0x28
	VOID* DllBase;                                                          //0x30
	VOID* EntryPoint;                                                       //0x38
	ULONG SizeOfImage;                                                      //0x40
	struct _UNICODE_STRING FullDllName;                                     //0x48
	struct _UNICODE_STRING BaseDllName;                                     //0x58
	ULONG Flags;                                                            //0x68
	USHORT LoadCount;                                                       //0x6c
	union
	{
		USHORT SignatureLevel : 4;                                            //0x6e
		USHORT SignatureType : 3;                                             //0x6e
		USHORT Frozen : 2;                                                    //0x6e
		USHORT HotPatch : 1;                                                  //0x6e
		USHORT Unused : 6;                                                    //0x6e
		USHORT EntireField;                                                 //0x6e
	} u1;                                                                   //0x6e
	VOID* SectionPointer;                                                   //0x70
	ULONG CheckSum;                                                         //0x78
	ULONG CoverageSectionSize;                                              //0x7c
	VOID* CoverageSection;                                                  //0x80
	VOID* LoadedImports;                                                    //0x88
	union
	{
		VOID* Spare;                                                        //0x90
		struct _KLDR_DATA_TABLE_ENTRY* NtDataTableEntry;                    //0x90
	};
	ULONG SizeOfImageNotRounded;                                            //0x98
	ULONG TimeDateStamp;                                                    //0x9c
} KLDR_DATA_TABLE_ENTRY, * PKLDR_DATA_TABLE_ENTRY;