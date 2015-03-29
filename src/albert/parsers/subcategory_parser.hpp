#pragma once

#include <functional>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/optional.hpp>

#include "../util/html_parser.hpp"
#include "../util/html_watcher.hpp"
#include "../util/html_recorder.hpp"
#include "../util/util.hpp"

namespace supermarx
{
	class subcategory_parser : public html_parser::default_handler
	{
	public:
		typedef std::string subcategory_uri_t;
		typedef std::function<void(subcategory_uri_t)> subcategory_callback_t;

	private:
		enum state_e {
			S_INIT,
			S_SUBCATEGORIES,
			S_DONE
		};

		subcategory_callback_t subcategory_callback;

		html_watcher_collection wc;

		state_e state;

	public:
		subcategory_parser(subcategory_callback_t subcategory_callback_)
		: subcategory_callback(subcategory_callback_)
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
			wc.startElement();

			switch(state)
			{
			case S_INIT:
				if(atts.getValue("data-appie") == "subcategorynav")
				{
					state = S_SUBCATEGORIES;

					wc.add([&]() {
						state = S_DONE;
					});
				}
			break;
			case S_SUBCATEGORIES:
				if(qName == "a")
					subcategory_callback(atts.getValue("href"));
			break;
			case S_DONE:
			break;
			}
		}

		virtual void endElement(const std::string& /* namespaceURI */, const std::string& /* localName */, const std::string& /* qName */)
		{
			wc.endElement();
		}
	};
}
