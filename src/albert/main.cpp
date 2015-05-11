#include <albert/scraper.hpp>

#include <boost/program_options.hpp>

#include <vector>
#include <iostream>

#include <supermarx/api/client.hpp>

struct cli_options
{
	std::string api_host;
	bool dry_run;
	bool extract_images;
	size_t extract_images_limit;
	size_t ratelimit;
	bool silent;
};

int read_options(cli_options& opt, int argc, char** argv)
{
	boost::program_options::options_description o_general("General options");
	o_general.add_options()
			("help,h", "display this message")
			("api,a", boost::program_options::value(&opt.api_host), "api host to send results to")
			("dry-run,d", "does not send products to api when set")
			("extract-images,i", "extract product images")
			("extract-images-limit,l", boost::program_options::value(&opt.extract_images_limit), "amount of images allowed to download in one session (default: 60)")
			("ratelimit,r", boost::program_options::value(&opt.ratelimit), "minimal time between each request in milliseconds (default: 30000)")
			("silent,s", "do not write status reports to cerr");

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
		opt.api_host = "https://api.supermarx.nl";

	opt.dry_run = vm.count("dry-run");
	opt.extract_images = vm.count("extract-images");

	if(!vm.count("extract-images-limit"))
		opt.extract_images_limit = 60;

	if(!vm.count("ratelimit"))
		opt.ratelimit = 30000;

	opt.silent = vm.count("silent");

	return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	cli_options opt;

	int result = read_options(opt, argc, argv);
	if(result != EXIT_SUCCESS)
		return result;

	supermarx::api::client api(opt.api_host, "albert (libsupermarx-api)");
	const supermarx::id_t supermarket_id = 1;
	size_t images_downloaded = 0;

	supermarx::scraper s([&](
		std::string const& source_uri,
		boost::optional<std::string> const& image_uri_opt,
		supermarx::product const& product,
		supermarx::datetime retrieved_on,
		supermarx::confidence c,
		supermarx::scraper::problems_t problems
	) {
		if(!opt.silent)
		{
			std::cerr << "Product '" << product.name << "' [" << product.identifier << "] ";

			if(product.price == product.orig_price)
				std::cerr << product.price/100.0f << " EUR";
			else
				std::cerr << product.orig_price/100.0f << " EUR -> " << product.price/100.0f << " EUR";

			if(product.discount_amount > 1)
				std::cerr << " (at " << product.discount_amount << ')';

			std::cerr << std::endl;
		}

		if(!opt.dry_run)
			api.add_product(product, supermarket_id, retrieved_on, c, problems);

		if(opt.extract_images && image_uri_opt)
		{
			bool permission = false;
			if(opt.dry_run)
				permission = true;
			else
			{
				supermarx::api::product_summary ps(api.get_product(supermarket_id, product.identifier));
				if(!ps.imagecitation_id)
					permission = true;
			}

			if(permission && images_downloaded < opt.extract_images_limit)
			{
				supermarx::raw img(s.download_image(*image_uri_opt));

				if(!opt.dry_run)
				{
					try
					{
					api.add_product_image_citation(
						supermarket_id,
						product.identifier,
						*image_uri_opt,
						source_uri,
						supermarx::datetime_now(),
						std::move(img)
					);
					} catch(std::runtime_error const& e)
					{
						std::cerr << "Catched error whilst pushing image for " << product.identifier << " " << *image_uri_opt << std::endl;
					}
				}

				images_downloaded++;
			}
		}
	}, opt.ratelimit);

	s.scrape();

	return EXIT_SUCCESS;
}
