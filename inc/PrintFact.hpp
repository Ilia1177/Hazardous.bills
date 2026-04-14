#ifndef PRINTFACT_HPP
# define PRINTFACT_HPP
# include "Facture.hpp"
# include <ctime>
# include <fstream>
# include <iostream>
# include <map>
# include <sstream>
# include "Font.hpp"
# include <sqlite3.h>

# define BILL_NUMBER_LENGTH 10
# define FACTURE_DIR "factures/"
# define DEVIS_DIR "devis/"
# define TICKET_DIR "tickets/"
# define VERSION "v1.4"
# define TEST_PRINT 5
# define MAX_INPUT_LENGTH 16
# define MAX_EMAIL_LENGTH 48
# define MAX_TITLE_LENGTH 31
# define BILL_NUMBER_LENGTH 10
# define RISOPRINT_PRICE 20.0

const std::string HELP = R"(Available options:
--print=job		: print job example
--print=all		: print all documents
--print=devis	: print all devis
--print=facture	: print all factures
--print=<id>	: print document id<int64>
--view=complete	: print in complete mode
--view=preview	: print in preview mode
--version		: print the program version
--verbose		: debug output
--help			: output this message
)";

const std::string EMAIL_BODY = R"(Bonjour,

Veuillez trouver votre document en piece jointe.

Cordialement,
Hazardous Press
)";

extern bool g_interrupt;

const double EPSILON = std::numeric_limits<double>::epsilon();

class Database;
class Risography;
class Facture;
class Facturier
{
  public:
    ~Facturier();
    Facturier(void);

	void		verbose(const std::string&);
	void		run(int ac, char *av[]);
    bool        main_menu();
    void 		make_new_facture();
	std::string generate_header();
	std::string generate_infos();
	std::string generate_footer();

	std::string get_config(const std::string& str);
	Font		get_font();
	std::string get_conditions_of_sales();

    void read_section_from(const std::string&, std::map<std::string, double>&, std::ifstream &file);
    void read_section_from(const std::string&, std::map<std::string, std::pair<int, double> >&, std::ifstream &file);
    void read_section_from(const std::string&, std::map<double, double>&, std::ifstream &file);

	const std::map<std::string, double>& 					getConsumablePrices() const;
    const std::map<std::string, std::pair<int, double> >&	getPaperPrices() const;
	const std::map<double, double>& 						getShippingPrices()const;


  private:
    Facturier(const std::string& estimateNum);
    Facturier(const Facturier& other);
    Facturier& operator=(const Facturier& other);

	void _initialize();
	bool _verb;
	int _margin;
	int _width;
	int _contentWidth;

    std::map<std::string, double> 					_consumablePrices;
    std::map<std::string, std::pair<int, double> >	_paperPrices;
	std::map<double, double>						_shippingCosts;
	std::map<std::string, std::string>				_config;

	std::string		_header;
	std::string		_conditions_of_sales;
	Font			_font;

	Facture* 		_facture;
    Database* _db;
};

#endif
