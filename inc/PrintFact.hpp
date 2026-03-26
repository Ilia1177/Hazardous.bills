#ifndef PRINTFACT_HPP
#define PRINTFACT_HPP
#include "ThermalPrinter.hpp"
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
// #include <limits>

// #include <filesystem>
# define BILL_NUMBER_LENGTH 10

#define INVOICE_DIR "factures/"
#define ESTIMATE_DIR "devis/"
#define TICKET_DIR "tickets/"
#define MAX_INPUT_LENGTH 16
#define MAX_EMAIL_LENGTH 48
#define MAX_TITLE_LENGTH 31
#define BILL_NUMBER_LENGTH 10
#define RISOPRINT_PRICE 20.0

extern bool g_interrupt;

enum billType {
	INVOICE,
	ESTIMATE,
	TICKET
};

const double EPSILON = std::numeric_limits<double>::epsilon();
std::string  get_bill_filename(const std::string& pattern);
void         stream_find_replace(std::stringstream& ss, const std::string& search,
                                 const std::string& replace);

size_t dial_menu(std::vector<std::string>& menu);
void clearScreen();
void clearInputLine();
void clearLine();
void eraseLines(int line);
void moveCursorUp(int lines);
bool user_input(const std::string& str, std::string& input, bool blocking, size_t maxLength = MAX_INPUT_LENGTH);
bool user_value(const std::string& str, int& value, bool blocking);
bool user_value(const std::string& str, double& value, bool blocking);

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
void txtToPng(const std::string& txtFile, const std::string& pngFile);

class PrintFact
{
  public:
    ~PrintFact();
    PrintFact(void);
	void cleanup();
	std::string formated_date();
	std::string end_str(int size, int offset, bool reset = false);
	void register_infos();
	void register_job();
    void make_printjob();
    void register_header();
    void register_footer();
	void register_bill();
	void receives(const std::string&);
	void save_as(std::string& billNumber, std::string& clientName);
	std::string bill_number(bool facture);
    bool turn_pdf(std::ifstream&, const std::string&);
    void read_section_from(const std::string&, std::map<std::string, double>&);
    void read_section_from(const std::string&, std::map<std::string, std::pair<int, double> >&);
    void read_section_from(const std::string&, std::map<double, double>&);
    void define_job();
	void define_copy();
	void define_format();
	void define_paper();
	void define_colors(bool faceA);
	void define_title();
    void define_client();
    void define_graphics();
	void define_shipping();
	void define_discount();
    void calc_expend();
	void calc_shipping_fees();
	void to_zero();
    void        make_bill_from();
    bool        take_order();
    void        display_menu();
    void        new_ticket();
    void        to_png(std::string& folder, std::string& txtName);
    void        save_as(std::string& clientName);
    std::string bill_number(enum billType);
    bool        turn_pdf(const std::string&);
    void new_job();

    template <typename T1, typename T2>
    std::string new_job_line(const std::string& start, T1 value1, T2 value2,
                             const std::string& end)
    {
        // static int fieldNb;
        int width1 = 17;
        int width2 = 12;
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

  private:
    PrintFact(const std::string& estimateNum);
    PrintFact(const PrintFact& other);
    PrintFact& operator=(const PrintFact& other);

	void _initialize();
	int _margin;
	int _width;
	int _contentWidth;

    std::map<std::string, double> _consumablePrices;
    std::map<std::string, std::pair<int, double> > _paperPrices;
	std::map<double, double>	_shippingCosts;

    ThermalPrinter _printer;

	std::string		  _headerJob;
	std::string		  _footerJob;
	std::string		  _headerTicket;
	std::string		  _footerTicket;
    std::ifstream     _data;
    std::ifstream     _header;
    std::ifstream     _footer;
    std::stringstream _outStream;

    std::string _fileName;
    std::string _billNumber;
    std::string _clientName;
    std::string _title;
    std::string _clientAddress;
    std::string _paperName;
	std::string	_customln;

	double _totalDiscount;
	double _totalGraphics;
    double _total;
    double _totalLabor;
    double _totalInk;
    double _totalJob;
    double _totalPaper;
	double _totalShaping;
	double _totalCustom;

	double _unitPrice;

	double _discount;
	double _weight;
	double _fees;
    double _format;
    double _sheetPrice;
    double _sheetWeight;
    double _labor;
	double _graphics;
	int    _staples;
	int	   _folds;
    int    _sheetNb;
    int    _totalMasters;
    int    _copy;
    int    _layers;
    int    _colorsFaceA;
    int    _colorsFaceB;
    bool   _facture;
    tm     _date;
};

#endif
