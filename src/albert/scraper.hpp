#pragma once

#include <functional>

#include <supermarx/product.hpp>
#include <supermarx/raw.hpp>
#include <supermarx/util/downloader.hpp>

namespace supermarx
{
	class scraper
	{
	public:
		using problems_t = std::vector<std::string>;
		using callback_t = std::function<void(const std::string&, boost::optional<std::string> const&, const product&, datetime, confidence, problems_t)>;

	private:
		callback_t callback;
		downloader dl;

	public:
		scraper(callback_t _callback, unsigned int ratelimit = 5000);
		scraper(scraper&) = delete;
		void operator=(scraper&) = delete;

		void scrape();
		raw download_image(const std::string& uri);
	};
}
