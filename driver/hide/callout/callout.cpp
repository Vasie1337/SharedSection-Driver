#include "callout.hpp"

uintptr_t g_ret_instruction_addr = 0; //referenced by asm_callout
uintptr_t g_rop_gadget_addr = 0; //referenced by asm_callout

static void thread_boostrapper(void* start_context, void* kernel_stack_preserve);

NTSTATUS CreateThread(uintptr_t StartRoutine)
{
	auto ntoskrnl = modules::get_kernel_module("ntoskrnl.exe");
	if (!ntoskrnl)
		return STATUS_NOT_FOUND;

	auto section = modules::get_section(ntoskrnl, ".text");
	if (!section)
		return STATUS_NOT_FOUND;

	g_ret_instruction_addr = scanner::find_pattern(ntoskrnl.base, ntoskrnl.size, "\xC3", "x");
	if (!g_ret_instruction_addr)
		return STATUS_INTERNAL_ERROR;

	//48 8B E5 48 8B AD ? ? ? ? 48 81 C4 ? ? ? ? 48 CF
	/*
	mov rsp, rbp
	mov rbp, [rbp+????????h]	;4 byte reference
	add rsp, ????????h	;ditto
	iretq

	note that the first offset does not matter but the second offset is assumed to never change. it is therefore hardcoded within the assembly routines and assumed to be 0xE8.
	*/
	g_rop_gadget_addr = scanner::find_pattern(ntoskrnl.base, ntoskrnl.size, "\x48\x8B\xE5\x48\x8B\xAD\x00\x00\x00\x00\x48\x81\xC4\x00\x00\x00\x00\x48\xCF", "xxxxxx????xxx????xx");
	if (!g_rop_gadget_addr)
		return STATUS_INTERNAL_ERROR;

	size_t stack_size = PAGE_SIZE * 8;
	uintptr_t real_stack = (uintptr_t)ExAllocatePool(NonPagedPool, stack_size);
	if (!real_stack)
		return STATUS_INTERNAL_ERROR; //failed to allocate stack memory

	crt::memset((void*)real_stack, 0, stack_size);

	/*
	cli
	mov rdx, rsp ;store stack pointer; this value is always unaligned
	mov rsp, ????????????????h ;stack pointer to load
	movabs rax, ????????????????h ;routine to jump to
	jmp rax
	*/
	UCHAR thread_start_shellcode[] = { 
		0xFA, 
		0x48, 0x89, 0xE2, 
		0x48, 0xBC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0xFF, 0xE0 
	};
	UCHAR* shellcode_base = (UCHAR*)ExAllocatePool(NonPagedPool, sizeof(thread_start_shellcode));
	if (!shellcode_base)
		return STATUS_INTERNAL_ERROR;

	//note that shellcode_base is not freed in this implementation, and neither is the real thread stack.
	crt::memcpy(shellcode_base, &thread_start_shellcode[0], sizeof(thread_start_shellcode));

	*(ULONG64*)(&shellcode_base[6]) = real_stack + stack_size - 40; //allocate an aligned stack frame for the real stack; and point the stack pointer to the stack base
	*(ULONG64*)(&shellcode_base[16]) = StartRoutine;

	//create system thread
	OBJECT_ATTRIBUTES object_attr{};
	InitializeObjectAttributes(&object_attr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

	HANDLE thread_handle = NULL;
	NTSTATUS status = PsCreateSystemThread(&thread_handle, 0, &object_attr, NULL, NULL, (PKSTART_ROUTINE)shellcode_base, NULL);
	if (!NT_SUCCESS(status))
		return status;

	ZwClose(thread_handle);

	return STATUS_SUCCESS;
}