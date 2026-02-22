#include <regex>
#include <format>
#include <functional>

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
	static constexpr ImVec4		cell_alt_bg					{ 1.f, 1.f, 1.f, 0.1f };
	static constexpr ImVec4		cell_highlight				{ 1.f, 1.f, 1.f, 0.3f };

	// A pair containing a split index and a table column id.
	using SplitCell = std::pair< uint32_t, int >;
	static constexpr SplitCell invalid_cell{ Uint32_Max, -1 };
	static SplitCell active_cell{ invalid_cell };

	static void split_cell_input_text( const char* _label, std::string _text, bool _row_hovered, SplitCell _cell, std::function<void( std::string_view )> _callback )
	{
		const bool cell_hovered = _row_hovered && ImGui::TableGetHoveredColumn() == _cell.second || active_cell == _cell;
		if( cell_hovered )
			ImGui::PushStyleColor( ImGuiCol_FrameBg, cell_highlight );

		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
		ImGui::InputText( _label, &_text );

		// If the item is active, we edit the active_cell value to keep it highlighted.
		if( ImGui::IsItemActive() )
			active_cell = _cell;

		// If the input text has been used, we call the given callback to edit the split data that has been modified.
		if( ImGui::IsItemDeactivatedAfterEdit() )
			_callback( _text );

		// If the cell was active and isn't anymore, we reset the active cell to stop the highlight.
		// We separate this test from the previous one in case the user deactivate the input text with Return or Escape without editing its content.
		if( ImGui::IsItemDeactivated() )
			active_cell = invalid_cell;

		if( cell_hovered )
			ImGui::PopStyleColor();
	}

	static bool display_split_infos( Split& _split, Options::DateFormat _date_format, bool _display_alt_bg, bool _row_hovered )
	{
		bool edited{ false };
		ImGui::PushID( &_split );
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex( 0 );

		ImGui::Text( "%s %u (%u)", g_pFZN_LocMgr->get_string( LocID::session_short ).data(), _split.m_session_index, _split.m_split_index );

		std::string lol{};
		bool cell_hovered{ false };
		static std::pair< uint32_t, int > active_cell{ Uint32_Max, -1 };
		if( Utils::is_date_valid( _split.m_date ) )
		{
			ImGui::TableSetColumnIndex( 1 );

			// I don't understand what's happening here. The two first column are column id 0 for some reason, no matter what I do. I'm scared.
			split_cell_input_text( "##date", Utils::date_to_str( _split.m_date, _date_format ), _row_hovered, { _split.m_split_index, 0 }, [ & ]( std::string_view _text )
				{
					SplitDate entered_date = Utils::get_date_from_string( _text, _date_format );

					if( Utils::is_date_valid( entered_date ) )
					{
						_split.m_date = entered_date;
						edited = true;
					}
				} );
		}

		ImGui::TableSetColumnIndex( 2 );

		split_cell_input_text( "##segment_time", Utils::time_to_str( _split.m_segment_time ), _row_hovered, { _split.m_split_index, 1 }, [ & ]( std::string_view _text )
			{
				SplitTime entered_time = Utils::get_time_from_string( _text );

				if( Utils::is_time_valid( entered_time ) )
				{
					_split.m_segment_time = entered_time;
					edited = true;
				}
			} );

		ImGui::TableSetColumnIndex( 3 );
		ImGui::Text( Utils::time_to_str( _split.m_run_time ).c_str() );

		if( _display_alt_bg )
			ImGui::GetCurrentTable()->RowBgColor[ 1 ] = ImGui::GetColorU32( cell_alt_bg );
		ImGui::PopID();

		return edited;
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

	void Game::display( SplitDate& _last_split_date, bool& _display_alt_bg )
	{
		const Options::Data& options{ g_splits_app->get_options().get_options_datas() };
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
					ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, { 4.f, 0.f } );
					ImGui::PushStyleColor( ImGuiCol_FrameBg, ImGui_fzn::color::transparent );
					
					int split_id{ 0 };
					bool edited{ false };
					for( Split& split : m_splits )
					{
						if( _last_split_date != split.m_date )
						{
							_last_split_date = split.m_date;
							_display_alt_bg = !_display_alt_bg;
						}

						edited |= display_split_infos( split, options.m_date_format, _display_alt_bg, ImGui::TableGetHoveredRow() == split_id );
						++split_id;
					}
					ImGui::PopStyleColor();
					ImGui::PopStyleVar();

					if( edited )
					{
						_refresh_game_data();
						g_pFZN_Core->PushEvent( new Event( Event::Type::game_data_changed ) );
					}
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
				if( ImGui_fzn::deactivable_button( g_pFZN_LocMgr->get_string( LocID::btn_update ).data(), m_new_session_time.empty(), false, { 65.f, 0.f } ) )
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
		const std::string popup_name{ g_pFZN_LocMgr->get_string( LocID::game_finished_title ) };

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
		// If there is no valid begin date in the global stats, there won't be in the game either sor there is no need to go further.
		if( Utils::is_date_valid( g_splits_app->get_splits_manager().get_stats().get_begin_date() ) == false )
			return;

		const Options::Data& options{ g_splits_app->get_options().get_options_datas() };

		ImGui::Separator();
		
		const std::string string_number_days_format{ fzn::Tools::Sprintf( "%s%s)", "%s (%u ", g_pFZN_LocMgr->get_string( LocID::stat_day ).data() ) };
		std::string remaining_day_session_format{ "%u " };
		fzn::Tools::sprintf_cat( remaining_day_session_format, "%s%s", g_pFZN_LocMgr->get_string( LocID::stat_day ).data(), " | %u " );
		fzn::Tools::sprintf_cat( remaining_day_session_format, "%s%s", g_pFZN_LocMgr->get_string( LocID::stat_played_day ).data(), " | %u " );
		fzn::Tools::sprintf_cat( remaining_day_session_format, "%s", g_pFZN_LocMgr->get_string( LocID::stat_session ).data() );

		if( has_sessions() )
		{
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::first_session ), "%s", Utils::date_to_str( m_stats.m_begin_date, options.m_date_format ).c_str() );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::avg_play_time_played_day ), string_number_days_format.c_str(), Utils::time_to_str( m_stats.m_avg_session_played_day ).c_str(), m_stats.m_played_days );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::avg_play_time_day ), string_number_days_format.c_str(), Utils::time_to_str( m_stats.m_avg_session_day ).c_str(), m_stats.m_days_since_start );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::remaining ), remaining_day_session_format.c_str(), m_stats.m_remaining_days, m_stats.m_remaining_played_days, m_stats.m_remaining_sessions );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::est_last_day ), "%s", Utils::date_to_str( m_stats.m_end_date, options.m_date_format ).c_str() );
		}
		else
		{
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::gray, Utils::localised_label_colon( LocID::first_session ), "%s", Utils::date_to_str( Utils::today(), options.m_date_format ).c_str() );
			ImGui::SameLine();
			ImGui_fzn::helper_simple_tooltip( g_pFZN_LocMgr->get_string( LocID::first_session_tooltip ).data() );

			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::avg_play_time_played_day ), "%s", Utils::time_to_str( m_stats.m_avg_session_played_day ).c_str() );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::avg_play_time_day ), "%s", Utils::time_to_str( m_stats.m_avg_session_day ).c_str() );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::remaining ), remaining_day_session_format.c_str(), m_stats.m_remaining_days, m_stats.m_remaining_played_days, m_stats.m_remaining_sessions );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::est_last_day ), "%s", Utils::date_to_str( m_stats.m_end_date, options.m_date_format ).c_str() );
		}
	}

	void Game::state_combo_box( Game::State& _state )
	{
		static uint32_t state_none_id{ static_cast<uint32_t>( Game::State::none ) };
		static uint32_t state_current_id{ static_cast<uint32_t>( Game::State::current ) };
		static uint32_t state_count_id{ static_cast<uint32_t>( Game::State::COUNT ) };
		
		uint32_t state_id{ static_cast<uint32_t>( _state ) };

		if( ImGui::BeginCombo( "##StateCombo", Game::get_str_from_state( _state, true ) ) )
		{
			for( uint32_t state{ 0 }; state < state_count_id; ++state )
			{
				if( state == state_none_id || state == state_current_id )
					continue;

				if( ImGui::Selectable( Game::get_str_from_state( static_cast<Game::State>( state ), true ), state == state_id ) )
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

	const char* Game::get_state_str( bool _localised ) const
	{
		return get_str_from_state( m_state, _localised );
	}

	const char* Game::get_str_from_state( Game::State _state, bool _localised )
	{
		if( _localised == false )
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

		switch( _state )
		{
			case Game::State::finished:
				return g_pFZN_LocMgr->get_string( LocID::game_state_finished ).data();
			case Game::State::abandonned:
				return g_pFZN_LocMgr->get_string( LocID::game_state_abandonned ).data();
			case Game::State::playing:
				return g_pFZN_LocMgr->get_string( LocID::game_state_playing ).data();
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

		_refresh_game_data();
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
	}

	/**
	* @brief Calculate at which date the game could be finished, either by using its stats if it has any sessions, or the global stats compiled from all the previous games.
	**/
	void Game::compute_end_date()
	{
		const SplitTime remaining_time{ m_estimation - m_played };

		// Approximation from global stats.
		if( Utils::is_date_valid( m_stats.m_begin_date ) == false )
		{
			const auto& global_stats = g_splits_app->get_splits_manager().get_stats();

			// If there is no valid begin date in the global stat, nothing will be able to be computed so there's no need continuing.
			if( Utils::is_date_valid( global_stats.get_begin_date() ) == false )
				return;

			m_stats.m_avg_session_day = global_stats.get_avg_session_day();
			m_stats.m_avg_session_played_day = global_stats.get_avg_session_played_day();

			if( Utils::is_time_valid( m_stats.m_avg_session_played_day ) )
				m_stats.m_remaining_played_days = remaining_time / m_stats.m_avg_session_played_day;

			m_stats.m_avg_sessions_days = global_stats.get_avg_sessions_played_day();
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
			m_splits.push_back( { _parsing_infos.m_split_index, 1, _parsing_infos.m_total_time } );
		}

		const SplitTime tmp_delta{ m_played - m_estimation };

		// Update the delta if the game is finished, or the estimate has been exceeded.
		if( m_state == State::finished || has_sessions() && tmp_delta > std::chrono::seconds{ 0 } )
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

		_game[ "State" ] = get_state_str( false );

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
		if( ImGui::BeginPopupContextItem( "game_right_click" ) )
		{
			_pop_state_colors( _state );

			if( ImGui::BeginMenu( g_pFZN_LocMgr->get_string( LocID::set_state ).data() ) )
			{
				if( ImGui::MenuItem( g_pFZN_LocMgr->get_string( LocID::game_state_current ).data(), 0, false, are_sessions_over() == false ) )
				{
					m_state = State::current;

					Event* game_event = new Event( Event::Type::new_current_game_selected );
					game_event->m_game_event.m_game = this;

					g_pFZN_Core->PushEvent( game_event );
				}

				if( ImGui::MenuItem( g_pFZN_LocMgr->get_string( LocID::game_state_playing ).data(), 0, false, has_sessions() ) ) {}
				if( ImGui::MenuItem( g_pFZN_LocMgr->get_string( LocID::game_state_finished ).data(), 0, false, has_sessions() ) ) {}
				if( ImGui::MenuItem( g_pFZN_LocMgr->get_string( LocID::game_state_abandonned ).data() ) ) {}

				ImGui::EndMenu();
			}

			if( ImGui::Selectable( g_pFZN_LocMgr->get_string( LocID::set_cover ).data() ) )
			{
				_select_cover();
			}

			if( m_cover != nullptr && ImGui::Selectable( g_pFZN_LocMgr->get_string( LocID::remove_cover ).data() ) )
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
			ImGui::Text( Utils::localised_label_colon( LocID::estimate ).c_str() );
			ImGui::SameLine();
			ImGui::SetNextItemWidth( 70.f );
			std::string estimate = Utils::time_to_str( m_estimation );
			if( ImGui::InputText( "##Estimate", &estimate, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank ) )
			{
				m_estimation = Utils::get_time_from_string( estimate );
				g_pFZN_Core->PushEvent( new Event( Event::Type::game_data_changed ) );
			}
			ImGui::TableNextColumn();

			if( _state == State::finished || _state != State::none && m_delta > std::chrono::seconds{ 0 } )
			{
				if( m_delta < std::chrono::seconds{ 0 } )
				{
					ImGui::Text( Utils::localised_label_colon( LocID::delta ).c_str() );
					ImGui::SameLine();
					ImGui::Text( Utils::time_to_str( m_delta ).c_str() );
				}
				else
				{
					ImGui::Text( Utils::localised_label_colon( LocID::delta ).c_str() );
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
		const std::string path = fzn::Tools::open_file( "", g_pFZN_LocMgr->get_string( LocID::select_cover_title ) );

		if( path.empty() )
			return;

		m_cover = g_pFZN_DataMgr->LoadTexture( m_name, path );
		m_cover_data = Utils::get_cover_data( path );
	}

	/**
	* @brief Game data full refresh, recompute game time and stats.
	**/
	void Game::_refresh_game_data()
	{
		_refresh_game_time();
		_compute_game_stats();

		if( are_sessions_over() == false )
			compute_end_date();
	}

	/**
	* @brief Compute all game stats from its estimate, time played and sessions.
	**/
	void Game::_compute_game_stats()
	{
		const SplitTime remaining_time{ m_estimation - m_played };
		std::vector< ComboStat > played_days;

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

			if( Utils::is_date_valid( split.m_date ) == false )
				continue;

			if( std::ranges::find( played_days, split.m_date, &ComboStat::m_date ) == played_days.end() )
			{
				played_days.push_back( { .m_date = split.m_date } );
			}
		}

		m_stats.m_average_session_time /= m_splits.size();

		if( Utils::is_date_valid( m_stats.m_begin_date ) == false )
			return;

		m_stats.m_days = Utils::days_between_dates( m_stats.m_begin_date, m_splits.back().m_date ) + 1;

		if( played_days.empty() == false )
		{
			m_stats.m_played_days = played_days.size();
			m_stats.m_avg_session_played_day = m_played / m_stats.m_played_days;
			m_stats.m_remaining_played_days = remaining_time / m_stats.m_avg_session_played_day;

			m_stats.m_avg_sessions_days = m_splits.size() / static_cast< float >( m_stats.m_played_days );
		}

		m_stats.m_days_since_start = Utils::days_between_dates( m_stats.m_begin_date, Utils::today() );

		if( m_stats.m_days_since_start > 0 )
			m_stats.m_avg_session_day = m_played / m_stats.m_days_since_start;
		else
			m_stats.m_avg_session_day = m_played;
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
			ImGui::TextColored( ImGui_fzn::color::light_yellow, Utils::localised_label_colon( LocID::played ).c_str() );
			second_column_text( Utils::time_to_str( m_played ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, Utils::localised_label_colon( LocID::estimate ).c_str() );
			second_column_text( Utils::time_to_str( m_estimation ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, Utils::localised_label_colon( LocID::delta ).c_str() );
			second_column_text( Utils::time_to_str( m_delta ).c_str() );

			if( has_sessions() == false )
			{
				ImGui::EndTable();
				return;
			}

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, Utils::localised_label_colon( LocID::nb_sessions ).c_str() );
			second_column_text( fzn::Tools::Sprintf( "%d", m_splits.size() ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, Utils::localised_label_colon( LocID::game_stat_played_day ).c_str() );
			second_column_text( fzn::Tools::Sprintf( "%d", m_stats.m_played_days ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, Utils::localised_label_colon( LocID::game_stat_day ).c_str() );
			second_column_text( fzn::Tools::Sprintf( "%d", m_stats.m_days ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, Utils::localised_label_colon( LocID::game_stat_avg_session ).c_str() );
			second_column_text( Utils::time_to_str( m_stats.m_average_session_time ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, Utils::localised_label_colon( LocID::shortest_session ).c_str() );
			second_column_text( Utils::time_to_str( m_stats.m_shortest_session ).c_str() );

			ImGui::TableNextColumn();
			ImGui::TextColored( ImGui_fzn::color::light_yellow, Utils::localised_label_colon( LocID::longest_session ).c_str() );
			second_column_text( Utils::time_to_str( m_stats.m_longest_sesion ).c_str() );

			ImGui::EndTable();
		}
	}
}