#define PRINT_DEBUG 1

#if PRINT_DEBUG
#define printf(text, ...) DbgPrintEx(DPFLTR_IHVBUS_ID, 0, text, ##__VA_ARGS__)
#else
#define printf(text, ...)
#endif

#define dereference(ptr)(uint64)(ptr + *(int*)((unsigned int*)ptr + 3 ) + 7)
#define in_range(x,a,b)(x >= a && x <= b) 
#define get_bits(x)(in_range((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xA) : (in_range(x,'0','9') ? x - '0' : 0))
#define get_byte(x)(get_bits(x[0]) << 4 | get_bits(x[1]))
#define to_lower_i(Char)((Char >= 'A' && Char <= 'Z') ? (Char + 32) : Char)
#define to_lower_c(Char)((Char >= (char*)'A' && Char <= (char*)'Z') ? (Char + 32) : Char)

#define print_bytes(address, size) \
{ \
	for (int i = 0; i < size; i++) \
	{ \
		printf("%02X ", *(unsigned char*)(address + i)); \
	} \
}

