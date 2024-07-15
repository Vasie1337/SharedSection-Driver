#pragma once
#include <include.hpp>

namespace physical
{
	NTSTATUS ReadPhysical(uint64 Address, void* Buffer, size_t Size, size_t* Bytes)
	{
		MM_COPY_ADDRESS Target = { 0 };
		Target.PhysicalAddress.QuadPart = Address;
		return MmCopyMemory(Buffer, Target, Size, MM_COPY_MEMORY_PHYSICAL, Bytes);
	}

	NTSTATUS WritePhysical(uint64 Address, void* Buffer, size_t Size, size_t* Bytes)
	{
		if (!Address)
			return STATUS_UNSUCCESSFUL;

		PHYSICAL_ADDRESS Target = { 0 };
		Target.QuadPart = Address;

		void* Mapped = MmMapIoSpaceEx(Target, Size, PAGE_READWRITE);
		if (!Mapped)
			return STATUS_UNSUCCESSFUL;

		crt::memcpy(Mapped, Buffer, Size);
		MmUnmapIoSpace(Mapped, Size);
		*Bytes = Size;

		return STATUS_SUCCESS;
	}

	uint64 TranslateLinear(uint64 Cr3, uint64 Address)
	{
		Cr3 &= ~0xf;

		uint64 virt_addr = Address & ~(~0ul << 12);
		uint64 pte = ((Address >> 12) & (0x1ffll));
		uint64 pt = ((Address >> 21) & (0x1ffll));
		uint64 pd = ((Address >> 30) & (0x1ffll));
		uint64 pdp = ((Address >> 39) & (0x1ffll));
		uint64 p_mask = ((~0xfull << 8) & 0xfffffffffull);

		size_t readsize = 0;
		uint64 pdpe = 0;
		ReadPhysical(Cr3 + 8 * pdp, &pdpe, sizeof(pdpe), &readsize);
		if (~pdpe & 1) {
			return 0;
		}

		uint64 pde = 0;
		ReadPhysical((pdpe & p_mask) + 8 * pd, &pde, sizeof(pde), &readsize);
		if (~pde & 1) {
			return 0;
		}

		/* 1GB large page, use pde's 12-34 bits */
		if (pde & 0x80)
			return (pde & (~0ull << 42 >> 12)) + (Address & ~(~0ull << 30));

		uint64 pteAddr = 0;
		ReadPhysical((pde & p_mask) + 8 * pt, &pteAddr, sizeof(pteAddr), &readsize);
		if (~pteAddr & 1) {
			return 0;
		}

		/* 2MB large page */
		if (pteAddr & 0x80) {
			return (pteAddr & p_mask) + (Address & ~(~0ull << 21));
		}

		Address = 0;
		ReadPhysical((pteAddr & p_mask) + 8 * pte, &Address, sizeof(Address), &readsize);
		Address &= p_mask;

		if (!Address) {
			return 0;
		}

		return Address + virt_addr;
	}

	NTSTATUS ReadMemory(uint64 Cr3, void* Address, void* AllocatedBuffer, size_t Size, size_t* Read)
	{
		if (Cr3 == 0 || !Address || !AllocatedBuffer || Size == 0 || !Read)
		{
			return STATUS_INVALID_PARAMETER;
		}

		size_t CurOffset = 0;
		size_t TotalSize = Size;
		NTSTATUS Status = STATUS_SUCCESS;

		while (TotalSize)
		{
			uint64 CurPhysAddr = physical::TranslateLinear(Cr3, reinterpret_cast<uint64>(Address) + CurOffset);
			if (!CurPhysAddr)
			{
				return STATUS_UNSUCCESSFUL;
			}

			uint64 ReadSize = min(PAGE_SIZE - (CurPhysAddr & 0xFFF), TotalSize);
			size_t BytesRead = 0;

			Status = physical::ReadPhysical(CurPhysAddr, reinterpret_cast<void*>(reinterpret_cast<uint64>(AllocatedBuffer) + CurOffset), ReadSize, &BytesRead);

			TotalSize -= BytesRead;
			CurOffset += BytesRead;

			if (Status != STATUS_SUCCESS || BytesRead == 0)
			{
				break;
			}
		}

		*Read = CurOffset;
		return Status;
	}

	NTSTATUS WriteMemory(uint64 Cr3, void* Address, void* Buffer, size_t Size, size_t* Written)
	{
		if (Cr3 == 0 || !Address || !Buffer || Size == 0 || !Written)
		{
			return STATUS_INVALID_PARAMETER;
		}

		size_t CurOffset = 0;
		size_t TotalSize = Size;
		NTSTATUS Status = STATUS_SUCCESS;

		while (TotalSize)
		{
			uint64 CurPhysAddr = physical::TranslateLinear(Cr3, reinterpret_cast<uint64>(Address) + CurOffset);
			if (!CurPhysAddr)
			{
				return STATUS_UNSUCCESSFUL;
			}

			uint64 WriteSize = min(PAGE_SIZE - (CurPhysAddr & 0xFFF), TotalSize);
			size_t BytesWritten = 0;

			Status = physical::WritePhysical(CurPhysAddr, reinterpret_cast<void*>(reinterpret_cast<uint64>(Buffer) + CurOffset), WriteSize, &BytesWritten);

			TotalSize -= BytesWritten;
			CurOffset += BytesWritten;

			if (Status != STATUS_SUCCESS || BytesWritten == 0)
			{
				break;
			}
		}

		*Written = CurOffset;
		return Status;
	}
}