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

		void create_xml_child_element_with_text( tinyxml2::XMLDocument& _document, tinyxml2::XMLElement* _container, std::string_view _child_name, std::string_view _text )
		{
			if( _container == nullptr )
				return;

			tinyxml2::XMLElement* new_elem{ _document.NewElement( _child_name.data() ) };
			new_elem->SetText( _text.data() );
			_container->InsertEndChild( new_elem );
		}
	}
}
