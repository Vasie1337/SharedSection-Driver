#include <include.hpp>

int main()
{
	if (!Comm::Open())
	{
		std::cerr << "Failed to open comm" << std::endl;
		return 1;
	}

	int pid = Utils::FindProcessId("explorer.exe");
	if (!pid)
	{
		std::cerr << "Failed to find explorer" << std::endl;
		return 1;
	}

	Comm::SetTargetPid(pid);

	std::uint64_t Base = Comm::GetBase(); 
	std::cout << "Base: " << std::hex << Base << std::endl;

	std::uint64_t Cr3 = Comm::GetCr3(); 
	std::cout << "Cr3: " << std::hex << Cr3 << std::endl;

	Comm::Close();

	getchar();
}