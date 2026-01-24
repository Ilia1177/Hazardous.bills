#include "PrintFact.hpp"
#include "hpdf.h"
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

namespace fs = std::filesystem;


// Destructor
PrintFact::~PrintFact(void) {cleanup();}

// Default constructor
PrintFact::PrintFact()
{
	_initialize();
    if (!_data || !_data.is_open())
        throw std::runtime_error("Cannot open database");
    read_section_from("consumable", _consumablePrices);
    read_section_from("paper", _paperPrices);
    read_section_from("shipping", _shippingCosts);

    _facture = false;
	if (_header.is_open()) {
		std::string firstLine;
		std::getline(_header, firstLine);
		_width = firstLine.length();
		_margin = 4;
		_contentWidth = _width - _margin * 2;
	};
}

void PrintFact::display_menu() {
	std::cout << "    Make your choice ****\n";
	std::cout << "    1. Nouvelle facture.\n";
	std::cout << "    2. Facture d'apres devis.\n";
	std::cout << "    3. Nouveau ticket.\n";
	std::cout << "    4. Exit Hazardous billing process.\n";
}

std::string ticket_entry(std::string des, int quantity, double price) {
	std::stringstream line;

	std::vector<size_t> widths = {16,14,12};
	des = "|" + des;
	if (des.length() > widths[0]) 
		des = des.substr(0, widths[0] - 1) + ".";
	line << des;

	if (quantity > 0)
		line <<  fit("|", widths[0] - des.length() + 1) + fit(quantity, widths[2]) << "|";
	else
		line << fit("|", widths[0] - des.length() + 1) + fit (" ", widths[2]) << "|";

	line << fit(price, widths[2]) + "|\n";
	return "  " + line.str();
}

void PrintFact::new_ticket() {

	std::string line;

	_outStream.str("");
	_outStream.clear();
	std::ifstream header;
	header.open("ascii/header-ticket.txt");
    if (header.is_open()) { 
        std::stringstream buffer;
        buffer << header.rdbuf();
        _outStream << buffer.str();
    }

	user_input("Destinataire: ", _clientName, false);
	user_input("Intitulié: ", _title, false, MAX_TITLE_LENGTH);
	_billNumber = bill_number(TICKET);

	if (!_clientName.empty()) {
		_outStream << "Ticket number: " << _billNumber << "\n";
		if (!_clientName.empty())
			_outStream << "À: " << _clientName << "\n";
		_outStream << "***********************************************\n";
	}

	_outStream << "  |-----------------------------------------|  \n";
	_outStream << "  |  désignation  |  quantité  |  prix (u)  |  \n";
	_outStream << "  |-----------------------------------------|  \n";

	std::string entry;
	int quantity = 0;
	double price = 0.0;

	_total = 0.0;
	do {
		std::cout << "1. RisoPrint\n2. Custom\n3. Finish\n4. Exit & abort ticket\n";
		user_input("Select: ", entry, true);
		if (entry == "1") {
			user_value("Quantité ? : ", quantity, true);
			_outStream << ticket_entry("RisoPrint", quantity, RISOPRINT_PRICE);
			_total += (RISOPRINT_PRICE * quantity);
		} else if (entry == "2") {
			user_input("Objet ? : ", entry, true);
			user_value("Quantité ?:", quantity, true);
			user_value("Prix ?:", price, true);
			_outStream << ticket_entry(entry, quantity, price);
			_total += (price * quantity);
		} else if (entry == "3") {
			break;
		} else if (entry == "exit" || entry == "4") {
			return;
		}
	} while (!g_interrupt);

	_outStream << "  |=========================================|  \n";
	_outStream << ticket_entry("TOTAL TTC", 0, _total);
	_outStream << "  |-----------------------------------------|  \n";

	std::ofstream file;
	std::string fileName = _billNumber;
	file.open("tickets/" + fileName + ".txt");
	if (!file.is_open()) 
		return;
	file << _outStream.str();
	file.close();

	std::cout << _outStream.str();
	if (!user_input("Print tickets ? ", entry, false))
		return;
	if (entry == "y" || entry =="Y") {
		if (!_printer.is_open()) {
			user_input("Open device at: ", entry, false, 56);
			if (!_printer.init(entry))
				std::cerr << "Error init device.\n";
		}
		txtToPng("tickets/" + fileName + ".txt", "tickets/png/" + fileName + ".png");
		std::cout << "Ascii made\n";
		_printer.align(CENTER);
		_printer.printPNG("tickets/png/" + fileName + ".png");
	}
	user_input("Press any key to continue. ", entry, false, 0);
}

