#pragma once
#include <include.hpp>

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