#pragma once

#include <supermarx/raw.hpp>
#include <supermarx/util/cached_downloader.hpp>

#include <supermarx/scraper/scraper_prototype.hpp>

namespace supermarx
{
	class scraper
	{
	public:
		using problems_t = scraper_prototype::problems_t;
		using product_callback_t = scraper_prototype::product_callback_t;
		using tag_hierarchy_callback_t = scraper_prototype::tag_hierarchy_callback_t;

	private:
		product_callback_t product_callback;
		tag_hierarchy_callback_t tag_hierarchy_callback;

		cached_downloader dl;
		bool register_tags;

	public:
		scraper(product_callback_t _product_callback, tag_hierarchy_callback_t _tag_hierarchy_callback, unsigned int ratelimit = 5000, bool cache = false, bool register_tags = false);
		scraper(scraper&) = delete;
		void operator=(scraper&) = delete;

		void scrape();
		raw download_image(const std::string& uri);
	};
}
