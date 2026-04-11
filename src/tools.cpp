
#include "PrintFact.hpp"
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <exception>
#include <utility>

namespace fs = std::filesystem;
// Helper function to prompt and read a line
std::string userline(std::string label, bool blocking)
{
	bool fail = false;
	std::string value;
	do {
		if (fail)
			clearInputLine();
		std::cout << label << ": ";
		std::getline(std::cin, value);
		if (!fail) {
			label = "Field needed ! " + label;
			fail = true;
		}
	} while (blocking && value.empty() && std::cin.good() && !g_interrupt);
    return value;
}

size_t dial_menu(const std::vector<std::string>& menu)
{
	int margin = 4;
	if (g_interrupt)
		return 0;
	int choice = -1;
	for (size_t i = 0; i < menu.size(); i++) {
		std::cout << fit(" ", margin) << i << ". " << menu[i] << "\n";
	}
	std::cout << "\n";
	while (!g_interrupt) {
		if (!user_value(fit(" ", margin) + "Select", choice, true))
			return 0;
		if (choice < 0 || static_cast<size_t>(choice) > menu.size() + 1) {
			clearInputLine();
			std::cout << fit(" ", margin) << "Choice not available... ";
		} else {
			eraseLines(menu.size() + 2);
			std::cout << fit(" ", margin) << "* " << menu[choice] << "\n\n";
			break;
		}
	};
	return static_cast<size_t>(choice);
}
std::string end_str(int size, int offset, bool reset)
{
    static bool firstUse = true;
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::string poem;
    static std::ifstream* text = nullptr;  // lazy-initialized

    if (reset) {
        firstUse = true;  // also fix: reset should allow re-init
        delete text;
        text = nullptr;
        poem.clear();
        return "";
    }

    std::string off(" ");
    for (int i = 0; i < offset; i++) {
        off += " ";
    }

    if (firstUse) {
        std::uniform_int_distribution<size_t> distribution(0, 4);
        int r = distribution(generator);
        poem = (r > 0) ? "poem" + std::to_string(r) + ".txt" : "";

        if (!poem.empty()) {
            text = new std::ifstream(poem);
            if (!text->is_open()) {
                delete text;
                text = nullptr;
                poem.clear();
            }
        }
        firstUse = false;
    }

    if (text && text->is_open()) {
        std::string line;
        if (std::getline(*text, line)) {
            return off + line + "\n";
        }
    }

    return off + random_string(size, {" ", "█"}) + "\n";
}

// std::string end_str(int size, int offset, bool reset)
// {
// 	static bool firstUse = true;
//     static std::random_device rd;
//     static std::mt19937 generator(rd());
// 	static std::string poem;
//
// 	if (reset) {
// 		firstUse = false;
// 		return "";
// 	}
// 	std::string off(" ");
//     for (int i = 0; i < offset; i++) {
//         off += " ";
//     }
// 	if (firstUse) {
// 		std::uniform_int_distribution<size_t> distribution(0, 4);
// 		int r = distribution(generator);
// 		if (r > 0) {
// 			poem = "poem" + std::to_string(r) + ".txt";
// 		} else {
// 			poem.clear();
// 		}
// 	}
// 	firstUse = false;
// 	if (!poem.empty()) {
// 		static std::ifstream text(poem);
// 		if (text && text.is_open()) {
// 			std::string line;
// 			std::getline(text, line);
// 			return (off + line + "\n");
// 		}
// 	}
//     return off + random_string(size, {" ", "█"}) + "\n";
// }
//
void clearInputLine()
{
    std::cout << "\033[1A\r\033[2K" << std::flush;
}

void clearScreen()
{
    std::cout << "\033[2J\033[H";
}

void moveCursor(int row, int col)
{
    std::cout << "\033[" << row << ";" << col << "H";
}

void eraseLines(int lines)
{
    for (int i = 0; i < lines; ++i) {
        std::cout << "\033[1A\r\033[2K";
    }
    std::cout << std::flush;
}

void moveCursorUp(int lines)
{
    std::cout << "\033[" << lines << "A";
}

void clearLine()
{
	std::cout << "\033[2K";
}

