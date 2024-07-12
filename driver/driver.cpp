#include <include.hpp>

//typedef __int64(__fastcall* tInitializeFunction)(UINT64 a1);
//typedef __int64(__fastcall* tQueryCounterFunction)();
//typedef __int64(__fastcall* tAcknowledgeInterruptFunction)(UINT64 a1);
//
//struct HalTimer
//{
//	UINT64 SomeNumber; // 0x9000000001i64 // 0
//	tInitializeFunction Initialize; // 8
//	tQueryCounterFunction QueryCounter; // 16
//	tAcknowledgeInterruptFunction AcknowledgeInterrupt; // 24
//};
//
//__int64 InitMyTimer(UINT64 a1)
//{
//	printf("InitMyTimer: %llx\n", a1);
//	return 0;
//}
//
//__int64 QueryMyCounter()
//{
//	printf("QueryMyCounter\n");
//	return 0;
//}
//
//__int64 AcknowledgeMyInterrupt(UINT64 a1)
//{
//	printf("AcknowledgeMyInterrupt: %llx\n", a1);
//	return 0;
//}
//
//NTSTATUS HalpTimerRegister(HalTimer* Timer, UNICODE_STRING* ResourceIdString)
//{
//	UINT64 buffer = (UINT64)ExAllocatePool(NonPagedPool, 0x1000);
//	if (!buffer)
//	{
//		printf("Failed to allocate buffer\n");
//		return STATUS_UNSUCCESSFUL;
//	}
//
//	memset((void*)buffer, 0, 0x1000);
//
//	*(void**)(buffer + 104) = Timer->Initialize;
//	*(void**)(buffer + 120) = Timer->AcknowledgeInterrupt;
//
//	v17 = (__int64*)qword_C4C028;
//	if (*(ULONG_PTR**)qword_C4C028 != &HalpRegisteredTimers)
//		__fastfail(3u);
//	LODWORD(HalpRegisteredTimerCount) = HalpRegisteredTimerCount + 1;
//	*(_QWORD*)Buffer_1 = &HalpRegisteredTimers;
//	*(_QWORD*)(Buffer_1 + 8) = v17;
//	*v17 = Buffer_1;
//	qword_C4C028 = Buffer_1;
//	if ((*(_DWORD*)(Buffer_1 + 224) & 0x100000) != 0)
//		HalpTimerAuxiliaryClockEnabled = 1;
//
//
//    return STATUS_SUCCESS;
//}

EXTERN_C NTSTATUS Entry(void* param1, void* param2)
{
	//DATA_ENTRY ntoskrnl = modules::get_kernel_module("ntoskrnl.exe");
	//if (!ntoskrnl)
	//{
	//	printf("Failed to find ntoskrnl.exe\n");
	//	return STATUS_UNSUCCESSFUL;
	//}
	//
	//printf("ntoskrnl: %llx\n", ntoskrnl.base);
	//
	//HalTimer Timer{};
	//Timer.SomeNumber = 0x9000000001i64;
	//Timer.Initialize = InitMyTimer;
	//Timer.QueryCounter = QueryMyCounter;
	//Timer.AcknowledgeInterrupt = AcknowledgeMyInterrupt;
	//
	//UNICODE_STRING Name{};
	//RtlInitUnicodeString(&Name, L"Timer");
	//
	////NTSTATUS Status = HalpTimerRegister(&Timer, &Name);
	//
	//printf("HalpTimerRegister: 0x%X\n", Status);

	if (!comm::Initialize())
	{
		printf("Failed to init shared memory\n");
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}
