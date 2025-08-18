#include <format>

#include <tinyXML2/tinyxml2.h>
#include <FZN/Tools/Tools.h>

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

		SplitTime get_time_from_string( std::string_view _time, std::string_view _format /*= "%H:%M:%S" */ )
		{
			std::stringstream stream;

			size_t first_colon = _time.find_first_of( ':' );

			// If there is a dot in the string, it means we got it from the .lss and the time is greater than a day
			if( size_t dot = _time.find_first_of( '.' ); first_colon != std::string::npos && dot != std::string::npos && dot < first_colon )
			{
				const int days = std::stoi( std::string{ _time.substr( 0, dot ) } );

				++dot;
				int hours = std::stoi( std::string{ _time.substr( dot, first_colon - dot ) } );

				stream << fzn::Tools::Sprintf( "%d:%s", days * 24 + hours, _time.substr( first_colon + 1 ).data() );
			}
			else
				stream << _time;

			std::chrono::duration<int, std::milli> ret_time{};
			std::chrono::from_stream( stream, _format.data(), ret_time );

			return ret_time;
		}

		std::string time_to_str( const SplitTime& _time, bool _floor_seconds /*= true */, bool _separate_days /*= false*/ )
		{
			std::string time_string{};

			auto durations_hours = duration_cast<std::chrono::hours>(_time );
			int days = durations_hours.count() / 24;
			int hours = durations_hours.count() - days * 24;

			if( _separate_days && days > 0 )
			{
				time_string = fzn::Tools::Sprintf( "%d.%02d:", days, hours );
				time_string += std::format( "{:%M:}", _time );
			}
			else
				time_string = std::format( "{:%H:%M:}", _time );

			if( _floor_seconds )
				time_string += std::format( "{:%S}", std::chrono::floor< std::chrono::seconds >( _time ) );
			else
				time_string += std::format( "{:%S}", _time );

			return time_string;
		}

		bool is_time_valid( const SplitTime& _time )
		{
			return _time != SplitTime{};
		}
	}
}
