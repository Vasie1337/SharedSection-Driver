#pragma once
#include <include.hpp>

namespace hide::offsets
{
	// Process
	inline uint64 ThreadListHead = 0x5e0;

	// Thread
	inline uint64 MiscFlags = 0x74;
	inline uint64 StartAddress = 0x450;
	inline uint64 Win32StartAddress = 0x4D0;
	inline uint64 ThreadListEntry = 0x4E8;
	 
	inline uint64 AlertAble = 4;
	inline uint64 SystemThread = 10;
	inline uint64 ApcQueuable = 14;

	inline void Load()
	{
		RTL_OSVERSIONINFOW Version{};
		RtlGetVersion(&Version);

		if (22000 <= Version.dwBuildNumber)
		{
			StartAddress = 0x4A0;
			Win32StartAddress = 0x520;
			ThreadListEntry = 0x538;
		}
	}
}

namespace hide::patterns
{
	inline const char* PspCidTableSig = "\x48\x8B\x0D\x00\x00\x00\x00\x48\x89\x7C\x24\x00\x45\x33\xC9";
	inline const char* PspCidTableMask = "xxx????xxxx?xxx";

	inline const char* LookUpHandleSig = "\x8B\x01\x48\x83\xE2";
	inline const char* LookUpHandleMask = "xxxxx";

	inline const char* DestroyHandleSig = "\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x00\x48\x83\x79\x00\x00\x49\x8B\xE8";
	inline const char* DestroyHandleMask = "xxxx?xxxx?xxxx?xxxx?xxx??xxx";
}