bool PrintFact::take_order()
{
	std::vector<std::string> list;

	list = {"Nouveau devis", "Facture d'après devis", "Nouveau ticket", "Exit"};

	while (!g_interrupt) {
		clearScreen();
		to_zero();
		std::cout << _headerJob;
		switch(dial_menu(list)) {
			case 1: make_printjob(); break;
			case 2: make_bill_from(); break;
			case 3: new_ticket(); break;
			case 4:
			case 0:
			default:
				return false;
		}
	}

	return true;
}

void PrintFact::make_bill_from()
{
	std::string bill_num;

	while(!g_interrupt) {
		user_input("Numeros de devis: ", bill_num, true);
		if (bill_num.length() != BILL_NUMBER_LENGTH) {
			clearInputLine();
			std::cerr << "Invalid number. ";
		} else if (get_bill_filename(bill_num).empty()) {
			clearInputLine();
			std::cerr << "No devis at number " << bill_num << ". ";
		} else {
			std::ifstream estimate;
			std::string fileName = get_bill_filename(bill_num);
			size_t pos = fileName.find(bill_num);
			size_t dot = fileName.find(".");
			if (pos + 11 == std::string::npos || dot == std::string::npos)
				throw std::invalid_argument("Invalid file name.");
			std::string clientName = fileName.substr(pos + 11);
			std::string fullPath = "devis/" + fileName;
			estimate.open(fullPath);
			if (!estimate || !estimate.is_open()) {
				throw std::invalid_argument(fileName + " is an invoice allready.");
			}
			std::string line;
			std::string newBillNumber = bill_number(INVOICE);
			while(getline(estimate, line)) {
				size_t found = line.find(bill_num);
				if (found != std::string::npos) {
					line = "    Facture n°" + newBillNumber + " du:" + formated_date() + " d'après devis n°" + bill_num + "E";
				} 
				_outStream << line + "\n";
			}
			line.clear();
			std::cout << _outStream.str();
			user_input("Enregistrer la facture ? (y/n): ", line, false);
			if (line == "y" || line == "Y") {
				std::string folder = "factures/";
				std::string outfileName = newBillNumber + clientName;
				std::ofstream outfile(folder + outfileName);
				if (!outfile.is_open()) 
					throw std::runtime_error("Could not open file for writing.");
				outfile << _outStream.str();
				outfile.close();
				std::cout << outfileName << " has been register.\n";
				outfileName.resize(outfileName.size() < 4 ? 0 : outfileName.size() - 4);
				to_png(folder, outfileName);
				user_input("Press any key to continue", line, false, 0);
			}
			return;
		}
	}
}

void PrintFact::receives(const std::string& arg)
{
	if (arg == "-d") {
		std::string testName = _clientName + "-test";
		make_printjob();
		save_as(testName);
	} else {
		take_order();
	}
	return;
}


void PrintFact::cleanup() {
    if (_footer && _footer.is_open()) {
        _footer.close();
    }
    if (_data && _data.is_open()) {
        _data.close();
    }
    if (_header && _header.is_open()) {
        _header.close();
    }
	if (_printer.is_open()) {
		_printer.close_device();
	}
}

void PrintFact::_initialize() 
{
	to_zero();
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
    _data.open("prices.ini");
    _header.open("ascii/header-printjob.txt");
    _footer.open("ascii/footer-printjob.txt");
	if (_header.is_open()) {
		std::stringstream buffer;
		buffer << _header.rdbuf();
		_headerJob = buffer.str();
		_header.clear();              // clear EOF + error flags
		_header.seekg(0, std::ios::beg); //
	} else {
		_headerJob.clear();
	}
	if (_footer.is_open()) {
		std::stringstream buffer;
		buffer << _footer.rdbuf();
		_footerJob = buffer.str();
		_footer.clear();              // clear EOF + error flags
		_footer.seekg(0, std::ios::beg); //
	} else {
		_footerJob.clear();
	}

}

