#ifndef TOOLS_H
# define TOOLS_H
# include <iostream>
# include <map>
# include <sstream>
# include "Font.hpp"

bool send_email_smtp(const std::string& to,
                     const std::string& subject,
                     const std::string& body,
                     const std::string& attachment_path = "",
                     const std::string& from     = "atelier@example.com",
                     const std::string& smtp_url = "smtps://smtp.gmail.com:465",
                     const std::string& user     = "atelier@gmail.com",
                     const std::string& password = "your_app_password");

void make_png(std::string fileStream, Font font, std::string_view  outputPath);
std::string random_string(size_t length, const std::vector<std::string>& charset);
bool user_value(const std::string& str, int& value, bool blocking = true);
bool user_value(const std::string& str, double& value, bool blocking = true);
size_t dial_menu(const std::vector<std::string>& menu);
std::string userline(std::string label, bool bloacking = true);
void eraseLines(int lines);
void clearLine();
bool user_value(const std::string& str, double& value, bool blocking);
void clearScreen();
void clearInputLine();

std::string end_str(int size = 22, int offset = 0, bool reset = false);
size_t dial_menu(const std::vector<std::string>& menu);
std::string userline(std::string label, bool blocking);
void trim(std::string& str);
std::stringstream printMap(const std::map<std::string, std::pair<int, double> >& m);
std::map<std::string, std::string> load_config(const std::string& path);
template <typename T> 
std::string fit(T value, int width, int precision = 2, char fillChar = 32) {
	std::ostringstream oss;
    oss << std::setfill(fillChar);
	oss << std::setw(width);
	oss << std::fixed;
	oss << std::setprecision(precision);
	oss << value;
    return oss.str();
};

template <typename T1, typename T2>
std::string new_line( const std::string& start, T1 value1, T2 value2, std::string end = "")
{
	if (end.empty())
		end = end_str();
	// static int fieldNb;
	int width1 = 19;
	int width2 = 10;
	int width3 = 8;
	int width4 = 22;

	std::string        sep = "|";
	std::ostringstream oss;
	oss << fit(sep, 4 + sep.length()) << std::fixed << std::setprecision(2);
	oss << start << fit(sep, width1 - start.length() + sep.length());
	oss << std::setw(width2) << value1 << sep;
	oss << std::setw(width3) << value2 << sep;
	oss << std::setw(width4) << end;
	return oss.str();
};
#endif
