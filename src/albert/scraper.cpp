#include "scraper.hpp"

#include <iostream>
#include <deque>
#include <fstream>
#include <boost/locale.hpp>

#include "parsers/category_parser.hpp"
#include "parsers/subcategory_parser.hpp"
#include "parsers/product_parser.hpp"

namespace supermarx
{
	scraper::scraper(callback_t callback_)
	: callback(callback_)
	, dl("supermarx albert/1.0", 5000) // Rate limit of 5 seconds
	{}

	void scraper::scrape()
	{
		static const std::string domain_uri = "http://www.ah.nl";
		std::deque<std::string> todo;

		category_parser c_p(
			[&](const category_parser::category_uri_t c)
		{
			subcategory_parser sc_p(
				[&](const subcategory_parser::subcategory_uri_t sc)
			{
				todo.push_back(sc);
			});

			sc_p.parse(dl.fetch(domain_uri + c));
		});

		c_p.parse(dl.fetch(domain_uri + "/producten"));

		while(!todo.empty())
		{
			std::string current_uri = todo.front();
			todo.pop_front();

			product_parser p_p(
			[&](const std::string uri)
			{
				todo.push_front(uri);
			},
			callback);

			p_p.parse(dl.fetch(domain_uri + current_uri));
		}
	}
}
