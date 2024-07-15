#include <include.hpp>

int main()
{
	if (!Comm::Open("explorer.exe"))
	{
		std::cout << "Failed to open communication" << std::endl;
		return 1;
	}

	std::uint64_t Base = Comm::GetBase();
	std::cout << "Base: " << std::hex << Base << std::endl;

	std::uint64_t Cr3 = Comm::GetCr3();
	std::cout << "Cr3: " << std::hex << Cr3 << std::endl;



	Comm::Close();

	return 0;
}