#pragma once

#include <albert/scraper.hpp>
#include <albert/page.hpp>
#include <jsoncpp/json/json.h>

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
				conf = confidence::LOW;
				return;
			}
		}
		else
		{
			report_problem_understanding("unit", unit);
			conf = confidence::LOW;
		}

		volume *= multiplier;
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

	inline void parse_product(Json::Value const& product, page_t const& current_page, scraper::callback_t const& callback)
	{
		std::string identifier(product["id"].asString());
		std::string name(product["description"].asString());
		remove_hyphens(name);

		uint64_t volume;
		measure volume_measure;
		interpret_unit(product["unitSize"].asString(), volume, volume_measure);

		uint64_t orig_price = static_cast<uint64_t>(product["priceLabel"]["now"].asFloat() * 100.0f); // In (euro)cents
		uint64_t price = 0; // In (euro)cents, with discount applied
		uint64_t discount_amount = 1;

		datetime valid_on = datetime_now();

		message::product_base p({
			identifier,
			name,
			volume,
			volume_measure,
			orig_price,
			price,
			discount_amount,
			valid_on
		});

//		std::cerr << name << " [" << identifier << "]" << std::endl;

		callback(
			current_page.uri,
			parse_image_uri(product),
			p,
			current_page.tags,
			datetime_now(),
			conf,
			probs
		);
	}

public:
	static void interpret(Json::Value const& product, page_t const& current_page, scraper::callback_t const& callback)
	{
		interpreter i;
		i.parse_product(product, current_page, callback);
	}
};

}
