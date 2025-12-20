#include <regex>
#include <format>

#include <tinyXML2/tinyxml2.h>

#include <FZN/Tools/Tools.h>
#include <FZN/Tools/Logging.h>
#include <FZN/UI/ImGui.h>
#include <FZN/Managers/FazonCore.h>
#include <FZN/Managers/DataManager.h>

#include "../External/base64.hpp"

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


	static void display_split_infos( const Split& _split, Options::DateFormat _date_format )
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex( 0 );

		ImGui::Text( "sess. %u (%u)", _split.m_session_index, _split.m_split_index );

		if( Utils::is_date_valid( _split.m_date ) )
		{
			ImGui::TableSetColumnIndex( 1 );
			ImGui::Text( Utils::date_to_str( _split.m_date, _date_format ).c_str() );
		}

		ImGui::TableSetColumnIndex( 2 );
		ImGui::Text( Utils::time_to_str( _split.m_segment_time ).c_str() );

		ImGui::TableSetColumnIndex( 3 );
		ImGui::Text( Utils::time_to_str( _split.m_run_time ).c_str() );
	}

	Game::Game( const Desc& _desc, Utils::ParsingInfos& _parsing_infos )
	{
		m_name = _desc.m_name;
		m_state = _desc.m_state;
		m_estimation = _desc.m_estimation;

		m_splits.push_back( { _parsing_infos.m_split_index, 1, _parsing_infos.m_total_time, _desc.m_played } );

		_refresh_game_time();

		if( Utils::is_time_valid( m_played ) )
			++_parsing_infos.m_split_index;

		_parsing_infos.m_total_time += m_played;

		_compute_game_stats();
	}

	void Game::display()
	{
		const Options::OptionsDatas& options{ g_splits_app->get_options().get_options_datas() };
		// Copying the current state to avoid it changing in the middle of the frame and have imgui push/pop mismatches.
		const State game_state{ m_state };
		ImGui::PushID( m_name.c_str() );
		_push_state_colors( game_state );

		const bool header_open = ImGui::CollapsingHeader( m_name.c_str(), is_current() ? ImGuiTreeNodeFlags_DefaultOpen : 0 );
		const bool header_hovered = ImGui::IsItemHovered();
		
		_right_click( game_state );

		if( Utils::is_time_valid( m_played ) )
		{
			std::string game_time{ Utils::time_to_str( m_played ) };
			const float game_time_width{ ImGui::CalcTextSize( game_time.c_str() ).x };

			ImGui::SameLine( ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x - game_time_width );
			ImGui::Text( game_time.c_str() );
		}

		_pop_state_colors( game_state );

		if( header_hovered )
			_tooltip();

		if( header_open )
		{
			_handle_game_background( game_state );
			
			ImGui::Indent();

			_estimate_and_delta( game_state );

			if( ImGui::BeginTable( "splits_infos", 4 ) )
			{
				// Put here for better spacing in the app.
				if( has_sessions() )
				{
					for( Split& split : m_splits )
						display_split_infos( split, options.m_date_format );
				}
				ImGui::EndTable();
			}
			
			if( are_sessions_over() == false )
			{
				ImGui::PushItemWidth( 80.f );
				ImGui::InputTextWithHint( "##new_session_time", "00:00:00", &m_new_session_time );
				ImGui::SameLine();
				ImGui::InputTextWithHint( "##new_session_date", "yyyy-mm-dd", &m_new_session_date );
				ImGui::PopItemWidth();

				ImGui::SameLine( ImGui::GetContentRegionAvail().x - 165.f + ImGui::GetStyle().IndentSpacing - ImGui::GetStyle().WindowPadding.x * 2.f );

				ImGui::SetNextItemWidth( 100.f );
				Game::state_combo_box( m_new_session_state );

				ImGui::SameLine();
				if( ImGui_fzn::deactivable_button( "Update", m_new_session_time.empty(), false, { 65.f, 0.f } ) )
					_add_new_session_time();

				ImGui::Spacing();
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
				_refresh_game_time();
				break;
			}
		};
	}

	bool Game::display_finished_stats()
	{
		std::string popup_name{ "Game finished!" };

		if( m_finished_game_popup == false )
		{
			m_finished_game_popup = true;
			_compute_game_stats();
			compute_end_date();
			ImGui::OpenPopup( popup_name.c_str() );
		}

		if( m_finished_game_popup )
		{
			ImVec2 popup_size{};
			ImGui::SetWindowFontScale( 2.f );
			const ImVec2 game_name_size{ ImGui::CalcTextSize( m_name.c_str() ) };
			popup_size.y = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
			ImGui::SetWindowFontScale( 1.f );

			popup_size.x = std::max( 356.f, game_name_size.x ) + ImGui::GetStyle().WindowPadding.x * 2.f;
			popup_size.y += Utils::game_cover_size.y + ImGui::GetStyle().WindowPadding.y * 2.f + ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
			sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();

			ImGui::SetNextWindowPos( { window_size.x * 0.5f - popup_size.x * 0.5f, window_size.y * 0.5f - popup_size.y * 0.5f }, ImGuiCond_Appearing );
			ImGui::SetNextWindowSize( popup_size );

			if( ImGui::BeginPopupModal( popup_name.c_str(), &m_finished_game_popup, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize ) )
			{
				ImGui::SetWindowFontScale( 2.f );
				ImGui::Text( m_name.c_str() );
				ImGui::SetWindowFontScale( 1.f );
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				if( m_cover != nullptr )
				{
					ImGui::Image( *m_cover, Utils::game_cover_size );
					ImGui::SameLine();
				}

				_display_game_stats_table( popup_size.x );

				ImGui::EndPopup();
			}
		}

		return m_finished_game_popup;
	}

	void Game::display_end_date_predition()
	{
		const Options::OptionsDatas& options{ g_splits_app->get_options().get_options_datas() };

		ImGui::Separator();
		
		if( has_sessions() )
		{
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "First session:", "%s", Utils::date_to_str( m_stats.m_begin_date, options.m_date_format ).c_str() );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Average play time by day:", "%s (%u day(s))", Utils::time_to_str( m_stats.m_avg_session_played_day ).c_str(), m_stats.m_played_days );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Average play time since beginning:", "%s (%u day(s))", Utils::time_to_str( m_stats.m_avg_session_day ).c_str(), m_stats.m_days_since_start );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Remaining:", "%u day(s) | %u played day(s) | %u session(s)", m_stats.m_remaining_days, m_stats.m_remaining_played_days, m_stats.m_remaining_sessions );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Estimated last day:", "%s", Utils::date_to_str( m_stats.m_end_date, options.m_date_format ).c_str() );
		}
		else
		{
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::gray, "First session:", "%s", Utils::date_to_str( Utils::today(), options.m_date_format ).c_str() );
			ImGui::SameLine();
			ImGui_fzn::helper_simple_tooltip( "This game doesn't have any session yet \nThe prediction is based on global stats and the end date is calculated from the current day." );

			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Average play time by day:", "%s", Utils::time_to_str( m_stats.m_avg_session_played_day ).c_str() );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Average play time since beginning:", "%s", Utils::time_to_str( m_stats.m_avg_session_day ).c_str() );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Remaining:", "%u day(s) | %u played day(s) | %u session(s)", m_stats.m_remaining_days, m_stats.m_remaining_played_days, m_stats.m_remaining_sessions );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Estimated last day:", "%s", Utils::date_to_str( m_stats.m_end_date, options.m_date_format ).c_str() );
		}
	}

	void Game::state_combo_box( Game::State& _state )
	{
		static uint32_t state_none_id{ static_cast<uint32_t>( Game::State::none ) };
		static uint32_t state_current_id{ static_cast<uint32_t>( Game::State::current ) };
		static uint32_t state_count_id{ static_cast<uint32_t>( Game::State::COUNT ) };
		
		uint32_t state_id{ static_cast<uint32_t>( _state ) };

		if( ImGui::BeginCombo( "##StateCombo", Game::get_str_from_state( _state ) ) )
		{
			for( uint32_t state{ 0 }; state < state_count_id; ++state )
			{
				if( state == state_none_id || state == state_current_id )
					continue;

				if( ImGui::Selectable( Game::get_str_from_state( static_cast<Game::State>( state ) ), state == state_id ) )
					_state = static_cast<Game::State>( state );
			}

			ImGui::EndCombo();
		}
	}

	bool Game::contains_split_index( uint32_t _index ) const
	{
		if( m_splits.empty() )
			return false;

		return m_splits.front().m_split_index <= _index && m_splits.back().m_split_index >= _index;
	}

	bool Game::has_sessions() const
	{
		if( m_splits.empty() )
			return false;

		if( m_splits.size() == 1 && Utils::is_time_valid( m_splits.back().m_segment_time ) == false )
			return false;

		return true;
	}

	const char* Game::get_state_str() const
	{
		return get_str_from_state( m_state );
	}

	const char* Game::get_str_from_state( Game::State _state )
	{
		switch( _state )
		{
			case Game::State::none:
				return "None";
			case Game::State::current:
				return "Current";
			case Game::State::finished:
				return "Finished";
			case Game::State::abandonned:
				return "Abandonned";
			case Game::State::playing:
				return "Playing";
			case Game::State::COUNT:
			default:
				return "COUNT";
		};
	}

	Game::State Game::get_state_from_str( std::string_view _state ) const
	{
		if( _state == "Current" || _state == "current" )
			return State::current;

		if( _state == "Finished" || _state == "finished" )
			return State::finished;

		if( _state == "Abandonned" || _state == "abandonned" )
			return State::abandonned;

		if( _state == "Playing" || _state == "playing" )
			return State::playing;

		return State::none;
	}

	SplitTime Game::get_run_time() const
	{
		SplitTime last_valid_time{};

		for( const Split& split : m_splits )
		{
			if( Utils::is_time_valid( split.m_run_time ) )
				last_valid_time = split.m_run_time;
		}

		return last_valid_time;
	}

	SplitTime Game::get_played() const
	{
		return m_played;
	}

	SplitTime Game::get_last_valid_segment_time() const
	{
		SplitTime last_segment{};

		for( const Split& split : m_splits )
		{
			if( Utils::is_time_valid( split.m_segment_time ) )
				last_segment = split.m_segment_time;
		}

		return last_segment;
	}

	/**
	* @brief Add a session to the game, from timer or manual add. Run time will be determined thanks to the game splits themselves.
	* @param _time The time of the session we want to add.
	* @param _date The date of the session.
	* @param _state The new state of the game.
	**/
	void Game::add_session( const SplitTime& _time, const SplitDate& _date, State _state )
	{
		if( m_splits.empty() || Utils::is_time_valid( _time ) == false )
			return;

		// If there is only one split that has no segment time, that means the session we add is the first one on the game and we want to update the existing split.
		// This split was created when reading the json and contains the necessary informations to add a new session (run time and split index)
		if( has_sessions() == false )
		{
			Split& last_split{ m_splits.back() };

			last_split.m_run_time += _time;
			last_split.m_segment_time = _time;
			last_split.m_date = _date;

			m_stats.m_begin_date = last_split.m_date;
		}
		// If there are more than one split, sessions have already been added to the game and we can use their informations for the one we want to add.
		else
		{
			const Split& last_split{ m_splits.back() };
			
			Split new_split{ last_split.m_split_index + 1, last_split.m_session_index + 1 };

			new_split.m_run_time = last_split.m_run_time + _time;
			new_split.m_segment_time = _time;
			new_split.m_date = _date;

			m_splits.push_back( std::move( new_split ) );
		}

		m_state = _state;

		_refresh_game_time();
		_compute_game_stats();

		if( are_sessions_over() == false )
			compute_end_date();
	}

	/**
	* @brief Update game datas by incrementing its splits indexes and adding a time to their run time.
	* @param _delta_to_add The time delta that has been added on a game before this one that we need to add.
	**/
	void Game::update_data( const SplitTime& _delta_to_add )
	{
		for( Split& split : m_splits )
		{
			++split.m_split_index;
			split.m_run_time += _delta_to_add;
		}

		if( m_state != State::none )
			m_played += m_delta;
	}

	/**
	* @brief Calculate at which date the game could be finished, either by using its stats if it has any sessions, or the global stats compiled from all the previous games.
	**/
	void Game::compute_end_date()
	{
		const SplitTime played{ m_played };
		const SplitTime remaining_time{ m_estimation - played };
		std::vector< ComboStat > played_days;

		if( Utils::is_date_valid( m_stats.m_begin_date ) )
		{
			for( const Split& split : m_splits )
			{
				if( Utils::is_date_valid( split.m_date ) == false )
					continue;

				if( std::ranges::find( played_days, split.m_date, &ComboStat::m_date ) == played_days.end() )
				{
					played_days.push_back( { .m_date = split.m_date } );
				}
			}

			if( played_days.empty() == false )
			{
				m_stats.m_played_days = played_days.size();
				m_stats.m_avg_session_played_day = played / m_stats.m_played_days;
				m_stats.m_remaining_played_days = remaining_time / m_stats.m_avg_session_played_day;

				m_stats.m_avg_sessions_days = m_splits.size() / static_cast<float>( m_stats.m_played_days );
			}

			m_stats.m_days_since_start = Utils::days_between_dates( m_stats.m_begin_date, Utils::today() );

			if( m_stats.m_days_since_start > 0 )
				m_stats.m_avg_session_day = played / m_stats.m_days_since_start;
			else
				m_stats.m_avg_session_day = played;
		}
		// Approximation from global stats.
		else
		{
			const auto& global_stats = g_splits_app->get_splits_manager().get_stats();

			m_stats.m_avg_session_day = global_stats.get_avg_session_day();
			m_stats.m_avg_session_played_day = global_stats.get_avg_session_played_day();

			if( Utils::is_time_valid( m_stats.m_avg_session_played_day ) )
				m_stats.m_remaining_played_days = remaining_time / m_stats.m_avg_session_played_day;

			m_stats.m_avg_sessions_days = global_stats.get_avg_sessions_days();
		}

		if( Utils::is_time_valid( m_stats.m_avg_session_day ) )
			m_stats.m_remaining_days = remaining_time / m_stats.m_avg_session_day;

		m_stats.m_remaining_sessions = ceil( m_stats.m_remaining_played_days * m_stats.m_avg_sessions_days );
		m_stats.m_end_date = Utils::add_days_to_date( Utils::today(), m_stats.m_remaining_days );
	}

	/**
	* @brief Read the Json value containing all the informations about the game.
	* @param _game The game informations.
	* @param [in out] _parsing_infos State of the parsing.
	* @return True if this is the current game.
	**/
	bool Game::read( const Json::Value& _game, Utils::ParsingInfos& _parsing_infos )
	{
		m_name = _game[ "Name" ].asString();
		m_estimation = Utils::get_time_from_string( _game[ "Estimate" ].asString() );

		m_state = get_state_from_str( _game[ "State" ].asString() );
		m_cover_data = _game[ "Cover" ].asString();

		if( m_cover_data.empty() == false )
		{
			std::string decoded_data = base64::from_base64( m_cover_data );
			m_cover = g_pFZN_DataMgr->load_texture_from_memory( m_name, decoded_data.data(), decoded_data.size() );
		}

		Json::Value sessions = _game[ "Sessions" ];

		m_played = SplitTime{};

		for( Json::Value::iterator it_session = sessions.begin(); it_session != sessions.end(); ++it_session )
		{
			const Json::Value& session = *it_session;
			auto session_infos{ fzn::Tools::split( session.asString(), ',' ) };

			if( session_infos.empty() )
				continue;

			Split new_split{ _parsing_infos.m_split_index, m_splits.size() + 1 };

			new_split.m_segment_time = Utils::get_time_from_string( session_infos.at( 0 ) );

			if( session_infos.size() > 1 )
			{
				SplitDate session_date = Utils::get_date_from_string( session_infos.at( 1 ) );

				if( session_date != SplitDate{} )
					new_split.m_date = session_date;
			}

			if( Utils::is_time_valid( new_split.m_segment_time ) == false )
				continue;

			_parsing_infos.m_total_time += new_split.m_segment_time;
			m_played += new_split.m_segment_time;

			new_split.m_run_time = _parsing_infos.m_total_time;

			if( m_splits.empty() )
				m_stats.m_begin_date = new_split.m_date;

			m_splits.push_back( std::move( new_split ) );
			++_parsing_infos.m_split_index;
		}

		if( are_sessions_over() == false && m_splits.empty() )
		{
			m_state = State::none;
			m_splits.push_back( { _parsing_infos.m_split_index, 1, _parsing_infos.m_total_time } );
		}

		const SplitTime tmp_delta{ m_played - m_estimation };

		// Update the delta if the game is finished, or the estimate has been exceeded.
		if( m_state == State::finished || m_state != State::none && tmp_delta > std::chrono::seconds{ 0 } )
			m_delta = tmp_delta;

		if( has_sessions() )
			_compute_game_stats();

		compute_end_date();

		return m_state == State::current;
	}

	/**
	* @brief Write the game infos into the given Json value
	* @param [in out] _game The Json value that will hold the game informations.
	**/
	void Game::write( Json::Value& _game ) const
	{
		_game[ "Name" ] = m_name;
		_game[ "Estimate" ] = Utils::time_to_str( m_estimation ).c_str();
		
		if( m_cover_data.empty() == false )
			_game[ "Cover" ] = m_cover_data;

		if( m_state == State::none )
			return;

		_game[ "State" ] = get_state_str();

		std::string session_infos{};

		for( uint32_t split_index{ 0 }; split_index < m_splits.size(); ++split_index )
		{
			if( Utils::is_time_valid( m_splits[ split_index ].m_segment_time ) == false )
				return;

			session_infos = Utils::time_to_str( m_splits[ split_index ].m_segment_time ).c_str();

			if( Utils::is_date_valid( m_splits[ split_index ].m_date ) )
				session_infos += fzn::Tools::Sprintf( ", %s", Utils::date_to_str( m_splits[ split_index ].m_date ).c_str() );

			_game[ "Sessions" ][ split_index ] = session_infos.c_str();
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

		SplitDate segment_date = Utils::get_date_from_string( m_new_session_date );
		m_new_session_date.clear();

		if( m_state == State::current && m_new_session_state == State::playing )
			m_new_session_state = m_state;

		add_session( new_segment_time, segment_date, m_new_session_state );

		Event* game_event = new Event( Event::Type::session_added );
		game_event->m_game_event.m_game = this;
		game_event->m_game_event.m_game_finished = are_sessions_over();

		g_pFZN_Core->PushEvent( game_event );
		m_new_session_state = State::playing;
	}

	void Game::_refresh_game_time()
	{
		m_played = SplitTime{};
		m_delta = SplitTime{};

		for( Split& split : m_splits )
			m_played += split.m_segment_time;

		const SplitTime tmp_delta{ m_played - m_estimation };

		// Update the delta if the game is finished, or the estimate has been exceeded.
		if( m_state == State::finished || m_state != State::none && tmp_delta > std::chrono::seconds{ 0 } )
			m_delta = tmp_delta;
	}

	void Game::_refresh_state()
	{
		if( m_state == State::abandonned )
			return;

		const State prev_state{ m_state };
		const Game* current_game{ g_splits_app->get_current_game() };

		// If the last split doesn't have a segment time, it meas the game is still ready to recieve new sessions.
		// If there is a segment time, it means we don't want to add sessions anymore, and the game is finished.
		if( Utils::is_time_valid( m_splits.back().m_segment_time ) )
		{
			m_state = State::finished;
			return;
		}

		if( current_game != nullptr && current_game == this )
		{
			m_state = State::current;
			return;
		}

		// An ongoing game will have more than one split, as there is always an empty one for its next session in addition to already submitted sessions.
		if( m_splits.size() > 1 )
		{
			m_state = State::playing;
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
			case State::abandonned:
			{
				ImGui::PushStyleColor( ImGuiCol_Text,			ImGui_fzn::color::black );
				ImGui::PushStyleColor( ImGuiCol_Header,			Utils::Color::abandonned_game_header );
				ImGui::PushStyleColor( ImGuiCol_HeaderHovered,	Utils::Color::abandonned_game_header_hovered );
				ImGui::PushStyleColor( ImGuiCol_HeaderActive,	Utils::Color::abandonned_game_header_active );
				break;
			}
			case State::playing:
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
			case State::abandonned:
			case State::playing:
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

		// Estimate and Delta line
		rect_size.y += ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

		// splits lines
		rect_size.y += ImGui::GetStyle().ItemSpacing.y * 2.f;

		if( has_sessions() )
			rect_size.y += ImGui::GetTextLineHeightWithSpacing() * m_splits.size();

		// Add session line
		if( are_sessions_over() == false )
			rect_size.y += ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

		ImVec4 frame_bg_color{};

		switch( _state )
		{
			case Game::State::current:
			{
				frame_bg_color = Utils::Color::current_game_frame_bg;
				break;
			}
			case Game::State::finished:
			{
				frame_bg_color = Utils::Color::finished_game_frame_bg;
				break;
			}
			case Game::State::abandonned:
			{
				frame_bg_color = Utils::Color::abandonned_game_frame_bg;
				break;
			}
			case Game::State::playing:
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

	void Game::_right_click( State _state )
	{
		if( ImGui::BeginPopupContextItem("bjr") )
		{
			_pop_state_colors( _state );

			if( ImGui::BeginMenu( "Set State" ) )
			{
				if( ImGui::MenuItem( "Current", 0, false, are_sessions_over() == false ) )
				{
					m_state = State::current;

					Event* game_event = new Event( Event::Type::new_current_game_selected );
					game_event->m_game_event.m_game = this;

					g_pFZN_Core->PushEvent( game_event );
				}

				if( ImGui::MenuItem( "Finished", 0, false, has_sessions() ) ) {}
				if( ImGui::MenuItem( "Abandonned" ) ) {}
				if( ImGui::MenuItem( "Ongoing", 0, false, has_sessions() ) ) {}

				ImGui::EndMenu();
			}

			if( ImGui::Selectable( "Set Cover" ) )
			{
				_select_cover();
			}

			if( m_cover != nullptr && ImGui::Selectable( "Remove Cover" ) )
			{
				g_pFZN_DataMgr->UnloadTexture( m_name );
				m_cover = nullptr;
				m_cover_data.clear();
			}

			_push_state_colors( _state );
			ImGui::EndPopup();
		}
	}

	void Game::_tooltip()
	{
		ImVec2 popup_size{};
		const ImVec2 game_name_size{ ImGui::CalcTextSize( m_name.c_str() ) };

		popup_size.x = std::max( 356.f, game_name_size.x ) + ImGui::GetStyle().WindowPadding.x * 2.f;
		popup_size.y = Utils::game_cover_size.y + ImGui::GetStyle().WindowPadding.y * 2.f + ImGui::GetFrameHeightWithSpacing();
		sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();

		ImGui::SetNextWindowPos( { window_size.x * 0.5f - popup_size.x * 0.5f, window_size.y * 0.5f - popup_size.y * 0.5f }, ImGuiCond_Appearing );
		ImGui::SetNextWindowSize( popup_size );

		if( ImGui::BeginTooltip() )
		{
			ImGui::Text( m_name.c_str() );

			ImGui::Separator();

			if( m_cover != nullptr )
			{
				ImGui::Image( *m_cover, Utils::game_cover_size );

				ImGui::SameLine();
			}

			_display_game_stats_table( popup_size.x );

			ImGui::EndTooltip();
		}
	}

	void Game::_estimate_and_delta( State _state )
	{
		if( ImGui::BeginTable( "splits_infos", 4 ) )
		{
			ImGui::TableSetupColumn( "Estimate", ImGuiTableColumnFlags_WidthFixed, ImGui::GetContentRegionAvail().x * 0.5f );
			ImGui::TableSetupColumn( "Delta", ImGuiTableColumnFlags_WidthFixed );

			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text( "Estimate:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth( 70.f );
			std::string estimate = Utils::time_to_str( m_estimation );
			if( ImGui::InputText( "##Estimate", &estimate, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank ) )
			{
				m_estimation = Utils::get_time_from_string( estimate );
				g_pFZN_Core->PushEvent( new Event( Event::Type::game_estimate_changed ) );
			}
			ImGui::TableNextColumn();

			if( _state == State::finished || _state != State::none && m_delta > std::chrono::seconds{ 0 } )
			{
				if( m_delta < std::chrono::seconds{ 0 } )
				{
					ImGui::Text( "Delta:" );
					ImGui::SameLine();
					ImGui::Text( Utils::time_to_str( m_delta ).c_str() );
				}
				else
				{
					ImGui::Text( "Delta:" );
					ImGui::SameLine();
					ImGui::Text( "+%s", Utils::time_to_str( m_delta ).c_str() );
				}
			}

			ImGui::EndTable();
		}

		ImGui::Separator();
	}

	void Game::_select_cover()
	{
		char file[ 100 ];
		OPENFILENAME open_file_name;
		ZeroMemory( &open_file_name, sizeof( open_file_name ) );

		open_file_name.lStructSize = sizeof( open_file_name );
		open_file_name.hwndOwner = NULL;
		open_file_name.lpstrFile = file;
		open_file_name.lpstrFile[ 0 ] = '\0';
		open_file_name.nMaxFile = sizeof( file );
		open_file_name.lpstrFileTitle = NULL;
		open_file_name.nMaxFileTitle = 0;
		GetOpenFileName( &open_file_name );

		if( open_file_name.lpstrFile[ 0 ] != '\0' )
		{
			m_cover = g_pFZN_DataMgr->LoadTexture( m_name, open_file_name.lpstrFile );
			m_cover_data = Utils::get_cover_data( open_file_name.lpstrFile );
		}
	}

	/**
	* @brief Compute all game stats from its estimate, time played and sessions.
	**/
	void Game::_compute_game_stats()
	{
		m_stats.m_average_session_time = SplitTime{};
		m_stats.m_longest_sesion = SplitTime{};
		m_stats.m_shortest_session = Utils::get_time_from_string( "99:59:59" );

		if( m_splits.empty() )
			return;

		for( const Split& split : m_splits )
		{
			m_stats.m_average_session_time += split.m_segment_time;

			if( m_stats.m_longest_sesion < split.m_segment_time )
			{
				m_stats.m_longest_sesion = split.m_segment_time;
			}

			if( m_stats.m_shortest_session > split.m_segment_time )
			{
				m_stats.m_shortest_session = split.m_segment_time;
			}
		}

		m_stats.m_average_session_time /= m_splits.size();
	}

	/**
	* @brief Displayed computed game stats, weither be in its tooltip or in the finished game popup.
	**/
	void Game::_display_game_stats_table( float _window_width )
	{
		std::string time_str{};
		ImVec2 time_size{};
		const ImGuiStyle& style{ ImGui::GetStyle() };
		const float first_column_size{ 140.f };
		const float second_column_width{ _window_width - style.WindowPadding.x * 2.f - style.ItemSpacing.x - style.CellPadding.x  - first_column_size - Utils::game_cover_size.x };

		auto second_column_text = [&second_column_width]( const char* _text )
		{
			ImGui::TableNextColumn();
			ImVec2 text_size = ImGui::CalcTextSize( _text );
			ImGui::NewLine();
			ImGui::SameLine( second_column_width - text_size.x );
			ImGui::Text( _text );
		};

		if( ImGui::BeginTable( "stats_table", 2, ImGuiTableFlags_RowBg ) )
		{
			ImGui::TableSetupColumn( "labels", ImGuiTableColumnFlags_WidthFixed, first_column_size );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Played:" );
			second_column_text( Utils::time_to_str( m_played ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Estimate:" );
			second_column_text( Utils::time_to_str( m_estimation ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Delta:" );
			second_column_text( Utils::time_to_str( m_delta ).c_str() );

			if( has_sessions() == false )
			{
				ImGui::EndTable();
				return;
			}

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Number of sessions:" );
			second_column_text( fzn::Tools::Sprintf( "%d", m_splits.size() ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Average session:" );
			second_column_text( Utils::time_to_str( m_stats.m_average_session_time ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Shortest session:" );
			second_column_text( Utils::time_to_str( m_stats.m_shortest_session ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, "Longest session:" );
			second_column_text( Utils::time_to_str( m_stats.m_longest_sesion ).c_str() );

			ImGui::EndTable();
		}
	}
}