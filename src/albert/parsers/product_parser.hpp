#pragma once

#include <functional>
#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/optional.hpp>

#include <supermarx/product.hpp>

#include <supermarx/scraper/html_parser.hpp>
#include <supermarx/scraper/html_watcher.hpp>
#include <supermarx/scraper/html_recorder.hpp>
#include <supermarx/scraper/util.hpp>

namespace supermarx
{
	class product_parser : public html_parser::default_handler
	{
	public:
		typedef std::function<void(const product&, datetime, confidence)> product_callback_t;
		typedef std::function<void(std::string)> more_callback_t;

	private:
		enum state_e {
			S_INIT,
			S_PRODUCT,
			S_PRODUCT_NAME,
			S_PRODUCT_PRICE
		};

		more_callback_t more_callback;
		product_callback_t product_callback;

		boost::optional<html_recorder> rec;
		html_watcher_collection wc;

		state_e state;

		struct product_proto
		{
			std::string identifier, name;
			boost::optional<unsigned int> del_price, ins_price;
			boost::optional<std::string> shield, subtext;
			boost::optional<date> valid_on;
		};

		product_proto current_p;

		static unsigned int parse_price(const std::string& str)
		{
			static const boost::regex match_price("([0-9]*)\\.([0-9]*)");

			boost::smatch what;
			if(!boost::regex_match(str, what, match_price))
				throw std::runtime_error("Could not parse price " + str);

			return boost::lexical_cast<float>(what[1])*100 + boost::lexical_cast<unsigned int>(what[2]);
		}

		void deliver_product()
		{
			confidence conf = confidence::NEUTRAL;

			if(current_p.subtext)
			{
				std::string subtext = current_p.subtext.get();

				static const boost::regex match_deadline("t/m [0-9]+ [a-z]{3}");
				boost::smatch what;

				if(
					subtext == "dit product wordt ah biologisch" ||
					subtext == "bestel 2 dagen van tevoren" ||
					subtext == "nieuw in het assortiment" ||
					subtext == "belgisch assortiment in nl" ||
					subtext == "alleen online" ||
					subtext == "online aanbieding" ||
					boost::regex_match(subtext, what, match_deadline)
				)
				{
					/* No relevant data for us, ignore */
				}
				else if(
					subtext == "binnenkort verkrijgbaar"
				)
				{
					return; /* Product not yet available */
				}
				else
				{
					std::cerr << '[' << current_p.identifier << "] " << current_p.name << std::endl;
					std::cerr << "subtext: \'" << current_p.subtext.get() << '\'' << std::endl;
					conf = confidence::LOW;
				}
			}

			unsigned int orig_price, price;
			condition discount_condition = condition::ALWAYS;

			orig_price = current_p.del_price ? current_p.del_price.get() : current_p.ins_price.get();

			if(!current_p.ins_price)
			{
				std::cerr << "product without a price, ignore" << std::endl;
				return;
			}

			price = current_p.ins_price.get();

			if(current_p.shield)
			{
				std::string shield = current_p.shield.get();

				static const boost::regex match_percent_discount("([0-9]+)% korting");
				static const boost::regex match_euro_discount("([0-9]+) euro korting");
				static const boost::regex match_eurocent_discount("([0-9]+\\.[0-9]+) euro korting");

				static const boost::regex match_combination_discount("([0-9]+) voor (&euro; )?([0-9]+\\.[0-9]+)");
				boost::smatch what;

				if(
					shield == "bonus" ||
					shield == "actie"
				)
				{
					/* Already covered by ins/del, do nothing */
					if(!current_p.del_price)
					{
						std::stringstream sstr;
						sstr << "Del should be set for " << '[' << current_p.identifier << "] " << current_p.name;
						// Albert Heijn does this sometimes, "BONUS" without being clear what the real bonus is.
						//throw std::runtime_error(sstr.str());
						std::cerr << sstr.str() << std::endl;
					}
				}
				else if(boost::regex_match(shield, what, match_euro_discount))
				{
					if(!current_p.del_price)
						price -= boost::lexical_cast<unsigned int>(what[1])*100;
				}
				else if(boost::regex_match(shield, what, match_eurocent_discount))
				{
					if(!current_p.del_price)
						price -= boost::lexical_cast<float>(what[1])*100;
				}
				else if(boost::regex_match(shield, what, match_percent_discount))
				{
					if(!current_p.del_price)
						price *= 1.0 - boost::lexical_cast<float>(what[1])/100.0;
				}
				else if(shield == "2e halve prijs")
				{
					price *= 0.75;
					discount_condition = condition::AT_TWO;
				}
				else if(
					shield == "2=1" ||
					shield == "1 + 1 gratis"
				)
				{
					price *= 0.5;
					discount_condition = condition::AT_TWO;
				}
				else if(shield == "2 + 1 gratis")
				{
					price *= 0.67;
					discount_condition = condition::AT_THREE;
				}
				else if(boost::regex_match(shield, what, match_combination_discount))
				{
					unsigned int discount_amount = boost::lexical_cast<unsigned int>(what[1]);
					unsigned int discount_combination_price = boost::lexical_cast<float>(what[3])*100;

					if(discount_amount == 2)
						discount_condition = condition::AT_TWO;
					else if(discount_amount == 3)
						discount_condition = condition::AT_THREE;
					else
						throw std::runtime_error(std::string("Previously unseen discount_amount ") + std::to_string(discount_amount));

					price = discount_combination_price / discount_amount;
				}
				else
				{
					std::cerr << '[' << current_p.identifier << "] " << current_p.name << std::endl;
					std::cerr << "shield: \'" << current_p.shield.get() << '\'' << std::endl;
					conf = confidence::LOW;
				}
			}

			product_callback(product{
				 current_p.identifier,
				 current_p.name,
				 orig_price,
				 price,
				 discount_condition,
				 current_p.valid_on ? datetime(current_p.valid_on.get(), time()) : datetime_now(),
			 }, datetime_now(), conf);
		}

