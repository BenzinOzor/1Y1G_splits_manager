#include <tinyXML2/tinyxml2.h>

#include "Utils.h"


namespace SplitsMgr
{
	namespace Utils
	{
		std::string get_xml_child_element_text( tinyxml2::XMLElement* _container, std::string_view _child_name )
		{
			if( _container == nullptr )
				return {};

			tinyxml2::XMLElement* child_element = _container->FirstChildElement( _child_name.data() );

			if( child_element != nullptr && child_element->GetText() != nullptr )
			{
				return child_element->GetText();
			}

			return {};
		}
	}
}
