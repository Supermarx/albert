#include <albert/scraper.hpp>

#include <iostream>
#include <deque>
#include <fstream>
#include <boost/locale.hpp>

#include <albert/parsers/category_parser.hpp>
#include <albert/parsers/subcategory_parser.hpp>
#include <albert/parsers/product_parser.hpp>

#include <supermarx/util/stubborn.hpp>

namespace supermarx
{
	scraper::scraper(callback_t _callback, unsigned int _ratelimit)
	: callback(_callback)
	, dl("supermarx albert/1.0", _ratelimit)
	{}

	void scraper::scrape()
	{
		static const std::string domain_uri = "http://www.ah.nl";
		std::deque<std::string> todo;

		category_parser c_p(
			[&](const category_parser::category_uri_t c)
		{
			todo.emplace_back(domain_uri + c);
		});

		c_p.parse(
			stubborn::attempt<std::string>([&](){
				return dl.fetch(domain_uri + "/producten");
			})
		);

		while(!todo.empty())
		{
			subcategory_parser sc_p(
				[&](const subcategory_parser::subcategory_uri_t sc)
			{
				todo.push_back(domain_uri + sc);
			});

			product_parser p_p(
			[&](const std::string uri)
			{
				todo.push_front(domain_uri + uri);
			},
			callback);

			std::string current_uri = todo.front();
			todo.pop_front();

			std::string src = stubborn::attempt<std::string>([&](){
				return dl.fetch(current_uri);
			});

			sc_p.parse(src);
			p_p.parse(src);
		}
	}
}
