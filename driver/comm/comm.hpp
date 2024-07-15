#pragma once
#include <include.hpp>

using namespace return_spoofer;

class comm 
{
public:

	// Open shared section
	static NTSTATUS Open()
	{
		InitStrings();

		OBJECT_ATTRIBUTES ObjectAttributes{};
		InitializeObjectAttributes(&ObjectAttributes,
			&SectionName,
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
			NULL,
			NULL
		);

		Status = ZwOpenSection(
			&SectionHandle,
			SECTION_MAP_READ | SECTION_QUERY,
			&ObjectAttributes
		);
		if (!NT_SUCCESS(Status))
		{
			printf("Failed to open section: 0x%X\n", Status);
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
	static NTSTATUS OpenEvents()
	{
		Event = IoCreateNotificationEvent(&EventName, &EventHandle);
		if (!Event)
		{
			printf("Failed to create event\n");
			return STATUS_UNSUCCESSFUL;
		}

		EventResponse = IoCreateNotificationEvent(&EventResponseName, &EventResponseHandle);
		if (!EventResponse)
		{
			printf("Failed to create event response\n");
			return STATUS_UNSUCCESSFUL;
		}

		return STATUS_SUCCESS;
	}

	// Packet handler thread	
	static void Handler(void* Context) 
	{
		auto* Data = reinterpret_cast<shared_data*>(MappedBase);
		if (!Init(Data))
		{
			printf("Failed to initialize\n");
			goto DESTROY_AND_CLEANUP;
		}
		
		if (!thread::Hide())
		{
			printf("Failed to hide thread\n");
			goto DESTROY_AND_CLEANUP;
		}

		while (true)
		{
			KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, 0);

			PEPROCESS TargetProcess{};
			NTSTATUS Status = PsLookupProcessByProcessId(reinterpret_cast<HANDLE>(Data->process_id), &TargetProcess);
			if (!NT_SUCCESS(Status))
			{
				printf("Failed to lookup process by process id: 0x%X\n", Status);
				continue;
			}

			if (Data->size > CachePoolSize)
			{
				if (CachePool)
				{
					ExFreePool(CachePool);
				}

				CachePool = ExAllocatePool(NonPagedPoolNx, Data->size);
				if (!CachePool)
				{
					printf("Failed to allocate pool\n");
					continue;
				}

				CachePoolSize = Data->size;
			}

			switch (Data->type)
			{
				case comm_type::cr3:
				{
					const auto BaseAddress = reinterpret_cast<uint64>(PsGetProcessSectionBaseAddress(TargetProcess));
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
					const auto BaseAddress = reinterpret_cast<uint64>(PsGetProcessSectionBaseAddress(TargetProcess));
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
				case comm_type::init:
				{
					break;
				}
				default:
				{
					printf("Received unknown type\n");
					break;
				}
			}

			KeSetEvent(EventResponse, IO_NO_INCREMENT, FALSE);
		}

DESTROY_AND_CLEANUP:
		printf("Exiting thread\n");
		Cleanup();
		PsTerminateSystemThread(STATUS_SUCCESS);
	}

	// Init for client
	static bool Init(shared_data* Data)
	{
		ClientPID = reinterpret_cast<HANDLE>(Data->process_id);
		printf("Client PID: 0x%X\n", ClientPID);
		if (!ClientPID)
		{
			printf("Invalid client pid\n");
			return false;
		}

		Status = PsLookupProcessByProcessId(ClientPID, &ClientProcess);
		if (!NT_SUCCESS(Status))
		{
			printf("Failed to lookup client by process id: 0x%X\n", Status);
			return false;
		}

		printf("Client process: 0x%p\n", ClientProcess);

		ClientBaseAddress = reinterpret_cast<uint64>(PsGetProcessSectionBaseAddress(ClientProcess));
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

		KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
		return true;
	}

	// Cleanup
	static void Cleanup()
	{
		if (SectionHandle)
		{
			ZwClose(SectionHandle);
		}

		if (MappedBase)
		{
			MmUnmapViewInSystemSpace(MappedBase);
		}

		if (EventHandle)
		{
			ZwClose(EventHandle);
		}

		if (EventResponseHandle)
		{
			ZwClose(EventResponseHandle);
		}

		if (CachePool)
		{
			ExFreePool(CachePool);
		}

		if (ClientProcess)
		{
			ObfDereferenceObject(ClientProcess);
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