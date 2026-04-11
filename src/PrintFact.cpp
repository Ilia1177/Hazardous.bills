#include "PrintFact.hpp"
#include "hpdf.h"
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

#include "Database.hpp"
namespace fs = std::filesystem;

// Destructor
Facturier::~Facturier(void) {
    if (_prices && _prices.is_open()) {
        _prices.close();
    }
	if (_facture) {
		delete _facture;
	}
    if (_db) {
		delete _db;
	}
}

// Default constructor
Facturier::Facturier(): 
	_verb(false), 
	_margin(0), 
	_width(0), 
	_contentWidth(0), 
	_facture(nullptr),
	_err_db_msg(nullptr)
{
	_initialize();
	// _outStream.str("");
	// _outStream.clear();
    if (!_prices || !_prices.is_open())
        throw std::runtime_error("Cannot open database");
    read_section_from("consumable", _consumablePrices);
    read_section_from("paper", _paperPrices);
    read_section_from("shipping", _shippingCosts);


	std::stringstream buffer;
	std::ifstream headerFile("ascii/header-printjob.txt");
	if (headerFile.is_open()) {
		std::string firstLine;
		std::getline(headerFile, firstLine);
		_width = firstLine.length();
		_margin = 4;
		_contentWidth = _width - _margin * 2;
		buffer << headerFile.rdbuf();
		_header = buffer.str();
		buffer.str("");
		buffer.clear();
		headerFile.close();              // clear EOF + error flags
	} else {
		_header = "-- no header --";
	};

	std::ifstream footerFile("ascii/CGV.txt");
	if (footerFile.is_open()){
		buffer << footerFile.rdbuf();
		_conditions_of_sales = buffer.str();
		buffer.str("");
		buffer.clear();
		footerFile.close(); 
	} else {
		_conditions_of_sales = "-- no footer --";
	}

	_db = new Database("data/facture.db");
	verbose("Table created successfully");

    _font.family  = "Menlo";
    _font.size    = 13.0;
    _font.bold    = false;
    _font.fg_r = 0.10; _font.fg_g = 0.10; _font.fg_b = 0.10;  // near-black text
    _font.bg_r = 0.97; _font.bg_g = 0.97; _font.bg_b = 0.95;  // off-white background
    _font.padding = 24;
}

void Facturier::verbose(const std::string& txt) {
	if (_verb) {
		std::cout << BLUE;
		std::cout << txt;
		std::cout << RESET;
		std::cout << std::endl;
	}
}

bool Facturier::main_menu()
{
	std::vector<std::string> list;

	list = {	"exit",
				"Nouveau devis", 
				"Facture d'après devis", 
				"Nouveau ticket"};

	while (!g_interrupt) {
		clearScreen();
		std::string titlLabel = "!! RISOGRAPH INVOICER by hazardous editorial";
		std::string cmdsLabel = "ctrl-c to go back one menu // ctrl-d to quit";
		std::cout << fit("*", _width, 2, '*') << "\n";
		std::cout << fit(" ", _margin + (_contentWidth - titlLabel.length()) / 2);
		std::cout << titlLabel << "\n";
		std::cout << fit(" ", _margin + (_contentWidth - cmdsLabel.length()) / 2);
		std::cout << cmdsLabel << "\n";
		std::cout << fit("*", _width, 2, '*') << "\n\n";
		std::cout << generate_header();
		switch(dial_menu(list)) {
			case 0: return false;
			case 1: make_new_facture(); break;
			// case 2: make_bill_from(); break;
			// case 3: new_ticket(); break;
			default: return false;
		}
	}
	return true;
}

