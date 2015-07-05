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
		struct category_t
		{
			std::string uri;
			std::string name;
		};

		static const std::string domain_uri = "http://www.ah.nl";
		std::deque<category_t> todo;

		category_parser c_p(
			[&](const category_parser::category_info_t c)
		{
			todo.push_back({domain_uri + c.uri, c.name});
		});

		c_p.parse(
			stubborn::attempt<std::string>([&](){
				return dl.fetch(domain_uri + "/producten").body;
			})
		);

		while(!todo.empty())
		{
			category_t current_cat(todo.front());
			todo.pop_front();

			subcategory_parser sc_p(
				[&](const subcategory_parser::subcategory_info_t sc)
			{
				todo.push_back({domain_uri + sc.uri, sc.name});
			});

			product_parser p_p(
			[&](const std::string uri)
			{
				todo.push_back({domain_uri + uri, current_cat.name});
			},
			[&](const message::product_base& p, boost::optional<std::string> const& image_uri, datetime retrieved_on, confidence conf, problems_t probs)
			{
				callback(
					current_cat.uri,
					image_uri,
					p,
					{message::tag{current_cat.name, std::string("category")}},
					retrieved_on,
					conf,
					probs
				);
			});

			std::string src = stubborn::attempt<std::string>([&](){
				return dl.fetch(current_cat.uri).body;
			});

			sc_p.parse(src);
			p_p.parse(src);
		}
	}

	raw scraper::download_image(const std::string& uri)
	{
		std::string buf(dl.fetch(uri).body);
		return raw(buf.data(), buf.length());
	}
}
