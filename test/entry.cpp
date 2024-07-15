#include <include.hpp>

void atexit_handler_1()
{
	MessageBox(0, 0, 0, 0);
}


int main()
{
	const int result_1 = std::atexit(atexit_handler_1);
    if (result_1)
    {
        std::cerr << "Registration failed!\n";
        return EXIT_FAILURE;
    }

    Sleep(10000);

    std::cout << "Returning from main...\n";
    return EXIT_SUCCESS;
}