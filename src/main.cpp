#include "Facture.hpp"
#include "PrintFact.hpp"
#include <csignal>
#include <exception>
#include <map>
#include <fstream>

bool g_interrupt = false;

void signal_handler(int signal)
{
    if (signal == SIGINT) {
        g_interrupt = true;
    }
}

int main(int ac, char** av)
{
    // signal(SIGINT, signal_handler);

	struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);

	try {
		Facturier facturier;
		facturier.run(ac, av);
	} catch (std::invalid_argument& e) {
		std::cout << "Illegal option: " << e.what() << std::endl;
	} catch (std::exception& e) {
		std::cout << "Error: " << e.what() << std::endl;
	}

    return 0;
}
