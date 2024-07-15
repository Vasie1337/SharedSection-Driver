#pragma once
#include <include.hpp>

namespace physical::cr3
{
	uint64 StoredCr3 = 0;

	uint64 GetFromBase(uint64 Base)
	{
		if (!Base)
		{
			printf("Invalid base address\n");
			return 0;
		}

		VirtualAddress VirtualAddress = { 0 };
		VirtualAddress.Value = Base;

		PPHYSICAL_MEMORY_RANGE Ranges = MmGetPhysicalMemoryRanges();
		if (!Ranges)
		{
			printf("Failed to get physical memory ranges\n");
			return 0;
		}

		void* Buffer = ExAllocatePool(NonPagedPoolNx, PAGE_SIZE);
		if (!Buffer)
		{
			printf("Failed to allocate buffer\n");
			return 0;
		}

		for (int i = 0;; i++)
		{
			auto Range = &Ranges[i];

			if (!Range->BaseAddress.QuadPart || !Range->NumberOfBytes.QuadPart)
			{
				printf("Failed to find CR3\n");
				break;
			}

			uint64 PhysicalAddr = Range->BaseAddress.QuadPart;

			for (int j = 0; j < (Range->NumberOfBytes.QuadPart / PAGE_SIZE); j++, PhysicalAddr += PAGE_SIZE)
			{
				_MMPTE Entries[4] = {};

				size_t Bytes = 0;
				if (!NT_SUCCESS(physical::ReadPhysical(PhysicalAddr + 8 * VirtualAddress.pml4_index, reinterpret_cast<PVOID>(&Entries[0]), sizeof(_MMPTE), &Bytes)))
					continue;
				if (!Entries[0].u.Hard.Valid)
					continue;

				if (!NT_SUCCESS(physical::ReadPhysical((Entries[0].u.Hard.PageFrameNumber << 12) + 8 * VirtualAddress.pdpt_index, reinterpret_cast<PVOID>(&Entries[1]), sizeof(_MMPTE), &Bytes)))
					continue;
				if (!Entries[1].u.Hard.Valid)
					continue;

				if (!NT_SUCCESS(physical::ReadPhysical((Entries[1].u.Hard.PageFrameNumber << 12) + 8 * VirtualAddress.pd_index, reinterpret_cast<PVOID>(&Entries[2]), sizeof(_MMPTE), &Bytes)))
					continue;
				if (!Entries[2].u.Hard.Valid)
					continue;

				if (!NT_SUCCESS(physical::ReadPhysical((Entries[2].u.Hard.PageFrameNumber << 12) + 8 * VirtualAddress.pt_index, reinterpret_cast<PVOID>(&Entries[3]), sizeof(_MMPTE), &Bytes)))
					continue;
				if (!Entries[3].u.Hard.Valid)
					continue;

				if (!NT_SUCCESS(physical::ReadMemory(PhysicalAddr, reinterpret_cast<void*>(Base), Buffer, PAGE_SIZE, &Bytes)))
					continue;

				IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(Buffer);
				if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE)
				{
					IMAGE_NT_HEADERS* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uint64>(Buffer) + dosHeader->e_lfanew);
					if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
						continue;

					if (ntHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
					{
						IMAGE_NT_HEADERS64* ntHeaders64 = reinterpret_cast<IMAGE_NT_HEADERS64*>(ntHeaders);
						if (ntHeaders64->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC || ntHeaders64->OptionalHeader.ImageBase != Base)
							continue;
					}
					else if (ntHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
					{
						IMAGE_NT_HEADERS32* ntHeaders32 = reinterpret_cast<IMAGE_NT_HEADERS32*>(ntHeaders);
						if (ntHeaders32->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC || ntHeaders32->OptionalHeader.ImageBase != Base)
							continue;
					}
					else {
						continue;
					}

					ExFreePoolWithTag(Buffer, NULL);
					return PhysicalAddr;
				}
			}
		}

		printf("Failed to find CR3\n");
		ExFreePoolWithTag(Buffer, NULL);
		return 0;
	}
}