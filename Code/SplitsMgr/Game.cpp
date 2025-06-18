#include <regex>
#include <format>

#include <tinyXML2/tinyxml2.h>

#include <FZN/UI/ImGui.h>

#include "Game.h"
#include "Utils.h"


namespace SplitsMgr
{
	void Game::display()
	{
		if( ImGui::CollapsingHeader( m_name.c_str() ) )
		{
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
		}
	}

	tinyxml2::XMLElement* Game::parse_game( tinyxml2::XMLElement* _element )
	{
		if( _element == nullptr )
			return nullptr;

		tinyxml2::XMLElement* segment = _element;

		bool parsing_game{ true };

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

			m_splits.push_back( new_split );

			segment = segment->NextSiblingElement( "Segment" );
		}

		return segment;
	}
}