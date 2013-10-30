#pragma once

#include <memory>
#include <string>
#include <map>

typedef void CURL;

namespace supermarx
{
	class downloader
	{
	public:
		typedef std::map<std::string, std::string> formmap;
		typedef std::unique_ptr<CURL, std::function<void(CURL*)>> curl_ptr;
		
	private:
		std::string agent, referer, cookies;
		
		curl_ptr create_handle() const;
	
	public:
		downloader(const std::string& agent);
		
		std::string fetch(const std::string& url) const;
		std::string post(const std::string& url, const formmap& form) const;
		
		void set_cookies(const std::string& cookies);
		void set_referer(const std::string& referer);
		
		downloader(downloader&) = delete;
		void operator=(downloader&) = delete;
	};
}