void Facturier::make_bill_from()
{
	// std::string bill_num;
	//
	// while(!g_interrupt) {
	// 	user_input("Numeros de devis: ", bill_num, true);
	// 	if (bill_num.length() != BILL_NUMBER_LENGTH + 1) {
	// 		clearInputLine();
	// 		std::cerr << "Invalid number. Should be formatted as 'XXXXXX-XXXX'. ";
	// 	} else if (get_bill_filename(bill_num).empty()) {
	// 		clearInputLine();
	// 		std::cerr << "No devis at number " << bill_num << ". ";
	// 	} else {
	// 		std::ifstream estimate;
	// 		std::string fileName = get_bill_filename(bill_num);
	// 		size_t pos = fileName.find(bill_num);
	// 		size_t dot = fileName.find(".");
	// 		if (pos + 11 == std::string::npos || dot == std::string::npos)
	// 			throw std::invalid_argument("Invalid file name.");
	// 		std::string clientName = fileName.substr(pos + 11);
	// 		std::string fullPath = "devis/" + fileName;
	// 		estimate.open(fullPath);
	// 		if (!estimate || !estimate.is_open()) {
	// 			throw std::invalid_argument(fileName + " is an invoice allready.");
	// 		}
	// 		std::string line;
	// 		// std::string newBillNumber = bill_number(INVOICE);
	// 		while(getline(estimate, line)) {
	// 			size_t found = line.find(bill_num);
	// 			if (found != std::string::npos) {
	// 				line = "    Facture n°" + newBillNumber + " du:" + formated_date() + " d'après devis n°" + bill_num + "E";
	// 			} 
	// 			_outStream << line + "\n";
	// 		}
	// 		line.clear();
	// 		std::cout << _outStream.str();
	// 		user_input("Enregistrer la facture ? (y/n): ", line, false);
	// 		if (line == "y" || line == "Y") {
	// 			std::string folder = "factures/";
	// 			std::string outfileName = newBillNumber + clientName;
	// 			std::ofstream outfile(folder + outfileName);
	// 			if (!outfile.is_open()) 
	// 				throw std::runtime_error("Could not open file for writing.");
	// 			outfile << _outStream.str();
	// 			outfile.close();
	// 			std::cout << outfileName << " has been register.\n";
	// 			outfileName.resize(outfileName.size() < 4 ? 0 : outfileName.size() - 4);
	// 			to_png(folder, outfileName);
	// 			user_input("Press any key to continue", line, false, 0);
	// 		}
	// 		return;
	// 	}
	// }
}

void Facturier::run(int ac, char *av[])
{
	std::vector<std::string> args;

	bool print_job = false;
	for(int i = 1; i < ac; i++) {
		std::string arg(av[i]);
		if (arg == "--verbose") {
			_verb = true;
		} else if (arg.starts_with("--print=")) {
			std::string opt = arg.substr(8);
			if (opt=="test") {
				print_job = true;
			} else if (opt=="all") {
				_db->print_all_documents(Database::PREVIEW);
				return;
			} else {
				throw std::invalid_argument(arg);
			}
		} else if (arg == "--version") {
			std::cout << VERSION << std::endl;
			return;
		} else if (arg == "--help") {
			std::cout << HELP << std::endl;
			return;
		} else {
			throw std::invalid_argument(arg);
		}
	}

	if (print_job) {
		_facture = new Facture(this);
		_facture->header_strm = generate_header();
		_facture->infos_strm = generate_infos();
		_facture->jobs_strm = generate_job(new Risography(this));
		_facture->customs["Objet test"] = 100;
		_facture->custom_strm = generate_custom();
		_facture->footer_strm = generate_footer();
		std::cout << _facture->get_facture_stream();
	} else {
		main_menu();
	}
	return;
}



void Facturier::_initialize() 
{
	if (!fs::exists("factures")) {
		fs::create_directory("factures");
		fs::create_directory("factures/png");
		std::cout << "Factures & png directory created.\n";
	} else if (!fs::exists("factures/png")) {
		fs::create_directory("factures/png");
		std::cout << "factures/png Directory created.\n";
	}
	if (!fs::exists("devis")) {
		fs::create_directory("devis");
		fs::create_directory("devis/png");
		std::cout << "devis & png directory created.\n";
	} else if (!fs::exists("devis/png")) {
		fs::create_directory("devis/png");
		std::cout << "devis/png created\n";
	}
	if (!fs::exists("tickets")) {
		fs::create_directory("tickets");
		fs::create_directory("tickets/png");
		std::cout << "tickets/png directory created.\n";
	} else if (!fs::exists("tickets/png")) {
		fs::create_directory("tickets/png");
		std::cout << "tikets/png\n";
	}
    _consumablePrices.clear();
    _paperPrices.clear();
	_shippingCosts.clear();
    _prices.open("prices.ini");
}



