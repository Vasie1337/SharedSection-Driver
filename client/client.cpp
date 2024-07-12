#include <../.shared/shared.hpp>

#include <iostream>
#include <windows.h>

namespace registry
{
	bool create_key(const char* key, const char* value, void* data, size_t size)
	{
		HKEY hkey;
		if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, key, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hkey, nullptr) != ERROR_SUCCESS)
			return false;

		if (RegSetValueExA(hkey, value, 0, REG_BINARY, (BYTE*)data, size) != ERROR_SUCCESS)
		{
			RegCloseKey(hkey);
			return false;
		}

		RegCloseKey(hkey);
		return true;
	}
}

namespace shared_memory
{
	PSHARED_DATA mem = nullptr;

	bool create()
	{
		mem = reinterpret_cast<PSHARED_DATA>(VirtualAlloc(
			nullptr,
			sizeof(SHARED_DATA),
			MEM_COMMIT | MEM_RESERVE, 
			PAGE_READWRITE
		));
		if (!mem)
			return false;

		unsigned long current_pid = GetCurrentProcessId();

		if (!registry::create_key("Software\\shared_memory", "pid", &current_pid, sizeof(unsigned long)))
			return false;

		if (!registry::create_key("Software\\shared_memory", "address", &mem, sizeof(void*)))
			return false;

		return true;
	}

	void resolve_dtb()
	{
		mem->magic = 0x74;
		mem->type = comm_type::dirbase;
		mem->pid = GetCurrentProcessId();
		mem->delivered = false;

		while (!mem->delivered)
			Sleep(1);
	}

	void read(UINT64 address, UINT64 buffer, SIZE_T size)
	{
		mem->magic = 0x74;
		mem->type = comm_type::read;
		mem->pid = GetCurrentProcessId();
		mem->address = address;
		mem->buffer = buffer;
		mem->size = size;

		mem->delivered = false;

		while (!mem->delivered)
			Sleep(1);
	}
}

int main() 
{
	if (!shared_memory::create())
	{
		std::cout << "Failed to create shared memory" << std::endl;
		return 1;
	}

	shared_memory::resolve_dtb();

	std::cout << "Resolved DTB" << std::endl;

	//int sample = 1337;
	//int buffer = 0;
	//
	//shared_memory::read(
	//	reinterpret_cast<UINT64>(&sample), 
	//	reinterpret_cast<UINT64>(&buffer), 
	//	sizeof(int)
	//);
	//
	//std::cout << "Read: " << buffer << std::endl;

	getchar();

	return 0;
}