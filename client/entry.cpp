#include <include.hpp>

int main()
{
	if (!Comm::Open())
	{
		std::cerr << "Failed to open comm" << std::endl;
		return 1;
	}

	Comm::SetTargetPid(GetCurrentProcessId());

	std::uint64_t Base = Comm::GetBase();
	std::cout << "Base: " << std::hex << Base << std::endl;

	std::uint64_t Cr3 = Comm::GetCr3();
	std::cout << "Cr3: " << std::hex << Cr3 << std::endl;

	short Mz = Comm::Read<short>(Base);
	std::cout << "Mz: " << std::hex << Mz << std::endl;

	Comm::Close();

	getchar();
}