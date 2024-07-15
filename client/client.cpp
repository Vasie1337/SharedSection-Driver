#include <../.shared/shared.hpp>

#include <iostream>
#include <windows.h>
#include <signal.h>
#include <signal.h>
#include <chrono>

class Comm 
{
public:
	bool Open()
	{
		target_pid = 1856;

		section_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(shared_data), "Global\\SharedData");
		if (!section_handle)
		{
			std::cout << "Failed to create section: " << GetLastError() << std::endl;
			return 1;
		}

		shared_section = reinterpret_cast<shared_data*>(MapViewOfFile(section_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shared_data)));
		if (!shared_section)
		{
			std::cout << "Failed to map view of section: " << GetLastError() << std::endl;
			CloseHandle(section_handle);
			return false;
		}

		if (!OpenEvents())
		{
			return false;
		}

		Initialize();

		return true;
	}

	void Close()
	{
		shared_section->type = comm_type::destory;

		SetEvent(event_handle);
		WaitForSingleObject(event_handle_response, INFINITE);

		UnmapViewOfFile(shared_section);
		CloseHandle(section_handle);
		CloseHandle(event_handle);
	}

	std::uint64_t GetCr3()
	{
		shared_section->type = comm_type::cr3;
		shared_section->process_id = target_pid;

		SetEvent(event_handle);
		WaitForSingleObject(event_handle_response, INFINITE);

		return shared_section->buffer;
	}

	std::uint64_t GetBase()
	{
		shared_section->type = comm_type::base;
		shared_section->process_id = target_pid;

		SetEvent(event_handle);
		WaitForSingleObject(event_handle_response, INFINITE);

		return shared_section->buffer;
	}

	bool ReadPhysicalMemory(std::uint64_t Address, void* Buffer, std::size_t Size)
	{
		shared_section->type = comm_type::read_physical;
		shared_section->process_id = target_pid;
		shared_section->address = Address;
		shared_section->buffer = reinterpret_cast<std::uint64_t>(Buffer);
		shared_section->size = Size;

		SetEvent(event_handle);
		WaitForSingleObject(event_handle_response, INFINITE);

		return true;
	}

	bool WritePhysicalMemory(std::uint64_t Address, void* Buffer, std::size_t Size)
	{
		shared_section->type = comm_type::write_physical;
		shared_section->process_id = target_pid;
		shared_section->address = Address;
		shared_section->buffer = reinterpret_cast<std::uint64_t>(Buffer);
		shared_section->size = Size;

		SetEvent(event_handle);
		WaitForSingleObject(event_handle_response, INFINITE);

		return true;
	}

	template <typename T>
	T Read(std::uint64_t Address, std::size_t Size = sizeof(T))
	{
		T Buffer{};
		ReadPhysicalMemory(Address, &Buffer, Size);
		return Buffer;
	}

	template <typename T>
	bool Write(std::uint64_t Address, T Value)
	{
		return WritePhysicalMemory(Address, &Value, sizeof(T));
	}

private:
	void Initialize()
	{
		shared_section->type = comm_type::init;
		shared_section->process_id = GetCurrentProcessId();

		SetEvent(event_handle);
		WaitForSingleObject(event_handle_response, INFINITE);
	}
	
	bool OpenEvents()
	{
		event_handle = CreateEventA(NULL, FALSE, FALSE, "Global\\SharedMemEvent");
		if (!event_handle)
		{
			std::cout << "Failed to open event: " << GetLastError() << std::endl;
			return false;
		}

		event_handle_response = CreateEventA(NULL, FALSE, FALSE, "Global\\SharedMemEventResponse");
		if (!event_handle_response)
		{
			std::cout << "Failed to open response event: " << GetLastError() << std::endl;
			return false;
		}
		return true;
	}

	long target_pid{};

	shared_data* shared_section{};
	HANDLE section_handle{};

	HANDLE event_handle{};
	HANDLE event_handle_response{};
};

Comm comm{};

int main() 
{
	if (!comm.Open())
	{
		std::cout << "Failed to open communication" << std::endl;
		return 1;
	}

	std::uint64_t Base = comm.GetBase();
	std::cout << "Base: " << std::hex << Base << std::endl;

	std::uint64_t Cr3 = comm.GetCr3();
	std::cout << "Cr3: " << std::hex << Cr3 << std::endl;



	comm.Close();

	return 0;
}