#ifndef FACTURE_HPP
# define FACTURE_HPP
# include <iostream>
# include "Risography.hpp"

#include <map>
#include <ctime>

#define RED "\033[31m"
#define GREEN "\033[32m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

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
		void make_new_custom();
		std::string get_facture_stream();
		std::string get_formated_date(bool init = false);
		std::string get_type() const;
		std::string get_status() const;
		
		enum class STATUS {
			DRAFT,
			SENT,
			APPROVED,
			CONVERTED
		};
		enum class TYPE {
			FACTURE,
			DEVIS,
			TICKET,
			BROUILLON
		};

		double total;
		
		std::string client_name;
		std::string client_email;
		std::string client_addr;
		std::string client_tel;
		std::string number;
		tm			date;
		TYPE		type;
		STATUS		status;
		std::vector<Risography*> risoprints;
		
		Facturier* facturier;

		std::string header_strm;
		std::string infos_strm;
		std::string jobs_strm;
		std::string footer_strm;
		std::string custom_strm;

		std::map<std::string, double> customs;
	private:
		
		std::string generate_number(Facture::TYPE type);
		
};

#endif

