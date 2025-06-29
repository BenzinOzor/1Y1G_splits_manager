#include <regex>
#include <format>

#include <tinyXML2/tinyxml2.h>

#include <FZN/Tools/Tools.h>
#include <FZN/UI/ImGui.h>
#include <FZN/Managers/FazonCore.h>

#include "Event.h"
#include "Game.h"
#include "SplitsManagerApp.h"


namespace SplitsMgr
{
	static constexpr float		current_game_text_size		{ 120.f };
	static constexpr float		split_index_column_size		{ 28.f };		// ImGui::CalcTextSize( "9999" ).x
	static constexpr float		session_column_size			{ 84.f };		// ImGui::CalcTextSize( "session 9999" ).x
	static constexpr float		segment_time_column_size	{ 150.f };

	static constexpr ImVec4		frame_bg_current_game		{ 0.58f, 0.43f, 0.03f, 1.f };


	static void display_split_infos( const Split& _split, bool _display_session )
	{
		ImGui::TableNextColumn();
		ImGui::Text( "%u", _split.m_split_index );

		ImGui::TableNextColumn();

		if( _display_session )
			ImGui::Text( "session %u", _split.m_session_index );

		ImGui::TableNextColumn();
		ImGui::Text( std::format( "{:%H:%M:%S}", _split.m_run_time ).c_str() );

		ImGui::TableNextColumn();
		ImGui::Text( std::format( "{:%H:%M:%S}", _split.m_segment_time ).c_str() );
	}

	void Game::display()
	{
		// Copying the current state to avoid it changing in the middle of the frame and have imgui push/pop mismatches.
		const State game_state{ m_state };
		ImGui::PushID( m_name.c_str() );
		_push_state_colors( game_state );

		const bool header_open = ImGui::CollapsingHeader( m_name.c_str(), is_current() ? ImGuiTreeNodeFlags_DefaultOpen : 0 );

		_right_click();

		if( Utils::is_time_valid( m_time ) )
		{
			std::string game_time{ std::format( "{:%H:%M:%S} ", m_time ) };
			const float game_time_width{ ImGui::CalcTextSize( game_time.c_str() ).x };

			ImGui::SameLine( ImGui::GetContentRegionAvail().x - game_time_width );
			ImGui::Text( game_time.c_str() );
		}

		if( is_current() )
		{
			ImGui::SameLine( ImGui::GetContentRegionAvail().x * 0.5f - current_game_text_size * 0.5f );
			ImGui::Text( "- Current Game -" );
		}
		
		_pop_state_colors( game_state );

		if( header_open )
		{
			_handle_game_background( game_state );

			ImGui::Indent();
			if( ImGui::BeginTable( "splits_infos", 4 ) )
			{
				ImGui::TableSetupColumn( "split_index",		ImGuiTableColumnFlags_WidthFixed, split_index_column_size );
				ImGui::TableSetupColumn( "session",			ImGuiTableColumnFlags_WidthFixed, session_column_size );
				ImGui::TableSetupColumn( "time",			ImGuiTableColumnFlags_WidthStretch );
				ImGui::TableSetupColumn( "segment_time",	ImGuiTableColumnFlags_WidthFixed, segment_time_column_size );

				for( Split& split : m_splits )
				{
					display_split_infos( split, m_splits.size() > 1 );
				}

				ImGui::EndTable();
			}

			
			if( is_finished() == false && ImGui::BeginTable( "add_session_table", 2 ) )
			{
				ImGui::TableSetupColumn( "input_text", ImGuiTableColumnFlags_WidthFixed, ImGui::GetContentRegionAvail().x * 0.5f );
				ImGui::TableSetupColumn( "button", ImGuiTableColumnFlags_WidthFixed );

				ImGui::TableNextColumn();
				ImGui::PushItemWidth( -1 );
				ImGui::InputTextWithHint( "##new_session_time", "00:00:00", &m_new_session_time );
				ImGui::PopItemWidth();

				ImGui::TableNextColumn();

				ImGui::Checkbox( "Game Finished", &m_new_session_game_finished );
				ImGui::SameLine();
				if( ImGui_fzn::deactivable_button( "Add", m_new_session_time.empty() ) )
					_add_new_session_time();
				ImGui::EndTable();
			}
			ImGui::Unindent();
		}

		ImGui::PopID();
	}

