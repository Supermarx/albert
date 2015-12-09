#include <albert/scraper.hpp>

#include <iostream>
#include <deque>
#include <fstream>
#include <boost/locale.hpp>

#include <albert/util.hpp>
#include <albert/interpreter.hpp>

namespace supermarx
{
	static const std::string domain_uri = "http://www.ah.nl";
	static const std::string rest_uri = domain_uri + "/service/rest";

	scraper::scraper(product_callback_t _product_callback, tag_hierarchy_callback_t _tag_hierarchy_callback, unsigned int _ratelimit, bool _cache, bool _register_tags)
	: product_callback(_product_callback)
	, tag_hierarchy_callback(_tag_hierarchy_callback)
	, dl("supermarx albert/1.2", _ratelimit, _cache ? boost::optional<std::string>("./cache") : boost::none)
	, m(dl)
	, register_tags(_register_tags)
	, todo()
	, blacklist()
	, tag_hierarchy()
	{}

	Json::Value scraper::parse(std::string const& uri, std::string const& body)
	{
		Json::Value root;
		Json::Reader reader;

		if(!reader.parse(body, root, false))
		{
			dl.clear(uri);
			throw std::runtime_error("Could not parse json feed");
		}

		return root;
	}

	Json::Value scraper::download(std::string const& uri)
	{
		return stubborn::attempt<Json::Value>([&](){
			return parse(uri, dl.fetch(uri).body);
		});
	}

	bool scraper::is_blacklisted(std::string const& uri)
	{
		if(!blacklist.insert(uri).second) // Already visited uri
			return true;

		if(uri.find("%2F") != std::string::npos) // Bug in AH server, will get 404
			return true;

		if(uri.find("merk=100%") != std::string::npos) // Ditto
			return true;

		if(!boost::algorithm::starts_with(uri, rest_uri + "/producten/"))
			return true;

		return false;
	}

	void scraper::process(const page_t &current_page, const downloader::response &response)
	{
		/* Two modes:
		 * - register_tags, fetches all products via the "Filters", goes through them in-order.
		 * - !register_tags, fetches all products via inline ProductLanes and SeeMore widgets
		 * The first takes an order more time than the second.
		 * The first actually maps all categories and tags, which do not change often.
		 * I suggest you run a scrape with this tag at most once a week.
		 * Use sparingly.
		 */
		Json::Value cat_root(parse(current_page.uri, response.body));
		for(auto const& lane : cat_root["_embedded"]["lanes"])
		{
			if(register_tags && lane["id"].asString() == "Filters" && current_page.expand)
			{
				// Only process filters exhaustively if register_tags is enabled
				parse_filterlane(lane, current_page);
			}
			else if(lane["type"].asString() == "ProductLane")
			{
				parse_productlane(lane, current_page);
			}
			else if(!register_tags && lane["type"].asString() == "SeeMoreLane")
			{
				parse_seemorelane(lane, current_page);
			}
		}
	}

	void scraper::add_tag_to_hierarchy_f(std::vector<message::tag> const& _parent_tags, message::tag const& current_tag)
	{
		if(current_tag.category != "Soort")
			return; // Other categories do not possess an hierarchy.

		std::vector<message::tag> parent_tags(_parent_tags.size());
		std::reverse_copy(std::begin(_parent_tags), std::end(_parent_tags), std::begin(parent_tags));
		auto parent_it = std::find_if(std::begin(parent_tags), std::end(parent_tags), [&](message::tag const& t){
			return t.category == current_tag.category;
		});

		if(parent_it == std::end(parent_tags))
			return; // Do not add

		message::tag const& parent_tag = *parent_it;

		auto hierarchy_it(tag_hierarchy.find(parent_tag));
		if(hierarchy_it == std::end(tag_hierarchy))
			tag_hierarchy.emplace(parent_tag, std::set<message::tag>({ current_tag }));
		else
		{
			if(!hierarchy_it->second.emplace(current_tag).second)
				return;
		}

		// Emit hierarchy finding
		tag_hierarchy_callback(parent_tag, current_tag);
	}

