#include <regex>
#include <format>

#include <tinyXML2/tinyxml2.h>

#include <FZN/Tools/Tools.h>
#include <FZN/UI/ImGui.h>

#include "Game.h"
#include "SplitsManagerApp.h"


namespace SplitsMgr
{
	static constexpr float split_index_column_size = 28.f;	// ImGui::CalcTextSize( "9999" ).x
	static constexpr float session_column_size = 84;		// ImGui::CalcTextSize( "session 9999" ).x

	void Game::display()
	{
		const bool is_current_game = g_splits_app->has_json_been_opened() && contains_split_index( g_splits_app->get_current_split_index() );

		if( is_current_game )
		{
			ImGui::PushStyleColor( ImGuiCol_Text, ImGui_fzn::color::black );
			ImGui::PushStyleColor( ImGuiCol_Header, ImGui_fzn::color::light_yellow );
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImGui_fzn::color::bright_yellow );
			ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImGui_fzn::color::white );
		}

		const bool header_open = ImGui::CollapsingHeader( m_name.c_str(), is_current_game ? ImGuiTreeNodeFlags_DefaultOpen : 0 );
		
		if( is_current_game )
		{
			const float text_size = ImGui::CalcTextSize( "- Current Game - " ).x;
			ImGui::SameLine();
			ImGui::Text( "" );
			ImGui::SameLine( ImGui::GetContentRegionAvail().x - text_size );
			ImGui::Text( "- Current Game -" );
			ImGui::PopStyleColor( 4 );
		}

		if( header_open )
		{
			if( is_current_game )
			{
				const ImVec2 rect_top_left{ ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y - ImGui::GetStyle().ItemSpacing.y };

				ImVec2 rect_size{ ImGui::GetContentRegionAvail().x, 0.f };

				rect_size.y += ImGui::GetStyle().ItemSpacing.y + ImGui::GetTextLineHeightWithSpacing() * m_splits.size();
				rect_size.y += ImGui::GetStyle().ItemSpacing.y * 2.f + ImGui::GetFrameHeightWithSpacing();

				ImGui_fzn::rect_filled( { rect_top_left, rect_size }, { 0.58f, 0.43f, 0.03f, 1.f } );
			}

			ImGui::Indent();
			if( ImGui::BeginTable( "splits_infos", 3 ) )
			{
				ImGui::TableSetupColumn( "split_index", ImGuiTableColumnFlags_WidthFixed, split_index_column_size );
				ImGui::TableSetupColumn( "session", ImGuiTableColumnFlags_WidthFixed, session_column_size );
				ImGui::TableSetupColumn( "time", ImGuiTableColumnFlags_WidthFixed );

				if( m_splits.size() > 1 )
				{
					for( Split& split : m_splits )
					{
						ImGui::TableNextColumn();
						ImGui::Text( "%u", split.m_index );
						ImGui::TableNextColumn();
						ImGui::Text( "%s", split.m_name.c_str() );
						ImGui::TableNextColumn();
						ImGui::Text( std::format( "{:%H:%M:%S}", split.m_run_time ).c_str() );
					}
				}
				else
				{
					ImGui::TableNextColumn();
					const Split& split = m_splits.front();
					ImGui::Text( "%u", split.m_index );
					ImGui::TableNextColumn();
					ImGui::TableNextColumn();
					ImGui::Text( std::format( "{:%H:%M:%S}", split.m_run_time ).c_str() );
				}
				ImGui::EndTable();
			}

			ImGui::PushID( m_name.c_str() );
			if( ImGui::BeginTable( "add_session_table", 2 ) )
			{
				ImGui::TableSetupColumn( "input_text", ImGuiTableColumnFlags_WidthFixed, ImGui::GetContentRegionAvail().x * 0.5f );
				ImGui::TableSetupColumn( "button", ImGuiTableColumnFlags_WidthFixed );

				ImGui::TableNextColumn();
				ImGui::PushItemWidth( -1 );
				ImGui::InputTextWithHint( "##new_session_time", "00:00:00", &m_new_session_time );
				ImGui::PopItemWidth();

				ImGui::TableNextColumn();
				ImGui::Button( "Add" );
				ImGui::EndTable();
			}
			ImGui::Unindent();
			ImGui::PopID();
		}
	}

	bool Game::contains_split_index( uint32_t _index ) const
	{
		if( m_splits.empty() )
			return false;

		return m_splits.front().m_index <= _index && m_splits.back().m_index >= _index;
	}

	/**
	* @brief Parse splits file to fill the games sessions.
	* @param [in,out] _element Pointer to the current xml element. Will be modified while retrieving splits informations.
	* @param [in,out] _split_index Current split index value. To be stored in the created splits and incremented as the parsing goes along.
	**/
	tinyxml2::XMLElement* Game::parse_game( tinyxml2::XMLElement* _element, uint32_t& _split_index )
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

			Split new_split{ _split_index };

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
				/*std::string best_time = Utils::get_xml_child_element_text( best_time_el, "RealTime" );
				std::stringstream stream;
				stream << best_time;
				SplitTime session_estimate{};
				std::chrono::from_stream( stream, "%H:%M:%S", session_estimate );
				estimate += session_estimate;*/
				estimate += Utils::get_time_from_string( Utils::get_xml_child_element_text( best_time_el, "RealTime" ) );
			}

			m_splits.push_back( new_split );

			segment = segment->NextSiblingElement( "Segment" );
			++_split_index;
		}

		m_estimation = estimate;

		return segment;
	}

	/**
	* @brief Parse split times for this game. Will iterate in the array as long as it has splits.
	* @param [in,out] _it_splits The current iterator in the times read from the json file. Will be incremented while reading the times for the splits.
	* @return True if all the splits have retrieved a time. False if at least one doesn't have a time, meaning the timer stopped here and there's no need to go further.
	**/
	bool Game::parse_split_times( Json::Value::iterator& _it_splits )
	{
		for( Split& split : m_splits )
		{
			split.m_run_time = Utils::get_time_from_string( (*_it_splits)[ "Time" ].asString() );

			// If the run time indicates 0, we're at
			if( split.m_run_time == SplitTime{} )
				return false;

			++_it_splits;
		}
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