void PrintFact::to_zero() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    _date = *std::localtime(&now_time_t);
	_margin = 0;
	_width = 0;
	_contentWidth = 0;
	_outStream.str("");
	_outStream.clear();
    _billNumber.clear();
    _clientName.clear();
    _title.clear();
    _clientAddress.clear();
    _paperName.clear();

	_totalDiscount = 0.0;
	_totalGraphics = 0.0;
    _total = 0.0;
    _totalLabor = 0.0;
    _totalInk = 0.0;
    _totalJob = 0.0;
    _totalPaper = 0.0;
	_totalShaping = 0.0;

	_unitPrice = 0.0;
	_discount = 0.0;
	_weight = 0.0;
	_fees = 0.0;
    _format = 0.0;
    _sheetPrice = 0.0;
    _sheetWeight = 0.0;
    _labor = 0.0;
	_graphics = 0.0;

	_staples = 0;
	_folds = 0;
    _sheetNb = 0;
    _totalMasters = 0;
    _copy = 0;
    _layers = 0;
    _colorsFaceA = 0;
    _colorsFaceB = 0;
    _facture = false;

}

void PrintFact::new_job()
{
	end_str(0, 0, true);
	std::string line = "";

	user_input(fit(" ", 4) + "Intitulé : ", _title, false, MAX_TITLE_LENGTH);
	user_value(fit(" ", 4) + "Format (nombre de feuilles A3 pour un exemplaire): ", _format, true);
	user_value(fit(" ", 4) + "Copies: ", _copy, true);
	user_input(fit(" ", 4) + "Façonnage (y/n): ", line, false); 
	_unitPrice = _copy * _format;
	if (line == "y" || line == "Y") {
		user_value("Agrafe par ex: ", _staples, false);
		user_value("Plis par ex: ", _folds, false);
		if (_staples > 0) _unitPrice = _copy; // -> reliure !
	}
	user_value(fit(" ", 4) + "Couleurs face A: ", _colorsFaceA, true);
	user_value(fit(" ", 4) + "Couleurs face B: ", _colorsFaceB, false);
	while (!g_interrupt) {
    	std::cout << fit(" ", 4) + "Selection papier depuis:\n" + printMap(_paperPrices).str();
		user_input("Nom du papier & grammage (papier.gsm): ", _paperName, true);
        if (_paperPrices.find(_paperName) == _paperPrices.end()) {
            std::cerr << "Error: No such paper.\n";
        } else {
			std::pair<int, double> paperDetails = _paperPrices.find(_paperName)->second;
			_sheetWeight = paperDetails.first;
            _sheetPrice =  paperDetails.second;
			break;
        }
	}
	user_value(fit(" ", 4) + "Création graphique: ", _graphics, false);
	user_input(fit(" ", 4) + "Shipping (y/n): ", line, false);
	if (line == "y" || line == "Y") 
		_fees = std::numeric_limits<double>::infinity();
	user_value(fit(" ", 4) + "Réduction (%): ", _discount, false);
	if (g_interrupt)
		return;
    calc_expend();
	register_job();
	clearScreen();
	std::cout << _outStream.str();
	user_input("Add an other printing job ? (y/n) ", line, false);
	if (line == "y" || line == "Y") {
		_outStream << "    ****************************************************************\n\n";
		new_job();
	}
	// save_as(_clientName);
}

void PrintFact::calc_shipping_fees()
{
	_sheetWeight = static_cast<double>(_paperPrices[_paperName].first);
	_weight = _sheetNb * (_sheetWeight * 0.125 / 1000.0);

    if (_fees == std::numeric_limits<double>::infinity()) {
        auto it = _shippingCosts.lower_bound(_weight);
        if (it != _shippingCosts.end()) {
            _fees = it->second;
        } else {
            _fees = _shippingCosts.rbegin()->second;
        }
    } else {
        _fees = 0.0;
    }
}

