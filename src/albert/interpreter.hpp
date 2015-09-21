#pragma once

#include <albert/scraper.hpp>
#include <albert/page.hpp>

#include <jsoncpp/json/json.h>
#include <boost/algorithm/string.hpp>

namespace supermarx
{

class interpreter
{
private:
	confidence conf;
	std::vector<std::string> probs;

	interpreter()
	: conf(confidence::NEUTRAL)
	, probs()
	{}

	inline date first_monday(date d)
	{
		while(d.day_of_week() != weekday::Monday)
			d += boost::gregorian::date_duration(1);

		return d;
	}

	void report_problem_understanding(std::string const& field, std::string const& value)
	{
		std::stringstream sstr;
		sstr << "Unclear '" << field << "' with value '" << value << "'";

		probs.emplace_back(sstr.str());
	}

	struct volume_t
	{
		uint64_t volume;
		measure volume_measure;

		volume_t()
		: volume(1)
		, volume_measure(measure::UNITS)
		{}

		volume_t(uint64_t _volume, measure _volume_measure)
		: volume(_volume)
		, volume_measure(_volume_measure)
		{}

		volume_t(float _volume, measure _volume_measure)
		: volume_t(static_cast<uint64_t>(_volume), _volume_measure)
		{}
	};

	volume_t interpret_unit_detail(std::string unit)
	{
		boost::algorithm::trim(unit);

		static const std::set<std::string> irrelevant_words = {
			"wasbeurten", "tabl", "plakjes", "rollen", "bosjes", "porties", "zakjes", "sachets", "vel", "strips"
		};

		static const boost::regex match_stuks("(?:per)?(?: )*(?:ca.)?(?: )*([0-9]+)(?: )*(?:st|stuk|stuks|paar|set)");
		static const boost::regex match_pers("(?:ca. )?([0-9]+(?:-[0-9]+)?) pers\\.");

		static const boost::regex match_measure("(?:ca.)?(?: )*([0-9]+(?:\\.[0-9]+)?)(?: )*([a-z]+)(?:\\.)?");
		boost::smatch what;

		if(
			unit == "per stuk" ||
			unit == "per krop" ||
			unit == "per bos" ||
			unit == "per bosje" ||
			unit == "per doos" ||
			unit == "per set" ||
			unit == "per paar" ||
			unit == "per rol" ||
			unit == "per pak" ||
			unit == "éénkops" ||
			boost::regex_match(unit, what, match_pers)
		)
		{
			return volume_t();
		}
		else if(boost::regex_match(unit, what, match_stuks))
		{
			return {boost::lexical_cast<float>(what[1]), measure::UNITS};
		}
		else if(boost::regex_match(unit, what, match_measure))
		{
			float volume = boost::lexical_cast<float>(what[1]);
			std::string measure_type = what[2];

			if(measure_type == "g" || measure_type == "gr" || measure_type == "gram")
			{
				return {volume*1000.0f, measure::MILLIGRAMS};
			}
			else if(measure_type == "kg" || measure_type == "kilo")
			{
				return {volume*1000000.0f, measure::MILLIGRAMS};
			}
			else if(measure_type == "ml")
			{
				return {volume, measure::MILLILITRES};
			}
			else if(measure_type == "cl")
			{
				return {volume*10.0f, measure::MILLILITRES};
			}
			else if(measure_type == "lt" || measure_type == "liter")
			{
				return {volume*1000.0f, measure::MILLILITRES};
			}
			else if(measure_type == "m" || measure_type == "meter" || measure_type == "mtr")
			{
				return {volume*1000.0f, measure::MILLIMETRES};
			}
			else if(irrelevant_words.find(measure_type) != irrelevant_words.end())
			{
				return volume_t();
			}
			else
			{
				report_problem_understanding("measure_type", measure_type);
				conf = confidence::LOW;
				return volume_t();
			}
		}
		else
		{
			report_problem_understanding("unit", unit);
			conf = confidence::LOW;
			return volume_t();
		}
	}

	volume_t interpret_unit(std::string unit)
	{
		boost::algorithm::to_lower(unit);
		boost::algorithm::trim(unit);
		remove_hyphens(unit);

		static const boost::regex match_multi("([0-9]+)(?: )*[xX](?: )*(.+)");
		boost::smatch what;

		std::replace(unit.begin(), unit.end(), ',', '.');

		uint64_t multiplier = 1;
		if(boost::regex_match(unit, what, match_multi))
		{
			multiplier = boost::lexical_cast<uint64_t>(what[1]);
			unit = what[2];
		}

		volume_t result(interpret_unit_detail(unit));
		return {result.volume*multiplier, result.volume_measure};
	}

