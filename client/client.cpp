#include <../.shared/shared.hpp>

#include <iostream>
#include <windows.h>
#include <TlHelp32.h>
#include <chrono>

class Utils
{
public:
	static int FindProcessId(std::string ProcessName)
	{
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot == INVALID_HANDLE_VALUE)
		{
			std::cout << "Failed to create snapshot: " << GetLastError() << std::endl;
			return 0;
		}

		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(entry);

		if (!Process32First(snapshot, &entry))
		{
			std::cout << "Failed to get first process: " << GetLastError() << std::endl;
			CloseHandle(snapshot);
			return 0;
		}

		do
		{
			if (ProcessName.compare(entry.szExeFile) == 0)
			{
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}
		} while (Process32Next(snapshot, &entry));

		CloseHandle(snapshot);
		return 0;
	}
};

class Comm 
{
public:
	static bool Open(int ProcessId)
	{
		if (!ProcessId)
		{
			std::cout << "Invalid process id" << std::endl;
			return false;
		}

		target_pid = ProcessId;

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
	static bool Open(std::string ProcessName)
	{
		return Open(Utils::FindProcessId(ProcessName));
	}

	static void Close()
	{
		shared_section->type = comm_type::destory;

		SetEvent(event_handle);
		WaitForSingleObject(event_handle_response, INFINITE);

		UnmapViewOfFile(shared_section);
		CloseHandle(section_handle);
		CloseHandle(event_handle);
	}

	static std::uint64_t GetCr3()
	{
		shared_section->type = comm_type::cr3;
		shared_section->process_id = target_pid;

		SetEvent(event_handle);
		WaitForSingleObject(event_handle_response, INFINITE);

		return shared_section->buffer;
	}

	static std::uint64_t GetBase()
	{
		shared_section->type = comm_type::base;
		shared_section->process_id = target_pid;

		SetEvent(event_handle);
		WaitForSingleObject(event_handle_response, INFINITE);

		return shared_section->buffer;
	}

	static bool ReadPhysicalMemory(std::uint64_t Address, void* Buffer, std::size_t Size)
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

	static bool WritePhysicalMemory(std::uint64_t Address, void* Buffer, std::size_t Size)
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
	static void Initialize()
	{
		shared_section->type = comm_type::init;
		shared_section->process_id = GetCurrentProcessId();

		SetEvent(event_handle);
		WaitForSingleObject(event_handle_response, INFINITE);
	}
	
	static bool OpenEvents()
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

	inline static long target_pid{};
	inline static shared_data* shared_section{};
	inline static HANDLE section_handle{};
	inline static HANDLE event_handle{};
	inline static HANDLE event_handle_response{};
};

int main() 
{
	if (!Comm::Open("explorer.exe"))
	{
		std::cout << "Failed to open communication" << std::endl;
		return 1;
	}

	std::uint64_t Base = Comm::GetBase();
	std::cout << "Base: " << std::hex << Base << std::endl;

	std::uint64_t Cr3 = Comm::GetCr3();
	std::cout << "Cr3: " << std::hex << Cr3 << std::endl;



	Comm::Close();

	return 0;
}