void PrintFact::calc_expend()
{
    double inkPrice, masterPrice, graphicsPrice, shapingPrice;
    auto   itInk = _consumablePrices.find("ink");
    auto   itMaster = _consumablePrices.find("master");
    auto   itGraphics = _consumablePrices.find("graphics");
    auto   itShaping = _consumablePrices.find("fold/staple");
    if (itInk == _consumablePrices.end()) {
        throw std::invalid_argument("missing ink price");
    } else if (itMaster == _consumablePrices.end()) {
        throw std::invalid_argument("missing master price");
    } else if (itGraphics == _consumablePrices.end()) {
        throw std::invalid_argument("missing graphics price");
    } else if (itShaping == _consumablePrices.end()) {
        throw std::invalid_argument("missing fold/staple price");
    }
    inkPrice = itInk->second;
    masterPrice = itMaster->second;
    graphicsPrice = itGraphics->second;
	shapingPrice = itShaping->second;

	_layers = _colorsFaceA + _colorsFaceB;
	_sheetNb = _format * _copy + _layers * 5;
    _labor = (2 + _layers) * _format;
    _totalGraphics = _graphics * graphicsPrice;
    _totalPaper = _sheetNb * _sheetPrice;
    _totalInk = _sheetNb * (_colorsFaceB + _colorsFaceA) * inkPrice;
    _totalMasters = (_colorsFaceB + _colorsFaceA) * masterPrice;
    _totalLabor = _labor * 10;
	// _unitPrice holds the real copy number (copy of a books)
	_totalShaping = (_staples + _folds) * shapingPrice * _unitPrice;
    _totalJob = _totalPaper + _totalMasters + _totalInk + _totalLabor + _totalGraphics;
    _totalDiscount = _totalJob * (_discount / 100.0);
    calc_shipping_fees();
    _totalJob -= _totalDiscount + _fees;
	_unitPrice = _totalJob / _unitPrice;
	_total += _totalJob;
}

void PrintFact::register_header()
{
	_outStream << _headerJob;
    _outStream << "    Papier (nom.g/m2):" << fit("Consomable:\n", 36);

    typedef std::map<std::string, std::pair<int, double> >::iterator PaperIt;
    typedef std::map<std::string, double>::iterator ConsumableIt;

	int tabWidth = 20;
	int inBetweenMargin = 22;
    PaperIt paper = _paperPrices.begin();
    ConsumableIt consumable = _consumablePrices.begin();
    while (paper != _paperPrices.end() || consumable != _consumablePrices.end()) {
        if (paper != _paperPrices.end()) {
            _outStream << "    - " << paper->first + fit(paper->second.second, tabWidth - paper->first.length(), 2, '.');
            paper++;
        } else {
			inBetweenMargin = 48;
		}
        if (consumable != _consumablePrices.end()) {
            _outStream << fit("- ", inBetweenMargin) << consumable->first + fit(consumable->second, tabWidth - consumable->first.length(), 2, '.') + "\n";
            consumable++;
        } else {
            _outStream << "\n";
        }
    }
    _outStream << "\n";
}

std::string to_hex(int value) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << value;
    return oss.str();
}

void PrintFact::save_as(std::string& clientName)
{
    std::string       input;

	clearScreen();
	std::cout << _outStream.str();
	user_input("Save the bill > (y/n): ", input, false);
    if (input != "Y" && input != "y") {
		user_input("Quit without saving. Press any key to continue.", input, false, 0);
		return;
	}
	std::ofstream outfile;
	std::string folderName = "devis/";
	std::string truncName = sanitize_filename(clientName);
	user_input("As an invoice ? (y/n): ", input, true);
	if (input == "y" || input == "Y")
	{
		folderName = "factures/";
		if (!_facture) {
			_facture = true;
			std::string line;
			std::stringstream ss(_outStream.str());
			_outStream.str("");
			_outStream.clear();
			while(getline(ss, line)) {
				size_t found = line.find(_billNumber);
				if (found != std::string::npos) {
					_billNumber.replace(10, 1, "F");
					line = "    Facture n°" + _billNumber + " du: " + formated_date();
				}
				_outStream << line + "\n";
			}
		}
		// _fileName = _billNumber + "_" + truncName;
		// outfile.open("factures/" + _fileName + ".txt");
	} else {
		// _fileName = _billNumber + "_" + truncName;
		// outfile.open("devis/" + _fileName + ".txt");
	}
	_fileName = _billNumber + "_" + truncName;
	outfile.open(folderName + _fileName + ".txt");
	if (!outfile || !outfile.is_open())
		throw std::runtime_error("Could not open file for writing.");
	outfile << _outStream.str();
	outfile.close();
	std::cout << _fileName << " has been register.\n";
	to_png(folderName, _fileName);
	user_input("Press enter to continue", input, false);
}

void PrintFact::to_png(std::string& folder, std::string& fileName) {
	std::string txtPath = folder + fileName + ".txt";
	std::string pngPath = folder + "png/" + fileName + ".png";
	txtToPng(txtPath, pngPath);
	std::cout << pngPath << " has been saved as PNG\n";
}

