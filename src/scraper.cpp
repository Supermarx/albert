#include "scraper.hpp"

#include <iostream>
#include <stack>

#include "parsers/category_listing_parser.hpp"

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

		std::stack<std::string> stack;
		stack.emplace("http://www.ah.nl/appie/producten");

		while(!stack.empty())
		{
			const std::string url = stack.top();
			stack.pop();

			category_listing_parser cat_parser(
				[&](const category_listing_parser::category_crumb_t& crumb)
				{
					stack.emplace(url + "/" + crumb);
				}
			);

			std::cout << url << std::endl;
			cat_parser.parse(dl.fetch(url));
		}
	}
}
