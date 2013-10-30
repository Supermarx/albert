#pragma once

#include <functional>

#include <supermarx/product.hpp>

#include "util/downloader.hpp"


namespace supermarx
{
	class scraper
	{
	public:
		using callback_t = std::function<void(const Product&)>;

	private:
		callback_t callback;
		downloader dl;

	public:
		scraper(callback_t callback_);
		scraper(scraper&) = delete;
		void operator=(scraper&) = delete;

		void scrape();
	};
}
