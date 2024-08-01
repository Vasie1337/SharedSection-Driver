#include <include.hpp>

__forceinline EXTERN_C NTSTATUS Entry(PMDL MdlPtr, void* param2)
{
	if (!MdlPtr)
		return STATUS_INVALID_PARAMETER;

	hide::pfn::Clear(MdlPtr);
	hide::offsets::Load();

	const auto ntoskrnl = modules::get_kernel_module(skCrypt("ntoskrnl.exe"));
	if (!ntoskrnl)
	{
		printf("Failed to get ntoskrnl.exe\n");
		return STATUS_NOT_FOUND;
	}

	ret::trampoline_gadget = reinterpret_cast<void*>(scanner::find_pattern(
		ntoskrnl.base,
		"\xFF\x26",
		"xx"
	));
	if (!ret::trampoline_gadget)
	{
		printf("Failed to find ret trampoline gadget\n");
		return STATUS_NOT_FOUND;
	}

	return comm::Open();
}