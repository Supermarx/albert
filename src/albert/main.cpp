#include <albert/scraper.hpp>

#include <boost/program_options.hpp>

#include <vector>
#include <iostream>

#include <supermarx/api/api.hpp>

struct cli_options
{
	std::string api_host;
};

int read_options(cli_options& opt, int argc, char** argv)
{
	boost::program_options::options_description o_general("General options");
	o_general.add_options()
			("help,h", "display this message")
			("api,a", boost::program_options::value(&opt.api_host), "api host to send results to");

	boost::program_options::variables_map vm;
	boost::program_options::positional_options_description pos;

	boost::program_options::options_description options("Allowed options");
	options.add(o_general);

	try
	{
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(options).positional(pos).run(), vm);
	} catch(boost::program_options::unknown_option &e)
	{
		std::cerr << "Unknown option --" << e.get_option_name() << ", see --help." << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		boost::program_options::notify(vm);
	} catch(const boost::program_options::required_option &e)
	{
		std::cerr << "You forgot this: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	if(vm.count("help"))
	{
		std::cout
				<< "SuperMarx scraper for the Albert Heijn inventory. [https://github.com/SuperMarx/albert]" << std::endl
				<< "Usage: ./albert [options]" << std::endl
				<< std::endl
				<< o_general;

		return EXIT_FAILURE;
	}

	if(!vm.count("api"))
		opt.api_host = "http://api.supermarx.nl";

	return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	cli_options opt;

	int result = read_options(opt, argc, argv);
	if(result != EXIT_SUCCESS)
		return result;

	supermarx::api api(opt.api_host, "albert (libsupermarx-api)");

	supermarx::scraper s([&](const supermarx::product& product) {
		std::cerr << "Product '" << product.name << "' [" << product.identifier << "] ";

		if(product.price == product.orig_price)
			std::cerr << product.price/100.0f << " EUR";
		else
			std::cerr << product.orig_price/100.0f << " EUR -> " << product.price/100.0f << " EUR";

		switch(product.discount_condition)
		{
		case supermarx::condition::ALWAYS:
			break;
		case supermarx::condition::AT_TWO:
			std::cout << " (at two)";
			break;
		case supermarx::condition::AT_THREE:
			std::cout << " (at three)";
			break;
		}

		std::cout << std::endl;
		api.add_product(product, 1);
	});

	s.scrape();

	return EXIT_SUCCESS;
}
