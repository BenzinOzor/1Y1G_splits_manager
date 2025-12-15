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

		if( m_finished_game != nullptr )
		{
			if( m_finished_game->display_finished_stats() == false )
				m_finished_game = nullptr;
		}

		if( m_current_game == nullptr )
			return;

		ImVec4 timer_color = ImGui_fzn::color::white;

		if( m_chrono.is_paused() == false )
			timer_color = ImGui_fzn::color::light_green;
		else if( m_chrono.has_started() )
			timer_color = ImGui_fzn::color::gray;

		if( g_pFZN_InputMgr->IsActionPressed( "Refresh" ) )
			m_stats.refresh( m_games );

		ImGui::NewLine();
		std::string title = m_title;
		ImGui::SetWindowFontScale( 2.f );
		ImVec2 text_size = ImGui::CalcTextSize( title.c_str() );
		ImGui::NewLine();
		ImGui::SameLine( ImGui::GetContentRegionAvail().x * 0.5f - text_size.x * 0.5f );
		ImGui::Text( title.c_str() );
		ImGui::SetWindowFontScale( 1.f );
		ImGui::NewLine();

		_display_timers( timer_color );
		_display_controls();

		if( m_current_game != nullptr )
			m_current_game->display_end_date_predition();

		ImGui::SeparatorText( "List infos" );

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
		//_display_update_sessions_buttons();
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

				if( split_event->m_game_event.m_game_finished )
					m_finished_game = split_event->m_game_event.m_game;
				break;
			}
			case Event::Type::new_current_game_selected:
			{
				if( m_current_game != nullptr )
					m_current_game->set_state( m_current_game->has_sessions() ? Game::State::ongoing : Game::State::none );

				m_current_game = split_event->m_game_event.m_game;
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
	* @brief Open and read the Json file containing all games informations.
	* @param _path The path to the Json file.
	**/
	void SplitsManager::read_json( std::string_view _path )
	{
		auto file = std::ifstream{ _path.data() };

		if( file.is_open() == false )
			return;

		auto root = Json::Value{};
		file >> root;

		m_games.clear();
		m_games.reserve( 100 );

		m_title = root[ "Title" ].asString();

		Json::Value games = root[ "Games" ];
		Json::Value::iterator it_game = games.begin();
		Utils::ParsingInfos parsing_infos{};
		bool is_current_game{ false };

		for( Json::Value::iterator it_game = games.begin(); it_game != games.end(); ++it_game )
		{
			auto game = Game{};
			is_current_game = game.read( *it_game, parsing_infos );

			m_games.emplace_back( std::move( game ) );
			g_pFZN_Core->AddCallback( &m_games.back(), &Game::on_event, fzn::DataCallbackType::Event );

			if( is_current_game )
			{
				m_current_game = &m_games.back();
				m_run_time = m_current_game->get_run_time();
				g_pFZN_WindowMgr->SetWindowTitle( fzn::Tools::Sprintf( "1A1J - %s", m_current_game->get_name().c_str() ) );
			}
		}

		m_played = parsing_infos.m_total_time;

		_update_run_stats();
		m_sessions_updated = true;
	}

	/**
	* @brief Write games informations in the given Json root.
	* @param [in out] _root The Json root that will hold all the games informations
	**/
	void SplitsManager::write_json( Json::Value& _root )
	{
		_root[ "Title" ] = m_title.c_str();

		for( uint32_t game_index{ 0 }; game_index < m_games.size(); ++game_index )
		{
			m_games[ game_index ].write( _root[ "Games" ][ game_index ] );
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
			_stop();
			run_time = m_run_time + segment_time;
		}

		if( Utils::is_time_valid( segment_time ) == false )
			return;

		Game::State new_state{ Game::State::ongoing };

		if( _game_finished )
			new_state = Game::State::finished;
		else if( m_current_game->is_current() )
			new_state = Game::State::current;

		m_current_game->add_session( segment_time, Utils::today(), new_state );

		if( _game_finished )
			m_finished_game = m_current_game;

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
		ImGui::Spacing();
	}

	void SplitsManager::_display_controls()
	{
		const bool disable_start_split{ m_chrono.has_started() && m_chrono.is_paused() };
		const bool disable_pause_stop{ m_chrono.has_started() == false };
		const float button_size{ 75.f };
		const float all_buttons_size{ button_size * 3.f + ImGui::GetStyle().ItemSpacing.x * 2.f };

		ImGui::Separator();

		ImGui::NewLine();
		ImGui::SameLine( ImGui::GetContentRegionAvail().x * 0.5f - all_buttons_size * 0.5f - ImGui::GetStyle().WindowPadding.x );

		if( ImGui_fzn::deactivable_button( m_chrono.has_started() ? "Split" : "Start", disable_start_split, false, { button_size, 0.f } ) )
			_start_split();

		if( disable_pause_stop )
			ImGui::BeginDisabled();

		ImGui::SameLine();
		if( ImGui::Button( m_chrono.is_paused() && m_chrono.has_started() ? "Resume" : "Pause", { button_size, 0.f } ) )
			_toggle_pause();


		ImGui::SameLine();
		if( ImGui::Button( "Stop", { button_size, 0.f } ) )
			_stop();

		if( disable_pause_stop )
			ImGui::EndDisabled();
	}

	void SplitsManager::_display_update_sessions_buttons()
	{
		const float cursor_height{ ImGui::GetContentRegionMax().y - ImGui::GetFrameHeightWithSpacing() * 2.f - ImGui::GetStyle().FramePadding.y };
		ImGui::SetCursorPosY( cursor_height );

		ImGui::SeparatorText( "Update sessions" );

		const int nb_columns{ 2 };
		if( ImGui::BeginTable( "add_session_table", nb_columns ) )
		{
			const bool disable_buttons{ (m_current_game == nullptr || m_sessions_updated) && (m_chrono.has_started() == false || m_chrono.is_paused() == false) };

			if( disable_buttons )
				ImGui::BeginDisabled();

			const float column_size = ImGui::GetContentRegionAvail().x / nb_columns - ImGui::GetStyle().ItemSpacing.x / nb_columns;
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

			// Giving the last valid segment of the given game will make the current loop game adapt to it.
			game.update_data( last_segment_time );
		}
	}

	/**
	* @brief Update current game and global run time. Called after a session has been added to one of the games.
	**/
	void SplitsManager::_update_run_data()
	{
		if( m_current_game == nullptr )
			return;

		// Refresh run time from the current game, finished or not.
		m_run_time = m_current_game->get_run_time();

		if( m_current_game->is_finished() )
		{
			// If the current game is finished, we look for the next eligible game to be the current one.
			bool game_found{ false };
			for( Game& game : m_games )
			{
				// First, we look for the current game.
				if( game_found == false )
				{
					// We found the game, we continue one last time so we begin looking in the next ones as potential current game.
					if( &game == m_current_game )
						game_found = true;

					continue;
				}

				// If the game is finished, we can't use it as our current game, so we continue looking.
				if( game.is_finished() )
					continue;

				m_current_game = &game;
				m_current_game->set_state( Game::State::current );
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

		// Once all the stats have been computed, they can be used for the games that don't have any sessions and can't predict their end date
		for( Game& game : m_games )
		{
			if( game.has_sessions() == false )
				game.compute_end_date();
		}

		FZN_LOG( "Est. final time %s", Utils::time_to_str( m_estimated_final_time ).c_str() );
	}

	void SplitsManager::_handle_actions()
	{
		if( m_finished_game != nullptr )
		{
			if( g_pFZN_InputMgr->IsKeyPressed( sf::Keyboard::Escape ) || g_pFZN_InputMgr->IsActionPressed( "Start / Split" ) )
			{
				m_finished_game = nullptr;
				return;
			}
		}

		if( g_pFZN_InputMgr->IsActionPressed( "Start / Split" ) )
		{
			_start_split();
		}
		else if( g_pFZN_InputMgr->IsActionPressed( "Pause" ) )
		{
			_toggle_pause();
		}
		else if( g_pFZN_InputMgr->IsActionPressed( "Stop" ) )
		{
			_stop();
		}
	}

	void SplitsManager::_start_split()
	{
		if( m_chrono.is_paused() )
		{
			m_chrono.toggle_pause();
		}
		else
		{
			_update_sessions( true );
		}
	}

	void SplitsManager::_toggle_pause()
	{
		m_chrono.toggle_pause();
	}

	void SplitsManager::_stop()
	{
		m_chrono.stop();
	}

}
