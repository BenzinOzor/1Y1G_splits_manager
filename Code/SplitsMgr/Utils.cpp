#include <format>
#include <fstream>

#include <tinyXML2/tinyxml2.h>
#include <FZN/Tools/Tools.h>
#include <FZN/Tools/Logging.h>

#include "../External/base64.hpp"

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

		SplitDate get_date_from_string( std::string_view _date, std::string_view _format /*= "%F" */ )
		{
			std::stringstream stream{};
			stream << _date;

			std::chrono::year_month_day ret_date{};
			std::chrono::from_stream( stream, _format.data(), ret_date );

			return ret_date;
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
				time_string += std::format( "{:%S}", abs( std::chrono::floor< std::chrono::seconds >( _time ) ) );
			else
				time_string += std::format( "{:%S}", abs( _time ) );

			return time_string;
		}

		std::string date_to_str( const SplitDate& _date, Options::DateFormat _format /*= Options::DateFormat::ISO8601*/ )
		{
			if( Utils::is_date_valid( _date ) == false )
				return "<No Date>";

			switch( _format )
			{
				case Options::DMYName:
					return std::format( "{:%d %b %Y}", _date );

				case Options::DateFormat::ISO8601:
				default:
					return std::format( "{:%F}", _date );
			};
		}

		bool is_time_valid( const SplitTime& _time )
		{
			return _time != SplitTime{};
		}

		bool is_date_valid( const SplitDate& _date )
		{
			return _date != SplitDate{};
		}

		SplitDate today()
		{
			const std::chrono::time_point now{ std::chrono::system_clock::now() };
			
			return std::chrono::floor< std::chrono::days >( now );
		}

		std::string get_cover_data( std::string_view _cover_path )
		{
			std::ifstream cover_file( _cover_path.data(), std::ios::in | std::ios::binary | std::ios::ate );

			if( cover_file.is_open() == false )
			{
				FZN_LOG( "Can't open cover file." );
				return {};
			}

			const size_t byte_count = cover_file.tellg();
			cover_file.seekg( 0, std::ios::beg );

			std::string cover_data{};
			cover_data.resize( byte_count );

			cover_file.read( cover_data.data(), byte_count );

			cover_data = base64::to_base64( cover_data );

			return cover_data;
		}

		void window_bottom_table( uint8_t _nb_items, std::function<void( void )> _table_content_fct )
		{
			ImGui::NewLine();
			ImGui::NewLine();

			if( ImGui::BeginTable( "BottomTable", _nb_items + 1 ) )
			{
				ImGui::TableSetupColumn( "Empty", ImGuiTableColumnFlags_WidthStretch );

				for( uint8_t column{ 1 }; column < _nb_items + 1; ++column )
					ImGui::TableSetupColumn( fzn::Tools::Sprintf( "Button %u", column ).c_str(), ImGuiTableColumnFlags_WidthFixed );

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex( 1 );

				_table_content_fct();

				ImGui::EndTable();
			}
		}

		uint32_t days_between_dates( const SplitDate& _day_1, const SplitDate& _day_2 )
		{
			std::chrono::system_clock::time_point tp_day_1 = std::chrono::time_point<std::chrono::system_clock, std::chrono::days>( std::chrono::sys_days{ _day_1 } );
			std::chrono::system_clock::time_point tp_day_2 = std::chrono::time_point<std::chrono::system_clock, std::chrono::days>( std::chrono::sys_days{ _day_2 } );

			return std::chrono::duration_cast<std::chrono::days>( tp_day_2 - tp_day_1 ).count();
		}

		SplitDate add_days_to_date( const SplitDate& _start_day, uint32_t _nb_days )
		{
			std::chrono::system_clock::time_point tp_day_2 = std::chrono::time_point<std::chrono::system_clock, std::chrono::days>( std::chrono::sys_days{ _start_day } );
			
			return std::chrono::floor< std::chrono::days >( tp_day_2 + std::chrono::days{ _nb_days } );
		}
	}
}
