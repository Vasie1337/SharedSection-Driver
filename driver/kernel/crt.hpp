#pragma once
namespace crt
{
	__forceinline void* __cdecl memmove(void* Address, const void* Src, unsigned __int64 Size)
    {
        unsigned char* dest = static_cast<unsigned char*>(Address);
        const unsigned char* src = static_cast<const unsigned char*>(Src);

        if (dest == src) {
            return Address;
        }

        if (dest < src) {
            for (unsigned __int64 i = 0; i < Size; ++i) {
                dest[i] = src[i];
            }
        }
        else {
            for (unsigned __int64 i = Size; i > 0; --i) {
                dest[i - 1] = src[i - 1];
            }
        }

        return Address;
    }

	__forceinline void* __cdecl memset(void* Address, int Value, unsigned __int64 Size)
    {
        unsigned char* dest = static_cast<unsigned char*>(Address);
        unsigned char val = static_cast<unsigned char>(Value);

        for (unsigned __int64 i = 0; i < Size; ++i) {
            dest[i] = val;
        }

        return Address;
    }

	__forceinline void* __cdecl memcpy(void* Address, const void* Src, unsigned __int64 Size)
    {
        unsigned char* dest = static_cast<unsigned char*>(Address);
        const unsigned char* src = static_cast<const unsigned char*>(Src);

        for (unsigned __int64 i = 0; i < Size; ++i) {
            dest[i] = src[i];
        }

        return Address;
    }

	__forceinline int __cdecl memcmp(const void* First, const void* Second, unsigned __int64 Size)
    {
        const unsigned char* ptr1 = static_cast<const unsigned char*>(First);
        const unsigned char* ptr2 = static_cast<const unsigned char*>(Second);

        for (unsigned __int64 i = 0; i < Size; ++i) {
            if (ptr1[i] != ptr2[i]) {
                return (ptr1[i] < ptr2[i]) ? -1 : 1;
            }
        }

        return 0;
    }

	__forceinline unsigned __int64 __cdecl strlen(const char* Str)
	{
		unsigned __int64 len = 0;

		while (Str[len]) {
			++len;
		}

		return len;
	}

	__forceinline char* __cdecl strcpy(char* Dest, const char* Src)
	{
		char* dest = Dest;

		while ((*dest++ = *Src++) != '\0') {
			;
		}

		return Dest;
	}

	__forceinline int __cdecl strcmp(const char* Str1, const char* Str2)
	{
		while (*Str1 && *Str1 == *Str2) {
			++Str1;
			++Str2;
		}

		return *(unsigned char*)Str1 - *(unsigned char*)Str2;
	}

	__forceinline char* __cdecl strcat(char* Dest, const char* Src)
	{
		char* dest = Dest;

		while (*dest) {
			++dest;
		}

		while ((*dest++ = *Src++) != '\0') {
			;
		}

		return Dest;
	}

	__forceinline char* __cdecl strncpy(char* Dest, const char* Src, unsigned __int64 Count)
	{
		char* dest = Dest;

		while (Count && (*dest++ = *Src++)) {
			--Count;
		}

		if (Count) {
			while (--Count) {
				*dest++ = '\0';
			}
		}

		return Dest;
	}

	__forceinline int __cdecl strncmp(const char* Str1, const char* Str2, unsigned __int64 Count)
	{
		while (Count && *Str1 && *Str1 == *Str2) {
			++Str1;
			++Str2;
			--Count;
		}

		return Count ? *(unsigned char*)Str1 - *(unsigned char*)Str2 : 0;
	}

	__forceinline char* __cdecl strstr(const char* Str, const char* SubStr)
	{
		unsigned __int64 len = strlen(SubStr);

		while (*Str) {
			if (!strncmp(Str, SubStr, len)) {
				return const_cast<char*>(Str);
			}

			++Str;
		}

		return nullptr;
	}


}