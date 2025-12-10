#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/UI/ImGui.h>

#include "Event.h"
#include "Stats.h"
#include "SplitsManagerApp.h"

namespace SplitsMgr
{
	void Stats::display()
	{
		const Options::OptionsDatas& options{ g_splits_app->get_options().get_options_datas() };

		ImGui::SeparatorText( "Stats" );

		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Number of sessions:", "%u", m_nb_sessions );
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Average sessions per game:", "%.2f", m_avg_sessions );
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Average session time:", Utils::time_to_str( m_avg_session_time ).c_str() );
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Most sessions:", "%s (%u)", m_game_most_sessions.m_name.c_str(), m_game_most_sessions.m_nb_sessions );
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Longest sessions (avg):", "%s (%s)", m_game_longest_sessions.m_name.c_str(), Utils::time_to_str( m_game_longest_sessions.m_time ).c_str() );
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Single longest session:", "%s (%s)", m_game_longest_session.m_name.c_str(), Utils::time_to_str( m_game_longest_session.m_time ).c_str() );
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Shortest sessions (avg):", "%s (%s)", m_game_shortest_sessions.m_name.c_str(), Utils::time_to_str( m_game_shortest_sessions.m_time ).c_str() );
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Single shortest session:", "%s (%s)", m_game_shortest_session.m_name.c_str(), Utils::time_to_str( m_game_shortest_session.m_time ).c_str() );
		ImGui::Spacing();
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "First session:", "%s", Utils::date_to_str( m_begin_date, options.m_date_format ).c_str() );
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Average play time per day:", "%s", Utils::time_to_str( m_avg_session_day ).c_str() );
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Remaining days:", "%u", m_remaining_days );
		ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, "Estimated last day:", "%s", Utils::date_to_str( m_end_date, options.m_date_format ).c_str() );
	}

	void Stats::refresh( const Games& _games )
	{
		FZN_DBLOG( "Refreshing stats..." );
		_reset();

		for( const Game& game : _games )
		{
			if( game.get_state() == Game::State::none )
				continue;

			uint32_t nb_sessions{ 0 };
			SplitTime game_time{};
			++m_nb_games;

			const Splits& splits{ game.get_splits() };

			for( const Split& split : splits )
			{
				if( Utils::is_time_valid( split.m_segment_time ) == false )
					continue;

				++nb_sessions;
				game_time += split.m_segment_time;

				if( m_game_longest_session.m_time < split.m_segment_time )
				{
					m_game_longest_session.m_name = game.get_name();
					m_game_longest_session.m_time = split.m_segment_time;
				}

				if( m_game_shortest_session.m_time > split.m_segment_time )
				{
					m_game_shortest_session.m_name = game.get_name();
					m_game_shortest_session.m_time = split.m_segment_time;
				}
			}

			if( nb_sessions == 0 )
				continue;

			m_nb_sessions += nb_sessions;
			m_avg_sessions += nb_sessions;
			m_avg_session_time += game_time;
			game_time /= nb_sessions;

			if( m_game_most_sessions.m_nb_sessions < nb_sessions )
			{
				m_game_most_sessions.m_name = game.get_name();
				m_game_most_sessions.m_nb_sessions = nb_sessions;
			}

			if( m_game_longest_sessions.m_time < game_time )
			{
				m_game_longest_sessions.m_name = game.get_name();
				m_game_longest_sessions.m_time = game_time;
			}

			if( m_game_shortest_sessions.m_time > game_time )
			{
				m_game_shortest_sessions.m_name = game.get_name();
				m_game_shortest_sessions.m_time = game_time;
			}

			if( Utils::is_date_valid( m_begin_date ) == false || Utils::is_date_valid( game.get_begin_date() ) && m_begin_date > game.get_begin_date() )
				m_begin_date = game.get_begin_date();
		}

		if( m_nb_games == 0 || m_nb_sessions == 0 )
			return;

		m_avg_sessions /= m_nb_games;
		m_avg_session_time /= m_nb_sessions;

		if( Utils::is_date_valid( m_begin_date ) == false )
			return;

		std::chrono::system_clock::time_point tp_begin = std::chrono::time_point<std::chrono::system_clock, std::chrono::days>( std::chrono::sys_days{ m_begin_date } );
		std::chrono::system_clock::time_point tp_now = std::chrono::time_point<std::chrono::system_clock, std::chrono::days>( std::chrono::sys_days{ Utils::today() } );

		uint32_t days = std::chrono::duration_cast<std::chrono::days>( tp_now - tp_begin ).count();
		m_avg_session_day = g_splits_app->get_splits_manager().get_played() / days;
		m_remaining_days = g_splits_app->get_splits_manager().get_remaining_time() / m_avg_session_day;
		m_end_date = std::chrono::floor< std::chrono::days >( tp_now + std::chrono::days{ m_remaining_days } );
	}

	void Stats::_reset()
	{
		m_nb_sessions		= 0;
		m_avg_sessions		= 0.f;
		m_avg_session_time	= SplitTime{};
		m_nb_games			= 0;

		m_begin_date		= SplitDate{};

		m_game_most_sessions		= GameStat{};
		m_game_longest_sessions		= GameStat{};
		m_game_longest_session		= GameStat{};
		m_game_shortest_sessions	= GameStat{};
		m_game_shortest_session		= GameStat{};

		m_game_shortest_session.m_time = Utils::get_time_from_string( "99:59:59" );
		m_game_shortest_sessions.m_time = Utils::get_time_from_string( "99:59:59" );
	}
}
