#include <format>

#include <SFML/Graphics/Texture.hpp>

#include <Externals/json/json.h>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Tools.h>
#include <FZN/UI/ImGui.h>

#include "SplitsManager.h"
#include "Utils.h"


namespace SplitsMgr
{

	SplitsManager::SplitsManager()
	{
		g_pFZN_Core->AddCallback( this, &SplitsManager::on_event, fzn::DataCallbackType::Event );
	}

	SplitsManager::~SplitsManager()
	{
		g_pFZN_Core->RemoveCallback( this, &SplitsManager::on_event, fzn::DataCallbackType::Event );

		for( Game& game : m_games )
			g_pFZN_Core->RemoveCallback( &game, &Game::on_event, fzn::DataCallbackType::Event );
	}

	void SplitsManager::display_left_panel()
	{
		if( m_games.empty() )
			return;

		ImGui::PushStyleColor( ImGuiCol_Separator, ImGui_fzn::color::white );

		ImGui::BeginChild( "Games" );
		for( Game& game : m_games )
		{
			game.display();
		}
		ImGui::EndChild();

		ImGui::PopStyleColor();
	}

	void SplitsManager::display_right_panel()
	{
		_handle_actions();
		m_chrono.update();

		if( m_current_game == nullptr )
			return;

		ImVec4 timer_color = ImGui_fzn::color::white;

		if( m_chrono.is_paused() == false )
			timer_color = ImGui_fzn::color::light_green;
		else if( m_chrono.has_started() )
			timer_color = ImGui_fzn::color::gray;

		/*if( Utils::is_time_valid( m_delta ) == false )
			_update_run_stats();*/

		if( g_pFZN_InputMgr->IsActionPressed( "Refresh" ) )
			m_stats.refresh( m_games );

		ImGui::NewLine();
		std::string title = m_game_name;
		ImGui::SetWindowFontScale( 2.f );
		ImVec2 text_size = ImGui::CalcTextSize( title.c_str() );
		ImGui::NewLine();
		ImGui::SameLine( ImGui::GetContentRegionAvail().x * 0.5f - text_size.x * 0.5f );
		ImGui::Text( title.c_str() );
		ImGui::SetWindowFontScale( 1.f );
		ImGui::NewLine();

		_display_timers( timer_color );

		ImGui::SeparatorText( "List infos" );

		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Current split:", "%d", m_current_split );

		ImGui::Spacing();

		if( ImGui::BeginTable( "run_infos_2", 4 ) )
		{
			SplitTime timer{};
			SplitTime game_time{ m_current_game->get_played() + m_chrono.get_time() };
			SplitTime game_delta{ game_time - m_current_game->get_estimate() };
			const bool over_estimate{ game_time > m_current_game->get_estimate() };
			const SplitTime previous_delta{ m_current_game->get_played() - m_current_game->get_estimate() };

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Estimate:" );
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Played:" );
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Delta:" );

			ImGui::TableNextColumn();
			ImGui::Text( Utils::time_to_str( m_estimate ).c_str() );
			ImGui::TextColored( timer_color, Utils::time_to_str( m_played + m_chrono.get_time() ).c_str() );

			if( over_estimate )
				ImGui::TextColored( timer_color, Utils::time_to_str( m_delta + game_delta ).c_str() );
			else
				ImGui::Text( Utils::time_to_str( m_delta ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Rem. Time:" );
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Est. Final Time:" );

			ImGui::TableNextColumn();

			if( over_estimate )
				ImGui::TextColored( timer_color, Utils::time_to_str( m_remaining_time + previous_delta + game_delta ).c_str() );
			else
				ImGui::TextColored( timer_color, Utils::time_to_str( m_remaining_time - m_chrono.get_time() ).c_str() );

			if( over_estimate )
				ImGui::TextColored( timer_color, Utils::time_to_str( m_estimated_final_time + game_delta ).c_str() );
			else
				ImGui::Text( Utils::time_to_str( m_estimated_final_time ).c_str() );

			ImGui::EndTable();
		}

		m_stats.display();
		_display_update_sessions_buttons();
	}

	void SplitsManager::on_event()
	{
		const fzn::Event& fzn_event = g_pFZN_Core->GetEvent();

		if( fzn_event.m_eType != fzn::Event::eUserEvent || fzn_event.m_pUserData == nullptr )
			return;

		Event* split_event = static_cast< Event* >( fzn_event.m_pUserData );

		if( split_event == nullptr )
			return;

		switch( split_event->m_type )
		{
			case Event::Type::session_added:
			{
				_on_game_session_added( split_event->m_game_event );
				break;
			}
			case Event::Type::new_current_game_selected:
			{
				m_current_game = split_event->m_game_event.m_game;
				m_current_split = m_current_game->get_splits().back().m_split_index;
				g_pFZN_Core->PushEvent( new Event( Event::Type::json_done_reading ) );
				break;
			}
			case Event::Type::game_estimate_changed:
			{
				_update_run_stats();
				break;
			}
		};
	}

	/**
	* @brief Get the time of the run at the given split index.
	* @param _split_index The index of the split at which we want the run time.
	* @return The run time corresponding to the given split index.
	**/
	SplitTime SplitsManager::get_split_run_time( uint32_t _split_index ) const
	{
		for( const Game& game : m_games )
		{
			if( game.contains_split_index( _split_index ) == false )
				continue;

			return game.get_split_run_time( _split_index );
		}
	}

	/**
	* @brief Search for the last valid time in the "run", starting at the given game and then going back to previous ones.
	* @param _threshold_game The game from which we start looking for a valid run time.
	* @return The most recent valid run time. default SplitTime value if nothing has been found.
	**/
	SplitTime SplitsManager::get_last_valid_run_time( const Game* _threshold_game ) const
	{
		if( _threshold_game == nullptr )
			return {};

		SplitTime ret_time{};

		bool game_found{ false };
		for( int game_index{ (int)m_games.size() - 1 }; game_index >= 0; --game_index )
		{
			const Game& game{ m_games[ game_index ] };

			if( game_found == false )
			{
				if( &game == _threshold_game )
					game_found = true;
				else
					continue;
			}

			ret_time = game.get_run_time();

			if( Utils::is_time_valid( ret_time ) )
				break;
		}

		return ret_time;
	}

	void SplitsManager::read_lss( std::string_view _path )
	{
		auto xml_file = tinyxml2::XMLDocument{};

		if( xml_file.LoadFile( _path.data() ) )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Failure : %s (%s)", xml_file.ErrorName(), xml_file.ErrorStr() );
			return;
		}

		FZN_DBLOG( "Parsing %s...", _path.data() );

		tinyxml2::XMLElement* run = xml_file.FirstChildElement( "Run" );

		if( run == nullptr )
			return;

		m_games.clear();

		m_game_icon_desc = "\<![CDATA[" + Utils::get_xml_child_element_text( run, "GameIcon" ) + "]]\>";
		m_game_name = Utils::get_xml_child_element_text( run, "GameName" );
		m_category = Utils::get_xml_child_element_text( run, "CategoryName" );
		m_layout_path = Utils::get_xml_child_element_text( run, "LayoutPath" );

		const uint32_t final_year = std::stoul( m_category );
		const uint32_t nb_games = final_year - 1990;		// substracting 1991 then adding 1, because we do a game on the first year and the last, so one more than just final year - birth year.

		m_games.reserve( nb_games );

		tinyxml2::XMLElement* segments = run->FirstChildElement( "Segments" );

		if( segments == nullptr )
			return;

		tinyxml2::XMLElement* segment = segments->FirstChildElement( "Segment" );
		uint32_t split_index{ 0 };

		while( segment != nullptr )
		{
			auto game = Game{};

			segment = game.parse_game( segment, split_index );

			m_games.emplace_back( std::move( game ) );

			g_pFZN_Core->AddCallback( &m_games.back(), &Game::on_event, fzn::DataCallbackType::Event );
		}
	}

	void SplitsManager::read_json( std::string_view _path )
	{
		auto file = std::ifstream{ _path.data() };

		if( file.is_open() == false )
			return;

		auto root = Json::Value{};

		file >> root;
		
		m_current_split = root[ "CurrentSplitIndex" ].asUInt();
		m_run_time = Utils::get_time_from_string( root[ "CurrentTime" ].asString() );

		Json::Value splits = root[ "Splits" ];

		Json::Value::iterator it = splits.begin();
		SplitTime run_time{};

		for( Game& game : m_games )
		{
			// This game didn't fill all its splits, no need to go further.
			if( game.parse_split_times( it, run_time ) == false )
				break;
		}

		_refresh_current_game_ptr();

		g_pFZN_Core->PushEvent( new Event( Event::Type::json_done_reading ) );

		if( m_current_split == 0 )
			return;

		// If the last updated run time is the same as the one read in the json, it means the sessions can't be updated or it will create a segment time of 00:00:00
		if( run_time == m_run_time )
			m_sessions_updated = true;

		_update_run_stats();
	}

	void SplitsManager::load_covers( std::string_view _path )
	{
		for( Game& game : m_games )
			game.load_cover( _path );
	}

	void SplitsManager::read_all_in_one_file( std::string_view _path )
	{
		auto file = std::ifstream{ _path.data() };

		if( file.is_open() == false )
			return;

		auto root = Json::Value{};
		file >> root;

		m_games.clear();
		m_games.reserve( 100 );

		m_current_split = root[ "CurrentSplitIndex" ].asUInt();
		m_game_name = root[ "Title" ].asString();

		Json::Value games = root[ "Games" ];
		Json::Value::iterator it_game = games.begin();
		Utils::ParsingInfos parsing_infos{ m_current_split };

		for( Json::Value::iterator it_game = games.begin(); it_game != games.end(); ++it_game )
		{
			auto game = Game{};

			if( game.parse_game_aio( *it_game, parsing_infos ) )
			{
				m_run_time = game.get_run_time();
				g_pFZN_WindowMgr->SetWindowTitle( fzn::Tools::Sprintf( "1A1J - %s", game.get_name().c_str() ) );
			}

			m_games.emplace_back( std::move( game ) );
			g_pFZN_Core->AddCallback( &m_games.back(), &Game::on_event, fzn::DataCallbackType::Event );
		}

		m_played = parsing_infos.m_total_time;

		_refresh_current_game_ptr();

		_update_run_stats();
		m_sessions_updated = true;
	}

	void SplitsManager::write_lss( tinyxml2::XMLDocument& _document )
	{
		tinyxml2::XMLElement* run{ _document.NewElement( "Run" ) };
		run->SetAttribute( "version", "1.7.0" );
		_document.InsertEndChild( run );

		Utils::create_xml_child_element_with_text( _document, run, "GameIcon", m_game_icon_desc );
		Utils::create_xml_child_element_with_text( _document, run, "GameName", m_game_name );
		Utils::create_xml_child_element_with_text( _document, run, "CategoryName", m_category );
		Utils::create_xml_child_element_with_text( _document, run, "LayoutPath", m_layout_path );
		Utils::create_xml_child_element_with_text( _document, run, "Offset", "00:00:00" );
		Utils::create_xml_child_element_with_text( _document, run, "AttemptCount", fzn::Tools::Sprintf( "%u", m_nb_sessions ) );

		tinyxml2::XMLElement* segments{ _document.NewElement( "Segments" ) };

		for( Game& game : m_games )
		{
			game.write_game( _document, segments );
		}

		run->InsertEndChild( segments );
	}

	void SplitsManager::write_json( Json::Value& _root )
	{
		_root[ "CurrentSplitIndex" ] = m_current_split;
		_root[ "CurrentTime" ] = Utils::time_to_str( m_run_time, false, true ).c_str();

		for( const Game& game : m_games )
		{
			game.write_split_times( _root );
		}
	}

	void SplitsManager::write_all_in_one_file( Json::Value& _root )
	{
		_root[ "Title" ] = m_game_name.c_str();
		_root[ "CurrentSplitIndex" ] = m_current_split;

		for( uint32_t game_index{ 0 }; game_index < m_games.size(); ++game_index )
		{
			m_games[ game_index ].write_game_aio( _root, game_index );
		}
	}

	void SplitsManager::_refresh_current_game_ptr()
	{
		for( Game& game : m_games )
		{
			if( game.contains_split_index( m_current_split ) )
			{
				m_current_game = &game;
				return;
			}
		}
	}

	void SplitsManager::_update_sessions( bool _game_finished )
	{
		if( m_current_game == nullptr )
			return;

		SplitTime segment_time{};
		SplitTime run_time{};

		if( m_chrono.has_started() )
		{
			segment_time = m_chrono.get_time();
			m_chrono.stop();
			run_time = m_run_time + segment_time;
		}
		else
		{
			const SplitTime previous_run_time{ get_last_valid_run_time( m_current_game ) };
			segment_time = m_run_time - previous_run_time;
			run_time = m_run_time;
		}

		if( Utils::is_time_valid( segment_time ) == false )
			return;

		m_current_game->update_last_split( run_time, segment_time, _game_finished );

		m_sessions_updated = true;

		_update_games_data( m_current_game );
		_update_run_data();
		_update_run_stats();
	}

	void SplitsManager::_on_game_session_added( const Event::GameEvent& _event_infos )
	{
		_update_games_data( _event_infos.m_game );
		_update_run_data();
		_update_run_stats();
	}

	void SplitsManager::_display_timers( const ImVec4& _timer_color )
	{
		const float default_text_height = ImGui::CalcTextSize( "T" ).y;
		ImGui::SetWindowFontScale( 2.f );
		const float doubled_text_height = ImGui::CalcTextSize( "T" ).y;
		ImGui::SetWindowFontScale( 1.f );

		ImGui::SeparatorText( m_current_game->get_name().c_str() );

		if( m_current_game->get_cover() != nullptr )
			ImGui::Image( *m_current_game->get_cover(), Utils::game_cover_size );
		else
			ImGui::SetCursorPos( ImGui::GetCursorPos()+ ImVec2{ 0.f, Utils::game_cover_size.y + ImGui::GetStyle().ItemSpacing.y } );

		ImVec2 rect_size = ImGui::GetContentRegionAvail();
		rect_size.x -= ImGui::GetStyle().WindowPadding.x * 2.f;		// Left and Right window padding
		rect_size.x -= Utils::game_cover_size.x;					// The rectangle will be next to the cover so we take out its size
		rect_size.x -= ImGui::GetStyle().ItemSpacing.x;				// and the space between items.
		rect_size.y = Utils::game_cover_size.y;						// The rectangle is the same size as the cover.

		ImVec2 rect_pos = ImGui::GetCursorPos();
		rect_pos.x += Utils::game_cover_size.x;						// To place the rectangle next to the cover, we add its width
		rect_pos.x += ImGui::GetStyle().ItemSpacing.x;				// and the horizontal space between items.
		rect_pos.y -= Utils::game_cover_size.y;						// The cursors is under the cover so we substract its height
		rect_pos.y -= ImGui::GetStyle().ItemSpacing.y;				// and the vertical space between items.

		/*ImVec2 debug_rect_pos = ImGui::GetCursorScreenPos();
		debug_rect_pos.x += cover_size.x;							// To place the rectangle next to the cover, we add its width
		debug_rect_pos.x += ImGui::GetStyle().ItemSpacing.x;		// and the horizontal space between items.
		debug_rect_pos.y -= cover_size.y;							// The cursors is under the cover so we substract its height
		debug_rect_pos.y -= ImGui::GetStyle().ItemSpacing.y;		// and the vertical space between items.

		ImGui_fzn::rect_filled( { debug_rect_pos, rect_size }, ImGui_fzn::color::dark_gray );*/

		std::string time_str = Utils::time_to_str( m_chrono.get_time() );
		ImGui::SetWindowFontScale( 5.f );
		const ImVec2 session_time_size = ImGui::CalcTextSize( time_str.c_str() );
		ImGui::SetCursorPos( rect_pos + ImVec2{ rect_size.x * 0.5f - session_time_size.x * 0.5f, 0.f } );
		const ImVec2 session_time_pos = ImGui::GetCursorPos();
		ImGui::TextColored( _timer_color, time_str.c_str() );
		ImGui::SetWindowFontScale( 1.f );

		ImGui::SetCursorPos( session_time_pos + ImVec2{ 0.f, session_time_size.y } );
		if( ImGui::BeginTable( "timers", 2, 0, { session_time_size.x, 0.f} ) )
		{
			ImGui::TableSetupColumn( "labels", ImGuiTableColumnFlags_WidthFixed, 100.f );

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetCursorPos( { ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + doubled_text_height - default_text_height - ImGui::GetStyle().CellPadding.y } );
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Played" );

			ImGui::TableNextColumn();
			ImGui::SetWindowFontScale( 2.f );
			time_str = Utils::time_to_str( m_current_game->get_played() + m_chrono.get_time() );
			ImVec2 size = ImGui::CalcTextSize( time_str.c_str() );
			ImGui::NewLine();
			ImGui::SameLine( ImGui::GetContentRegionAvail().x - size.x );
			ImGui::TextColored( _timer_color, time_str.c_str() );
			ImGui::SetWindowFontScale( 1.f );

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetCursorPos( { ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + doubled_text_height - default_text_height - ImGui::GetStyle().CellPadding.y } );
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Estimate" );

			ImGui::TableNextColumn();
			ImGui::SetWindowFontScale( 2.f );
			time_str = Utils::time_to_str( m_current_game->get_estimate() );
			size = ImGui::CalcTextSize( time_str.c_str() );
			ImGui::NewLine();
			ImGui::SameLine( ImGui::GetContentRegionAvail().x - size.x );
			ImGui::TextColored( _timer_color, time_str.c_str() );
			ImGui::SetWindowFontScale( 1.f );

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetCursorPos( { ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + doubled_text_height - default_text_height - ImGui::GetStyle().CellPadding.y } );
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Delta" );

			ImGui::TableNextColumn();
			ImGui::SetWindowFontScale( 2.f );
			SplitTime delta = m_current_game->get_played() + m_chrono.get_time() - m_current_game->get_estimate();
			time_str = fzn::Tools::Sprintf( "%s%s", delta < std::chrono::seconds( 0 ) ? "" : "+", Utils::time_to_str( delta ).c_str() );
			size = ImGui::CalcTextSize( time_str.c_str() );
			ImGui::NewLine();
			ImGui::SameLine( ImGui::GetContentRegionAvail().x - size.x );
			ImGui::TextColored( _timer_color, time_str.c_str() );
			ImGui::SetWindowFontScale( 1.f );

			ImGui::EndTable();
		}

		ImGui::Spacing();
	}

	void SplitsManager::_display_update_sessions_buttons()
	{
		const float cursor_height{ ImGui::GetContentRegionMax().y - ImGui::GetFrameHeightWithSpacing() * 2.f - ImGui::GetStyle().FramePadding.y };
		ImGui::SetCursorPosY( cursor_height );

		ImGui::SeparatorText( "Update sessions" );

		const int nb_columns{ 3 };
		if( ImGui::BeginTable( "add_session_table", nb_columns ) )
		{
			const bool disable_buttons{ (m_current_game == nullptr || m_sessions_updated) && (m_chrono.has_started() == false || m_chrono.is_paused() == false) };

			if( disable_buttons )
				ImGui::BeginDisabled();

			const float column_size = ImGui::GetContentRegionAvail().x / nb_columns - ImGui::GetStyle().ItemSpacing.x / nb_columns;
			ImGui::TableSetupColumn( "btn1", ImGuiTableColumnFlags_WidthFixed, column_size );
			ImGui::TableSetupColumn( "btn2", ImGuiTableColumnFlags_WidthFixed, column_size );
			ImGui::TableSetupColumn( "btn3", ImGuiTableColumnFlags_WidthFixed, column_size );

			ImGui::TableNextColumn();
			if( ImGui::Button( "Game finished", { ImGui::GetContentRegionAvail().x, 0.f } ) )
				_update_sessions( true );

			ImGui::TableNextColumn();
			if( ImGui::Button( "Game still going", { ImGui::GetContentRegionAvail().x, 0.f } ) )
				_update_sessions( false );

			ImGui::TableNextColumn();
			if( ImGui::Button( "Crop run time", { ImGui::GetContentRegionAvail().x, 0.f } ) )
			{
				m_run_time = get_split_run_time( m_current_split - 1 );
				m_sessions_updated = true;
			}

			if( disable_buttons )
				ImGui::EndDisabled();

			ImGui::EndTable();
		}
	}

	/**
	* @brief Update splits index and run time of games coming after the given one.
	* @param _game The game that has been updated, and from which the update will start.
	**/
	void SplitsManager::_update_games_data( const Game* _game )
	{
		if( _game == nullptr )
			return;

		SplitTime run_time{};

		// Looping on all the games coming after the given one to update their data.
		bool game_found{ false };
		const Game* prev_game{ _game };		// The game that came before the one we're updating. Needed to retrieve game state.
		const SplitTime& last_segment_time{ _game->get_last_valid_segment_time() };
		for( Game& game : m_games )
		{
			if( game_found == false )
			{
				// We found the game, but we don't want to update this one, so we continue one last time.
				if( &game == _game )
					game_found = true;

				continue;
			}

			// Giving the last valid segment of the given game will make the current loop game adapt to it if it has times already
			// If the game is finished, we used its last split to set the new session time and didn't create a new split, so we don't need to increment the others.
			game.update_data( last_segment_time, _game->is_finished() == false );
		}
	}

	/**
	* @brief Update current split index, game, and global run time. Called after a session has been added to one of the games.
	**/
	void SplitsManager::_update_run_data()
	{
		// If the current game is still ongoing, we get the current split value from the games last split index.
		if( m_current_game->is_finished() == false )
		{
			m_current_split = m_current_game->get_splits().back().m_split_index;

			// Now we need to retrieve the closest valid run time to the current split.
			m_run_time = get_last_valid_run_time( m_current_game );
		}
		else
		{
			// If the current game is finished, we need to look for the next empty split, it can be the next one or not, depending on the ammount of sessions added ahead of time.
			bool game_found{ false };
			for( Game& game : m_games )
			{
				if( game_found == false )
				{
					// We found the game, we want to look for our split in the next game, se we continue one last time.
					if( &game == m_current_game )
					{
						game_found = true;
						m_run_time = game.get_run_time();
					}

					continue;
				}

				// If the game is finished, we can't use it as our current game, so we continue looking.
				if( game.is_finished() )
					continue;

				// If the game isn't finished, the last split won't have a run time, so that's the one we'll use.
				const Split& last_split{ game.get_splits().back() };

				m_current_split = last_split.m_split_index;
				m_current_game = &game;

				g_pFZN_Core->PushEvent( new Event( Event::Type::current_game_changed ) );

				return;
			}
		}
	}

	void SplitsManager::_update_run_stats()
	{
		m_nb_sessions = 0;
		m_estimate = SplitTime{};
		m_played = SplitTime{};
		m_delta = SplitTime{};
		m_remaining_time = SplitTime{};

		FZN_LOG( "Updating stats..." );

		for( const Game& game : m_games )
		{
			for( const Split& split : game.get_splits() )
			{
				if( Utils::is_time_valid( split.m_segment_time ) )
					++m_nb_sessions;
			}

			m_estimate += game.get_estimate();
			m_played += game.get_played();

			if( game.is_finished() )
				m_delta += game.get_delta();

			if( game.get_state() == Game::State::none )
				m_remaining_time += game.get_estimate();
			else if( game.get_state() == Game::State::ongoing || game.get_state() == Game::State::current )
			{
				const SplitTime played{ game.get_played() };

				if( played < game.get_estimate() )
				{
					SplitTime game_rem_time = game.get_estimate() - played;
					m_remaining_time += game_rem_time;
				}
			}

			FZN_LOG( "%s (%s) - est. %s / played %s / delta %s", game.get_name().c_str(), game.get_state_str(), Utils::time_to_str( game.get_estimate() ).c_str(), Utils::time_to_str( game.get_played() ).c_str(), Utils::time_to_str( game.get_delta() ).c_str() );
			FZN_LOG( "RUN - est. %s / rem. time %s / played %s / delta %s\n", Utils::time_to_str( m_estimate ).c_str(), Utils::time_to_str( m_remaining_time ).c_str(), Utils::time_to_str( m_played ).c_str(), Utils::time_to_str( m_delta ).c_str() );
		}

		m_estimated_final_time = m_remaining_time + m_played;
		m_stats.refresh( m_games );

		FZN_LOG( "Est. final time %s", Utils::time_to_str( m_estimated_final_time ).c_str() );
	}

	void SplitsManager::_handle_actions()
	{
		if( g_pFZN_InputMgr->IsActionPressed( "Start / Split" ) )
		{
			if( m_chrono.is_paused() && m_chrono.has_started() == false )
				m_chrono.start();
			else
			{
				_update_sessions( true );
				m_chrono.start();
			}
		}
		else if( g_pFZN_InputMgr->IsActionPressed( "Pause" ) )
		{
			m_chrono.toggle_pause();
		}
		else if( g_pFZN_InputMgr->IsActionPressed( "Stop" ) )
		{
			m_chrono.stop();
		}
	}
}
