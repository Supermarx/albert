#pragma once

#include <functional>
#include <stdexcept>
#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/optional.hpp>

#include <supermarx/message/product_base.hpp>

#include <supermarx/scraper/html_parser.hpp>
#include <supermarx/scraper/html_watcher.hpp>
#include <supermarx/scraper/html_recorder.hpp>
#include <supermarx/scraper/util.hpp>

namespace supermarx
{
	class product_parser : public html_parser::default_handler
	{
	public:
		typedef std::function<void(const message::product_base&, boost::optional<std::string>, datetime, confidence, std::vector<std::string>)> product_callback_t;
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
			boost::optional<std::string> image_uri;
			boost::optional<unsigned int> del_price, ins_price;
			boost::optional<std::string> shield, subtext;
			boost::optional<date> valid_on;
			boost::optional<std::string> unit;

			confidence conf = confidence::NEUTRAL;
			std::vector<std::string> problems;
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

		static date first_monday(date d)
		{
			while(d.day_of_week() != weekday::Monday)
				d += boost::gregorian::date_duration(1);

			return d;
		}

		void report_problem_understanding(std::string const& field, std::string const& value)
		{
			std::stringstream sstr;
			sstr << "Unclear '" << field << "' with value '" << value << "'";

			current_p.problems.emplace_back(sstr.str());
		}

