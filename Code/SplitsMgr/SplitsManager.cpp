#include <format>

#include <Externals/json/json.h>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Tools.h>

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

	void SplitsManager::display()
	{
		if( m_games.empty() )
			return;

		if( Utils::is_time_valid( m_delta ) == false )
			_update_run_stats();

		ImGui::NewLine();
		std::string title = m_game_name + " - " + m_category;
		ImGui::SetWindowFontScale( 2.f );
		ImVec2 text_size = ImGui::CalcTextSize( title.c_str() );
		ImGui::NewLine();
		ImGui::SameLine( ImGui::GetContentRegionAvail().x * 0.5f - text_size.x * 0.5f );
		ImGui::Text( title.c_str() );
		ImGui::SetWindowFontScale( 1.f );
		ImGui::NewLine();

		if( m_current_game != nullptr )
			ImGui::Text( "Current game: %s (split %u)", m_current_game->get_name().c_str(), m_current_split );

		if( ImGui::BeginTable( "run_infos", 5 ) )
		{
			ImGui::TableSetupColumn( "Estimate" );
			ImGui::TableSetupColumn( "Played" );
			ImGui::TableSetupColumn( "Delta" );
			ImGui::TableSetupColumn( "Rem. Time" );
			ImGui::TableSetupColumn( "Est. Final Time" );

			ImGui::TableHeadersRow();
			ImGui::TableNextColumn();
			ImGui::Text( Utils::time_to_str( m_estimate ).c_str() );

			ImGui::TableNextColumn();
			ImGui::Text( Utils::time_to_str( m_run_time ).c_str() );

			ImGui::TableNextColumn();
			ImGui::Text( Utils::time_to_str( m_delta ).c_str() );

			ImGui::EndTable();
		}

		ImGui::Text( "Current run time:" );
		ImGui::SameLine();
		ImGui::Text( Utils::time_to_str( m_run_time ).c_str() );

		ImGui::Text( "Number of sessions: %u", m_nb_sessions );

		ImGui::SeparatorText( "Update sessions" );

		if( ImGui::BeginTable( "add_session_table", 2 ) )
		{
			const bool disable_buttons{ m_current_game == nullptr || m_sessions_updated };

			if( disable_buttons )
				ImGui::BeginDisabled();

			const float column_size = ImGui::GetContentRegionAvail().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;
			ImGui::TableSetupColumn( "btn1", ImGuiTableColumnFlags_WidthFixed, column_size );
			ImGui::TableSetupColumn( "btn2", ImGuiTableColumnFlags_WidthFixed, column_size );

			ImGui::TableNextColumn();
			if( ImGui::Button( "Game finished", { ImGui::GetContentRegionAvail().x, 0.f } ) )
				_update_sessions( true );

			ImGui::TableNextColumn();
			if( ImGui::Button( "Game still going", { ImGui::GetContentRegionAvail().x, 0.f } ) )
				_update_sessions( false );

			if( disable_buttons )
				ImGui::EndDisabled();

			ImGui::EndTable();
		}

		ImGui::PushStyleColor( ImGuiCol_Separator, ImGui_fzn::color::white );

		for( Game& game : m_games )
		{
			game.display();
		}

		ImGui::PopStyleColor();
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
		_root[ "CurrentTime" ] = Utils::time_to_str( m_run_time, false ).c_str();

		for( const Game& game : m_games )
		{
			game.write_split_times( _root );
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

		const SplitTime previous_run_time{ get_last_valid_run_time( m_current_game ) };
		const SplitTime new_segment_time{ m_run_time - previous_run_time };

		m_current_game->update_last_split( m_run_time, new_segment_time, _game_finished );

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
		for( Game& game : m_games )
		{
			if( game_found == false )
			{
				// We found the game, but we don't want to update this one, so we continue one last time.
				if( &game == _game )
					game_found = true;

				continue;
			}

			const SplitTime game_run_time{ prev_game->get_run_time() };

			// We want to update the game run time if:
			//  - The previous game is finished. Meaning we caught up with previously added isolated sessions with the current game.
			const bool current_game_caught_up = _game == m_current_game && prev_game->is_finished();
			//  - The game run time is valid. Meaning we added a session a game before the current one, and want to adapt its time with the added session time.
			const bool valid_game_run_time = Utils::is_time_valid( game.get_run_time() );

			if( current_game_caught_up || valid_game_run_time )
				run_time = game_run_time;

			// Giving a valid run time to the game will make it adapt to it. Otherwise nothing but its split index will be updated.
			// If the game is finished, we used its last split to set the new session time and didn't create a new split, so we don't need to increment the others.
			game.update_data( run_time, _game->is_finished() == false );
			prev_game = &game;
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
						game_found = true;

					continue;
				}

				// If the game is finished, we can't use it as our current game, so we continue looking.
				if( game.is_finished() )
					continue;

				// If the game isn't finished, the last split won't have a run time, so that's the one we'll use.
				const Split& last_split{ game.get_splits().back() };

				m_current_split = last_split.m_split_index;
				m_current_game = &game;
				m_run_time = game.get_run_time();
			}
		}
	}

	void SplitsManager::_update_run_stats()
	{
		m_nb_sessions = 0;

		for( const Game& game : m_games )
		{
			for( const Split& split : game.get_splits() )
			{
				if( Utils::is_time_valid( split.m_segment_time ) )
					++m_nb_sessions;
			}

			m_estimate += game.get_estimate();
			m_delta += game.get_delta();
		}
	}

}
