#include "PrintFact.hpp"
#include "Database.hpp"
#include "hpdf.h"
#include "tools.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
namespace fs = std::filesystem;

// Destructor
Facturier::~Facturier(void)
{
    if (_facture) {
        delete _facture;
    }
    if (_db) {
        delete _db;
    }
}

// Default constructor
Facturier::Facturier() : _verb(false), _margin(0), _width(0), _contentWidth(0), _facture(nullptr)
{
    _consumablePrices.clear();
    _paperPrices.clear();
    _shippingCosts.clear();
    std::ifstream prices("prices.ini");
    _config = load_config("config.ini");
    _initialize();
    if (!prices || !prices.is_open())
        throw std::runtime_error("Cannot open prices.ini");
    read_section_from("consumable", _consumablePrices, prices);
    read_section_from("paper", _paperPrices, prices);
    read_section_from("shipping", _shippingCosts, prices);
    prices.close();

    std::stringstream buffer;
    std::ifstream     headerFile("ascii/header-printjob.txt");
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
        headerFile.close(); // clear EOF + error flags
    } else {
        _header = "";
    };

    std::ifstream footerFile("ascii/CGV.txt");
    if (footerFile.is_open()) {
        buffer << footerFile.rdbuf();
        _conditions_of_sales = buffer.str();
        buffer.str("");
        buffer.clear();
        footerFile.close();
    } else {
        _conditions_of_sales = "";
    }

    _db = new Database("data.db", this);
    verbose("Table created successfully");

    _font.family = "Menlo";
    _font.size = 13.0;
    _font.bold = false;
    _font.fg_r = 0.10;
    _font.fg_g = 0.10;
    _font.fg_b = 0.10; // near-black text
    _font.bg_r = 0.97;
    _font.bg_g = 0.97;
    _font.bg_b = 0.95; // off-white background
    _font.padding = 24;
}

void Facturier::verbose(const std::string& txt)
{
    if (_verb) {
        std::cout << BLUE;
        std::cout << txt;
        std::cout << RESET;
        std::cout << std::endl;
    }
}

bool Facturier::main_menu()
{
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

        switch (dial_menu({"Exit", "Nouveau devis", "Voir les documents"})) {
        case 1:
            make_new_facture();
            break;
        case 2:
            _db->browse_documents();
            break;
        default:
        case 0:
            return false;
        }
    }
    return true;
}

