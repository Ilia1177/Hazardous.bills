#ifndef THERMALPRINTER_HPP
# define THERMALPRINTER_HPP
# define MAX_PRINT_WIDTH 48
#include <iostream>

enum AlignDef {
	LEFT,
	RIGHT,
	CENTER
};

class ThermalPrinter
{
  public:
    ~ThermalPrinter();
    ThermalPrinter(void);
    ThermalPrinter(const std::string& devicePath);

	void printPNG(const std::string& filename);
	void writeCommand(const std::vector<unsigned char>& cmd, int timeout_sec = 5);
	bool open_device(const std::string& path);
	bool is_open();
	void close_device();

	bool check_connection();
	void reset();
	bool init(const std::string& path);
	void align(AlignDef);

  private:
    std::string _devicePath;
    int         _fd;

    ThermalPrinter(const ThermalPrinter& other);
    ThermalPrinter& operator=(const ThermalPrinter& other);
};

#endif
