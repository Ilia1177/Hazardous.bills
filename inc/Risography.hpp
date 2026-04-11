#ifndef RISOGRAPHY_HPP
# define RISOGRAPHY_HPP
# include <iostream>
# include <iomanip>
# include <fstream>
# include "PrintFact.hpp"
# include <sstream>       // ← missing, needed for ostringstream

class Facturier;

class Risography
{
    public:
        ~Risography();
        Risography(Facturier* facturier);
		Risography(		std::string desc,
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
						bool shipping_fee,
						double price_ink,
						double price_paper,
						double price_master,
						double price_graphism,
						double price_shaping,
						double price_imposition,
						double price_labor,
						int sheet_weight
						);
		// Risography(const Risography& other) = default;
		//       Risography &operator=(const Risography &other) = default;


		std::string generate_stream();
		void calc_quantity();
		double total_imposition();
		double total_shipping();
		double total_cost();
		double total_discount();
		double total_paper();
		double total_master();
		double total_ink();
		double total_labor();
		double total_graphic();
		double total_shaping();
		double rest_to_pay();

		// value get from user:
		std::string description;
		std::string paper;
		double format;
		int copy;
		int layersA;
		int layersB;
		int staples_per_unit;
		int cut_per_unit;
		int fold_per_unit;
		double 	labor_amount;
		double  graphic_amount;
		double	imposition_amount;
		double	discount_percentage;
		double	shipping_fees;

		// price get from ini files:
		double ink_price;
		double sheet_price;
		double master_price;
		double graphic_price;
		double shaping_price;
		double imposition_price;
		double labor_price;
		int		sheet_weight;

		// calculating quantity
		int		sheet_quantity;
		int		staple_quantity;
		int		masters_quantity;
		int 	run_quantity;
		double	weight;

		double total;
		double unit_cost;

		Facturier* facturier;
	private:

};

#endif

