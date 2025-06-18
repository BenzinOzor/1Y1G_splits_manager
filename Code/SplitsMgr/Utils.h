#pragma once

#include <string_view>


namespace tinyxml2
{
	class XMLElement;
}

namespace SplitsMgr
{
	namespace Utils
	{
		std::string get_xml_child_element_text( tinyxml2::XMLElement* _container, std::string_view _child_name );
	}
}