	static inline boost::optional<std::string> parse_image_uri(Json::Value const& product)
	{
		boost::optional<std::string> image_uri;
		size_t max_size = std::numeric_limits<size_t>::min();
		for(auto const& image : product["images"])
		{
			size_t size = image["width"].asUInt64() * image["height"].asUInt64();

			if(size <= max_size)
				continue;

			image_uri = image["link"]["href"].asString();
			size = max_size;
		}

		return image_uri;
	}

	struct prices_t
	{
		uint64_t orig_price;
		uint64_t price;
		uint64_t discount_amount;
	};

	inline prices_t parse_price(Json::Value const& product)
	{
		uint64_t price = static_cast<uint64_t>(product["priceLabel"]["now"].asFloat() * 100.0f); // In (euro)cents, with discount applied
		uint64_t orig_price = price;

		bool adapted_price = !product["priceLabel"]["was"].empty();
		if(adapted_price)
			orig_price = static_cast<uint64_t>(product["priceLabel"]["was"].asFloat() * 100.0f);

		uint64_t discount_amount = 1;

		if(!product["discount"]["label"].empty())
		{
			std::string label = product["discount"]["label"].asString();
			boost::algorithm::to_lower(label);
			remove_hyphens(label);

			static const boost::regex match_percent_discount("([0-9]+)% (?:probeer)?korting");
			static const boost::regex match_euro_discount("([0-9]+) euro (?:probeer)?korting");
			static const boost::regex match_eurocent_discount("([0-9]+\\.[0-9]+) euro (?:probeer)?korting");

			static const boost::regex match_combination_discount("([0-9]+) voor (&euro; )?([0-9]+\\.[0-9]+)");
			boost::smatch what;

			if(
				label == "bonus" ||
				label == "actie" ||
				label == "aanbieding" ||
				label == "route 99"
			)
			{
				/* Already covered by priceLabel.now and priceLabel.was, do nothing */

				if(!adapted_price)
				{
					// Price should be adapted
					// Albert Heijn does this sometimes, "BONUS" without being clear what the real bonus is.

					// Do nothing
				}
			}
			else if(boost::regex_match(label, what, match_euro_discount))
			{
				if(!adapted_price)
					price -= boost::lexical_cast<unsigned int>(what[1])*100;
			}
			else if(boost::regex_match(label, what, match_eurocent_discount))
			{
				if(!adapted_price)
					price -= boost::lexical_cast<float>(what[1])*100;
			}
			else if(boost::regex_match(label, what, match_percent_discount))
			{
				if(!adapted_price)
					price *= 1.0 - boost::lexical_cast<float>(what[1])/100.0;
			}
			else if(label == "2e halve prijs")
			{
				price *= 0.75;
				discount_amount = 2;
			}
			else if(
				label == "2=1" ||
				label == "2 = 1" ||
				label == "1 + 1 gratis" ||
				label == "2e gratis"
			)
			{
				price *= 0.5;
				discount_amount = 2;
			}
			else if(label == "2 + 1 gratis")
			{
				price *= 0.67;
				discount_amount = 3;
			}
			else if(boost::regex_match(label, what, match_combination_discount))
			{
				discount_amount = boost::lexical_cast<uint16_t>(what[1]);
				unsigned int discount_combination_price = boost::lexical_cast<float>(what[3])*100;

				price = discount_combination_price / discount_amount;
			}
			else
			{
				report_problem_understanding("discount-label", label);
				conf = confidence::LOW;
			}
		}

		return {orig_price, price, discount_amount};
	}

	inline datetime parse_valid_on(Json::Value const& product)
	{
		if(product["discount"]["period"].empty())
			return datetime_now();

		std::string period = product["discount"]["period"].asString();

		if(period == "vanaf maandag")
			return datetime(first_monday(datetime_now().date()));

		report_problem_understanding("discount-period", period);
		conf = confidence::LOW;

		return datetime_now();
	}

	inline void parse_product(Json::Value const& product, page_t const& current_page, scraper::product_callback_t const& callback)
	{
		std::string identifier(product["id"].asString());
		std::string name(product["description"].asString());
		remove_hyphens(name);

		volume_t volume = interpret_unit(product["unitSize"].asString());
		prices_t prices = parse_price(product);

		datetime valid_on = parse_valid_on(product);

		message::product_base p({
			identifier,
			name,
			volume.volume,
			volume.volume_measure,
			prices.orig_price,
			prices.price,
			prices.discount_amount,
			valid_on,
			current_page.tags
		});

		callback(
			current_page.uri,
			parse_image_uri(product),
			p,
			datetime_now(),
			conf,
			probs
		);
	}

public:
	static void interpret(Json::Value const& product, page_t const& current_page, scraper::product_callback_t const& callback)
	{
		interpreter i;
		i.parse_product(product, current_page, callback);
	}
};

}
