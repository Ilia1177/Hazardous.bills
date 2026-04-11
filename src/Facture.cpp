#include "Facture.hpp"
#include <cmath>
#include <chrono>
#include <sstream>
#include "PrintFact.hpp"
// Default constructor
Facture::Facture(Facturier* f):
	total(0), client_name(""), client_email(""), type(Facture::TYPE::DEVIS), facturier(f)
{
	if (!f) {
		throw std::runtime_error("facturier not ready");
	}
    // auto now = std::chrono::system_clock::now();
    // auto now_time_t = std::chrono::system_clock::to_time_t(now);
    get_formated_date(true);
	number = generate_number(type);
}

// Destructor
Facture::~Facture(void) {
	for (size_t i = 0; i < risoprints.size(); i++) {
		delete risoprints[i];
	}
}

std::string Facture::get_formated_date(bool init)
{
    std::stringstream current_date;
	if (init) {
		auto now = std::chrono::system_clock::now();
		auto now_time_t = std::chrono::system_clock::to_time_t(now);
		date = *std::localtime(&now_time_t);
	}
    current_date << fit(date.tm_mday, 2, 2, '0')
				<< "/" << fit(date.tm_mon + 1, 2, 2, '0')
				<< "/" << date.tm_year + 1900;
	return current_date.str();
}

std::string Facture::get_facture_stream() {
	std::stringstream ss;
	ss << header_strm;
	ss << infos_strm;
	ss << jobs_strm;
	ss << custom_strm;
	ss << footer_strm;
	return ss.str();
}

std::string Facture::get_status() const {
	switch (status) {
		case Facture::STATUS::DRAFT: return "draft";
		case Facture::STATUS::SENT: return "sent";
		case Facture::STATUS::APPROVED: return "approved";
		case Facture::STATUS::CONVERTED: return "converted";
	}
}

std::string Facture::get_type() const {
	switch(type) {
		case Facture::TYPE::DEVIS: return "devis";
		case Facture::TYPE::FACTURE: return "facture";
		case Facture::TYPE::TICKET: return "ticket";
		default: return "";
	}
}

std::string Facture::generate_number(Facture::TYPE type) {
	std::stringstream ss;
	int inc = 0;

	do {
		ss.str("");
		ss.clear();
		ss << (date.tm_year + 1900) % 100 
			<< fit(date.tm_mon + 1, 2, 2, '0')
			<< fit(date.tm_mday, 2, 2, '0') << "-"
			<< fit(to_hex(date.tm_hour + date.tm_min + inc), 3, 0, '0');
		inc++;
	} while (!get_bill_filename(ss.str()).empty());

	switch (type) {
		case Facture::TYPE::FACTURE:
			ss << "F";
			break;
		case Facture::TYPE::DEVIS:
			ss << "E";
			break;
		case Facture::TYPE::TICKET:
			ss << "T";
			break;
		default:
		  ss << "U";
	}
	return ss.str();
}

void Facture::make_new_custom() {
	std::string object = userline("objet");
	double price = 0.0;
	user_value("price", price);
	auto [it, inserted] = customs.emplace(object, price);
	if (!inserted) {
		it->second += price;
	}
	this->total += price;
}

Risography* Facture::add_riso_print() {

	facturier->verbose("Add riso print->");

	Risography* job = new Risography(facturier);

	job->description = userline("Description");
	user_value("Nombre de feuilles A3 par ouvrage", job->format);
	user_value("Nombre de copies de l'ouvrage", job->copy);
	
	std::string answer = userline("Faconnage (y/n)", false);
	if (answer == "y" || answer == "Y") {
		user_value("Agraphe par ouvrage", job->staple_quantity);
		user_value("Coupe ou plis par ouvrage", job->fold_per_unit);
	}

	job->unit_cost = static_cast<double>(job->copy) * job->format;

	user_value("Layers A", job->layersA);
	user_value("Layers B", job->layersB, false);
	while (!g_interrupt) {
    	std::cout << fit(" ", 4) + "Selection papier depuis:\n" + printMap(facturier->getPaperPrices()).str();
		job->paper = userline("Papier (papier.gsm)");
		if (job->paper == "none") {
			job->sheet_weight = facturier->getPaperPrices().find("evercopy.90")->second.first;
			job->sheet_price = facturier->getPaperPrices().find("evercopy.90")->second.second;
			job->paper = "";
			break;
		}
        if (facturier->getPaperPrices().find(job->paper) == facturier->getPaperPrices().end()) {
            std::cerr << "Error: No such paper.\n";
        } else {
			std::pair<int, double> paperDetails = facturier->getPaperPrices().find(job->paper)->second;
			job->sheet_weight = paperDetails.first;
            job->sheet_price =  paperDetails.second;
			break;
        }
	}
	user_value("Création graphique", job->graphic_amount, false);
	user_value("Imposition", job->imposition_amount, false);
	if (userline("Shipping (y/n)", false) == "y") {
		job->shipping_fees = std::numeric_limits<double>::infinity(); // Trigger shipping calculation
	}
	user_value("Réduction (%)", job->discount_percentage, false);
	job->calc_quantity();
	this->total += job->rest_to_pay();
	risoprints.push_back(job);
	return job;
}
