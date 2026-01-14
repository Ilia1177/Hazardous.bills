#include "PrintFact.hpp"
#include <csignal>
#include <locale>
#include <exception>

bool g_interrupt = false;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        // std::cout << "\n\nReceived Ctrl+C (SIGINT)" << std::endl;
        // std::cout << "Quit the Hazardous Collective" << std::endl;
		// throw std::exception();
		g_interrupt = true;
    }
}

int main(int ac, char** av) {
    // std::setlocale(LC_ALL, "en_US.UTF-8");
    signal(SIGINT, signal_handler);
	std::string arg("");
	std::ifstream estimate;
	if (ac > 1) {
		arg = av[1];
	}
	try {
		PrintFact billing;
		billing.receives(arg);
	} catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
