#pragma once
#include <include.hpp>

namespace pml4
{
	LONGLONG StoredDatabase;
	void* MmPfnDatabase;

	void* SplitMemory(void* SearchBase, SIZE_T SearchSize, const void* Pattern, SIZE_T PatternSize)
	{
		const UCHAR* searchBase = static_cast<const UCHAR*>(SearchBase);
		const UCHAR* pattern = static_cast<const UCHAR*>(Pattern);

		for (SIZE_T i = 0; i <= SearchSize - PatternSize; ++i) {
			SIZE_T j = 0;
			for (; j < PatternSize; ++j) {
				if (searchBase[i + j] != pattern[j])
					break;
			}

			if (j == PatternSize)
				return const_cast<UCHAR*>(&searchBase[i]);
		}

		return nullptr;
	}

	NTSTATUS InitMmPfnDatabase()
	{
		struct MmPfnDatabaseSearchPattern
		{
			const UCHAR* bytes;
			SIZE_T bytes_size;
			bool hard_coded;
		};

		MmPfnDatabaseSearchPattern patterns;

		// Windows 10 x64 Build 14332+
		static const UCHAR kPatternWin10x64[] = {
			0x48, 0x8B, 0xC1,        // mov     rax, rcx
			0x48, 0xC1, 0xE8, 0x0C,  // shr     rax, 0Ch
			0x48, 0x8D, 0x14, 0x40,  // lea     rdx, [rax + rax * 2]
			0x48, 0x03, 0xD2,        // add     rdx, rdx
			0x48, 0xB8,              // mov     rax, 0FFFFFA8000000008h
		};

		patterns.bytes = kPatternWin10x64;
		patterns.bytes_size = sizeof(kPatternWin10x64);
		patterns.hard_coded = true;

		const auto p_MmGetVirtualForPhysical = reinterpret_cast<UCHAR*>(MmGetVirtualForPhysical);
		if (!p_MmGetVirtualForPhysical) {
			return STATUS_PROCEDURE_NOT_FOUND;
		}

		auto found = reinterpret_cast<UCHAR*>(SplitMemory(p_MmGetVirtualForPhysical, 0x20, patterns.bytes, patterns.bytes_size));
		if (!found) {
			return STATUS_UNSUCCESSFUL;
		}


		found += patterns.bytes_size;
		if (patterns.hard_coded) {
			MmPfnDatabase = *reinterpret_cast<void**>(found);
		}
		else {
			const auto mmpfn_address = *reinterpret_cast<ULONG_PTR*>(found);
			MmPfnDatabase = *reinterpret_cast<void**>(mmpfn_address);
		}

		MmPfnDatabase = PAGE_ALIGN(MmPfnDatabase);

		return STATUS_SUCCESS;
	}

	UINT64 GetDirBaseFromBase(void* Base)
	{
		if (!NT_SUCCESS(InitMmPfnDatabase()))
			return 0;

		virt_addr_t virt_base{ Base };

		size_t read{};

		auto ranges = MmGetPhysicalMemoryRanges();

		for (int i = 0;; i++) {

			auto elem = &ranges[i];

			if (!elem->BaseAddress.QuadPart || !elem->NumberOfBytes.QuadPart)
				break;

			uintptr_t current_phys_address = elem->BaseAddress.QuadPart;

			for (int j = 0; j < (elem->NumberOfBytes.QuadPart / 0x1000); j++, current_phys_address += 0x1000) {

				_MMPFN* pnfinfo = (_MMPFN*)((uintptr_t)MmPfnDatabase + (current_phys_address >> 12) * sizeof(_MMPFN));

				if (pnfinfo->u4.PteFrame == (current_phys_address >> 12)) {
					MMPTE pml4e{};
					if (!NT_SUCCESS(physical::Read(current_phys_address + 8 * virt_base.pml4_index, &pml4e, 8, &read)))
						continue;

					if (!pml4e.u.Hard.Valid)
						continue;

					MMPTE pdpte{};
					if (!NT_SUCCESS(physical::Read((pml4e.u.Hard.PageFrameNumber << 12) + 8 * virt_base.pdpt_index, &pdpte, 8, &read)))
						continue;

					if (!pdpte.u.Hard.Valid)
						continue;

					MMPTE pde{};
					if (!NT_SUCCESS(physical::Read((pdpte.u.Hard.PageFrameNumber << 12) + 8 * virt_base.pd_index, &pde, 8, &read)))
						continue;

					if (!pde.u.Hard.Valid)
						continue;

					MMPTE pte{};
					if (!NT_SUCCESS(physical::Read((pde.u.Hard.PageFrameNumber << 12) + 8 * virt_base.pt_index, &pte, 8, &read)))
						continue;

					if (!pte.u.Hard.Valid)
						continue;

					return current_phys_address;
				}
			}
		}
		return 0;
	}
}