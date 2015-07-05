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
	class horizontalfilter_parser : public html_parser::default_handler
	{
	public:
		struct horizontalfilter_info_t
		{
			std::string uri;
			std::string name;
			std::string tagcategory;
		};

		typedef std::function<void(horizontalfilter_info_t)> horizontalfilter_callback_t;

	private:
		enum state_e {
			S_INIT,
			S_FILTERCATEGORIES,
			S_FILTERCATEGORY,
			S_DONE
		};

		horizontalfilter_callback_t horizontalfilter_callback;

		boost::optional<html_recorder> rec;
		html_watcher_collection wc;

		state_e state;

		bool block_rec;
		boost::optional<std::string> tagcategory;

	public:
		horizontalfilter_parser(horizontalfilter_callback_t horizontalfilter_callback_)
		: horizontalfilter_callback(horizontalfilter_callback_)
		, rec()
		, wc()
		, state(S_INIT)
		, block_rec(false)
		, tagcategory()
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

			const std::string att_class = atts.getValue("class");

			switch(state)
			{
			case S_INIT:
				if(atts.getValue("data-appie") == "horizontalfilter")
				{
					state = S_FILTERCATEGORIES;

					wc.add([&]() {
						state = S_DONE;
					});
				}
			break;
			case S_FILTERCATEGORIES:
				if(util::contains_attr("hidden-category-items", att_class))
				{
					state = S_FILTERCATEGORY;

					tagcategory.reset(atts.getValue("data-category"));

					wc.add([&]() {
						state = S_FILTERCATEGORIES;
						tagcategory.reset();
					});
				}
			break;
			case S_FILTERCATEGORY:
				if(qName == "a" && util::contains_attr("filterbox-option-link", att_class))
				{
					std::string href = atts.getValue("href");
					rec = html_recorder([this, href](std::string ch) {
						horizontalfilter_callback(
							horizontalfilter_info_t{
								href,
								util::sanitize(ch),
								tagcategory.get()
							}
						);
					});
				}
				else if(qName == "span")
				{
					block_rec = true;
					wc.add([&]() {
						block_rec = false;
					});
				}
			break;
			case S_DONE:
			break;
			}
		}

		virtual void characters(const std::string& ch)
		{
			if(rec && !block_rec)
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
