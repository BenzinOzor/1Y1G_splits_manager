#include <regex>
#include <format>

#include <tinyXML2/tinyxml2.h>

#include <FZN/Tools/Tools.h>
#include <FZN/UI/ImGui.h>

#include "Game.h"
#include "Utils.h"


namespace SplitsMgr
{
	void Game::display()
	{
		if( ImGui::CollapsingHeader( m_name.c_str() ) )
		{
			ImGui::Indent();
			if( m_splits.size() > 1 )
			{
				for( Split& split : m_splits )
				{
					ImGui::Text( "%s - ", split.m_name.c_str() );
					ImGui::SameLine();
					ImGui::Text( std::format( "{:%H:%M:%S}", split.m_played ).c_str() );
				}
			}
			else
			{
				ImGui::Text( std::format("{:%H:%M:%S}", m_splits.front().m_played ).c_str() );
			}

			ImGui::Button( "Add Session", { ImGui::GetContentRegionAvail().x, 0.f } );
			ImGui::Unindent();
		}
	}

	tinyxml2::XMLElement* Game::parse_game( tinyxml2::XMLElement* _element )
	{
		if( _element == nullptr )
			return nullptr;

		tinyxml2::XMLElement* segment = _element;

		bool parsing_game{ true };

		SplitTime estimate{};

		while( parsing_game )
		{
			std::string split_name = Utils::get_xml_child_element_text( segment, "Name" );
			
			if( split_name.empty() )
				return segment->NextSiblingElement( "Segment" );

			Split new_split{};

			if( split_name.at( 0 ) == '-' )
			{
				split_name = split_name.substr( 1 );
			}
			else if( split_name.at( 0 ) == '{' )
			{
				std::regex reg( "\\{(.*)\\} (.*)" );
				std::smatch matches;
				std::regex_match( split_name, matches, reg );

				if( matches.size() > 2 )
				{
					m_name = matches[ 1 ].str();
					split_name = matches[ 2 ].str();
				}

				// This is the closing subsplit of the game.
				parsing_game = false;
			}
			else
			{
				m_name = split_name;

				// The game is only this split.
				parsing_game = false;
			}

			new_split.m_name = split_name;
			
			if( std::string icon = Utils::get_xml_child_element_text( segment, "Icon" ); icon.empty() == false )
				m_icon_desc = "<![CDATA[" + icon + "]]>";

			// Best segments are used for game time estimations.
			if( tinyxml2::XMLElement* best_time_el = segment->FirstChildElement( "BestSegmentTime" ) )
			{
				std::string best_time = Utils::get_xml_child_element_text( best_time_el, "RealTime" );
				std::stringstream stream;
				stream << best_time;
				SplitTime session_estimate{};
				std::chrono::from_stream( stream, "%H:%M:%S", session_estimate );
				estimate += session_estimate;
			}

			m_splits.push_back( new_split );

			segment = segment->NextSiblingElement( "Segment" );
		}

		m_estimation = estimate;

		return segment;
	}

	void Game::write_game( tinyxml2::XMLDocument& _document, tinyxml2::XMLElement* _segments )
	{
		if( _segments == nullptr )
			return;

		if( m_splits.size() == 1 )
		{
			tinyxml2::XMLElement* split{ _document.NewElement( "Segment" ) };

			Utils::create_xml_child_element_with_text( _document, split, "Name", m_splits.front().m_name.c_str() );
			Utils::create_xml_child_element_with_text( _document, split, "Icon", m_icon_desc.c_str() );

			if( m_estimation == SplitTime{} )
				Utils::create_xml_child_element_with_text( _document, split, "BestSegmentTime", "" );
			else
			{
				tinyxml2::XMLElement* best_segment{ _document.NewElement( "BestSegmentTime" ) };
				Utils::create_xml_child_element_with_text( _document, best_segment, "RealTime", std::format( "{:%H:%M:%S}", m_estimation ) );
				split->InsertEndChild( best_segment );
			}

			tinyxml2::XMLElement* split_times{ _document.NewElement( "SplitTimes" ) };
			tinyxml2::XMLElement* split_time{ _document.NewElement( "SplitTime" ) };
			split_time->SetAttribute( "name", "Personal Best" );
			split_times->InsertEndChild( split_time );
			split->InsertEndChild( split_times );

			_segments->InsertEndChild( split );

			return;
		}

		uint32_t session_number{ 0 };
		SplitTime session_estimate{ m_estimation / m_splits.size() };
		for( Split& split : m_splits )
		{
			tinyxml2::XMLElement* segment{ _document.NewElement( "Segment" ) };
			std::string split_name;

			if( &split == &m_splits.back() )
			{
				split_name = "{" + m_name + "} " + split.m_name;
				Utils::create_xml_child_element_with_text( _document, segment, "Icon", m_icon_desc.c_str() );
			}
			else
			{
				split_name = "-" + split.m_name;
				Utils::create_xml_child_element_with_text( _document, segment, "Icon", "" );
			}

			Utils::create_xml_child_element_with_text( _document, segment, "Name", split_name.c_str() );
			
			if( m_estimation == SplitTime{} )
				Utils::create_xml_child_element_with_text( _document, segment, "BestSegmentTime", "" );
			else
			{
				tinyxml2::XMLElement* best_segment{ _document.NewElement( "BestSegmentTime" ) };
				Utils::create_xml_child_element_with_text( _document, best_segment, "RealTime", std::format( "{:%H:%M:%S}", m_estimation ) );
				segment->InsertEndChild( best_segment );
			}

			tinyxml2::XMLElement* split_times{ _document.NewElement( "SplitTimes" ) };
			tinyxml2::XMLElement* split_time{ _document.NewElement( "SplitTime" ) };
			split_time->SetAttribute( "name", "Personal Best" );
			split_times->InsertEndChild( split_time );
			segment->InsertEndChild( split_times );

			_segments->InsertEndChild( segment );
		}
	}
}