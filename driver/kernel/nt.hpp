#pragma once
#include <include.hpp>

typedef unsigned long long uint64;
typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
	HANDLE Section;
	void* MappedBase;
	void* ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

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
	SystemLocksInformation,
	SystemStackTraceInformation,
	SystemPagedPoolInformation,
	SystemNonPagedPoolInformation,
	SystemHandleInformation,
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation,
	SystemSummaryMemoryInformation,
	SystemNextEventIdInformation,
	SystemEventIdsInformation,
	SystemCrashDumpInformation,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeperation,
	SystemPlugPlayBusInformation,
	SystemDockInformation,
	SystemProcessorSpeedInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation,
	SystemBigPoolInformation = 0x42
} SYSTEM_INFORMATION_CLASS, * PSYSTEM_INFORMATION_CLASS;

struct VirtualAddress {
	union {
		uint64 Value;
		struct {
			uint64 offset : 12;
			uint64 pt_index : 9;
			uint64 pd_index : 9;
			uint64 pdpt_index : 9;
			uint64 pml4_index : 9;
			uint64 reserved : 16;
		};
	};
};

struct _MMPTE_HARDWARE
{
	ULONGLONG Valid : 1;              
	ULONGLONG Dirty1 : 1;             
	ULONGLONG Owner : 1;              
	ULONGLONG WriteThrough : 1;       
	ULONGLONG CacheDisable : 1;       
	ULONGLONG Accessed : 1;           
	ULONGLONG Dirty : 1;              
	ULONGLONG LargePage : 1;          
	ULONGLONG Global : 1;             
	ULONGLONG CopyOnWrite : 1;        
	ULONGLONG Unused : 1;             
	ULONGLONG Write : 1;              
	ULONGLONG PageFrameNumber : 40;   
	ULONGLONG ReservedForSoftware : 4;
	ULONGLONG WsleAge : 4;            
	ULONGLONG WsleProtection : 3;     
	ULONGLONG NoExecute : 1;          
};

struct _MMPTE
{
	union
	{
		struct _MMPTE_HARDWARE Hard;
	} u;
};


EXTERN_C_START

NTSTATUS NTAPI ZwQuerySystemInformation(ULONG SystemInformationClass, void* SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

NTSTATUS NTAPI MmCopyVirtualMemory(PEPROCESS SourceProcess, void* SourceAddress, PEPROCESS TargetProcess, void* TargetAddress, SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode, PSIZE_T ReturnSize);

void* NTAPI PsGetProcessSectionBaseAddress(PEPROCESS Process);

inline POBJECT_TYPE MmSectionObjectType = 0;

NTSTATUS __fastcall ZwIsProcessInJob(void* a1, void* a2);

EXTERN_C_END