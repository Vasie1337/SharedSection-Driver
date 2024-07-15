#include <include.hpp>

int main()
{
	if (!Comm::Open()) // Always call before mapping the driver
	{
		std::cerr << "Failed to open comm" << std::endl;
		return 1;
	}

	// Map the driver

	long TargetPid = 0;
	while (1)
	{
		Sleep(1000);

		TargetPid = Utils::FindProcessId("r5apex.exe");
		if (TargetPid)
		{
			Comm::SetTargetPid(TargetPid);
			break;
		}
	}

	// Now you can do whatever you want with the driver

	std::uint64_t Base = Comm::GetBase();
	std::cout << "Base: " << std::hex << Base << std::endl;

	std::uint64_t Cr3 = Comm::GetCr3();
	std::cout << "Cr3: " << std::hex << Cr3 << std::endl;

	short Mz = Comm::Read<short>(Base);
	std::cout << "Mz: " << std::hex << Mz << std::endl;

	Comm::Close(); // Always call after you're done with the driver
}