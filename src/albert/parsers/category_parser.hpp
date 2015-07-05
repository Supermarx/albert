#pragma once

#include <functional>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/optional.hpp>

#include <supermarx/scraper/html_parser.hpp>
#include <supermarx/scraper/html_watcher.hpp>
#include <supermarx/scraper/html_recorder.hpp>
#include <supermarx/scraper/util.hpp>

namespace supermarx
{
	class category_parser : public html_parser::default_handler
	{
	public:
		struct category_info_t
		{
			std::string uri;
			std::string name;
		};

		typedef std::function<void(category_info_t)> category_callback_t;

	private:
		enum state_e {
			S_INIT,
			S_CATEGORIES,
			S_DONE
		};

		category_callback_t category_callback;

		boost::optional<html_recorder> rec;
		html_watcher_collection wc;

		state_e state;

	public:
		category_parser(category_callback_t category_callback_)
		: category_callback(category_callback_)
		, rec()
		, wc()
		, state(S_INIT)
		{}

		template<typename T>
		void parse(T source)
		{
			html_parser::parse(source, *this);
		}

		virtual void startElement(const std::string& /* namespaceURI */, const std::string& /* localName */, const std::string& qName, const AttributesT& atts)
		{
			if(rec)
				rec.get().startElement();

			wc.startElement();

			switch(state)
			{
			case S_INIT:
				if(atts.getValue("data-class") == "category-navigation-items")
				{
					state = S_CATEGORIES;

					wc.add([&]() {
						state = S_DONE;
					});
				}
			break;
			case S_CATEGORIES:
				if(qName == "a")
				{
					std::string href = atts.getValue("href");
					rec = html_recorder([this, href](std::string ch) {
						category_callback(category_info_t{href, util::sanitize(ch)});
					});
				}
			break;
			case S_DONE:
			break;
			}
		}

		virtual void characters(const std::string& ch)
		{
			if(rec)
				rec.get().characters(ch);
		}

		virtual void endElement(const std::string& /* namespaceURI */, const std::string& /* localName */, const std::string& /* qName */)
		{
			if(rec && rec.get().endElement())
				rec = boost::none;

			wc.endElement();
		}
	};
}
