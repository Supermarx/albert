#include "downloader.hpp"

#include <memory>
#include <curl/curl.h>

namespace supermarx
{
	static size_t downloader_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
	{
		size_t realsize = size * nmemb;
		
		std::string *mem = (std::string *)userp;
		mem->append((char *)contents, realsize);
		
		return realsize;
	}

	downloader::curl_ptr downloader::create_handle() const
	{
		static bool initialized = false;
	
		if(!initialized)
		{
			curl_global_init(CURL_GLOBAL_ALL);
			initialized = true;
		}
		
		CURL* handle = curl_easy_init();
		
		curl_easy_setopt(handle, CURLOPT_REFERER, referer.c_str());
		curl_easy_setopt(handle, CURLOPT_COOKIE, cookies.c_str());
		curl_easy_setopt(handle, CURLOPT_USERAGENT, agent.c_str());
		curl_easy_setopt(handle, CURLOPT_ENCODING, "UTF-8");
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, downloader_write_callback);
		
		return curl_ptr(handle, &curl_easy_cleanup);
	}
	
	downloader::downloader(const std::string& agent)
	: agent(agent)
	, referer()
	, cookies()
	{}
	
	std::string downloader::fetch(const std::string& url) const
	{
		std::string result;
		curl_ptr handle(create_handle());
		
		curl_easy_setopt(handle.get(), CURLOPT_URL, url.c_str());
		curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, (void *)&result);
		curl_easy_perform(handle.get());
		
		return result;
	}
	
	std::string downloader::post(const std::string& url, const downloader::formmap& form) const
	{
		std::string result, formdata;
		curl_ptr handle(create_handle());
		
		{
			bool first_arg = true;
			for(const auto& kvp : form)
			{
				if(!first_arg)
					formdata.append("&");
				else
					first_arg = false;
			
				formdata.append(kvp.first);
				formdata.append("=");
			
				{
					char *arg = curl_easy_escape(handle.get(), kvp.second.c_str(), kvp.second.size());
					formdata.append(arg);
					curl_free(arg);
				}
			}
		}
		
		curl_slist *headerlist = nullptr;
		curl_slist_append(headerlist, "Expect:");
		curl_easy_setopt(handle.get(), CURLOPT_HTTPHEADER, headerlist);
		
		curl_easy_setopt(handle.get(), CURLOPT_URL, url.c_str());
		curl_easy_setopt(handle.get(), CURLOPT_POSTFIELDS, formdata.c_str());
		curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, (void *)&result);
		
		curl_easy_perform(handle.get());
		
		curl_slist_free_all(headerlist);
		
		return result;
	}
	
	void downloader::set_referer(const std::string& r)
	{
		referer = r;
	}
	
	void downloader::set_cookies(const std::string& c)
	{
		cookies = c;
	}
}