	void Game::on_event()
	{
		const fzn::Event& fzn_event = g_pFZN_Core->GetEvent();

		if( fzn_event.m_eType != fzn::Event::eUserEvent || fzn_event.m_pUserData == nullptr )
			return;

		Event* split_event = static_cast<Event*>( fzn_event.m_pUserData );

		if( split_event == nullptr )
			return;

		switch( split_event->m_type )
		{
			case Event::Type::json_done_reading:
			case Event::Type::current_game_changed:
			{
				_refresh_state();
				break;
			}
		};
	}

	bool Game::contains_split_index( uint32_t _index ) const
	{
		if( m_splits.empty() )
			return false;

		return m_splits.front().m_split_index <= _index && m_splits.back().m_split_index >= _index;
	}

	SplitsMgr::SplitTime Game::get_run_time() const
	{
		SplitTime last_valid_time{};

		for( const Split& split : m_splits )
		{
			if( Utils::is_time_valid( split.m_run_time ) )
				last_valid_time = split.m_run_time;
		}

		return last_valid_time;
	}

	/**
	* @brief Get the time of the run at the given split index.
	* @param _split_index The index of the split at which we want the run time.
	* @return The run time corresponding to the given split index.
	**/
	SplitTime Game::get_split_run_time( uint32_t _split_index ) const
	{
		if( auto it_split = std::ranges::find( m_splits, _split_index, &Split::m_split_index ); it_split != m_splits.end() )
			return it_split->m_run_time;

		return {};
	}

	/**
	* @brief Update the last split available on the game because a session has just been made.
	* @param _run_time The new (total) run time tu put in the split.
	* @param _segment_time The time of this split. The previous one isn't necessarily in the same game so it has to be given.
	* @param _game_finished True if the game is finished with this new time. If not, some adaptations tho the splits will be needed.
	**/
	void Game::update_last_split( const SplitTime& _run_time, const SplitTime& _segment_time, bool _game_finished )
	{
		Split& last_split{ m_splits.back() };
		last_split.m_run_time = _run_time;
		last_split.m_segment_time = _segment_time;

		_refresh_game_time();

		if( _game_finished == false )
		{
			Split new_split{ .m_split_index = last_split.m_split_index + 1, .m_session_index = last_split.m_session_index + 1 };
			m_splits.push_back( new_split );
		}

		_refresh_state();
	}

	/**
	* @brief Increment all split indexes because a session has been added before this game.
	**/
	void Game::update_data( SplitTime& _last_run_time, bool _incremeted_splits_index )
	{
		for( Split& split : m_splits )
		{
			if( _incremeted_splits_index )
				++split.m_split_index;

			// If we already have a run time and a new one is given, we calculate a new run time with the given one.
			if( Utils::is_time_valid( split.m_run_time ) && Utils::is_time_valid( _last_run_time ) )
			{
				split.m_run_time = _last_run_time + split.m_segment_time;
				_last_run_time = split.m_run_time;
			}
		}
	}

