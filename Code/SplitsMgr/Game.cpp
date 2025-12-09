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


	static void display_split_infos( const Split& _split, bool _display_session )
	{
		ImGui::TableNextColumn();
		ImGui::Text( "%u", _split.m_split_index );

		ImGui::TableNextColumn();

		if( _display_session )
			ImGui::Text( "session %u", _split.m_session_index );

		ImGui::TableNextColumn();
		ImGui::Text( Utils::time_to_str( _split.m_run_time ).c_str() );

		ImGui::TableNextColumn();
		ImGui::Text( Utils::time_to_str( _split.m_segment_time ).c_str() );
	}

	void Game::display()
	{
		// Copying the current state to avoid it changing in the middle of the frame and have imgui push/pop mismatches.
		const State game_state{ m_state };
		ImGui::PushID( m_name.c_str() );
		_push_state_colors( game_state );

		const bool header_open = ImGui::CollapsingHeader( m_name.c_str(), is_current() ? ImGuiTreeNodeFlags_DefaultOpen : 0 );
		const bool header_hovered = ImGui::IsItemHovered();
		
		_right_click( game_state );

		if( Utils::is_time_valid( m_time ) )
		{
			std::string game_time{ Utils::time_to_str( m_time ) };
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

			
			if( sessions_over() == false && ImGui::BeginTable( "add_session_table", 2 ) )
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
			_compute_finished_game_stats();
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

				std::string time_str{};
				ImVec2 time_size{};

				auto second_column_text = []( const char* _text )
				{
					ImGui::TableNextColumn();
					ImVec2 text_size = ImGui::CalcTextSize( _text );
					ImGui::NewLine();
					ImGui::SameLine( ImGui::GetContentRegionAvail().x - text_size.x );
					ImGui::Text( _text );
				};

				if( ImGui::BeginTable( "stats_table", 2, ImGuiTableFlags_RowBg ) )
				{
					ImGui::TableSetupColumn( "labels", ImGuiTableColumnFlags_WidthFixed, 140.f );

					ImGui::TableNextColumn();
					ImGui::TextColored( ImGui_fzn::color::light_yellow, "Played:" );
					second_column_text( Utils::time_to_str( m_time ).c_str() );

					ImGui::TableNextColumn();
					ImGui::TextColored( ImGui_fzn::color::light_yellow, "Estimate:" );
					second_column_text( Utils::time_to_str( m_estimation ).c_str() );

					ImGui::TableNextColumn();
					ImGui::TextColored( ImGui_fzn::color::light_yellow, "Delta:" );
					second_column_text( Utils::time_to_str( m_delta ).c_str() );

					ImGui::TableNextColumn();
					ImGui::TextColored( ImGui_fzn::color::light_yellow, "Number of sessions:" );
					second_column_text( fzn::Tools::Sprintf( "%d", m_splits.size() ).c_str() );

					ImGui::TableNextColumn();
					ImGui::TextColored( ImGui_fzn::color::light_yellow, "Average session:" );
					second_column_text( Utils::time_to_str( m_finished_game_stats.m_average_session_time ).c_str() );

					ImGui::TableNextColumn();
					ImGui::TextColored( ImGui_fzn::color::light_yellow, "Shortest session:" );
					second_column_text( Utils::time_to_str( m_finished_game_stats.m_shortest_session ).c_str() );

					ImGui::TableNextColumn();
					ImGui::TextColored( ImGui_fzn::color::light_yellow, "Longest session:" );
					second_column_text( Utils::time_to_str( m_finished_game_stats.m_longest_sesion ).c_str() );

					ImGui::EndTable();
				}

				ImGui::EndPopup();
			}
		}

		return m_finished_game_popup;
	}

	bool Game::contains_split_index( uint32_t _index ) const
	{
		if( m_splits.empty() )
			return false;

		return m_splits.front().m_split_index <= _index && m_splits.back().m_split_index >= _index;
	}

	const char* Game::get_state_str() const
	{
		switch( m_state )
		{
			case State::none:
				return "None";
			case State::current:
				return "Current";
			case State::finished:
				return "Finished";
			case State::abandonned:
				return "Abandonned";
			case State::ongoing:
				return "Ongoing";
			case State::COUNT:
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

		if( _state == "Ongoing" || _state == "ongoing" )
			return State::ongoing;

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
		return m_time;
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

		if( Utils::is_time_valid( _run_time ) )
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
	void Game::update_data( const SplitTime& _delta_to_add, bool _incremeted_splits_index )
	{
		for( Split& split : m_splits )
		{
			if( _incremeted_splits_index )
				++split.m_split_index;

			if( m_state != State::none )
				split.m_run_time += _delta_to_add;
		}

		if( m_state != State::none )
			m_time += m_delta;
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

		m_time = SplitTime{};

		for( Json::Value::iterator it_session = sessions.begin(); it_session != sessions.end(); ++it_session )
		{
			const Json::Value& session = *it_session;
			Split new_split{ _parsing_infos.m_split_index, m_splits.size() + 1 };
			SplitTime session_time = Utils::get_time_from_string( session.asString() );

			if( Utils::is_time_valid( session_time ) == false )
				continue;

			_parsing_infos.m_total_time += session_time;
			m_time += session_time;

			new_split.m_segment_time = session_time;
			new_split.m_run_time = _parsing_infos.m_total_time;

			m_splits.push_back( std::move( new_split ) );
			++_parsing_infos.m_split_index;
		}

		if( sessions_over() == false )
		{
			if( m_splits.empty() )
				m_state = State::none;

			m_splits.push_back( { _parsing_infos.m_split_index, m_splits.size() + 1 } );
		}

		const SplitTime tmp_delta{ m_time - m_estimation };

		// Update the delta if the game is finished, or the estimate has been exceeded.
		if( m_state == State::finished || m_state != State::none && tmp_delta > std::chrono::seconds{ 0 } )
			m_delta = tmp_delta;

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

		for( uint32_t split_index{ 0 }; split_index < m_splits.size(); ++split_index )
		{
			if( Utils::is_time_valid( m_splits[ split_index ].m_segment_time ) == false )
				return;

			_game[ "Sessions" ][ split_index ] = Utils::time_to_str( m_splits[ split_index ].m_segment_time ).c_str();
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
		SplitTime run_time = get_run_time();

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
		m_delta = SplitTime{};

		for( Split& split : m_splits )
		{
			m_time += split.m_segment_time;
		}

		const SplitTime tmp_delta{ m_time - m_estimation };

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
			case State::abandonned:
			{
				ImGui::PushStyleColor( ImGuiCol_Text,			ImGui_fzn::color::black );
				ImGui::PushStyleColor( ImGuiCol_Header,			Utils::Color::abandonned_game_header );
				ImGui::PushStyleColor( ImGuiCol_HeaderHovered,	Utils::Color::abandonned_game_header_hovered );
				ImGui::PushStyleColor( ImGuiCol_HeaderActive,	Utils::Color::abandonned_game_header_active );
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
			case State::abandonned:
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

		// Estimate and Delta line
		rect_size.y += ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

		// splits lines
		rect_size.y += ImGui::GetStyle().ItemSpacing.y + ImGui::GetTextLineHeightWithSpacing() * m_splits.size();
		rect_size.y += ImGui::GetStyle().ItemSpacing.y;

		// Add session line
		if( is_current() || m_state == State::ongoing )
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
			case Game::State::ongoing:
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
			//ImGui::PushStyleColor( ImGuiCol_Text, ImGui_fzn::color::white );
			_pop_state_colors( _state );
			
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
		if( ImGui::BeginTooltip() )
		{
			ImGui::Text( m_name.c_str() );

			ImGui::Separator();

			if( m_cover != nullptr )
			{
				ImGui::Image( *m_cover, Utils::game_cover_size );

				ImGui::SameLine();
			}

			if( ImGui::BeginTable( "tooltip_table", 2 ) )
			{
				ImGui::TableNextColumn();
				ImGui::TextColored( ImGui_fzn::color::light_yellow, "Estimate:" );
				ImGui::TextColored( ImGui_fzn::color::light_yellow, "Played:" );
				ImGui::TextColored( ImGui_fzn::color::light_yellow, "Delta:" );

				ImGui::TableNextColumn();
				ImGui::Text( Utils::time_to_str( m_estimation ).c_str() );
				ImGui::Text( Utils::time_to_str( m_time ).c_str() );
				ImGui::Text( Utils::time_to_str( m_delta ).c_str() );

				ImGui::EndTable();
			}

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

	void Game::_compute_finished_game_stats()
	{
		m_finished_game_stats.m_average_session_time = SplitTime{};
		m_finished_game_stats.m_longest_sesion = SplitTime{};
		m_finished_game_stats.m_shortest_session = Utils::get_time_from_string( "99:59:59" );

		if( m_splits.empty() )
			return;

		for( const Split& split : m_splits )
		{
			m_finished_game_stats.m_average_session_time += split.m_segment_time;

			if( m_finished_game_stats.m_longest_sesion < split.m_segment_time )
			{
				m_finished_game_stats.m_longest_sesion = split.m_segment_time;
			}

			if( m_finished_game_stats.m_shortest_session > split.m_segment_time )
			{
				m_finished_game_stats.m_shortest_session = split.m_segment_time;
			}
		}

		m_finished_game_stats.m_average_session_time /= m_splits.size();
	}

}