#include "Risography.hpp"
#include "PrintFact.hpp"
// Default constructor

Risography::Risography(	std::string desc,
						std::string paper,
						double format,
						int copy,
						int layersA,
						int layersB,
						int staples,
						int cut,
						int fold,
						double labor_nb,
						double graphism_nb,
						double imposition_nb,
						double discount,
						bool fee,
						double price_ink,
						double price_paper,
						double price_master,
						double price_graphism,
						double price_shaping,
						double price_imposition,
						double price_labor,
						int sheet_weight):
	description(desc),
	paper(paper),
	format(format),
	copy(copy),
	layersA(layersA),
	layersB(layersB),
	staples_per_unit(staples),
	cut_per_unit(cut),
	fold_per_unit(fold),
	labor_amount(labor_nb),
	graphic_amount(graphism_nb),
	imposition_amount(imposition_nb),
	discount_percentage(discount),
	shipping_fees(fee ? std::numeric_limits<double>::infinity() : 0.0),
	ink_price(price_ink),
	sheet_price(price_paper),
	master_price(price_master),
	graphic_price(price_graphism),
	shaping_price(price_shaping),
	imposition_price(price_imposition),
	labor_price(price_labor),
	sheet_weight(sheet_weight),
	sheet_quantity(0),
	staple_quantity(0),
	masters_quantity(0),
	run_quantity(0),
	weight(0.0),
	total(0.0),
	unit_cost(0),
	facturier(nullptr) 
{
		facturier = new Facturier;
	if (!facturier) throw std::runtime_error("facturier not ready");

	facturier->verbose("New print job created");
	std::map<std::string, double> consumableList = facturier->getConsumablePrices();
	std::map<std::string, std::pair<int, double> > paperList = facturier->getPaperPrices();
	std::map<double, double> shipping = facturier->getShippingPrices();	

    auto   itInk = consumableList.find("ink");
    auto   itMaster = consumableList.find("master");
    auto   itGraphics = consumableList.find("graphics");
    auto   itShaping = consumableList.find("fold/staple");
    auto   itImposition = consumableList.find("imposition");
    auto   itLabor = consumableList.find("print(/color)");
    if (itInk == consumableList.end()) {
        throw std::runtime_error("missing ink price");
    } else if (itMaster == consumableList.end()) {
        throw std::runtime_error("missing master price");
    } else if (itGraphics == consumableList.end()) {
        throw std::runtime_error("missing graphics price");
    } else if (itShaping == consumableList.end()) {
        throw std::runtime_error("missing fold/staple price");
    } else if (itImposition == consumableList.end()) {
        throw std::runtime_error("missing imposition price");
	} else if (itLabor == consumableList.end()) {
        throw std::runtime_error("missing labor price");
	}
	imposition_price = itImposition->second;
    ink_price = itInk->second;
    master_price = itMaster->second;
    graphic_price = itGraphics->second;
	shaping_price = itShaping->second;
	labor_price = itLabor->second;
		calc_quantity();
		total_cost();
}

Risography::Risography(Facturier* f):
	description(""),
	paper(""),
	format(0),
	copy(0),
	layersA(0),
	layersB(0),
	staples_per_unit(0),
	cut_per_unit(0),
	fold_per_unit(0),
	labor_amount(0),
	graphic_amount(0),
	imposition_amount(0),
	discount_percentage(0),
	shipping_fees(0),
	ink_price(0),
	sheet_price(0),
	master_price(0),
	graphic_price(0),
	shaping_price(0),
	imposition_price(0),
	labor_price(0),
	sheet_weight(0),
	sheet_quantity(0),
	staple_quantity(0),
	masters_quantity(0),
	run_quantity(0),
	weight(0),
	total(0),
	unit_cost(0),
	facturier(f)
{
	if (!f) throw std::runtime_error("facturier not ready");

	facturier->verbose("New print job created");
	std::map<std::string, double> consumableList = facturier->getConsumablePrices();
	std::map<std::string, std::pair<int, double> > paperList = facturier->getPaperPrices();
	std::map<double, double> shippingList = facturier->getShippingPrices();	

    auto   itInk = consumableList.find("ink");
    auto   itMaster = consumableList.find("master");
    auto   itGraphics = consumableList.find("graphics");
    auto   itShaping = consumableList.find("fold/staple");
    auto   itImposition = consumableList.find("imposition");
    auto   itLabor = consumableList.find("print(/color)");
    if (itInk == consumableList.end()) {
        throw std::runtime_error("missing ink price");
    } else if (itMaster == consumableList.end()) {
        throw std::runtime_error("missing master price");
    } else if (itGraphics == consumableList.end()) {
        throw std::runtime_error("missing graphics price");
    } else if (itShaping == consumableList.end()) {
        throw std::runtime_error("missing fold/staple price");
    } else if (itImposition == consumableList.end()) {
        throw std::runtime_error("missing imposition price");
	} else if (itLabor == consumableList.end()) {
        throw std::runtime_error("missing labor price");
	}
	imposition_price = itImposition->second;
    ink_price = itInk->second;
    master_price = itMaster->second;
    graphic_price = itGraphics->second;
	shaping_price = itShaping->second;
	labor_price = itLabor->second;
}

