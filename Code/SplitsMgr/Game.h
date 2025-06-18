#pragma once

#include <string>
#include <vector>
#include <chrono>


namespace tinyxml2
{
	class XMLElement;
	class XMLDocument;
}

namespace SplitsMgr
{
	using SplitTime = std::chrono::duration<int, std::milli>;

	struct Split
	{
		std::string m_name;
		SplitTime m_played;
	};
	using Splits = std::vector< Split >;

	class Game
	{
	public:
		void display();

		tinyxml2::XMLElement* parse_game( tinyxml2::XMLElement* _element );
		void write_game( tinyxml2::XMLDocument& _document, tinyxml2::XMLElement* _segments );

	private:
		std::string m_name;
		std::string m_icon_desc;
		SplitTime m_estimation;

		Splits m_splits;
	};
}