std::string Facturier::generate_header()
{
	std::stringstream ss;
	verbose("Register header");
	ss << _header << std::endl;;
    ss << "    Papier (nom.g/m2):" << fit("Consomable:\n", 36);

    typedef std::map<std::string, std::pair<int, double> >::iterator PaperIt;
    typedef std::map<std::string, double>::iterator ConsumableIt;

	int tabWidth = 20;
	int inBetweenMargin = 22;
    PaperIt paper = _paperPrices.begin();
    ConsumableIt consumable = _consumablePrices.begin();
    while (paper != _paperPrices.end() || consumable != _consumablePrices.end()) {
        if (paper != _paperPrices.end()) {
            ss << "    - " << paper->first + fit(paper->second.second, tabWidth - paper->first.length(), 2, '.');
            paper++;
        } else {
			inBetweenMargin = 48;
		}
        if (consumable != _consumablePrices.end()) {
            ss << fit("- ", inBetweenMargin) << consumable->first + fit(consumable->second, tabWidth - consumable->first.length(), 2, '.') + "\n";
            consumable++;
        } else {
            ss << "\n";
        }
    }
    ss << "\n";
	return ss.str();

}

std::string Facturier::generate_infos()
{
	verbose("Register infos");
	std::stringstream ss;
	std::string address;
	// if (_facture) {
	address = _facture->client_email.empty() ? "" : " (" + _facture->client_email + ")";
	// }
	ss << "    ****************************************************************\n";
	ss << "    Devis n°" + _facture->number + " du: " + _facture->get_formated_date() + "\n";
	ss << "    à: " << _facture->client_name << address << "\n";
	ss << "    ****************************************************************\n\n";
	return ss.str();
}

std::string to_hex(int value) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << value;
    return oss.str();
}

// void Facturier::save_as(std::string& clientName)
// {
//     std::string       input;
//
// 	clearScreen();
// 	std::cout << _outStream.str();
// 	user_input("Save the bill > (y/n): ", input, false);
//     if (input != "Y" && input != "y") {
// 		user_input("Quit without saving. Press any key to continue.", input, false, 0);
// 		return;
// 	}
// 	std::ofstream outfile;
// 	std::string folderName = "devis/";
// 	std::string truncName = sanitize_filename(clientName);
// 	user_input("As an invoice ? (y/n): ", input, true);
// 	if (input == "y" || input == "Y")
// 	{
// 		folderName = "factures/";
// 		if (!_facture) {
// 			// _facture = true;
// 			std::string line;
// 			std::stringstream ss(_outStream.str());
// 			_outStream.str("");
// 			_outStream.clear();
// 			while(getline(ss, line)) {
// 				size_t found = line.find(_facture->number);
// 				if (found != std::string::npos) {
// 					_facture->number.replace(10, 1, "F");
// 					line = "    Facture n°" + _facture->number + " du: " + _facture->get_formated_date(true);
// 				}
// 				_outStream << line + "\n";
// 			}
// 		}
// 		std::string fileName = _facture->number + "_" + truncName;
// 		outfile.open("factures/" + fileName + ".txt");
// 	} else {
// 		// _fileName = _billNumber + "_" + truncName;
// 		// outfile.open("devis/" + _fileName + ".txt");
// 	}
// 	// _fileName = _billNumber + "_" + truncName;
// 	// outfile.open(folderName + _fileName + ".txt");
// 	if (!outfile || !outfile.is_open())
// 		throw std::runtime_error("Could not open file for writing.");
// 	outfile << _outStream.str();
// 	outfile.close();
// 	// std::cout << _fileName << " has been register.\n";
// 	// to_png(folderName, _fileName);
// 	// user_input("Press enter to continue", input, false);
// }
//
void Facturier::to_png(std::string& folder, std::string& fileName) {
	std::string txtPath = folder + fileName + ".txt";
	std::string pngPath = folder + "png/" + fileName + ".png";
	txtToPng(txtPath, pngPath);
	std::cout << pngPath << " has been saved as PNG\n";
}

void Facturier::make_new_facture()
{
	_facture = new Facture(this);

	_facture->header_strm = generate_header();

	_facture->client_name = userline("client name");
	_facture->client_email = userline("client address", false);

	_facture->infos_strm = generate_infos();
	clearScreen();

	std::cout << _facture->header_strm;
	std::cout << _facture->infos_strm;

	std::string newJob("");
	std::vector<std::string> menu;

	menu = {	"Exit & save",
				"new job",
				"custom line"};

	bool done = false;
	while(!g_interrupt && !done) {
		switch(dial_menu(menu)) {
			case 0: done = true; break;
			case 1: 
				newJob = generate_job(_facture->add_riso_print());
				_facture->jobs_strm += newJob;
				break;
			case 2: 
				_facture->make_new_custom();
			default: break;
		}
		clearScreen();
		std::cout << _facture->get_facture_stream();
	}

	_facture->custom_strm = generate_custom();
	_facture->footer_strm = generate_footer();
	std::cout << _facture->get_facture_stream();

	switch (dial_menu({	"Exit without saving", 
						"invoice", 
						"estimate",
						"draft"})) {
		case 0: break;
		case 1: save_facture_as(_facture);
		case 2: save_facture_as(_facture);
		default: break;
	}

	clearScreen();
	_facture->get_facture_stream();
}

