#pragma once
#include <include.hpp>

class comm 
{
public:

	// Open shared section
	__forceinline static NTSTATUS Open()
	{
		InitStrings();

		OBJECT_ATTRIBUTES ObjectAttributes{};
		InitializeObjectAttributes(&ObjectAttributes,
			&SectionName,
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
			NULL,
			NULL
		);

		LARGE_INTEGER MaximumSize{};
		MaximumSize.QuadPart = 1024 * 1024;

		Status = ZwCreateSection(
			&SectionHandle,
			SECTION_ALL_ACCESS,
			&ObjectAttributes,
			&MaximumSize,
			PAGE_READWRITE,
			SEC_COMMIT,
			NULL
		);
		if (!NT_SUCCESS(Status)) {
			printf("Failed to create section: 0x%X\n", Status);
			Cleanup();
			return Status;
		}

		PVOID SectionObject{};
		Status = ObReferenceObjectByHandle(
			SectionHandle,
			SECTION_MAP_READ | SECTION_QUERY,
			MmSectionObjectType,
			KernelMode,
			&SectionObject,
			nullptr
		);
		if (!NT_SUCCESS(Status))
		{
			printf("Failed to reference object by handle: 0x%X\n", Status);
			Cleanup();
			return Status;
		}

		Status = MmMapViewInSystemSpace(
			SectionObject, 
			&MappedBase, 
			&MappedSize
		);
		if (!NT_SUCCESS(Status))
		{
			printf("Failed to map view in system space: 0x%X\n", Status);
			Cleanup();
			return Status;
		}

		Status = OpenEvents();
		if (!NT_SUCCESS(Status))
		{
			printf("Failed to open event: 0x%X\n", Status);
			Cleanup();
			return Status;
		}
		printf("Event handle: 0x%p\n", EventHandle);

		HANDLE ThreadHandle{};
		Status = PsCreateSystemThread(
			&ThreadHandle, 
			0, 
			0, 
			0, 
			0, 
			Handler, 
			0
		);
		if (!NT_SUCCESS(Status))
		{
			printf("Failed to create system thread: 0x%X\n", Status);
			Cleanup();
			return Status;
		}

		ZwClose(ThreadHandle);
		return STATUS_SUCCESS;
	}

private:

	// Events
	__forceinline static NTSTATUS OpenEvents()
	{
		Event = IoCreateNotificationEvent(&EventName, &EventHandle);
		if (!Event)
		{
			printf("Failed to create event\n");
			return STATUS_UNSUCCESSFUL;
		}
		KeClearEvent(Event);

		EventResponse = IoCreateNotificationEvent(&EventResponseName, &EventResponseHandle);
		if (!EventResponse)
		{
			printf("Failed to create event response\n");
			return STATUS_UNSUCCESSFUL;
		}
		KeClearEvent(EventResponse);

		return STATUS_SUCCESS;
	}

	// Packet handler thread	
	__forceinline static void Handler(void* Context)
	{
		PEPROCESS TargetProcess{};
		shared_data* Data = reinterpret_cast<shared_data*>(MappedBase);

		if (!hide::thread::Hide())
		{
			printf("Failed to hide thread\n");
			goto DESTROY_AND_CLEANUP;
		}

		while (true)
		{
			ret::spoof_call(&KeWaitForSingleObject, (PVOID)Event, (KWAIT_REASON)Executive, (KPROCESSOR_MODE)KernelMode, (BOOLEAN)FALSE, (PLARGE_INTEGER)nullptr);
			ret::spoof_call(&KeClearEvent, (PRKEVENT)Event);

			if (Data->type == comm_type::none)
			{
				continue;
			}

			if (Data->type == comm_type::init)
			{
				printf("Received init\n");
				if (!Init(Data))
				{
					printf("Failed to initialize\n");
					goto DESTROY_AND_CLEANUP;
				}
				ret::spoof_call(&KeSetEvent, (PRKEVENT)EventResponse, (KPRIORITY)IO_NO_INCREMENT, (BOOLEAN)FALSE);
				continue;
			}

			if (Data->size > CachePoolSize)
			{
				if (CachePool)
				{
					ExFreePool(CachePool);
				}

				CachePool = ret::spoof_call(&ExAllocatePool, (POOL_TYPE)NonPagedPoolNx, (SIZE_T)Data->size);
				if (!CachePool)
				{
					printf("Failed to allocate pool\n");
					ret::spoof_call(&KeSetEvent, (PRKEVENT)EventResponse, (KPRIORITY)IO_NO_INCREMENT, (BOOLEAN)FALSE);
					continue;
				}

				CachePoolSize = Data->size;
				printf("Allocated new cache pool: 0x%p\n", CachePool);
			}

			NTSTATUS Status = ret::spoof_call(&PsLookupProcessByProcessId, reinterpret_cast<HANDLE>(Data->process_id), &TargetProcess);
			if (!NT_SUCCESS(Status))
			{
				printf("Failed to lookup process by process id: 0x%X\n", Status);
				ret::spoof_call(&KeSetEvent, (PRKEVENT)EventResponse, (KPRIORITY)IO_NO_INCREMENT, (BOOLEAN)FALSE);
				continue;
			}

			switch (Data->type)
			{
				case comm_type::cr3:
				{
					printf("Received cr3\n");
					const auto BaseAddress = reinterpret_cast<uint64>(ret::spoof_call(&PsGetProcessSectionBaseAddress, TargetProcess));
					if (!BaseAddress)
					{
						printf("Failed to get base address\n");
						break;
					}
					const auto DirBase = physical::cr3::GetFromBase(BaseAddress);
					if (!DirBase)
					{
						printf("Failed to get DirBase\n");
						break;
					}

					physical::cr3::StoredCr3 = DirBase;
					Data->buffer = DirBase;
					break;
				}
				case comm_type::base:
				{
					printf("Received base\n");
					const auto BaseAddress = reinterpret_cast<uint64>(ret::spoof_call(&PsGetProcessSectionBaseAddress, TargetProcess));
					if (!BaseAddress)
					{
						printf("Failed to get base address\n");
						break;
					}

					Data->buffer = BaseAddress;
					break;
				}
				case comm_type::read_physical:
				{
					printf("Received read physical\n");
					physical::ReadMemory(physical::cr3::StoredCr3, reinterpret_cast<void*>(Data->address), CachePool, Data->size, &Data->size);
					if (!Data->size)
					{
						printf("Failed to read memory\n");
						break;
					}
					physical::WriteMemory(ClientCr3, reinterpret_cast<void*>(Data->buffer), CachePool, Data->size, &Data->size);
					if (!Data->size)
					{
						printf("Failed to write memory\n");
						break;
					}
					break;
				}
				case comm_type::write_physical:
				{
					printf("Received write physical\n");
					physical::ReadMemory(ClientCr3, reinterpret_cast<void*>(Data->buffer), CachePool, Data->size, &Data->size);
					if (!Data->size)
					{
						printf("Failed to read memory\n");
						break;
					}
					physical::WriteMemory(physical::cr3::StoredCr3, reinterpret_cast<void*>(Data->address), CachePool, Data->size, &Data->size);
					if (!Data->size)
					{
						printf("Failed to write memory\n");
						break;
					}
					break;
				}
				case comm_type::destory:
				{
					printf("Received destory\n");
					goto DESTROY_AND_CLEANUP;
				}
				default:
				{
					printf("Received unknown type\n");
					break;
				}
			}

			ret::spoof_call(&KeSetEvent, (PRKEVENT)EventResponse, (KPRIORITY)IO_NO_INCREMENT, (BOOLEAN)FALSE);
		}

DESTROY_AND_CLEANUP:
		printf("Exiting thread\n");
		ret::spoof_call(&KeSetEvent, (PRKEVENT)EventResponse, (KPRIORITY)IO_NO_INCREMENT, (BOOLEAN)FALSE);
		Cleanup();
		ret::spoof_call(&PsTerminateSystemThread, STATUS_SUCCESS);
	}

