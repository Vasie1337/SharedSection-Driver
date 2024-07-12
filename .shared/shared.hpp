#pragma once

enum class comm_type : unsigned long
{
	test = -1,
	read = 0,
	write = 1,
	base = 2,
	dirbase = 3,
};

typedef struct _SHARED_DATA
{
	unsigned short magic;

	comm_type type;

	unsigned long pid;

	unsigned long long address;
	unsigned long long buffer;
	unsigned long long size;

	volatile bool delivered;

	bool IsValid() const
	{
		return magic == 0x74;
	}

} SHARED_DATA, *PSHARED_DATA;