std::string Facturier::generate_footer()
{
	std::stringstream ss;
	verbose("Register footer");
    ss << "    ****************************************************************\n";
    ss << "    Total des Sômmes dues:" + fit(_facture->total, 41) + "€\n";
    ss << "    ****************************************************************\n";
	ss << _conditions_of_sales;
	return ss.str();
}

std::string Facturier::generate_custom() {
	std::stringstream ss;
	
	if (_facture->customs.size() > 0) {
		ss << "    |                   objet                    |      prix       |\n";
		ss << "    |============================================|=================|\n";
	}
	for (const auto& [key, value] : _facture->customs) {
		ss << fit("|", 5) << key << fit("|", 45 - key.length()) << fit(value, 16) + "€|\n"; 
		ss << "    |--------------------------------------------|-----------------|\n";
	}
	ss << "\n";
	return ss.str();
}

std::string Facturier::generate_job(Risography* job)
{
	if (!job) {
		verbose("job is null...");
		return "";
	}
	verbose("generate print job");

	std::stringstream ss;
	if (_facture->risoprints.size() > 1)
		ss << "    ****************************************************************\n\n";
    std::string colorVerso =
        (job->layersB > 0) ? " - " + std::to_string(job->layersB) + "/verso" : "";
    std::string colorStr = std::to_string(job->layersA) + "/recto" + colorVerso;
    std::string copyStr = std::to_string(job->copy);
    int         offset = 68 - 14 - 22 - 1;
    ss << "    Intitulé: " << job->description << end_str(22, offset - job->description.length());
    ss << "    Couleurs: " << colorStr << end_str(22, offset - colorStr.length());
    ss << "    Copies  : " << copyStr << end_str(22, offset - copyStr.length());
    ss << "    Papier  : " << job->paper << end_str(22, offset - job->paper.length());

	verbose("generate print job 1");
    std::string feesStr = (job->total_shipping() > 0.0) ? fit(job->total_shipping(), 8) : fit("--", 8);
    std::string totalDiscountStr =
        (job->total_discount() > 0.0) ? fit(job->total_discount() * -1, 8) : "      --";
	verbose("generate print job 2");
    std::string discountStr = (job->discount_percentage > 0.0) ? fit(job->discount_percentage, 12) : "--";

	verbose("generate print job 3");
    ss << "    |---------------------------------------|" + end_str();
    ss << "    |    désignation    | quantité |  prix  |" + end_str();
    ss << "    |===================|==========|========|" + end_str();
	ss << new_job_line("feuilles A3", job->sheet_quantity, job->total_paper());
	ss << new_job_line("masters", job->masters_quantity, job->total_master());
	ss << new_job_line("encre (passages)", job->sheet_quantity * job->masters_quantity, job->total_ink());
	ss << new_job_line("plis/coupe/agrafe", job->staple_quantity + job->fold_per_unit, job->total_shaping());
    ss << "    |---------------------------------------|" << end_str();
	ss << new_job_line("impression", job->labor_amount, job->total_labor());
	ss << new_job_line("graphisme", job->graphic_amount, job->total_graphic());
	ss << new_job_line("imposition", job->imposition_amount, job->total_imposition(), end_str());
    ss << "    |---------------------------------------|" + end_str();
	ss << new_job_line("remise (%)", discountStr, totalDiscountStr);
	ss << new_job_line("port (kg)", job->weight, feesStr);
    ss << "    |=======================================|" + end_str();
    ss << "    |sous total (ttc)" + fit(job->total, 22) + "€" + "|" + end_str();
    ss << "    |=======================================|" + end_str();
    ss << "    |" + fit(job->unit_cost, 32) + "€/copie|" + end_str() + "\n";

	verbose("finish generating job");
	return ss.str();
}