	/**
	* @brief Parse splits file to fill the games sessions.
	* @param [in]		_element Pointer to the current xml element. Will be modified while retrieving splits informations.
	* @param [in,out]	_split_index Current split index value. To be stored in the created splits and incremented as the parsing goes along.
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

			if( split_name.at( 0 ) == '{' )
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
			else if( split_name.at( 0 ) != '-' )
			{
				m_name = split_name;

				// The game is only this split.
				parsing_game = false;
			}
			
			if( std::string icon = Utils::get_xml_child_element_text( segment, "Icon" ); icon.empty() == false )
				m_icon_desc = "<![CDATA[" + icon + "]]>";

			// Best segments are used for game time estimations.
			if( tinyxml2::XMLElement* best_time_el = segment->FirstChildElement( "BestSegmentTime" ) )
			{
				estimate += Utils::get_time_from_string( Utils::get_xml_child_element_text( best_time_el, "RealTime" ) );
			}

			new_split.m_session_index = m_splits.size() + 1;
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
	bool Game::parse_split_times( Json::Value::iterator& _it_splits, SplitTime& _last_time )
	{
		for( Split& split : m_splits )
		{
			split.m_run_time = Utils::get_time_from_string( (*_it_splits)[ "Time" ].asString() );

			// If the run time indicates 0, we're at
			if( Utils::is_time_valid( split.m_run_time ) == false )
				return false;

			split.m_segment_time = split.m_run_time - _last_time;
			_last_time = split.m_run_time;

			m_time += split.m_segment_time;

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

			Utils::create_xml_child_element_with_text( _document, split, "Name", m_name.c_str() );
			Utils::create_xml_child_element_with_text( _document, split, "Icon", m_icon_desc.c_str() );

			if( Utils::is_time_valid( m_estimation ) == false )
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
				split_name = "{" + m_name + "} " + fzn::Tools::Sprintf( "session %u", split.m_session_index );
				Utils::create_xml_child_element_with_text( _document, segment, "Icon", m_icon_desc.c_str() );
			}
			else
			{
				split_name = "-" + fzn::Tools::Sprintf( "session %u", split.m_session_index );
				Utils::create_xml_child_element_with_text( _document, segment, "Icon", "" );
			}

			Utils::create_xml_child_element_with_text( _document, segment, "Name", split_name.c_str() );
			
			if( Utils::is_time_valid( m_estimation ) == false )
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

	void Game::write_split_times( Json::Value& _root ) const
	{
		auto get_split_name = [ this ]( const Split& _split )
		{
			if( m_splits.size() == 1 )
				return m_name;

			if( _split.m_session_index < m_splits.size() )
				return std::format( "-session {}", _split.m_session_index );

			return fzn::Tools::Sprintf( "{%s} session %u", m_name.c_str(), _split.m_session_index );
		};

		for( const Split& split : m_splits )
		{
			_root[ "Splits" ][ split.m_split_index ][ "Time" ] = split.m_run_time != SplitTime{} ? std::format( "{:%H:%M:%S}", split.m_run_time ).c_str() : Json::Value{};
			_root[ "Splits" ][ split.m_split_index ][ "Name" ] = get_split_name( split );
		}
	}

	/**
	* @brief Add a new session to the game using m_new_session_time.
	**/
	void Game::_add_new_session_time()
	{
		if( m_new_session_time.empty() )
			return;

		SplitTime new_segment_time = Utils::get_time_from_string( m_new_session_time );

		m_new_session_time.clear();

		if( Utils::is_time_valid( new_segment_time ) == false )
			return;

		const uint32_t last_split_index{ m_splits.back().m_split_index };
		SplitTime run_time = g_splits_app->get_splits_manager().get_last_valid_run_time( this );

		if( Utils::is_time_valid( run_time ) )
			run_time += new_segment_time;

		update_last_split( run_time, new_segment_time, m_new_session_game_finished );

		Event* game_event = new Event( Event::Type::session_added );
		game_event->m_game_event.m_game = this;
		game_event->m_game_event.m_split = last_split_index;
		game_event->m_game_event.m_game_finished = m_new_session_game_finished;

		g_pFZN_Core->PushEvent( game_event );
	}

	void Game::_refresh_game_time()
	{
		m_time = SplitTime{};

		for( Split& split : m_splits )
		{
			m_time += split.m_segment_time;
		}
	}

	void Game::_refresh_state()
	{
		const Game* current_game{ g_splits_app->get_current_game() };

		if( current_game != nullptr && current_game == this )
		{
			m_state = State::current;
			return;
		}

		// If the last split doesn't have a segment time, it meas the game is still ready to recieve new sessions.
		// If there is a segment time, it means we don't want to add sessions anymore, and the game is finished.
		if( Utils::is_time_valid( m_splits.back().m_segment_time ) )
		{
			m_state = State::finished;
			return;
		}

		// An ongoing game will have more than one split, as there is always an empty one for its next session in addition to already submitted sessions.
		if( m_splits.size() > 1 )
		{
			m_state = State::ongoing;
			return;
		}

		// If no condition above matched, it means the game has been untouched for now, and doesn't have any state.
		m_state = State::none;
	}