	// Init for client
	__forceinline static bool Init(shared_data* Data)
	{
		ClientPID = reinterpret_cast<HANDLE>(Data->process_id);
		printf("Client PID: 0x%X\n", ClientPID);
		if (!ClientPID)
		{
			printf("Invalid client pid\n");
			return false;
		}

		Status = ret::spoof_call(&PsLookupProcessByProcessId, ClientPID, &ClientProcess);
		if (!NT_SUCCESS(Status))
		{
			printf("Failed to lookup client by process id: 0x%X\n", Status);
			return false;
		}

		printf("Client process: 0x%p\n", ClientProcess);

		ClientBaseAddress = reinterpret_cast<uint64>(ret::spoof_call(&PsGetProcessSectionBaseAddress, ClientProcess));
		printf("Client base address: 0x%p\n", ClientBaseAddress);
		if (!ClientBaseAddress)
		{
			printf("Failed to get client base address\n");
			return false;
		}

		ClientCr3 = physical::cr3::GetFromBase(ClientBaseAddress);
		printf("Client cr3: 0x%p\n", ClientCr3);
		if (!ClientCr3)
		{
			printf("Failed to get client cr3\n");
			return false;
		}
		return true;
	}

	// Cleanup
	__forceinline static void Cleanup()
	{
		if (SectionHandle)
		{
			ret::spoof_call(&ZwClose, SectionHandle);
		}

		if (MappedBase)
		{
			ret::spoof_call(&MmUnmapViewInSystemSpace, MappedBase);
		}

		if (EventHandle)
		{
			ret::spoof_call(&ZwClose, EventHandle);
		}

		if (EventResponseHandle)
		{
			ret::spoof_call(&ZwClose, EventResponseHandle);
		}

		if (CachePool)
		{
			ret::spoof_call(&ExFreePool, CachePool);
		}

		if (ClientProcess)
		{
			ret::spoof_call(&ObfDereferenceObject, (PVOID)ClientProcess);
		}
	}

	// Strings
	static void InitStrings()
	{
		RtlInitUnicodeString(&SectionName, L"\\BaseNamedObjects\\Global\\SharedData");
		RtlInitUnicodeString(&EventName, L"\\BaseNamedObjects\\SharedMemEvent");
		RtlInitUnicodeString(&EventResponseName, L"\\BaseNamedObjects\\SharedMemEventResponse");
	}

private:

	// Strings
	inline static UNICODE_STRING SectionName{};
	inline static UNICODE_STRING EventName{};
	inline static UNICODE_STRING EventResponseName{};

	// Global
	inline static NTSTATUS Status{};

	// Section
	inline static HANDLE SectionHandle{};
	inline static PVOID MappedBase{};
	inline static SIZE_T MappedSize{};

	// Client
	inline static PEPROCESS ClientProcess{};
	inline static HANDLE ClientPID{};
	inline static uint64 ClientBaseAddress{};
	inline static uint64 ClientCr3{};

	// Events
	inline static HANDLE EventHandle{};
	inline static PKEVENT Event{};
	inline static HANDLE EventResponseHandle{};
	inline static PKEVENT EventResponse{};

	// Cache
	inline static void* CachePool{};
	inline static size_t CachePoolSize{};
};