void Facturier::read_section_from(const std::string& section, std::map<std::string, std::pair<int, double> >& list)
{
    std::string line, paperName, gram;
	int gramValue;
	double price;
	std::pair<int, double> infos;
    std::string start = "[" + section + "]";

    while (std::getline(_prices, line)) {
        if (line == start)
            break;
    }
    while (std::getline(_prices, line)) {
		trim(line);
		if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[') break;
        std::size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos)
			throw std::invalid_argument("Bad input: missing '='");
		try {
			paperName = line.substr(0, delimiterPos);
			price = std::stod(line.substr(delimiterPos + 1));
			delimiterPos = paperName.find('.');
			if (delimiterPos == std::string::npos) 
				throw std::invalid_argument("Bad input: missing '.'");
			gramValue = std::stoi(paperName.substr(delimiterPos + 1));
			infos = std::make_pair(gramValue, price);
			list[paperName] = infos;
		} catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << "\n";
		}
    }
    _prices.clear();                 // Clear any error flags
    _prices.seekg(0, std::ios::beg); // Move the get pointer to the beginning of the file
}

void Facturier::save_facture_as(Facture* facture)
{
    std::string       input;

	if (!facture)
		return;

	clearScreen();
	Facture::TYPE type = facture->type;

	std::string fileName;
	std::ofstream outfile;
	std::string folderName = "devis/";
	std::string truncName = sanitize_filename(facture->client_name);
	if (type == Facture::TYPE::FACTURE)
	{
		folderName = "factures/";
		facture->type = Facture::TYPE::FACTURE;
			std::string line;
			std::stringstream ss(facture->infos_strm);
			facture->infos_strm.clear();
			while(getline(ss, line)) {
				size_t found = line.find(_facture->number);
				if (found != std::string::npos) {
					_facture->number.replace(10, 1, "F");
					line = "    Facture n°" + _facture->number + " du: " + _facture->get_formated_date(true);
				}
				facture->infos_strm += line + "\n";
			}
		outfile.open("factures/" + fileName + ".txt");
	} else {
		facture->type = Facture::TYPE::DEVIS;
	}
	_db->save_facture(facture);
	fileName = _facture->number + "_" + truncName;
	// outfile.open(folderName + fileName + ".txt");
	// if (!outfile || !outfile.is_open())
	// 	throw std::runtime_error("Could not open file for writing.");
	// outfile << facture->header_strm;
	// outfile << facture->infos_strm;
	// outfile << facture->jobs_strm;
	// outfile << facture->footer_strm;
	// outfile.close();

	// std::cout << fileName << ".txt has been register.\n";
	make_png(facture->get_facture_stream(), _font, fileName + ".png");
	// to_png(folderName, fileName);
	std::cout << fileName << ".png has been register.\n";
	userline("Press enter to continue", false);
}

void Facturier::read_section_from(const std::string& section, std::map<double, double>& list)
{
    std::string line;
    std::string start = "[" + section + "]";

    while (std::getline(_prices, line)) {
        if (line == start)
            break;
    }
    while (std::getline(_prices, line)) {
		trim(line);
		if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[') break;
        std::size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string keyStr = line.substr(0, delimiterPos);
            std::string valueStr = line.substr(delimiterPos + 1);
            double      value;
            double      key;
            try {
                key = std::stod(keyStr);
                value = std::stod(valueStr);
                list[key] = value;
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid number format for value: " << valueStr << std::endl;
            } catch (const std::out_of_range& e) {
                std::cerr << "Value out of range: " << valueStr << std::endl;
            }
        } else {
            std::cerr << "Invalid line format: " << line << std::endl;
        }
    }
    _prices.clear();                 // Clear any error flags
    _prices.seekg(0, std::ios::beg); // Move the get pointer to the beginning of the file
}

void Facturier::read_section_from(const std::string& section, std::map<std::string, double>& list)
{
    std::string line;
    std::string start = "[" + section + "]";

    while (std::getline(_prices, line)) {
        if (line == start)
            break;
    }
    while (std::getline(_prices, line)) {
		trim(line);
		if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[') break;
        std::size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string valueStr = line.substr(delimiterPos + 1);
            double      value;
            try {
                value = std::stod(valueStr);
                list[key] = value;
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid number format for value: " << valueStr << std::endl;
            } catch (const std::out_of_range& e) {
                std::cerr << "Value out of range: " << valueStr << std::endl;
            }
        } else {
            std::cerr << "Invalid line format: " << line << std::endl;
        }
    }
    _prices.clear();                 // Clear any error flags
    _prices.seekg(0, std::ios::beg); // Move the get pointer to the beginning of the file
}