		void interpret_shield(std::string const& shield, uint64_t& price, uint64_t& discount_amount)
		{
			static const boost::regex match_percent_discount("([0-9]+)% (?:probeer)?korting");
			static const boost::regex match_euro_discount("([0-9]+) euro (?:probeer)?korting");
			static const boost::regex match_eurocent_discount("([0-9]+\\.[0-9]+) euro (?:probeer)?korting");

			static const boost::regex match_combination_discount("([0-9]+) voor (&euro; )?([0-9]+\\.[0-9]+)");
			boost::smatch what;

			if(
				shield == "bonus" ||
				shield == "actie" ||
				shield == "route 99"
			)
			{
				/* Already covered by ins/del, do nothing */

				if(!current_p.del_price)
				{
					// Del should be set
					// Albert Heijn does this sometimes, "BONUS" without being clear what the real bonus is.

					// Do nothing
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
				discount_amount = 2;
			}
			else if(
				shield == "2=1" ||
				shield == "2 = 1" ||
				shield == "1 + 1 gratis"
			)
			{
				price *= 0.5;
				discount_amount = 2;
			}
			else if(shield == "2 + 1 gratis")
			{
				price *= 0.67;
				discount_amount = 3;
			}
			else if(boost::regex_match(shield, what, match_combination_discount))
			{
				discount_amount = boost::lexical_cast<uint16_t>(what[1]);
				unsigned int discount_combination_price = boost::lexical_cast<float>(what[3])*100;

				price = discount_combination_price / discount_amount;
			}
			else
			{
				report_problem_understanding("shield", shield);
				current_p.conf = confidence::LOW;
			}
		}

		void interpret_subtext(std::string const& subtext)
		{
			static const boost::regex match_deadline("t/m [0-9]+ [a-z]{3}");
			static const boost::regex match_start("vanaf ([0-9]+) ([a-z]{3})");
			boost::smatch what;

			if(
				subtext == "dit product wordt ah biologisch" ||
				subtext == "bestel 2 dagen van tevoren" ||
				subtext == "nieuw in het assortiment" ||
				subtext == "belgisch assortiment in nl" ||
				subtext == "alleen online" ||
				subtext == "online aanbieding" ||
				subtext == "vandaag" ||
				subtext == "t/m maandag" ||
				boost::regex_match(subtext, what, match_deadline)
			)
			{
				/* No relevant data for us, ignore */
			}
			else if(subtext == "binnenkort verkrijgbaar")
			{
				return; /* Product not yet available */
			}
			else if(subtext == "vanaf maandag")
			{
				current_p.valid_on = first_monday(datetime_now().date());
			}
			else if(boost::regex_match(subtext, what, match_start))
			{
				const static std::array<std::string, 12> months({{"jan", "feb", "maa", "apr", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec"}});
				for(size_t i = 0; i < months.size(); ++i)
					if(what[2] == months[i])
					{
						current_p.valid_on = next_occurance(i+1, boost::lexical_cast<size_t>(what[1]));
						return;
					}

				report_problem_understanding("date_start", subtext);
				current_p.conf = confidence::LOW;
			}
			else
			{
				report_problem_understanding("subtext", subtext);
				current_p.conf = confidence::LOW;
			}
		}

		void interpret_unit(std::string unit, uint64_t& volume, measure& volume_measure)
		{
			static const boost::regex match_multi("([0-9]+)(?: )?[xX](?: )?(.+)");

			static const boost::regex match_stuks("(?:ca. )?([0-9]+) (?:st|stuk|stuks)");
			static const boost::regex match_irrelevant_stuks("(?:ca. )?([0-9]+) (?:wasbeurten|tabl|plakjes|rollen|bosjes|porties|zakjes|sachets)");
			static const boost::regex match_pers("(?:ca. )?([0-9]+(?:-[0-9]+)?) pers\\.");

			static const boost::regex match_measure("(?:ca. )?([0-9]+(?:\\.[0-9]+)?)(?: )?(g|gr|gram|kg|kilo|ml|cl|lt|liter)(?:\\.)?");
			boost::smatch what;

			std::replace(unit.begin(), unit.end(), ',', '.');

			uint64_t multiplier = 1;
			if(boost::regex_match(unit, what, match_multi))
			{
				multiplier = boost::lexical_cast<uint64_t>(what[1]);
				unit = what[2];
			}

			if(
				unit == "per stuk" ||
				unit == "per krop" ||
				unit == "per bos" ||
				unit == "per bosje" ||
				unit == "per doos" ||
				unit == "per set" ||
				unit == "éénkops" ||
				boost::regex_match(unit, what, match_irrelevant_stuks) ||
				boost::regex_match(unit, what, match_pers)
			)
			{
				// Do nothing
			}
			else if(boost::regex_match(unit, what, match_stuks))
			{
				volume = boost::lexical_cast<float>(what[1]);
			}
			else if(boost::regex_match(unit, what, match_measure))
			{
				std::string measure_type = what[2];

				if(measure_type == "g" || measure_type == "gr" || measure_type == "gram")
				{
					volume = boost::lexical_cast<float>(what[1])*1000.0;
					volume_measure = measure::MILLIGRAMS;
				}
				else if(measure_type == "kg" || measure_type == "kilo")
				{
					volume = boost::lexical_cast<float>(what[1])*1000000.0;
					volume_measure = measure::MILLIGRAMS;
				}
				else if(measure_type == "ml")
				{
					volume = boost::lexical_cast<float>(what[1]);
					volume_measure = measure::MILLILITERS;
				}
				else if(measure_type == "cl")
				{
					volume = boost::lexical_cast<float>(what[1])*100.0;
					volume_measure = measure::MILLILITERS;
				}
				else if(measure_type == "lt" || measure_type == "liter")
				{
					volume = boost::lexical_cast<float>(what[1])*1000.0;
					volume_measure = measure::MILLILITERS;
				}
				else
				{
					report_problem_understanding("measure_type", measure_type);
					current_p.conf = confidence::LOW;
					return;
				}
			}
			else
			{
				report_problem_understanding("unit", unit);
				current_p.conf = confidence::LOW;
			}

			volume *= multiplier;
		}

		void deliver_product()
		{
			if(current_p.subtext)
				interpret_subtext(*current_p.subtext);

			uint64_t orig_price, price;
			uint64_t discount_amount = 1;

			if(!current_p.ins_price)
				return; // Product without a price, ignore

			orig_price = current_p.del_price ? current_p.del_price.get() : current_p.ins_price.get();

			price = current_p.ins_price.get();

			if(current_p.shield)
				interpret_shield(*current_p.shield, price, discount_amount);

			uint64_t volume = 1;
			measure volume_measure = measure::UNITS;

			if(current_p.unit)
				interpret_unit(*current_p.unit, volume, volume_measure);

			product_callback(
				message::product_base{
					current_p.identifier,
					current_p.name,
					volume,
					volume_measure,
					orig_price,
					price,
					discount_amount,
					current_p.valid_on ? datetime(current_p.valid_on.get(), time()) : datetime_now()
				},
				current_p.image_uri,
				datetime_now(),
				current_p.conf,
				current_p.problems
			);
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
				else if(qName == "p" && util::contains_attr("unit", att_class))
				{
					rec = html_recorder([&](std::string ch) {
						if(!current_p.unit)
							current_p.unit = "";

						current_p.unit->append(util::sanitize(ch));
					});
				}
				else if(qName == "img" && atts.getValue("data-appie") == "lazyload")
				{
					std::string image_uri = atts.getValue("data-original");
					if(image_uri == "" || boost::algorithm::ends_with(image_uri, "product-card-no-product-image.png"))
						break;

					static const boost::regex match_image_uri("(.+)_200x200_JPG\\.JPG");
					boost::smatch what;

					if(boost::regex_match(image_uri, what, match_image_uri))
						current_p.image_uri = what[1] + "_LowRes_JPG.JPG"; // Replace with fullres variant
					else
						current_p.image_uri = image_uri; // Some other (old) image)
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
