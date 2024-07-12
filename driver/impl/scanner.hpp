#pragma once
#include <include.hpp>

namespace scanner
{
    unsigned long long find_pattern(unsigned long long base, size_t range, const char* pattern, const char* mask)
    {
        const auto check_mask = [](const char* base, const char* pattern, const char* mask) -> bool
        {
            for (; *mask; ++base, ++pattern, ++mask)
            {
                if (*mask == 'x' && *base != *pattern)
                {
                    return false;
                }
            }
            return true;
        };

        range = range - crt::strlen(mask);

        for (size_t i = 0; i < range; ++i)
        {
            if (check_mask(reinterpret_cast<const char*>(base) + i, pattern, mask))
            {
                return base + i;
            }
        }

        return 0;
    }

    unsigned long long find_pattern(unsigned long long base, const char* pattern, const char* mask)
    {
        const PIMAGE_NT_HEADERS headers = reinterpret_cast<PIMAGE_NT_HEADERS>(base + reinterpret_cast<PIMAGE_DOS_HEADER>(base)->e_lfanew);
        const PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(headers);

        for (size_t i = 0; i < headers->FileHeader.NumberOfSections; i++)
        {
            const PIMAGE_SECTION_HEADER section = &sections[i];

            if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
            {
                const auto match = find_pattern(base + section->VirtualAddress, section->Misc.VirtualSize, pattern, mask);

                if (match)
                {
                    return match;
                }
            }
        }

        return 0;
    }

    unsigned long long find_pattern(unsigned long long module_base, const char* pattern)
    {
        auto pattern_ = pattern;
        unsigned long long first_match = 0;

        if (module_base == 0)
        {
            return 0;
        }

        const auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(module_base + reinterpret_cast<IMAGE_DOS_HEADER*>(module_base)->e_lfanew);

        for (unsigned long long current = module_base; current < module_base + nt->OptionalHeader.SizeOfImage; current++)
        {
            if (*pattern_ == '\0')
            {
                return first_match;
            }

            if (*(reinterpret_cast<const unsigned int*>(pattern_)) == '\?' || *(reinterpret_cast<const unsigned int*>(current)) == get_byte(pattern_))
            {
                if (first_match == 0)
                {
                    first_match = current;
                }

                if (pattern_[2] == '\0')
                {
                    return first_match;
                }

                if (*(reinterpret_cast<const unsigned short*>(pattern_)) == '\?\?' || *(reinterpret_cast<const unsigned int*>(pattern_)) != '\?')
                {
                    pattern_ += 3;
                }
                else
                {
                    pattern_ += 2;
                }
            }
            else
            {
                pattern_ = pattern;
                first_match = 0;
            }
        }

        return 0;
    }

    unsigned __int64 search_byte_sequence(unsigned __int64 base, unsigned __int64 size, const unsigned char byte_sequence[])
    {
        for (unsigned __int64 i = 0; i < size - sizeof(byte_sequence); ++i)
        {
            if (crt::memcmp(reinterpret_cast<const void*>(base + i), byte_sequence, sizeof(byte_sequence)) == 0)
            {
                return base + i;
            }
        }
        return 0;
    }

    unsigned long long find_pattern(const DATA_ENTRY& module, const char* pattern, const char* mask)
    {
        return find_pattern(module.base, pattern, mask);
    }

    unsigned long long find_pattern(const DATA_ENTRY& module, const char* pattern)
    {
        return find_pattern(module.base, pattern);
    }

    unsigned long long find_pattern(const DATA_ENTRY& module, size_t range, const char* pattern, const char* mask)
    {
        return find_pattern(module.base, range, pattern, mask);
    }

    unsigned __int64 search_byte_sequence(const DATA_ENTRY& module, const unsigned char byte_sequence[])
    {
        return search_byte_sequence(module.base, module.size, byte_sequence);
    }
}