	void scraper::parse_filterlane(Json::Value const& lane, page_t const& current_page)
	{
		for(auto const& filter_bar : lane["_embedded"]["items"])
		{
			if(filter_bar["resourceType"].asString() != "FilterBar")
				std::runtime_error("Unexpected element under Filters");

			for(auto const& filter : filter_bar["_embedded"]["filters"])
			{
				std::string filter_label = filter["label"].asString();
				for(auto const& cat : filter["_embedded"]["filterItems"])
				{
					std::string uri = cat["navItem"]["link"]["href"].asString();
					std::string title = cat["label"].asString();
					remove_hyphens(title);

					message::tag tag({title, filter_label});
					add_tag_to_hierarchy_f(current_page.tags, tag);

					std::vector<message::tag> tags;
					tags.insert(tags.end(), current_page.tags.begin(), current_page.tags.end());
					tags.push_back(tag);

					std::string new_uri = rest_uri + uri;
					page_t new_page = {new_uri, title, true, tags};

					if(is_blacklisted(new_uri))
						continue;

					m.schedule(std::move(new_uri), [&, new_page](downloader::response const& response) {
						process(new_page, response);
					});
				}

				break; // Only process first in bar, until none are left. (will fetch everything eventually)
			}
		}
	}

	void scraper::parse_productlane(Json::Value const& lane, page_t const& current_page)
	{
		std::string lane_name = lane["id"].asString();

		std::vector<message::tag> tags;
		tags.insert(tags.end(), current_page.tags.begin(), current_page.tags.end());
		tags.push_back({lane_name, std::string("Soort")});

		for(auto const& lane_item : lane["_embedded"]["items"])
		{
			std::string lane_type = lane_item["type"].asString();
			if(lane_type == "Product")
			{
				auto const& product = lane_item["_embedded"]["productCard"]["_embedded"]["product"];
				interpreter::interpret(product, current_page, product_callback);
			}
			else if(!register_tags && lane_type == "SeeMore")
			{
				if(lane_item["navItem"]["link"]["pageType"] == "legacy") // Old-style page in SeeMore Editorial
					continue;

				std::string new_uri = rest_uri + lane_item["navItem"]["link"]["href"].asString();
				page_t new_page = {new_uri, lane_name, true, tags};

				if(is_blacklisted(new_uri))
					continue;

				m.schedule(std::move(new_uri), [&, new_page](downloader::response const& response) {
					process(new_page, response);
				});
			}
		}
	}

	void scraper::parse_seemorelane(Json::Value const& lane, page_t const& current_page)
	{
		for(auto const& lane_item : lane["_embedded"]["items"])
		{
			std::string title = lane_item["text"]["title"].asString();
			remove_hyphens(title);
			std::string uri = lane_item["navItem"]["link"]["href"].asString();

			std::vector<message::tag> tags;
			tags.insert(tags.end(), current_page.tags.begin(), current_page.tags.end());
			tags.push_back({title, std::string("Soort")});

			std::string new_uri = rest_uri + uri;
			page_t new_page = {new_uri, title, true, tags};

			if(is_blacklisted(new_uri))
				continue;

			m.schedule(std::move(new_uri), [&, new_page](downloader::response const& response) {
				process(new_page, response);
			});
		}
	}

	void scraper::scrape()
	{
		Json::Value producten_root(download(rest_uri + "/producten"));

		for(auto const& lane : producten_root["_embedded"]["lanes"])
		{
			if(lane["id"].asString() != "productCategoryNavigation")
				continue;

			for(auto const& cat : lane["_embedded"]["items"])
			{
				std::string title = cat["title"].asString();
				std::string uri = cat["navItem"]["link"]["href"].asString();

				remove_hyphens(title);

				std::string new_uri = rest_uri + uri;
				page_t new_page = {new_uri, title, true, {message::tag{title, std::string("Soort")}}};

				m.schedule(std::move(new_uri), [&, new_page](downloader::response const& response) {
					process(new_page, response);
				});
			}
		}

		m.process_all();
	}

	raw scraper::download_image(const std::string& uri)
	{
		std::string buf(dl.fetch(uri).body);
		return raw(buf.data(), buf.length());
	}
}
