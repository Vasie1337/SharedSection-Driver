#pragma once
#include <include.hpp>

namespace physical
{
	NTSTATUS Read(UINT64 Address, void* Buffer, SIZE_T Size, SIZE_T* Bytes)
	{
		MM_COPY_ADDRESS TargetAddress = { 0 };
		TargetAddress.PhysicalAddress.QuadPart = Address;

		return MmCopyMemory(Buffer, TargetAddress, Size, MM_COPY_MEMORY_PHYSICAL, Bytes);
	}

	NTSTATUS Write(UINT64 Address, void* Buffer, SIZE_T Size, SIZE_T* Bytes)
	{
		PHYSICAL_ADDRESS TargetAddress = { 0 };
		TargetAddress.QuadPart = Address;

		void* MappedAddress = MmMapIoSpaceEx(TargetAddress, Size, PAGE_READWRITE);
		if (!MappedAddress)
			return STATUS_UNSUCCESSFUL;

		crt::memcpy(MappedAddress, Buffer, Size);

		*Bytes = Size;
		MmUnmapIoSpace(MappedAddress, Size);

		return STATUS_SUCCESS;
	}

	UINT64 TranslateLinear(UINT64 DirBase, UINT64 Address)
	{
		DirBase &= ~0xf;

		auto virt_addr = Address & ~(~0ul << 12);
		auto pte = ((Address >> 12) & (0x1ffll));
		auto pt = ((Address >> 21) & (0x1ffll));
		auto pd = ((Address >> 30) & (0x1ffll));
		auto pdp = ((Address >> 39) & (0x1ffll));
		auto p_mask = ((~0xfull << 8) & 0xfffffffffull);

		size_t readsize = 0;
		UINT64 pdpe = 0;
		Read(DirBase + 8 * pdp, &pdpe, sizeof(pdpe), &readsize);
		if (~pdpe & 1) {
			return 0;
		}

		UINT64 pde = 0;
		Read((pdpe & p_mask) + 8 * pd, &pde, sizeof(pde), &readsize);
		if (~pde & 1) {
			return 0;
		}

		/* 1GB large page, use pde's 12-34 bits */
		if (pde & 0x80)
			return (pde & (~0ull << 42 >> 12)) + (Address & ~(~0ull << 30));

		UINT64 pteAddr = 0;
		Read((pde & p_mask) + 8 * pt, &pteAddr, sizeof(pteAddr), &readsize);
		if (~pteAddr & 1) {
			return 0;
		}

		/* 2MB large page */
		if (pteAddr & 0x80) {
			return (pteAddr & p_mask) + (Address & ~(~0ull << 21));
		}

		Address = 0;
		Read((pteAddr & p_mask) + 8 * pte, &Address, sizeof(Address), &readsize);
		Address &= p_mask;

		if (!Address) {
			return 0;
		}

		return Address + virt_addr;
	}

	UINT64 FindMin(INT32 g, SIZE_T f)
	{
		INT32 h = (INT32)f;
		ULONG64 result = 0;

		result = (((g) < (h)) ? (g) : (h));

		return result;
	}
}