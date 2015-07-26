#include <albert/scraper.hpp>

#include <iostream>
#include <deque>
#include <fstream>
#include <boost/locale.hpp>

#include <jsoncpp/json/json.h>

#include <supermarx/util/stubborn.hpp>

#include <albert/page.hpp>
#include <albert/util.hpp>
#include <albert/interpreter.hpp>

namespace supermarx
{
	Json::Value parse_json(std::string const& src)
	{
		Json::Value root;
		Json::Reader reader;

		if(!reader.parse(src, root, false))
			throw std::runtime_error("Could not parse json feed");

		return root;
	}

	scraper::scraper(callback_t _callback, unsigned int _ratelimit, bool _cache, bool _register_tags)
	: callback(_callback)
	, dl("supermarx albert/1.1", _ratelimit, _cache ? boost::optional<std::string>("./cache") : boost::none)
	, register_tags(_register_tags)
	{}

	void scraper::scrape()
	{
		auto dl_f([&](std::string const& uri) -> std::string {
			return stubborn::attempt<std::string>([&](){
				return dl.fetch(uri).body;
			});
		});

		static const std::string domain_uri = "http://www.ah.nl";
		static const std::string rest_uri = domain_uri + "/service/rest";
		std::deque<page_t> todo;
		std::set<std::string> blacklist;

		{
			Json::Value producten_root(parse_json(dl_f(rest_uri + "/producten")));

			for(auto const& lane : producten_root["_embedded"]["lanes"])
			{
				if(lane["id"].asString() != "productCategoryNavigation")
					continue;

				for(auto const& cat : lane["_embedded"]["items"])
				{
					std::string title = cat["title"].asString();
					std::string uri = cat["navItem"]["link"]["href"].asString();

					remove_hyphens(title);

					todo.push_back({rest_uri + uri, title, true, {message::tag{title, std::string("Soort")}}});
				}
			}
		}

		while(!todo.empty())
		{
			page_t current_page(todo.front());
			todo.pop_front();

			if(!blacklist.insert(current_page.uri).second) // Already visited uri
				continue;

			//if(current_page.uri.find('/') != std::string::npos) // Bug in AH server, will get 404
			//	continue;

			if(current_page.uri.find("merk=100%") != std::string::npos) // Ditto
				continue;

			/* Two modes:
			 * - register_tags, fetches all products via the "Filters", goes through them in-order.
			 * - !register_tags, fetches all products via inline ProductLanes and SeeMore widgets
			 * The first takes an order more time than the second.
			 * The first actually maps all categories and tags, which do not change often.
			 * I suggest you run a scrape with this tag at most once a week.
			 * Use sparingly.
			 */
			Json::Value cat_root(parse_json(dl_f(current_page.uri)));
			for(auto const& lane : cat_root["_embedded"]["lanes"])
			{
				if(register_tags && lane["id"].asString() == "Filters" && current_page.expand)
				{
					// Only process filters exhaustively if register_tags is enabled
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

								std::vector<message::tag> tags;
								tags.insert(tags.end(), current_page.tags.begin(), current_page.tags.end());
								tags.push_back({title, filter_label});

								todo.push_front({rest_uri + "/" + uri, title, true, tags});
							}

							break; // Only process first in bar, until none are left. (will fetch everything eventually)
						}
					}
				}
				else if(lane["type"].asString() == "ProductLane")
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
							interpreter::interpret(product, current_page, callback);
						}
						else if(!register_tags && lane_type == "SeeMore")
						{
							if(lane_item["navItem"]["link"]["pageType"] == "legacy") // Old-style page in SeeMore Editorial
								continue;

							std::string uri = lane_item["navItem"]["link"]["href"].asString();
							todo.push_front({rest_uri + "/" + uri, lane_name, true, tags});
						}
					}
				}
				else if(!register_tags && lane["type"].asString() == "SeeMoreLane")
				{
					for(auto const& lane_item : lane["_embedded"]["items"])
					{
						std::string title = lane_item["text"]["title"].asString();
						remove_hyphens(title);
						std::string uri = lane_item["navItem"]["link"]["href"].asString();

						std::vector<message::tag> tags;
						tags.insert(tags.end(), current_page.tags.begin(), current_page.tags.end());
						tags.push_back({title, std::string("Soort")});

						todo.push_front({rest_uri + "/" + uri, title, true, tags});
					}
				}
			}
		}
	}

	raw scraper::download_image(const std::string& uri)
	{
		std::string buf(dl.fetch(uri).body);
		return raw(buf.data(), buf.length());
	}
}
