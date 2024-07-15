#pragma once
#include <include.hpp>

namespace modules
{
	typedef class DATA_ENTRY
	{
	public:
		bool operator!() const
		{
			return !base || !size;
		}

		uint64 base;
		uint64 size;
	};

	PRTL_PROCESS_MODULES get_modules()
	{
		NTSTATUS Status = STATUS_SUCCESS;
		ULONG Bytes = 0;
		PRTL_PROCESS_MODULES Modules = NULL;

		Status = ZwQuerySystemInformation(SystemModuleInformation, 0, Bytes, &Bytes);
		if (Status != STATUS_INFO_LENGTH_MISMATCH)
			return NULL;

		Modules = (PRTL_PROCESS_MODULES)ExAllocatePool(NonPagedPool, Bytes);
		if (!Modules)
			return NULL;

		Status = ZwQuerySystemInformation(SystemModuleInformation, Modules, Bytes, &Bytes);
		if (!NT_SUCCESS(Status))
		{
			ExFreePool(Modules);
			return NULL;
		}

		return Modules;
	}

	DATA_ENTRY get_kernel_module(const char* module_name)
	{
		DATA_ENTRY entry = { 0 };

		PRTL_PROCESS_MODULES Modules = get_modules();

		for (ULONG i = 0; i < Modules->NumberOfModules; i++)
		{
			if (crt::strstr(to_lower_c((const char*)Modules->Modules[i].FullPathName), module_name))
			{
				entry.base = (uint64)Modules->Modules[i].ImageBase;
				entry.size = (uint64)Modules->Modules[i].ImageSize;
				break;
			}
		}

		ExFreePool(Modules);
		return entry;
	}

	DATA_ENTRY get_section(DATA_ENTRY module, const char* section_name)
	{
		PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)module.base;
		PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)(module.base + dos_header->e_lfanew);
		PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt_headers);

		for (unsigned i = 0; i < nt_headers->FileHeader.NumberOfSections; i++)
		{
			if (crt::strstr((const char*)section->Name, section_name))
			{
				DATA_ENTRY entry = { 0 };
				entry.base = module.base + section->VirtualAddress;
				entry.size = section->Misc.VirtualSize;
				return entry;
			}

			section++;
		}

		return { 0 };
	}
}