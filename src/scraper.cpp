#include "scraper.hpp"

#include <iostream>

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
		std::cout << dl.fetch("http://www.ah.nl/appie/producten") << std::endl;
	}
}
