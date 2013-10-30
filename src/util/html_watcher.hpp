#pragma once

#include <string>
#include <functional>
#include <list>

namespace supermarx
{
	class html_watcher
	{
	public:
		typedef std::function<void()> callback_t;
	
	private:
		size_t depth;
		callback_t f;
		
	public:
		html_watcher(callback_t f)
		: depth(0)
		, f(f)
		{}
		
		void startElement()
		{
			depth++;
		}
		
		bool endElement()
		{
			if(depth == 0)
			{
				f();
				return true;
			}
			
			depth--;
			return false;
		}
	};
	
	class html_watcher_collection
	{
		std::list<html_watcher> watchers;
		
	public:
		void add(html_watcher::callback_t f)
		{
			watchers.emplace_back(f);
		}
		
		void startElement()
		{
			for(auto& w : watchers)
				w.startElement();
		}
		
		void endElement()
		{
			watchers.remove_if([](html_watcher& w) { return w.endElement(); });
		}
	};
}
