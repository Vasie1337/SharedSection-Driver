#include <include.hpp>

EXTERN_C NTSTATUS Entry(PMDL MdlPtr, void* param2)
{
	hide::offsets::Load();
	if (MdlPtr)
	{
		hide::pfn::Clear(MdlPtr);
	}

	return comm::Open();
}