void Facturier::run(int ac, char* av[])
{
    std::vector<std::string> args;
    Database::PrintMode      mode = Database::PREVIEW;

    for (int i = 1; i < ac; i++) {
        std::string arg(av[i]);
        if (arg == "--verbose") {
            _verb = true;
        } else if (arg.starts_with("--view=")) {
            std::string opt = arg.substr(7);
            if (opt == "preview") {
                mode = Database::PREVIEW;
            } else if (opt == "complete") {
                mode = Database::COMPLETE;
            } else {
                throw std::invalid_argument(arg);
            }
        } else if (arg.starts_with("--print=")) {
            std::string opt = arg.substr(8);
            bool        isNumber =
                !opt.empty() && std::all_of(opt.begin(), opt.end(), [](unsigned char c) {
                    return std::isdigit(c);
                });
            if (opt == "test") {
                _facture = new Facture(this);
                _facture->riso_prints.push_back(new Risography(this));
                _facture->customs.push_back(std::make_pair("Objet test", std::make_pair(3, 0.5)));
                _facture->customs.push_back(
                    std::make_pair("Objet test 2", std::make_pair(35, 1.5)));
                _facture->client =
                    new Client("Client test", "email test", "addr test", "teltest", "siret");
                std::cout << _facture->generate_stream();
            } else if (opt == "all") {
                _db->print_all_documents(mode);
            } else if (opt == "devis") {
                _db->print_all_documents(mode, Facture::TYPE::DEVIS);
            } else if (opt == "ticket") {
                _db->print_all_documents(mode, Facture::TYPE::TICKET);
            } else if (opt == "facture") {
                _db->print_all_documents(mode, Facture::TYPE::FACTURE);
            } else if (isNumber) {
                int      id = std::atoi(opt.c_str());
                Facture* f = _db->get_document(id);
                if (f)
                    std::cout << f->generate_stream();
            } else {
                throw std::invalid_argument(arg);
            }
            return;
        } else if (arg.starts_with("--convert=")) {
            std::string opt = arg.substr(10);
            bool        isNumber =
                !opt.empty() && std::all_of(opt.begin(), opt.end(), [](unsigned char c) {
                    return std::isdigit(c);
                });
            if (!isNumber) {
                throw std::invalid_argument(arg);
            }
            int      id = std::atoi(opt.c_str());
            Facture* f = _db->get_document(id);
            if (f && f->type != Facture::TYPE::FACTURE) {
                _db->update_document(id, Facture::TYPE::FACTURE, Facture::STATUS::CONVERTED);
                make_png(f->generate_stream(), _font, "documents/factures/" + f->get_filename());
            } else if (f) {
                std::cout << "document " << id << " déja enregistré en tant que facture";
            }
            return;
        } else if (arg.starts_with("--send=")) {
            std::string opt = arg.substr(7);
            bool        isNumber =
                !opt.empty() && std::all_of(opt.begin(), opt.end(), [](unsigned char c) {
                    return std::isdigit(c);
                });
            if (!isNumber) {
                throw std::invalid_argument(arg);
            } else {
                int      id = std::atoi(opt.c_str());
                Facture* f = _db->get_document(id);
                if (f) {
                    f->generate_stream();
                    if (userline("Send document ? (y/n)") != "y")
                        return;
                    if (f->client->get_email().empty()) {
                        throw std::invalid_argument("No email in this document");
                    }
                    std::string folder =
                        f->type == Facture::TYPE::FACTURE ? "factures/" : "devis/";
                    std::string filepath = "documents/" + folder + f->get_filename();
                    make_png(f->generate_stream(), _font, filepath);
                    send_email_smtp(f->client->get_email(),
                                    "Devis " + f->number,
                                    EMAIL_BODY,
                                    folder + f->get_filename(),
                                    get_config("from"),
                                    get_config("url"),
                                    get_config("user"),
                                    get_config("password"));
                    std::cout << f->get_type() << " has been send to " << f->client->get_email();
                }
                return;
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

    main_menu();
    return;
}

std::string Facturier::get_config(const std::string& str) { return _config[str]; }

void Facturier::_initialize()
{
    if (!fs::exists("documents")) {
        fs::create_directory("documents");
        std::cout << "documents directory created.\n";
    }
    if (!fs::exists("documents/factures")) {
        fs::create_directory("documents/factures");
        std::cout << "documents/factures directory created.\n";
    }
    if (!fs::exists("documents/devis")) {
        fs::create_directory("documents/devis");
        std::cout << "documents/devis directory created.\n";
    } else if (!fs::exists("documents/tickets")) {
        fs::create_directory("documents/tickets");
        std::cout << "documents/tickets directory created\n";
    }
}

std::string Facturier::generate_header()
{
    std::stringstream ss;
    verbose("header stream");
    ss << _header << std::endl;
    ;
    ss << "    Papier (nom.g/m2):" << fit("Consomable:\n", 36);

    typedef std::map<std::string, std::pair<int, double>>::iterator PaperIt;
    typedef std::map<std::string, double>::iterator                 ConsumableIt;

    int          tabWidth = 20;
    int          inBetweenMargin = 22;
    PaperIt      paper = _paperPrices.begin();
    ConsumableIt consumable = _consumablePrices.begin();
    while (paper != _paperPrices.end() || consumable != _consumablePrices.end()) {
        if (paper != _paperPrices.end()) {
            ss << "    - "
               << paper->first +
                      fit(paper->second.second, tabWidth - paper->first.length(), 2, '.');
            paper++;
        } else {
            inBetweenMargin = 48;
        }
        if (consumable != _consumablePrices.end()) {
            ss << fit("- ", inBetweenMargin)
               << consumable->first +
                      fit(consumable->second, tabWidth - consumable->first.length(), 2, '.') +
                      "\n";
            consumable++;
        } else {
            ss << "\n";
        }
    }
    ss << "\n";
    return ss.str();
}

// std::string Facturier::generate_infos()
// {
// 	verbose("Register infos");
// 	std::stringstream ss;
// 	std::string address;
// 	// if (_facture) {
// 	address = _facture->client->get_email().empty() ? "" : " (" + _facture->client->get_email() +
// ")";
// 	// }
// 	ss << "    ****************************************************************\n";
// 	ss << "    Devis n°" + _facture->number + " du: " + _facture->date + "\n";
// 	ss << "    à: " << _facture->client->get_name() << address << "\n";
// 	ss << "    ****************************************************************\n\n";
// 	return ss.str();
// }

// std::string to_hex(int value) {
//     std::ostringstream oss;
//     oss << std::hex << std::uppercase << value;
//     return oss.str();
// }

Font Facturier::get_font() { return _font; }

std::string Facturier::get_conditions_of_sales() { return _conditions_of_sales; }

void Facturier::make_new_facture()
{
    if (_facture)
        delete _facture;
    _facture = new Facture(this);
    _facture->client = new Client;
    _facture->client->set_name(userline("client name"));
    _facture->client->set_email(userline("client address", false));

    clearScreen();
    std::cout << generate_header();
    std::cout << _facture->generate_infos();

    bool done = false;
    while (!g_interrupt && !done) {
        switch (dial_menu({"Exit & enregistrer", "Nouvelle risography", "Nouvelle ligne"})) {
        case 0:
            done = true;
            break;
        case 1:
            _facture->add_riso_print();
            break;
        case 2:
            _facture->add_custom_line();
            break;
        default:
            break;
        }
        clearScreen();
        std::cout << _facture->generate_stream();
    }

    std::string filepath = "documents/";
    switch (dial_menu({"Ne pas enregistrer",
                       "Enregistrer devis",
                       "Enregistrer facture",
                       "Enregistrer brouillon"})) {
    case 1:
        _facture->status = Facture::STATUS::APPROVED;
        _facture->type = Facture::TYPE::DEVIS;
        _db->save_facture(_facture);
        filepath += "devis/" + _facture->get_filename();
        make_png(_facture->generate_stream(), _font, filepath);
        break;
    case 2:
        _facture->status = Facture::STATUS::APPROVED;
        _facture->type = Facture::TYPE::FACTURE;
        _db->save_facture(_facture);
        filepath += "factures/" + _facture->get_filename();
        make_png(_facture->generate_stream(), _font, filepath);
        break;
    case 3:
        _facture->status = Facture::STATUS::DRAFT;
        _facture->type = Facture::TYPE::DEVIS;
        _db->save_facture(_facture);
        break;
    case 0:
    default:
        break;
    }

    clearScreen();
    _facture->generate_stream();
    if (_facture->status == Facture::STATUS::APPROVED) {
        if (userline("Envoyer document par email ? (y/n)") == "y") {
            send_email_smtp(_facture->client->get_email(),
                            _facture->get_type() + " " + _facture->number,
                            EMAIL_BODY,
                            filepath,
                            get_config("from"),
                            get_config("url"),
                            get_config("user"),
                            get_config("password"));
        }
    }
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

// std::string Facturier::generate_custom() {
// 	std::stringstream ss;
//
// 	if (_facture->customs.size() > 0) {
// 		ss << "    |                   objet                    |      prix       |\n";
// 		ss << "    |============================================|=================|\n";
// 	}
// 	for (const auto& [key, value] : _facture->customs) {
// 		ss << fit("|", 5) << key << fit("|", 45 - key.length()) << fit(value, 16) + "€|\n";
// 		ss << "    |--------------------------------------------|-----------------|\n";
// 	}
// 	ss << "\n";
// 	return ss.str();
// }

// std::string Facturier::generate_job(Risography* job)
// {
// 	if (!job) {
// 		verbose("job is null...");
// 		return "";
// 	}
// 	verbose("generate print job");
//
// 	std::stringstream ss;
// 	if (_facture->riso_prints.size() > 1)
// 		ss << "    ****************************************************************\n\n";
//     std::string colorVerso =
//         (job->layersB > 0) ? " - " + std::to_string(job->layersB) + "/verso" : "";
//     std::string colorStr = std::to_string(job->layersA) + "/recto" + colorVerso;
//     std::string copyStr = std::to_string(job->copy);
//     int         offset = 68 - 14 - 22 - 1;
//     ss << "    Intitulé: " << job->description << end_str(22, offset -
//     job->description.length()); ss << "    Couleurs: " << colorStr << end_str(22, offset -
//     colorStr.length()); ss << "    Copies  : " << copyStr << end_str(22, offset -
//     copyStr.length()); ss << "    Papier  : " << job->paper << end_str(22, offset -
//     job->paper.length());
//
// 	verbose("generate print job 1");
//     std::string feesStr = (job->total_shipping() > 0.0) ? fit(job->total_shipping(), 8) :
//     fit("--", 8); std::string totalDiscountStr =
//         (job->total_discount() > 0.0) ? fit(job->total_discount() * -1, 8) : "      --";
// 	verbose("generate print job 2");
//     std::string discountStr = (job->discount_percentage > 0.0) ? fit(job->discount_percentage,
//     12) : "--";
//
// 	verbose("generate print job 3");
//     ss << "    |---------------------------------------|" + end_str();
//     ss << "    |    désignation    | quantité |  prix  |" + end_str();
//     ss << "    |===================|==========|========|" + end_str();
// 	ss << new_line("feuilles A3", job->sheet_quantity, job->total_paper());
// 	ss << new_line("masters", job->masters_quantity, job->total_master());
// 	ss << new_line("encre (passages)", job->sheet_quantity * job->masters_quantity,
// job->total_ink()); 	ss << new_line("plis/coupe/agrafe", job->staple_quantity +
// job->fold_per_unit, job->total_shaping());
//     ss << "    |---------------------------------------|" << end_str();
// 	ss << new_line("impression", job->labor_amount, job->total_labor());
// 	ss << new_line("graphisme", job->graphic_amount, job->total_graphic());
// 	ss << new_line("imposition", job->imposition_amount, job->total_imposition(), end_str());
//     ss << "    |---------------------------------------|" + end_str();
// 	ss << new_line("remise (%)", discountStr, totalDiscountStr);
// 	ss << new_line("port (kg)", job->weight, feesStr);
//     ss << "    |=======================================|" + end_str();
//     ss << "    |sous total (ttc)" + fit(job->total, 22) + "€" + "|" + end_str();
//     ss << "    |=======================================|" + end_str();
//     ss << "    |" + fit(job->unit_cost, 32) + "€/copie|" + end_str() + "\n";
//
// 	verbose("finish generating job");
// 	return ss.str();
// }

void Facturier::read_section_from(const std::string&                             section,
                                  std::map<std::string, std::pair<int, double>>& list,
                                  std::ifstream&                                 file)
{
    std::string            line, paperName, gram;
    int                    gramValue;
    double                 price;
    std::pair<int, double> infos;
    std::string            start = "[" + section + "]";

    while (std::getline(file, line)) {
        if (line == start)
            break;
    }
    while (std::getline(file, line)) {
        trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line[0] == '[')
            break;
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
    file.clear();                 // Clear any error flags
    file.seekg(0, std::ios::beg); // Move the get pointer to the beginning of the file
}

// void Facturier::save_facture_as(Facture* facture)
// {
//     std::string       input;
//
// 	if (!facture)
// 		return;
//
// 	clearScreen();
// 	Facture::TYPE type = facture->type;
//
// 	std::string fileName;
// 	std::ofstream outfile;
// 	std::string folderName = "devis/";
// 	std::string truncName = facture->client->get_name();
// 	// std::string truncName = sanitize_filename(facture->client_name);
// 	if (type == Facture::TYPE::FACTURE)
// 	{
// 		folderName = "factures/";
// 		facture->type = Facture::TYPE::FACTURE;
// 			std::string line;
// 			std::stringstream ss(facture->infos_strm);
// 			facture->infos_strm.clear();
// 			while(getline(ss, line)) {
// 				size_t found = line.find(_facture->number);
// 				if (found != std::string::npos) {
// 					_facture->number.replace(10, 1, "F");
// 					line = "    Facture n°" + _facture->number + " du: " +
// _facture->get_formated_date(true);
// 				}
// 				facture->infos_strm += line + "\n";
// 			}
// 		outfile.open("factures/" + fileName + ".txt");
// 	} else {
// 		facture->type = Facture::TYPE::DEVIS;
// 	}
// 	_db->save_facture(facture);
// 	fileName = _facture->number + "_" + truncName;
// 	// outfile.open(folderName + fileName + ".txt");
// 	// if (!outfile || !outfile.is_open())
// 	// 	throw std::runtime_error("Could not open file for writing.");
// 	// outfile << facture->header_strm;
// 	// outfile << facture->infos_strm;
// 	// outfile << facture->jobs_strm;
// 	// outfile << facture->footer_strm;
// 	// outfile.close();
//
// 	// std::cout << fileName << ".txt has been register.\n";
// 	make_png(facture->generate_stream(), _font, fileName + ".png");
// 	// to_png(folderName, fileName);
// 	std::cout << fileName << ".png has been register.\n";
// 	userline("Press enter to continue", false);
// }

void Facturier::read_section_from(const std::string& section, std::map<double, double>& list,
                                  std::ifstream& file)
{
    std::string line;
    std::string start = "[" + section + "]";

    while (std::getline(file, line)) {
        if (line == start)
            break;
    }
    while (std::getline(file, line)) {
        trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line[0] == '[')
            break;
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
    file.clear();                 // Clear any error flags
    file.seekg(0, std::ios::beg); // Move the get pointer to the beginning of the file
}

void Facturier::read_section_from(const std::string& section, std::map<std::string, double>& list,
                                  std::ifstream& file)
{
    std::string line;
    std::string start = "[" + section + "]";

    while (std::getline(file, line)) {
        if (line == start)
            break;
    }
    while (std::getline(file, line)) {
        trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line[0] == '[')
            break;
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
    file.clear();                 // Clear any error flags
    file.seekg(0, std::ios::beg); // Move the get pointer to the beginning of the file
}

const std::map<std::string, double>& Facturier::getConsumablePrices() const
{
    return _consumablePrices;
};
const std::map<std::string, std::pair<int, double>>& Facturier::getPaperPrices() const
{
    return _paperPrices;
};
const std::map<double, double>& Facturier::getShippingPrices() const { return _shippingCosts; };
