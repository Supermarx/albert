#include <supermarx/scraper/scraper_cli.hpp>
#include <albert/scraper.hpp>

int main(int argc, char** argv)
{
	return supermarx::scraper_cli<supermarx::scraper>::exec(1, "albert", "Albert Heijn", argc, argv);
}