	public:
		product_parser(more_callback_t more_callback_, product_callback_t product_callback_)
		: more_callback(more_callback_)
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
			static const boost::regex match_url_offset(".*offset=([0-9]+)");

			if(rec)
				rec.get().startElement();

			wc.startElement();

			const std::string att_class = atts.getValue("class");

			switch(state)
			{
			case S_INIT:
				if(atts.getValue("data-appie") == "productpreview")
				{
					//Reset product
					current_p = product_proto();

					state = S_PRODUCT;
					wc.add([&]() {
						state = S_INIT;

						deliver_product();
					});
				}
				else if(util::contains_attr("appender", att_class))
				{
					boost::smatch what;
					if(boost::regex_match(atts.getValue("href"), what, match_url_offset))
						more_callback(what[0]);
				}
			break;
			case S_PRODUCT:
				if(util::contains_attr("detail", att_class))
				{
					state = S_PRODUCT_NAME;
					wc.add([&]() { state = S_PRODUCT; });
				}
				if(util::contains_attr("detail-addtolist", att_class))
					current_p.identifier = atts.getValue("data-productid");
				if(util::contains_attr("shield", att_class))
					rec = html_recorder(
						[&](std::string ch) {
							std::string tmp = util::sanitize(ch);
							boost::algorithm::to_lower(tmp);
							if(tmp != "")
								current_p.shield = tmp;
						});
				else if(util::contains_attr("price", att_class))
				{
					state = S_PRODUCT_PRICE;
					wc.add([&]() { state = S_PRODUCT; });
				}
			break;
			case S_PRODUCT_NAME:
				if(qName == "h2")
					rec = html_recorder([&](std::string ch) { current_p.name = util::sanitize(ch); });
				else if(util::contains_attr("info", att_class))
					rec = html_recorder(
						[&](std::string ch) {
							std::string tmp = util::sanitize(ch);
							boost::algorithm::to_lower(tmp);
							if(tmp != "")
								current_p.subtext = tmp;
						});
			break;
			case S_PRODUCT_PRICE:
				if(qName == "del")
					rec = html_recorder([&](std::string ch) { current_p.del_price = parse_price(util::sanitize(ch)); });
				else if(qName == "ins")
					rec = html_recorder([&](std::string ch) { current_p.ins_price = parse_price(util::sanitize(ch)); });
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
