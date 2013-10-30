#pragma once

#include <functional>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include "../util/html_parser.hpp"

namespace supermarx
{
	class category_listing_parser : public html_parser::default_handler
	{
	public:
		typedef std::string category_crumb_t;
		typedef std::function<void(category_crumb_t)> category_callback_t;

	private:
		category_callback_t callback;

	public:
		category_listing_parser(category_callback_t callback_)
		: callback(callback_)
		{}

		template<typename T>
		void parse(T source)
		{
			html_parser::parse(source, *this);
		}

		virtual void startElement(const std::string& /* namespaceURI */, const std::string& /* localName */, const std::string& qName, const AttributesT& atts)
		{
			static const boost::regex has_category_class("category");
			static const boost::regex match_url_breadcrumb(".*/([^/]*)");

			if(qName != "a")
				return;

			if(boost::regex_search(atts.getValue("class"), has_category_class))
			{
				boost::smatch what;
				if(boost::regex_match(atts.getValue("href"), what, match_url_breadcrumb))
					callback(what[1]);
			}
		}
	};
}