	void Game::_push_state_colors( State _state )
	{
		switch( _state )
		{
			case State::current:
			{
				ImGui::PushStyleColor( ImGuiCol_Text,			ImGui_fzn::color::black );
				ImGui::PushStyleColor( ImGuiCol_Header,			Utils::Color::current_game_header );
				ImGui::PushStyleColor( ImGuiCol_HeaderHovered,	Utils::Color::current_game_header_hovered );
				ImGui::PushStyleColor( ImGuiCol_HeaderActive,	Utils::Color::current_game_header_active );
				break;
			}
			case State::finished:
			{
				ImGui::PushStyleColor( ImGuiCol_Text,			ImGui_fzn::color::black );
				ImGui::PushStyleColor( ImGuiCol_Header,			Utils::Color::finished_game_header );
				ImGui::PushStyleColor( ImGuiCol_HeaderHovered,	Utils::Color::finished_game_header_hovered );
				ImGui::PushStyleColor( ImGuiCol_HeaderActive,	Utils::Color::finished_game_header_active );
				break;
			}
			case State::ongoing:
			{
				ImGui::PushStyleColor( ImGuiCol_Text,			ImGui_fzn::color::black );
				ImGui::PushStyleColor( ImGuiCol_Header,			Utils::Color::ongoing_game_header );
				ImGui::PushStyleColor( ImGuiCol_HeaderHovered,	Utils::Color::ongoing_game_header_hovered );
				ImGui::PushStyleColor( ImGuiCol_HeaderActive,	Utils::Color::ongoing_game_header_active );
				break;
			}
		};
	}

	void Game::_pop_state_colors( State _state )
	{
		switch( _state )
		{
			case State::current:
			case State::finished:
			case State::ongoing:
			{
				ImGui::PopStyleColor( 4 );
				break;
			}
		};
	}

	void Game::_handle_game_background( State _state )
	{
		if( _state == State::none )
			return;

		const ImVec2 rect_top_left{ ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y - ImGui::GetStyle().ItemSpacing.y };

		ImVec2 rect_size{ ImGui::GetContentRegionAvail().x, 0.f };

		// splits lines
		rect_size.y += ImGui::GetStyle().ItemSpacing.y + ImGui::GetTextLineHeightWithSpacing() * m_splits.size();
		rect_size.y += ImGui::GetStyle().ItemSpacing.y;

		// Add session line
		if( is_current() || m_state == State::ongoing )
			rect_size.y += ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

		ImVec4 frame_bg_color{};

		switch( _state )
		{
			case SplitsMgr::Game::State::current:
			{
				frame_bg_color = frame_bg_current_game;
				break;
			}
			case SplitsMgr::Game::State::finished:
			{
				frame_bg_color = Utils::Color::finished_game_frame_bg;
				break;
			}
			case SplitsMgr::Game::State::ongoing:
			{
				frame_bg_color = Utils::Color::ongoing_game_frame_bg;
				break;
			}
			default:
			{
				frame_bg_color = ImGui_fzn::color::transparent;
				break;
			}
		}

		ImGui_fzn::rect_filled( { rect_top_left, rect_size }, frame_bg_color );
	}

	void Game::_right_click()
	{
		if( ImGui::BeginPopupContextItem("bjr") )
		{
			ImGui::PushStyleColor( ImGuiCol_Text, ImGui_fzn::color::white );
			
			const bool disable_item{ is_current() || is_finished() };

			if( disable_item )
				ImGui::BeginDisabled();

			if( ImGui::Selectable( "Set Current Game" ) )
			{
				Event* game_event = new Event( Event::Type::new_current_game_selected );
				game_event->m_game_event.m_game = this;

				g_pFZN_Core->PushEvent( game_event );
			}

			if( disable_item )
				ImGui::EndDisabled();

			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}
	}
}