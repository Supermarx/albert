#include "scraper.hpp"

#include <iostream>
#include <stack>
#include <fstream>
#include <boost/locale.hpp>

#include "parsers/category_listing_parser.hpp"

namespace supermarx
{
	scraper::scraper(callback_t callback_)
	: callback(callback_)
	, dl("supermarx albert/1.0")
	{}

	void scraper::scrape()
	{
		struct stack_e
		{
			std::string url;
			int offset;

			stack_e(std::string url_, int offset_)
			: url(url_)
			, offset(offset_)
			{}
		};

		std::stack<stack_e> stack;
		stack.emplace("http://www.ah.nl/appie/producten", 0);

		size_t i = 0;
		while(!stack.empty())
		{
			const stack_e top = stack.top();
			stack.pop();

			category_listing_parser cat_parser(
				[&](const category_listing_parser::category_crumb_t& crumb)
				{
					stack.emplace(top.url + "/" + crumb, 0);
				},
				[&](int offset)
				{
					stack.emplace(top.url, offset);
				},
				callback
			);

			const std::string url = top.url+"?offset="+boost::lexical_cast<std::string>(top.offset);
			std::cout << url << std::endl;

			//Albert Heijn uses iso88591 unfortunately
			cat_parser.parse(boost::locale::conv::to_utf<char>(dl.fetch(url), "iso88591"));
			i++;
		}
	}
}
