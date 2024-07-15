#pragma once
#include <include.hpp>

namespace thread
{
	typedef void* (__fastcall* tLookUpHandle)(void*, HANDLE);
	typedef long long(__fastcall* tDestoyHandle)(void*, void*, void*);

	bool ClearPspCidTable(PETHREAD _Thread)
	{
		const auto mod = modules::get_kernel_module("ntoskrnl.exe");
		if (!mod)
		{
			printf("Failed to get ntoskrnl.exe\n");
			return false;
		}

		const auto PspCidTableAddress = scanner::find_pattern(
			mod.base,
			hide::patterns::PspCidTableSig, 
			hide::patterns::PspCidTableMask
		);
		if (!PspCidTableAddress)
		{
			printf("Failed to find PspCidTable\n");
			return false;
		}

		void* PspCidTable = *(void**)(*(unsigned int*)(PspCidTableAddress + 3ULL) + (uint64)PspCidTableAddress + 7ULL);

		const auto LookUpHandle = reinterpret_cast<tLookUpHandle>(scanner::find_pattern(
			mod.base, 
			hide::patterns::LookUpHandleSig, 
			hide::patterns::LookUpHandleMask
		));

		const auto DestroyHandle = reinterpret_cast<tDestoyHandle>(scanner::find_pattern(
			mod.base, 
			hide::patterns::DestroyHandleSig, 
			hide::patterns::DestroyHandleMask
		));

		if (!LookUpHandle || !DestroyHandle)
		{
			printf("Failed to find LookUpHandle or DestroyHandle\n");
			return false;
		}

		const auto ThreadId = PsGetThreadId(_Thread);
		if (!ThreadId)
		{
			printf("Failed to get thread id\n");
			return false;
		}

		void* Entry = LookUpHandle(PspCidTable, ThreadId);
		if (!Entry)
		{
			printf("Failed to lookup entry\n");
			return false;
		}

		DestroyHandle(PspCidTable, ThreadId, Entry);
		return true;
	}

	void SwapThreadValues(PETHREAD _Current, PETHREAD _Target)
	{
		const auto Current = reinterpret_cast<uint64>(_Current);
		const auto Target = reinterpret_cast<uint64>(_Target);

		*(void**)(Current + hide::offsets::StartAddress) = *(void**)(Target + hide::offsets::StartAddress);
		*(void**)(Current + hide::offsets::Win32StartAddress) = *(void**)(Target + hide::offsets::Win32StartAddress);
	}

	bool IsAdressOutsideModulelist(uint64 Address)
	{
		if (!Address)
			return true;

		PRTL_PROCESS_MODULES Modules = modules::get_modules();

		for (ULONG i = 0; i < Modules->NumberOfModules; i++)
		{
			const auto Module = Modules->Modules[i];

			if (Address >= (ULONG64)Module.ImageBase &&
				Address <= (ULONG64)Module.ImageBase + Module.ImageSize)
			{
				return false;
			}
		}
		
		return true;
	}

	PETHREAD GetValidThread()
	{
		for (ULONG ThreadID = 4; ThreadID < 0xFFFF; ThreadID += 4)
		{
			PETHREAD Thread = nullptr;
			if (!NT_SUCCESS(PsLookupThreadByThreadId((HANDLE)ThreadID, &Thread)) || !Thread)
				continue;

			if (!PsIsSystemThread(Thread))
			{
				ObDereferenceObject(Thread);
				continue;
			}

			if (Thread == PsGetCurrentThread())
			{
				ObDereferenceObject(Thread);
				continue;
			}

			const auto StartAddress = *(void**)(reinterpret_cast<uint64>(Thread) + hide::offsets::StartAddress);
			if (!StartAddress)
			{
				ObDereferenceObject(Thread);
				continue;
			}

			if (IsAdressOutsideModulelist(reinterpret_cast<uint64>(StartAddress)))
			{
				ObDereferenceObject(Thread);
				continue;
			}

			printf("Found valid thread with startaddress: %p\n", StartAddress);

			return Thread;
		}

		return nullptr;
	}

	bool Hide()
	{
		const auto Thread = reinterpret_cast<uint64>(PsGetCurrentThread());
		if (!Thread)
		{
			printf("Invalid param\n");
			return false;
		}

		*reinterpret_cast<uint32*>(Thread + hide::offsets::MiscFlags) &= ~(1ul << hide::offsets::SystemThread);
		*reinterpret_cast<uint32*>(Thread + hide::offsets::MiscFlags) &= ~(1ul << hide::offsets::AlertAble);
		*reinterpret_cast<uint32*>(Thread + hide::offsets::MiscFlags) &= ~(1ul << hide::offsets::ApcQueuable);

		printf("Spoofed thread values\n");

		//if (!ClearPspCidTable(_Thread))
		//{
		//	printf("Failed to clear PspCidTable\n");
		//	return false;
		//}
		//
		//printf("Cleared PspCidTable\n");

		const auto ValidThread = GetValidThread();
		if (!ValidThread)
		{
			printf("Failed to get a valid thread\n");
			return false;
		}

		SwapThreadValues(PsGetCurrentThread(), ValidThread);

		printf("Swapped thread values\n");

		printf("Hid thread\n");

		return true;
	}
}