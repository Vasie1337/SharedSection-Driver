#pragma once

enum class comm_type : unsigned long
{
	init = 1,
	destory = 2,
	read_physical = 3,
	write_physical = 4,
	base = 5,
	cr3 = 6,
};

struct shared_data
{
	comm_type type;

	unsigned long process_id;

	unsigned long long address;
	unsigned long long buffer;
	unsigned long long size;
};