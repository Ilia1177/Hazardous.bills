#include "Facture.hpp"
#include "PrintFact.hpp"
#include "tools.h"
#include <chrono>
#include <cmath>
#include <sstream>
// Default constructor
Facture::Facture(Facturier* f) :
    total(0), type(Facture::TYPE::DEVIS), status(Facture::STATUS::DRAFT), facturier(f)
{
    if (!f) {
        throw std::runtime_error("facturier not ready");
    }
    date = get_formated_date(true);
    number = generate_number(type);
}

// Destructor
Facture::~Facture(void)
{
    if (client)
        delete client;
    for (auto riso : riso_prints) {
        delete riso;
    }
}

std::string Facture::get_formated_date(bool init)
{
    std::stringstream current_date;
    if (init) {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        chrono = *std::localtime(&now_time_t);
    }
    current_date << fit(chrono.tm_mday, 2, 2, '0') << "/" << fit(chrono.tm_mon + 1, 2, 2, '0')
                 << "/" << chrono.tm_year + 1900;
    return current_date.str();
}

std::string Facture::generate_infos()
{
    facturier->verbose("infos stream");
    std::stringstream ss;
    std::string       address, t;
    // if (_facture) {
    switch (type) {
    case Facture::TYPE::FACTURE:
        t = "facture";
        break;
    case Facture::TYPE::DEVIS:
        t = "devis";
        break;
    case Facture::TYPE::TICKET:
        t = "ticket";
        break;
    default:
        t = "unknown";
    }
    address = client->get_email().empty() ? "" : " (" + client->get_email() + ")";
    // }
    ss << "    ****************************************************************\n";
    ss << "    " << t << " n°" + number + " du: " + date + "\n";
    ss << "    à: " << client->get_name() << address << "\n";
    ss << "    ****************************************************************\n\n";
    return ss.str();
}

std::string Facture::generate_riso()
{
    facturier->verbose("riso stream");
    std::stringstream ss;
    int               count = 0;
    for (auto it : riso_prints) {
        if (count > 0)
            ss << "****************************************************************\n";
        ss << it->generate_stream();
        count++;
    }
    return ss.str();
}

std::string Facture::generate_stream()
{
    std::stringstream ss;
    ss << facturier->generate_header();
    ss << generate_infos();
    ss << generate_riso();

    ss << generate_custom();
    ss << generate_footer();
    return ss.str();
}

std::string Facture::generate_custom()
{
    facturier->verbose("custom stream");
    std::stringstream ss("");

    if (customs.size() > 0) {
        ss << "    |         objet         | quantite |  prix unitaire  |  total  |\n";
        ss << "    |=======================|==========|=================|=========|\n";
        for (const auto& [key, value] : customs) {
            ss << fit("|", 5) << key;
            ss << fit("|", 24 - key.length());
            ss << fit(value.first, 10) << "|";
            ss << fit(value.second, 17) << "|";
            ss << fit(value.first * value.second, 8) << "€|\n";
            ss << "    |-----------------------|----------|-----------------|---------|\n";
        }
        ss << "\n";
    }
    return ss.str();
}

std::string Facture::generate_footer()
{
    std::stringstream ss;
    facturier->verbose("footer stream");
    ss << "    ****************************************************************\n";
    ss << "    Total des Sômmes dues:" + fit(get_total(), 41) + "€\n";
    ss << "    ****************************************************************\n";
    ss << facturier->get_conditions_of_sales();
    return ss.str();
}

std::string Facture::get_status() const
{
    switch (status) {
    case Facture::STATUS::DRAFT:
        return "draft";
    case Facture::STATUS::SENT:
        return "sent";
    case Facture::STATUS::APPROVED:
        return "approved";
    case Facture::STATUS::CONVERTED:
        return "converted";
    }
}

std::string Facture::get_type() const
{
    switch (type) {
    case Facture::TYPE::DEVIS:
        return "devis";
    case Facture::TYPE::FACTURE:
        return "facture";
    case Facture::TYPE::TICKET:
        return "ticket";
    default:
        return "";
    }
}

std::string Facture::get_filename() const
{
    if (!client)
        throw std::logic_error("no client in facture");
    std::stringstream filename("");

    filename << number << "-";

    for (unsigned char it : client->get_name()) {
        if (filename.str().length() > 21)
            break;
        if (std::isalnum(it))
            filename << (char)std::tolower(it);
    }

    filename << ".png";
    return filename.str();
}

double Facture::get_total() const
{
    double grand_total = 0.0;

    for (const auto& i : riso_prints)
        grand_total += i->rest_to_pay();

    for (const auto& [object, qty_price] : customs) {
        const auto& [quantity, price] = qty_price;
        grand_total += price * quantity;
    }
    return grand_total;
}

std::string Facture::generate_number(Facture::TYPE type)
{
    std::stringstream ss;

    // YYMMDD
    ss << std::setfill('0') << std::setw(2) << (chrono.tm_year % 100) << std::setw(2)
       << (chrono.tm_mon + 1) << std::setw(2) << chrono.tm_mday << "-";

    // 3-digit code from time
    int code = (chrono.tm_hour * 60 + chrono.tm_min) % 1000;

    ss << std::setw(3) << std::setfill('0') << code;

    // ss.str("");
    // ss.clear();
    // ss << (chrono.tm_year + 1900) % 100;
    // ss << fit(chrono.tm_mon + 1, 2, 2, '0');
    // ss << fit(chrono.tm_mday, 2, 2, '0') << "-";
    // ss << std::hex << std::setw(3) << std::setfill('0');
    // ss << chrono.tm_hour + chrono.tm_min;
    // ss << std::dec;

    switch (type) {
    case Facture::TYPE::FACTURE:
        ss << "F";
        break;
    case Facture::TYPE::DEVIS:
        ss << "D";
        break;
    case Facture::TYPE::TICKET:
        ss << "T";
        break;
    default:
        ss << "U";
    }
    return ss.str();
}

CustomLine Facture::add_custom_line()
{
    int         quantity = 0;
    double      price = 0.0;
    std::string object = userline("objet");
    user_value("quantity", quantity);
    user_value("price", price);
    std::pair<int, double>                         target = std::make_pair(quantity, price);
    std::pair<std::string, std::pair<int, double>> line(object, target);
    customs.push_back(line);
    return line;
}

Risography* Facture::add_riso_print()
{

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
        std::cout << fit(" ", 4) + "Selection papier depuis:\n" +
                         printMap(facturier->getPaperPrices()).str();
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
            std::pair<int, double> paperDetails =
                facturier->getPaperPrices().find(job->paper)->second;
            job->sheet_weight = paperDetails.first;
            job->sheet_price = paperDetails.second;
            break;
        }
    }
    user_value("Création graphique", job->graphic_amount, false);
    user_value("Imposition", job->imposition_amount, false);
    if (userline("Shipping (y/n)", false) == "y") {
        job->shipping_fees =
            std::numeric_limits<double>::infinity(); // Trigger shipping calculation
    }
    user_value("Réduction (%)", job->discount_percentage, false);
    job->calc_quantity();
    this->total += job->rest_to_pay();
    riso_prints.push_back(job);
    return job;
}
