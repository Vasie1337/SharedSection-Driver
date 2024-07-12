#pragma once
#include <include.hpp>

namespace ctx
{
	bool WriteProtected(void* Address, void* Buffer, unsigned __int64 Size)
	{
		NTSTATUS Status = STATUS_SUCCESS;

		PMDL Mdl = IoAllocateMdl(Address, Size, FALSE, FALSE, NULL);
		if (!Mdl)
			return false;

		MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);

		void* Mapped = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
		if (!Mapped)
		{
			MmUnlockPages(Mdl);
			IoFreeMdl(Mdl);
			return false;
		}

		Status = MmProtectMdlSystemAddress(Mdl, PAGE_READWRITE);
		if (!NT_SUCCESS(Status))
		{
			MmUnmapLockedPages(Mapped, Mdl);
			MmUnlockPages(Mdl);
			IoFreeMdl(Mdl);
			return false;
		}

		crt::memcpy(Mapped, Buffer, Size);

		Status = MmProtectMdlSystemAddress(Mdl, PAGE_READONLY);
		if (!NT_SUCCESS(Status))
		{
			MmUnmapLockedPages(Mapped, Mdl);
			MmUnlockPages(Mdl);
			IoFreeMdl(Mdl);
			return false;
		}

		MmUnmapLockedPages(Mapped, Mdl);
		MmUnlockPages(Mdl);
		IoFreeMdl(Mdl);
		return true;
	}

	void Sleep(unsigned long Milliseconds)
	{
		LARGE_INTEGER Interval;
		Interval.QuadPart = -10000 * Milliseconds;
		KeDelayExecutionThread(KernelMode, FALSE, &Interval);
	}

}