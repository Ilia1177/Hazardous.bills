#ifndef PRINTFACT_HPP
#define PRINTFACT_HPP
// #include "ThermalPrinter.hpp"
#include "Facture.hpp"
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
// #include <limits>

#include "Font.hpp"
#include <sqlite3.h>
// #include <filesystem>
# define BILL_NUMBER_LENGTH 10

#define INVOICE_DIR "factures/"
#define ESTIMATE_DIR "devis/"
#define TICKET_DIR "tickets/"
#define VERSION "v1.2"
#define TEST_PRINT 5
#define MAX_INPUT_LENGTH 16
#define MAX_EMAIL_LENGTH 48
#define MAX_TITLE_LENGTH 31
#define BILL_NUMBER_LENGTH 10
#define RISOPRINT_PRICE 20.0

const std::string HELP = R"(Available options:
--print=job : print job example
--version   : print the program version
--verbose   : debug output
--help      : output this message
)";

extern bool g_interrupt;

const double EPSILON = std::numeric_limits<double>::epsilon();
std::string  get_bill_filename(const std::string& pattern);
void         stream_find_replace(std::stringstream& ss, const std::string& search,
                                 const std::string& replace);

std::string end_str(int size = 22, int offset = 0, bool reset = false);
std::string userline(std::string label, bool bloacking = true);
size_t dial_menu(const std::vector<std::string>& menu);
void clearScreen();
void clearInputLine();
void clearLine();
void eraseLines(int line);
void moveCursorUp(int lines);
bool user_input(const std::string& str, std::string& input, bool blocking, size_t maxLength = MAX_INPUT_LENGTH);
bool user_value(const std::string& str, int& value, bool blocking = true);
bool user_value(const std::string& str, double& value, bool blocking = true);

std::string		  getUserLine(const std::string& str);
std::istream&     get_trimmed_line(const std::string& str, std::istream& fd, std::string& line);
std::string       sanitize_filename(const std::string& name);
std::string       random_string(size_t length, const std::vector<std::string>& charset);
std::stringstream printMap(const std::map<std::string, std::pair<int, double>>& m);
void              trim(std::string& str);
std::string       to_hex(int value);
template <typename T> std::string fit(T value, int width, int precision = 2, char fillChar = 32) { std::ostringstream oss;
    oss << std::setfill(fillChar) << std::setw(width) << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}
    	template <typename T1, typename T2>
		std::string new_line(	const std::string& start, 
									T1 value1, 
									T2 value2,
									 std::string end = "")
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
		}

void txtToPng(const std::string& txtFile, const std::string& pngFile);

class Database;
class Risography;
class Facture;
class Facturier
{
  public:
    ~Facturier();
    Facturier(void);

	void run(int ac, char *av[]);


    bool        main_menu();
	// std::string end_str(int size, int offset, bool reset = false);
    void make_new_facture();
	std::string generate_header();
	std::string generate_infos();
	std::string generate_job(Risography* job);
	std::string generate_footer();
	std::string generate_custom();

	void save_as(std::string& billNumber, std::string& clientName);
    void read_section_from(const std::string&, std::map<std::string, double>&);
    void read_section_from(const std::string&, std::map<std::string, std::pair<int, double> >&);
    void read_section_from(const std::string&, std::map<double, double>&);
    void        make_bill_from();
    void        new_ticket();
    void        to_png(std::string& folder, std::string& txtName);
    void        save_as(std::string& clientName);
	const std::map<std::string, double>& getConsumablePrices() const {return _consumablePrices;};
    const std::map<std::string, std::pair<int, double> >& getPaperPrices() const  {return _paperPrices;};
	const std::map<double, double>& getShippingPrices()const {return _shippingCosts;};


    template <typename T1, typename T2>
    std::string new_job_line(const std::string& start, T1 value1, T2 value2,
                             std::string end = "")
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
		// _outStream << oss.str();
		verbose("\t\t" + oss.str());
        return oss.str();
    }

	void verbose(const std::string&);

	void save_facture_as(Facture* facture);
  private:
    Facturier(const std::string& estimateNum);
    Facturier(const Facturier& other);
    Facturier& operator=(const Facturier& other);

	void _initialize();
	bool _verb;
	int _margin;
	int _width;
	int _contentWidth;

    std::map<std::string, double> _consumablePrices;
    std::map<std::string, std::pair<int, double> > _paperPrices;
	std::map<double, double>	_shippingCosts;

    // ThermalPrinter _printer;

	Facture* _facture;
	std::string		  _header;
	std::string		  _conditions_of_sales;
	// std::string		  _headerTicket;
	// std::string		  _footerTicket;
    std::ifstream     _prices;

    Database* _db;
    char*    _err_db_msg;

	Font _font;
	// std::ostream	_fileName;
};

#endif
