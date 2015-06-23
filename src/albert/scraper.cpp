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
	scraper::scraper(callback_t _callback, unsigned int _ratelimit, bool _cache)
	: callback(_callback)
	, dl("supermarx albert/1.0", _ratelimit, _cache ? boost::optional<std::string>("./cache") : boost::none)
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
			std::string current_uri = todo.front();
			todo.pop_front();

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
			[&](const message::product_base& p, boost::optional<std::string> const& image_uri, datetime retrieved_on, confidence conf, problems_t probs)
			{
				callback(
					current_uri,
					image_uri,
					p,
					retrieved_on,
					conf,
					probs
				);
			});

			std::string src = stubborn::attempt<std::string>([&](){
				return dl.fetch(current_uri);
			});

			sc_p.parse(src);
			p_p.parse(src);
		}
	}

	raw scraper::download_image(const std::string& uri)
	{
		std::string buf(dl.fetch(uri));
		return raw(buf.data(), buf.length());
	}
}