void PrintFact::make_printjob()
{
    register_header();
	register_infos();
	new_job();
    // register_job();
    register_footer();
	save_as(_clientName);
	clearScreen();
    // std::cout << _outStream.str();
}

void PrintFact::register_footer()
{
    _outStream << "    ****************************************************************\n";
    _outStream << "    Sômmes dues:" + fit(_total, 51) + "€\n";
    _outStream << "    ****************************************************************\n";
	_outStream << _footerJob;
}

std::string PrintFact::end_str(int size = 22, int offset = 0, bool reset)
{
	static bool firstUse = true;
    static std::random_device rd;
    static std::mt19937 generator(rd());
	static std::string poem;

	if (reset) {
		firstUse = false;
		return "";
	}
	std::string off(" ");
    for (int i = 0; i < offset; i++) {
        off += " ";
    }
	if (firstUse) {
		std::uniform_int_distribution<size_t> distribution(0, 4);
		int r = distribution(generator);
		if (r > 0) {
			poem = "poem" + std::to_string(r) + ".txt";
		} else {
			poem.clear();
		}
	}
	firstUse = false;
	if (!poem.empty()) {
		static std::ifstream text(poem);
		if (text && text.is_open()) {
			std::string line;
			std::getline(text, line);
			return (off + line + "\n");
		}
	}
    return off + random_string(size, {" ", "█"}) + "\n";
}



std::string PrintFact::bill_number(billType type) {
	std::stringstream ss;
	int inc = 0;

	formated_date();
	do {
		ss.str("");
		ss.clear();
		ss << (_date.tm_year + 1900) % 100 
			<< fit(_date.tm_mon + 1, 2, 2, '0')
			<< fit(_date.tm_mday, 2, 2, '0') << "-"
			<< fit(to_hex(_date.tm_hour + _date.tm_min + inc), 3, 0, '0');
		inc++;
	} while (!get_bill_filename(ss.str()).empty());

	switch (type) {
		case INVOICE:
			ss << "F";
			break;
		case ESTIMATE:
			ss << "E";
			break;
		case TICKET:
			ss << "T";
			break;
		default:
		  ss << "U";
	}
	return ss.str();
}

std::string PrintFact::formated_date() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    _date = *std::localtime(&now_time_t);

    std::stringstream current_date;
    current_date << fit(_date.tm_mday, 2, 2, '0')
				<< "/" << fit(_date.tm_mon + 1, 2, 2, '0')
				<< "/" << _date.tm_year + 1900;
	return current_date.str();
}

void PrintFact::register_infos() {

	// std::string current_date = formated_date();
	
	user_input(fit(" ", 4) + "Nom client ?: ", _clientName, true);
	user_input(fit(" ", 4) + "Adresse client: ?: ", _clientAddress, false, MAX_EMAIL_LENGTH);
    _outStream << "    ****************************************************************\n";
    std::string address = _clientAddress.empty() ? "" : " (" + _clientAddress + ")";
	_billNumber = bill_number(ESTIMATE);
    _outStream << "    Devis n°" + _billNumber + " du: " + formated_date() + "\n";
    _outStream << "    à: " << _clientName << address << "\n";
    _outStream << "    ****************************************************************\n\n";
}

