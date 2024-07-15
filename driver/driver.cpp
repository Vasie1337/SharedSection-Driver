#include <include.hpp>

EXTERN_C NTSTATUS Entry(void* param1, void* param2)
{
	hide::offsets::Load();

	return comm::Open();
}