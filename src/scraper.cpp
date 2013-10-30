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
		product appleflap{"Appleflaps", 2000};
		callback(appleflap);

		product mudcrab{"Mudcrab Sticks", 1337};
		callback(mudcrab);

		std::cout << dl.fetch("http://www.ah.nl/appie/producten") << std::endl;
	}
}
