#pragma once

#include <jsoncpp/json/json.h>

#include <supermarx/raw.hpp>
#include <supermarx/util/cached_downloader.hpp>
#include <supermarx/util/download_manager.hpp>

#include <supermarx/scraper/scraper_prototype.hpp>

#include <albert/page.hpp>

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
		download_manager m;

		bool register_tags;

		std::deque<page_t> todo;
		std::set<std::string> blacklist;

		std::map<message::tag, std::set<message::tag>> tag_hierarchy; // tag_hierarchy[parent] = children_set;
		size_t product_count, page_count, error_count;

		bool is_blacklisted(std::string const& uri);

		void order(page_t const& p);
		Json::Value parse(std::string const& uri, std::string const& body);
		Json::Value download(std::string const& uri);

		void process(page_t const& current_page, downloader::response const & response);

		void add_tag_to_hierarchy_f(std::vector<message::tag> const& _parent_tags, message::tag const& current_tag);

		void parse_filterlane(Json::Value const& lane, page_t const& current_page);
		void parse_productlane(Json::Value const& lane, page_t const& current_page);
		void parse_seemorelane(Json::Value const& lane, page_t const& current_page);

	public:
		scraper(product_callback_t _product_callback, tag_hierarchy_callback_t _tag_hierarchy_callback, unsigned int ratelimit = 5000, bool cache = false, bool register_tags = false);
		scraper(scraper&) = delete;
		void operator=(scraper&) = delete;

		void scrape();
		raw download_image(const std::string& uri);
	};
}
