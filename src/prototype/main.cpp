#include <albert/scraper.hpp>
#include <karl/config.hpp>
#include <karl/karl.hpp>

#include <vector>
#include <iostream>

int main()
{
	supermarx::config c("config.yaml");
	supermarx::karl karl(c.db_host, c.db_user, c.db_password, c.db_database);

	std::set<std::string> viewed_products;

	supermarx::scraper s([&](const supermarx::product& product) {
		if(!viewed_products.emplace(product.identifier).second)
			return;

		if(product.price == product.orig_price)
			std::cout << "Product '" << product.name << "' voor " << product.price/100.0f << " EUR";
		else
			std::cout << "Product '" << product.name << "' van " << product.orig_price/100.0f << " voor " << product.price/100.0f << " EUR";

		switch(product.discount_condition)
		{
		case supermarx::condition::ALWAYS:
			break;
		case supermarx::condition::AT_TWO:
			std::cout << " (bij aankoop van twee)";
			break;
		case supermarx::condition::AT_THREE:
			std::cout << " (bij aankoop van drie)";
			break;
		}

		std::cout << std::endl;

		karl.add_product(product, 1);
	});

	s.scrape();
}