double Risography::total_shipping()
{
	facturier->verbose("JOB: total shipping...");
	double shippingFees = 0;
	if (!paper.empty() && sheet_weight == 0) {
		sheet_weight = static_cast<double>(facturier->getPaperPrices().at(paper).first);
	} else if (sheet_weight == 0) {
		sheet_weight = static_cast<double>(facturier->getPaperPrices().at("evercopy.90").first);
	}

	// The weight of an A3 sheet is 0.125 meter squared
	weight = sheet_quantity * (sheet_weight * 0.125 / 1000.0);

    if (shipping_fees == std::numeric_limits<double>::infinity()) {
		auto shipping = facturier->getShippingPrices();
		auto it = shipping.lower_bound(weight);
        if (it != facturier->getShippingPrices().end()) {
            shippingFees = it->second;
        } else {
            shippingFees = facturier->getShippingPrices().rbegin()->second;
        }
    } else {
        shippingFees = 0.0;
    }
	return shippingFees;
}

double Risography::total_discount() {
	facturier->verbose("JOB: total discount...");
	return total_cost() * (discount_percentage / 100.0);
}

double Risography::total_paper() {
	facturier->verbose("JOB: total paper...");
	return sheet_quantity * sheet_price;
}

double Risography::total_master() {
	facturier->verbose("JOB: total master...");
	return masters_quantity * master_price;
}

double Risography::total_ink() {
	facturier->verbose("JOB: total ink...");
	return run_quantity * ink_price;
}

double Risography::total_labor() {
	facturier->verbose("JOB: total labor...");
	return labor_amount * labor_price;
}

double Risography::total_graphic() {
	facturier->verbose("JOB: total graphics...");
	return graphic_amount * graphic_price;
}

double Risography::total_shaping() {
	facturier->verbose("JOB: total shapping...");
	return (staples_per_unit + fold_per_unit + cut_per_unit) * copy * shaping_price;
}

double Risography::total_imposition() {
	return imposition_amount * imposition_price;
}

void Risography::calc_quantity() {

	facturier->verbose("calculating quantity...");
	masters_quantity = layersA + layersB;
	int printedSheets = static_cast<int>(std::ceil(format * copy));
	printedSheets += masters_quantity * TEST_PRINT;
	if (paper.empty()) {
		sheet_quantity = masters_quantity * TEST_PRINT;
	} else {
		sheet_quantity = printedSheets;
	}
	run_quantity = printedSheets * masters_quantity;
    labor_amount = (2 + masters_quantity) * format;
}

double Risography::total_cost() 
{
    double totalJob = 0;

	totalJob += total_ink();
	totalJob += total_labor();
	totalJob += total_graphic();
	totalJob += total_shaping();
	totalJob += total_paper();
	totalJob += total_master();
	totalJob += total_imposition();

	if (copy > 0)
		unit_cost = totalJob / copy;
	return totalJob;
}


double Risography::rest_to_pay() {
	total = total_cost() - total_discount() + total_shipping();
	return total;
}

std::string Risography::generate_stream()
{

	std::stringstream ss;
    std::string colorVerso =
        (layersB > 0) ? " - " + std::to_string(layersB) + "/verso" : "";
    std::string colorStr = std::to_string(layersA) + "/recto" + colorVerso;
    std::string copyStr = std::to_string(copy);
    int         offset = 68 - 14 - 22 - 1;
    ss << "    Intitulé: " << description << end_str(22, offset - description.length());
    ss << "    Couleurs: " << colorStr << end_str(22, offset - colorStr.length());
    ss << "    Copies  : " << copyStr << end_str(22, offset - copyStr.length());
    ss << "    Papier  : " << paper << end_str(22, offset - paper.length());

    std::string feesStr = (total_shipping() > 0.0) ? fit(total_shipping(), 8) : fit("--", 8);
	std::string totalDiscountStr;

	if (total_discount() > 0.0) {
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(2) << total_discount();
		totalDiscountStr = oss.str();
	} else {
		totalDiscountStr = "--";
	}

	std::string discountStr;
	if (discount_percentage > 0.0) {
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(2) << discount_percentage;
		discountStr = oss.str();
	} else {
		discountStr = "--";
	}

    ss << "    |---------------------------------------|" + end_str();
    ss << "    |    désignation    | quantité |  prix  |" + end_str();
    ss << "    |===================|==========|========|" + end_str();
	ss << new_line("feuilles A3", sheet_quantity, total_paper());
	ss << new_line("masters", masters_quantity, total_master());
	ss << new_line("encre (passages)", sheet_quantity * masters_quantity, total_ink());
	ss << new_line("plis/coupe/agrafe", staple_quantity + fold_per_unit, total_shaping());
    ss << "    |---------------------------------------|" << end_str();
	ss << new_line("impression", labor_amount, total_labor());
	ss << new_line("graphisme", graphic_amount, total_graphic());
	ss << new_line("imposition", imposition_amount, total_imposition(), end_str());
    ss << "    |---------------------------------------|" + end_str();
	ss << new_line("remise (%)", discountStr, totalDiscountStr);
	ss << new_line("port (kg)", weight, feesStr);
    ss << "    |=======================================|" + end_str();
    ss << "    |sous total (ttc)" + fit(total, 22) + "€" + "|" + end_str();
    ss << "    |=======================================|" + end_str();
    ss << "    |" + fit(unit_cost, 32) + "€/copie|" + end_str() + "\n";

	return ss.str();
}

// // Copy constructor
// Risography::Risography(const Risography &other) {(void) other;}
//
// // Assignment operator overload
// Risography &Risography::operator=(const Risography &other) {(void) other; return (*this);}

// Destructor
Risography::~Risography(void) {
}

