#include "scraper.hpp"

#include <vector>
#include <iostream>

int main()
{
	supermarx::scraper s([&](const supermarx::product& product) {
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
	});

	s.scrape();
}