void PrintFact::register_job()
{
    std::string colorVerso =
        (_colorsFaceB > 0) ? " - " + std::to_string(_colorsFaceB) + "/verso" : "";
    std::string colorStr = std::to_string(_colorsFaceA) + "/recto" + colorVerso;
    std::string copyStr = std::to_string(_copy);
    int         offset = 68 - 14 - 22 - 1;
    _outStream << "    Intitulé: " << _title << end_str(22, offset - _title.length());
    _outStream << "    Couleurs: " << colorStr << end_str(22, offset - colorStr.length());
    _outStream << "    Copies  : " << copyStr << end_str(22, offset - copyStr.length());
    _outStream << "    Papier  : " << _paperName << end_str(22, offset - _paperName.length());

    std::string feesStr = (_fees > 0.0) ? fit(_fees, 8) : fit("--", 8);
    std::string totalDiscountStr =
        (_totalDiscount > 0.0) ? fit(_totalDiscount * -1, 8) : "      --";
    std::string discountStr = (_discount > 0.0) ? fit(_discount, 12) : "--";

    _outStream << "    |---------------------------------------|" + end_str();
    _outStream << "    |   désignation   |  quantité  |  prix  |" + end_str();
    _outStream << "    |-----------------|------------|--------|" + end_str();
	_outStream << new_job_line("feuilles A3", _sheetNb, _totalPaper, end_str());
	_outStream << new_job_line("masters", _layers, _totalMasters, end_str());
	_outStream << new_job_line("encre (passages)", _sheetNb * _layers, _totalInk, end_str());
	_outStream << new_job_line("plis et/ou agrafe", _staples + _folds, _totalShaping, end_str());
    _outStream << "    |---------------------------------------|" << end_str();
	_outStream << new_job_line("impression", _labor, _totalLabor, end_str());
	_outStream << new_job_line("graphisme", _graphics, _totalGraphics, end_str());
	_outStream << new_job_line("imposition", 0, 0, end_str());
    _outStream << "    |---------------------------------------|" + end_str();
	_outStream << new_job_line("remise (%)", discountStr, totalDiscountStr, end_str());
	_outStream << new_job_line("port (kg)", _weight, feesStr, end_str());
    _outStream << "    |=======================================|" + end_str();
    _outStream << "    |TOTAL TTC" + fit(_totalJob, 29) + "€" + "|" + end_str();
    _outStream << "    |=======================================|" + end_str();
    _outStream << "    |" + fit(_unitPrice, 32) + "€/copie|" + end_str() + "\n";
}

void PrintFact::read_section_from(const std::string& section, std::map<std::string, std::pair<int, double> >& list)
{
    std::string line, paperName, gram;
	int gramValue;
	double price;
	std::pair<int, double> infos;
    std::string start = "[" + section + "]";

    while (std::getline(_data, line)) {
        if (line == start)
            break;
    }
    while (std::getline(_data, line)) {
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
    _data.clear();                 // Clear any error flags
    _data.seekg(0, std::ios::beg); // Move the get pointer to the beginning of the file
}

void PrintFact::read_section_from(const std::string& section, std::map<double, double>& list)
{
    std::string line;
    std::string start = "[" + section + "]";

    while (std::getline(_data, line)) {
        if (line == start)
            break;
    }
    while (std::getline(_data, line)) {
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
    _data.clear();                 // Clear any error flags
    _data.seekg(0, std::ios::beg); // Move the get pointer to the beginning of the file
}

void PrintFact::read_section_from(const std::string& section, std::map<std::string, double>& list)
{
    std::string line;
    std::string start = "[" + section + "]";

    while (std::getline(_data, line)) {
        if (line == start)
            break;
    }
    while (std::getline(_data, line)) {
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
    _data.clear();                 // Clear any error flags
    _data.seekg(0, std::ios::beg); // Move the get pointer to the beginning of the file
}


bool PrintFact::turn_pdf(const std::string& name)
{
	std::ifstream file("factures/" + name + ".txt");
    // Check if file is open
    if (!file.is_open()) {
        std::cerr << "Error: File stream is not open" << std::endl;
        return false;
    }

    // Create PDF document
    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    if (!pdf) {
        std::cerr << "Error: Cannot create PDF object" << std::endl;
        return false;
    }

    // Create first page
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

    // Set font
    HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", NULL);
    HPDF_Page_SetFontAndSize(page, font, 12);

    // Page dimensions
    float page_height = HPDF_Page_GetHeight(page);
    // float page_width = HPDF_Page_GetWidth(page);
    float margin = 50;
    float y_position = page_height - margin;
    float line_height = 15;

    // Read and write each line
    std::string line;
    while (std::getline(file, line)) {
        // Check if we need a new page
        if (y_position < margin) {
            page = HPDF_AddPage(pdf);
            HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
            HPDF_Page_SetFontAndSize(page, font, 12);
            y_position = page_height - margin;
        }

        // Write text
        HPDF_Page_BeginText(page);
        HPDF_Page_MoveTextPos(page, margin, y_position);
        HPDF_Page_ShowText(page, line.c_str());
        HPDF_Page_EndText(page);

        // Move to next line
        y_position -= line_height;
    }

    // Save PDF
    //
    HPDF_STATUS status = HPDF_SaveToFile(pdf, name.c_str());
    HPDF_Free(pdf);

    if (status != HPDF_OK) {
        std::cerr << "Error: Cannot save PDF file" << std::endl;
        return false;
    }

    return true;
}