bool user_value(const std::string& str, double& value, bool blocking) {
	std::string line;

	value = 0.0;
	while (!g_interrupt) {
		std::cout << str << ": ";
		if (!getline(std::cin, line)) 
			return false;
		if (g_interrupt)
			return false;
		try {
			value = std::stod(line);
		} catch (std::exception& e) {
			if (blocking) {
				clearInputLine();
				std::cerr << "Error: " << e.what() << ". ";
			}
		}
		if (blocking && value < EPSILON) {
			continue;
		} else if (value < 0.0) {
			continue;
		}
		break;
	} 
	return true;
}

bool user_value(const std::string& str, int& value, bool blocking) {
	std::string line;

	value = 0;
	while (!g_interrupt) {
		std::cout << str << ": ";
		if (!getline(std::cin, line)) 
			return false;
		if (g_interrupt)
			return false;
		try {
			value = std::stoi(line);
		} catch (std::exception& e) {
			if (blocking) {
				clearInputLine();
				std::cerr << "Error: " << e.what() << ". ";
				continue;
			}
		}
		if (blocking && value < 0) {
			clearInputLine();
			std::cerr << "Choice should be in menu... (" << value << "). ";
			continue;
		}
		break;
	}
	return true;
}

bool user_input(const std::string& str, std::string& input, bool blocking = true, size_t maxLength) {
	std::string line = "";

	while (!g_interrupt) {
		std::cout << str;
		if (!std::getline(std::cin, line))
			return false;
		if (g_interrupt)
			return false;
		trim(line);
		if (maxLength == 0)
			break;
		if (line.length() > maxLength) {
			clearInputLine();
			std::cout << maxLength << " characters max. ";
			continue;
		} else if (blocking && line.empty()) {
			clearInputLine();
			std::cout << "Field needed ! ";
			continue;
		}
		break;
	};
	input = line;
	return true;
}

std::string get_bill_filename(const std::string& pattern)
{
	std::string empty("");

    if (!fs::exists("factures") && !fs::exists("devis")) {
        return empty;
    }
    for (const auto& entry : fs::directory_iterator("factures")) {
        std::string filename = entry.path().filename().string();
        if (filename.find(pattern) != std::string::npos) {
            return filename;
        }
    }
    for (const auto& entry : fs::directory_iterator("devis")) {
        std::string filename = entry.path().filename().string();
        if (filename.find(pattern) != std::string::npos) {
            return filename;
        }
    }
    for (const auto& entry : fs::directory_iterator("tickets")) {
        std::string filename = entry.path().filename().string();
        if (filename.find(pattern) != std::string::npos) {
            return filename;
        }
    }
    return empty;
}

void stream_find_replace(std::stringstream& ss, const std::string& search, const std::string& replace) {
    std::string content = ss.str();
    size_t pos = content.find(search);
    
    if (pos != std::string::npos) {
        content.replace(pos, search.length(), replace);
        ss.str(content);
        ss.clear();
    }
}

// Usage
std::string random_string(size_t length, const std::vector<std::string>& charset)
{
    static std::random_device rd;
    static std::mt19937       generator(rd());

    std::uniform_int_distribution<size_t> distribution(0, charset.size() - 1);

    std::string result;

    for (size_t i = 0; i < length; ++i) {
        result += charset[distribution(generator)];
    }

    return result;
}

std::string sanitize_filename(const std::string& name)
{
    std::string result;

	size_t len = 0;

    for (char c : name) {
		if (len > 8) {
			break;
		}
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
            result += c;
			len++;
        } else if (!result.empty() && c == ' ') {
			continue;
        }
    }

    return result;
}

void trim(std::string& str)
{
	if (str.empty())
		return;
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        str.clear(); // All whitespace
        return;
    }

    str.erase(0, start);
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    str.erase(end + 1);
}

std::stringstream printMap(const std::map<std::string, std::pair<int, double> >& m)
{
    std::stringstream ss;
	std::pair<int, double> p;
    for (const auto& it : m) {
		p = it.second;
        ss << "    - " << std::setw(12) << it.first + " : " << std::setw(2) << p.second
           << std::endl;
    }
    return ss;
}

