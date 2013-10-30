#include "scraper.hpp"

#include <iostream>

#include "parsers/category_listing_parser.hpp"

namespace supermarx
{
	scraper::scraper(callback_t callback_)
	: callback(callback_)
	, dl("supermarx albert/1.0")
	{}

	void scraper::scrape()
	{
		product p;
		callback(p);

		category_listing_parser cat_parser([&](const category_listing_parser::category_crumb_t& crumb) { std::cout << crumb << std::endl; });
		cat_parser.parse(dl.fetch("http://www.ah.nl/appie/producten"));
	}
}
