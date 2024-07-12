#pragma once
#include <include.hpp>

namespace requests
{
	NTSTATUS ResolveDirBase(SHARED_DATA* Data)
	{
		PEPROCESS Process = nullptr;
		NTSTATUS Status = PsLookupProcessByProcessId(reinterpret_cast<HANDLE>(Data->pid), &Process);
		if (!NT_SUCCESS(Status))
		{
			printf("Failed to lookup process: 0x%X\n", Status);
			return Status;
		}

		void* BaseAddress = PsGetProcessSectionBaseAddress(Process);
		if (!BaseAddress)
		{
			printf("Failed to get base address\n");
			ObDereferenceObject(Process);
			return STATUS_UNSUCCESSFUL;
		}

		pml4::StoredDatabase = pml4::GetDirBaseFromBase(BaseAddress);
		ObDereferenceObject(Process);

		printf("StoredDatabase: %llx\n", pml4::StoredDatabase);

		return STATUS_SUCCESS;
	}

	NTSTATUS ReadMemory(SHARED_DATA* Data)
	{
		if (!pml4::StoredDatabase)
		{
			printf("StoredDatabase is not set\n");
			return STATUS_UNSUCCESSFUL;
		}

		if (!Data->address || !Data->size || !Data->buffer)
		{
			printf("Invalid parameters\n");
			return STATUS_UNSUCCESSFUL;
		}

		auto physical_address = physical::TranslateLinear(
			pml4::StoredDatabase,
			Data->address);

		if (!physical_address)
		{
			return STATUS_UNSUCCESSFUL;
		}

		auto final_size = physical::FindMin(
			PAGE_SIZE - (physical_address & 0xFFF),
			Data->size);

		size_t bytes = 0;
		if (!NT_SUCCESS(physical::Read(
			physical_address,
			reinterpret_cast<void*>(Data->buffer),
			final_size,
			&bytes)))
		{
			printf("failed to do read\n");
			return STATUS_UNSUCCESSFUL;
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS WriteMemory(SHARED_DATA* Data)
	{
		if (!pml4::StoredDatabase)
		{
			printf("StoredDatabase is not set\n");
			return STATUS_UNSUCCESSFUL;
		}

		if (!Data->address || !Data->size || !Data->buffer)
		{
			printf("Invalid parameters\n");
			return STATUS_UNSUCCESSFUL;
		}

		auto physical_address = physical::TranslateLinear(
			pml4::StoredDatabase,
			Data->address);

		if (!physical_address)
		{
			return STATUS_UNSUCCESSFUL;
		}

		auto final_size = physical::FindMin(
			PAGE_SIZE - (physical_address & 0xFFF),
			Data->size);

		size_t bytes = 0;
		if (!NT_SUCCESS(physical::Write(
			physical_address,
			reinterpret_cast<void*>(Data->buffer),
			final_size,
			&bytes)))
		{
			printf("failed to do write\n");
			return STATUS_UNSUCCESSFUL;
		}

		return STATUS_SUCCESS;
	}
}