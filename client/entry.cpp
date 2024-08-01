#include <include.hpp>

int main()
{
	if (!Comm::Open())
	{
		std::cerr << "Failed to open comm" << std::endl;
		return 1;
	}

	int pid = Utils::FindProcessId("FortniteClient-Win64-Shipping.exe");
	if (!pid)
	{
		std::cerr << "Failed to find FortniteClient-Win64-Shipping" << std::endl;
		return 1;
	}

	Comm::SetTargetPid(pid);

	std::uint64_t Base = Comm::GetBase(); 
	std::cout << "Base: " << std::hex << Base << std::endl;

	std::uint64_t Cr3 = Comm::GetCr3(); 
	std::cout << "Cr3: " << std::hex << Cr3 << std::endl;

	while (!GetAsyncKeyState(VK_INSERT))
	{
		uintptr_t Uworld = Comm::Read<uintptr_t>(Base + 0x118121a8);
		uintptr_t Uworld1 = Comm::Read<uintptr_t>(Base + 0x118011a8);
		std::cout << "Uworld: " << std::hex << Uworld << std::endl;
		std::cout << "Uworld1: " << std::hex << Uworld1 << std::endl;

		std::cout << "Press insert to close" << std::endl;
	}

	Comm::Close();

	getchar();
}