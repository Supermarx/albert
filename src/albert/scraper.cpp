#include <albert/scraper.hpp>

#include <iostream>
#include <deque>
#include <fstream>
#include <boost/locale.hpp>

#include <albert/parsers/category_parser.hpp>
#include <albert/parsers/subcategory_parser.hpp>
#include <albert/parsers/product_parser.hpp>
#include <albert/parsers/horizontalfilter_parser.hpp>

#include <supermarx/util/stubborn.hpp>

namespace supermarx
{
	scraper::scraper(callback_t _callback, unsigned int _ratelimit, bool _cache, bool _register_tags)
	: callback(_callback)
	, dl("supermarx albert/1.0", _ratelimit, _cache ? boost::optional<std::string>("./cache") : boost::none)
	, register_tags(_register_tags)
	{}

	void scraper::scrape()
	{
		struct page_t
		{
			std::string uri;
			std::string name;
			std::string tagcategory;
			bool expand;
		};

		static const std::string domain_uri = "http://www.ah.nl";
		std::deque<page_t> todo;

		category_parser c_p(
			[&](const category_parser::category_info_t c)
		{
			todo.push_back({domain_uri + c.uri, c.name, "category", true});
		});

		c_p.parse(
			stubborn::attempt<std::string>([&](){
				return dl.fetch(domain_uri + "/producten").body;
			})
		);

		while(!todo.empty())
		{
			page_t current_page(todo.front());
			todo.pop_front();

			subcategory_parser sc_p(
				[&](const subcategory_parser::subcategory_info_t sc)
			{
				todo.push_back({domain_uri + sc.uri, sc.name, "category", true});
			});

			std::vector<message::tag> hf_tags;
			horizontalfilter_parser hf_p(
				[&](const horizontalfilter_parser::horizontalfilter_info_t hf)
			{
				todo.push_back({domain_uri + hf.uri, hf.name, hf.tagcategory, false});
				hf_tags.push_back({hf.name, hf.tagcategory});
			});

			product_parser p_p(
			[&](const std::string uri)
			{
				// "More products"-button
				todo.push_back({domain_uri + uri, current_page.name, current_page.tagcategory, false});
			},
			[&](const message::product_base& p, boost::optional<std::string> const& image_uri, datetime retrieved_on, confidence conf, problems_t probs)
			{
				std::vector<message::tag> tags;
				tags.push_back({current_page.name, current_page.tagcategory});
				tags.insert(tags.begin(), hf_tags.begin(), hf_tags.end());
				callback(
					current_page.uri,
					image_uri,
					p,
					tags,
					retrieved_on,
					conf,
					probs
				);
			});

			try
			{
				std::string src = stubborn::attempt<std::string, downloader::error>([&](){
					return dl.fetch(current_page.uri).body;
				});

				sc_p.parse(src);
				p_p.parse(src);

				if(register_tags && current_page.expand)
					hf_p.parse(src);
			} catch(downloader::error e)
			{
				std::cerr << "Downloader error (continuing): " << e.what() << std::endl;
			}
		}
	}

	raw scraper::download_image(const std::string& uri)
	{
		std::string buf(dl.fetch(uri).body);
		return raw(buf.data(), buf.length());
	}
}
