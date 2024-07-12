#pragma once
#include <include.hpp>

namespace registry
{
	bool read_key(const wchar_t* key, const wchar_t* value, void* Data, ULONG size)
	{
		UNICODE_STRING KeyName{};
		RtlInitUnicodeString(&KeyName, key);

		OBJECT_ATTRIBUTES ObjectsAttributes{};
		InitializeObjectAttributes(
			&ObjectsAttributes,
			&KeyName,
			OBJ_CASE_INSENSITIVE,
			nullptr,
			nullptr
		);

		HANDLE Key{};
		if (ZwOpenKey(&Key, KEY_READ, &ObjectsAttributes) != STATUS_SUCCESS)
		{
			printf("Failed to open key\n");
			return false;
		}

		UNICODE_STRING ValueName;
		RtlInitUnicodeString(&ValueName, value);

		ULONG resultLength;
		NTSTATUS Status = ZwQueryValueKey(Key, &ValueName, KeyValuePartialInformation, nullptr, 0, &resultLength);

		if (Status != STATUS_BUFFER_TOO_SMALL)
		{
			printf("Failed to query value size %llx\n", Status);
			ZwClose(Key);
			return false;
		}

		KEY_VALUE_PARTIAL_INFORMATION* Info = (KEY_VALUE_PARTIAL_INFORMATION*)ExAllocatePool(NonPagedPool, resultLength);
		if (!Info)
		{
			printf("Failed to allocate memory\n");
			ZwClose(Key);
			return false;
		}

		Status = ZwQueryValueKey(Key, &ValueName, KeyValuePartialInformation, Info, resultLength, &resultLength);
		if (Status != STATUS_SUCCESS)
		{
			printf("Failed to query value %llx\n", Status);
			ExFreePool(Info);
			ZwClose(Key);
			return false;
		}

		if (Info->DataLength > size)
		{
			printf("Provided buffer is too small\n");
			ExFreePool(Info);
			ZwClose(Key);
			return false;
		}

		RtlCopyMemory(Data, Info->Data, Info->DataLength);

		ExFreePool(Info);
		ZwClose(Key);
		return true;
	}
}