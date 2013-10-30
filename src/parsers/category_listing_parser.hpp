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

#include "../product.hpp"

namespace supermarx
{
	class category_listing_parser : public html_parser::default_handler
	{
	public:
		typedef std::string category_crumb_t;
		typedef std::function<void(category_crumb_t)> category_callback_t;
		typedef std::function<void(product)> product_callback_t;

	private:
		enum state_e {
			S_INIT,
			S_PRODUCT,
			S_PRODUCT_PRICE
		};

		struct product_proto
		{
			std::string name, price, old_price;
		};

		category_callback_t category_callback;
		product_callback_t product_callback;

		boost::optional<html_recorder> rec;
		html_watcher_collection wc;

		state_e state;
		product_proto current_p;

		static unsigned int parse_price(const std::string& str)
		{
			static const boost::regex match_price("([0-9]*)\\.([0-9]*)");

			boost::smatch what;
			if(!boost::regex_match(str, what, match_price))
				throw std::runtime_error("Could not parse price " + str);

			return boost::lexical_cast<unsigned int>(what[1])*100 + boost::lexical_cast<unsigned int>(what[2]);
		}

	public:
		category_listing_parser(category_callback_t category_callback_, product_callback_t product_callback_)
		: category_callback(category_callback_)
		, product_callback(product_callback_)
		, rec()
		, wc()
		, state(S_INIT)
		, current_p()
		{}

		template<typename T>
		void parse(T source)
		{
			html_parser::parse(source, *this);
		}

		virtual void startElement(const std::string& /* namespaceURI */, const std::string& /* localName */, const std::string& qName, const AttributesT& atts)
		{
			static const boost::regex has_canvas_card_class("\\bcanvas_card\\b");
			static const boost::regex has_category_class("\\bcategory\\b");
			static const boost::regex has_product_class("\\bproduct\\b");

			static const boost::regex has_detail_class("\\bdetail\\b");
			static const boost::regex has_price_class("\\bprice\\b");

			static const boost::regex match_url_breadcrumb(".*/([^/]*)");

			if(rec)
				rec.get().startElement();

			wc.startElement();

			const std::string att_class = atts.getValue("class");

			switch(state)
			{
			case S_INIT:
				if(boost::regex_search(att_class, has_category_class) && boost::regex_search(att_class, has_canvas_card_class))
				{
					boost::smatch what;
					if(boost::regex_match(atts.getValue("href"), what, match_url_breadcrumb))
						category_callback(what[1]);
				}
				else if(boost::regex_search(att_class, has_product_class) && boost::regex_search(att_class, has_canvas_card_class))
				{
					//Reset product
					current_p = product_proto();

					state = S_PRODUCT;
					wc.add([&]() {
						state = S_INIT;

						product_callback(product{
							current_p.name,
							parse_price(current_p.price)
						});
					});
				}
			break;
			case S_PRODUCT:
				if(boost::regex_search(att_class, has_detail_class))
					rec = html_recorder([&](std::string ch) { current_p.name = util::sanitize(ch); });
				else if(boost::regex_search(att_class, has_price_class))
				{
					state = S_PRODUCT_PRICE;
					wc.add([&]() { state = S_PRODUCT; });
				}
			break;
			case S_PRODUCT_PRICE:
				if(qName == "del")
					rec = html_recorder([&](std::string ch) { current_p.old_price = util::sanitize(ch); });
				else if(qName == "ins")
					rec = html_recorder([&](std::string ch) { current_p.price = util::sanitize(ch); });
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
