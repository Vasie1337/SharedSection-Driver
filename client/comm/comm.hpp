#pragma once
#include <include.hpp>

class Comm
{
public:
	static void SetTargetPid(long pid) { target_pid = pid; }

	static bool Open();
	static void Close();
	
	static std::uint64_t GetCr3();
	static std::uint64_t GetBase();
	
	static bool ReadPhysicalMemory(std::uint64_t Address, void* Buffer, std::size_t Size);
	static bool WritePhysicalMemory(std::uint64_t Address, void* Buffer, std::size_t Size);
	
	template <typename T>
	static T Read(std::uint64_t Address, std::size_t Size = sizeof(T));
	
	template <typename T>
	static bool Write(std::uint64_t Address, T Value);
	

private:
	static void Initialize();
	static bool OpenEvents();
	
	inline static long target_pid{};

	inline static shared_data* shared_section{};
	inline static HANDLE section_handle{};

	inline static HANDLE event_handle{};
	inline static HANDLE event_handle_response{};
};

template<typename T>
inline T Comm::Read(std::uint64_t Address, std::size_t Size)
{
	T Buffer{};
	ReadPhysicalMemory(Address, &Buffer, Size);
	return Buffer;
}

template<typename T>
inline bool Comm::Write(std::uint64_t Address, T Value)
{
	return WritePhysicalMemory(Address, &Value, sizeof(T));
}
