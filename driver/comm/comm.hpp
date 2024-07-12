#pragma once
#include <include.hpp>

namespace comm
{
	inline void* SharedDataAddress = nullptr;
	inline unsigned long ClientPid = 0;
	inline PEPROCESS ClientProcess = nullptr;

	inline PKTIMER timer;
	inline PKDPC dpc;

	SHARED_DATA ReadData()
	{
		SHARED_DATA Data{};
		SIZE_T Bytes{};
		NTSTATUS Status = MmCopyVirtualMemory(ClientProcess, SharedDataAddress, PsGetCurrentProcess(), &Data, sizeof(Data), KernelMode, &Bytes);
		if (Status != STATUS_SUCCESS)
		{
			printf("Failed to copy memory: 0x%X\n", Status);
			return SHARED_DATA();
		}
		return Data;
	}

	void WriteData(SHARED_DATA Data)
	{
		SIZE_T Bytes{};
		NTSTATUS Status = MmCopyVirtualMemory(PsGetCurrentProcess(), &Data, ClientProcess, SharedDataAddress, sizeof(Data), KernelMode, &Bytes);
		if (Status != STATUS_SUCCESS)
		{
			printf("Failed to copy memory: 0x%X\n", Status);
		}
	}

	NTSTATUS HandleData(SHARED_DATA* Data)
	{
		switch (Data->type)
		{
		case comm_type::test:
		{
			printf("Received test\n");
			return STATUS_SUCCESS;
		}
		case comm_type::read:
		{
			printf("Received read\n");
			return requests::ReadMemory(Data);
		}
		case comm_type::write:
		{
			printf("Received write\n");
			return requests::WriteMemory(Data);
		}
		//case comm_type::base:
		//{
		//	printf("Received base\n");
		//	return requests::ResolveBase(Data);
		//}
		case comm_type::dirbase:
		{
			printf("Received dirbase\n");
			return requests::ResolveDirBase(Data);
		}
		default:
		{
			printf("Unknown type\n");
		}
		}
	}

	VOID Routine(_KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
	{
		ctx::Sleep(1);
		printf("Routine\n");

		auto Data = ReadData();
		if (!Data.IsValid())
		{
			KeCancelTimer(timer);
			ObDereferenceObject(ClientProcess);
			return;
		}
		if (!Data.delivered)
		{
			HandleData(&Data);
			Data.delivered = true;
			WriteData(Data);
		}

		LARGE_INTEGER DueTime{};
		DueTime.QuadPart = -10000;
		KeSetTimerEx(timer, DueTime, 1, dpc);
	}

	bool Initialize()
	{
		if (!registry::read_key(L"\\Registry\\Machine\\Software\\shared_memory", L"pid", &ClientPid, sizeof(unsigned long)))
		{
			printf("Failed to read client PID\n");
			return false;
		}

		if (!registry::read_key(L"\\Registry\\Machine\\Software\\shared_memory", L"address", &SharedDataAddress, sizeof(void*)))
		{
			printf("Failed to read shared memory address\n");
			return false;
		}

		if (!ClientPid || !SharedDataAddress)
		{
			printf("Invalid shared memory\n");
			return false;
		}

		NTSTATUS Status = PsLookupProcessByProcessId(reinterpret_cast<HANDLE>(ClientPid), &ClientProcess);
		if (!NT_SUCCESS(Status))
		{
			printf("Failed to lookup process: 0x%X\n", Status);
			return false;
		}

		timer = (PKTIMER)ExAllocatePool(NonPagedPool, sizeof(KTIMER));
		dpc = (PKDPC)ExAllocatePool(NonPagedPool, sizeof(KDPC));

		if (timer == NULL || dpc == NULL)
		{
			if (timer) ExFreePool(timer);
			if (dpc) ExFreePool(dpc);
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		KeInitializeTimerEx(timer, NotificationTimer);
		KeInitializeDpc(dpc, Routine, NULL);

		LARGE_INTEGER dueTime;
		dueTime.QuadPart = -10000;
		KeSetTimerEx(timer, dueTime, 1, dpc);


		//HANDLE ThreadHandle{};
		//Status = PsCreateSystemThread(
		//	&ThreadHandle,
		//	THREAD_ALL_ACCESS,
		//	NULL,
		//	NULL,
		//	NULL,
		//	Thread,
		//	NULL
		//);
		//
		//if (!NT_SUCCESS(Status))
		//{
		//	printf("Failed to create system thread: 0x%X\n", Status);
		//	ObDereferenceObject(ClientProcess);
		//	return false;
		//}
		//
		//ZwClose(ThreadHandle);

		return true;
	}
}