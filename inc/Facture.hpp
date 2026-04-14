#ifndef FACTURE_HPP
# define FACTURE_HPP
# include <iostream>
# include "Risography.hpp"
# include "Client.hpp"
#include <map>
#include <ctime>

#define RED "\033[31m"
#define GREEN "\033[32m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

typedef std::pair<std::string, std::pair<int, double>> CustomLine;
class Facturier;
class Risography;
class Facture
{
    public:
        ~Facture();
        Facture(Facturier* facturier);
        // Facture(const Facture& other) = default;
        // Facture &operator=(const Facture &other) = default;

		Risography* add_riso_print();
		CustomLine add_custom_line();

		std::string generate_custom();
		std::string generate_riso();
		std::string generate_infos();
		std::string generate_footer();
		std::string generate_stream();
		std::string get_formated_date(bool init = false);
		std::string get_type() const;
		std::string get_status() const;
		std::string get_filename() const;
		
		double get_total() const;

		// std::string generate_stream();
		enum class STATUS {
			DRAFT,
			SENT,
			APPROVED,
			CONVERTED
		};
		enum class TYPE {
			FACTURE,
			DEVIS,
			TICKET
		};

		Client *client;

		double total;
		
		// std::string client_name;
		// std::string client_email;
		// std::string client_addr;
		// std::string client_tel;
		std::string number;
		std::string date;
		tm			chrono;

		TYPE		type;
		STATUS		status;

		std::vector<Risography*> riso_prints;
		std::vector<CustomLine> customs;
		
		Facturier* facturier;

		// std::string header_strm;
		// std::string infos_strm;
		// std::string jobs_strm;
		// std::string footer_strm;
		// std::string custom_strm;

		// std::map<std::string, std::pair<int, double> > customs;
	private:
		
		std::string generate_number(Facture::TYPE type);
		
};

#endif

