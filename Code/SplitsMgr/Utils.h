#pragma once

#include <string_view>


namespace tinyxml2
{
	class XMLElement;
	class XMLDocument;
}

namespace SplitsMgr
{
	namespace Utils
	{
		std::string get_xml_child_element_text( tinyxml2::XMLElement* _container, std::string_view _child_name );
		void create_xml_child_element_with_text( tinyxml2::XMLDocument& _document, tinyxml2::XMLElement* _container, std::string_view _child_name, std::string_view _text );
	}
}
