#pragma once
#include <include.hpp>

namespace hide::pfn
{
    inline bool Clear(PMDL Mdl)
	{
        PPFN_NUMBER Array = MmGetMdlPfnArray(Mdl);
        if (!Array)
            return false;

        ULONG Count = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Mdl), MmGetMdlByteCount(Mdl));

        for (ULONG i = 0; i < Count; i++)
        {
            size_t bytes = 0;
            crt::memset(&Array[i], 0, sizeof(ULONG));

            printf("Clearing PFN: %p\n", &Array[i]);
        }

        return true;
	}
}