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
		const Options::Data& options{ g_splits_app->get_options().get_options_datas() };

		ImGui::SeparatorText( g_pFZN_LocMgr->get_string( LocID::stats_title ).data() );

		if( ImGui::BeginChild( "stats" ) )
		{
			const std::string string_number_days_format{ fzn::Tools::Sprintf( "%s%s)", "%s (%u ", g_pFZN_LocMgr->get_string( LocID::stat_day ).data() ) };
			std::string remaining_day_session_format{ "%u " };
			fzn::Tools::sprintf_cat( remaining_day_session_format, "%s%s", g_pFZN_LocMgr->get_string( LocID::stat_day ).data(), " | %u " );
			fzn::Tools::sprintf_cat( remaining_day_session_format, "%s%s", g_pFZN_LocMgr->get_string( LocID::stat_played_day ).data(), " | %u " );
			fzn::Tools::sprintf_cat( remaining_day_session_format, "%s", g_pFZN_LocMgr->get_string( LocID::stat_session ).data() );

			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::nb_sessions ), "%u", m_nb_sessions );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::avg_sessions_game ), "%.2f", m_avg_sessions );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::avg_sessions_played_day ), "%.2f", m_avg_sessions_played_day );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::avg_sessions_day ), "%.2f", m_avg_sessions_day );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::avg_session_time ), Utils::time_to_str( m_avg_session_time ).c_str() );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::most_sessions_game ), "%s (%u)", m_game_most_sessions.m_string.c_str(), m_game_most_sessions.m_number );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::longest_session ), "%s (%s)", m_game_longest_session.m_string.c_str(), Utils::time_to_str( m_game_longest_session.m_time ).c_str() );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::longest_sessions_avg ), "%s (%s)", m_game_longest_sessions.m_string.c_str(), Utils::time_to_str( m_game_longest_sessions.m_time ).c_str() );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::shortest_session ), "%s (%s)", m_game_shortest_session.m_string.c_str(), Utils::time_to_str( m_game_shortest_session.m_time ).c_str() );
			ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::shortest_sessions_avg ), "%s (%s)", m_game_shortest_sessions.m_string.c_str(), Utils::time_to_str( m_game_shortest_sessions.m_time ).c_str() );

			if( Utils::is_date_valid( m_begin_date ) )
			{
				ImGui::Separator();
				ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::most_sessions_day ), "%s (%u)", Utils::date_to_str( m_day_most_sessions.m_date, options.m_date_format ).c_str(), m_day_most_sessions.m_number );
				ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::longest_play_time_day ), "%s (%s)", Utils::date_to_str( m_day_longest_played.m_date, options.m_date_format ).c_str(), Utils::time_to_str( m_day_longest_played.m_time ).c_str() );
				ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::shortest_play_time_day ), "%s (%s)", Utils::date_to_str( m_day_shortest_played.m_date, options.m_date_format ).c_str(), Utils::time_to_str( m_day_shortest_played.m_time ).c_str() );
				ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::most_days_game ), "%s (%u)", m_game_most_days.m_string.c_str(), m_game_most_days.m_number );
				ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::fewest_days_game ), "%s (%u)", m_game_fewest_days.m_string.c_str(), m_game_fewest_days.m_number );

				ImGui::Separator();
				ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::first_session ), "%s", Utils::date_to_str( m_begin_date, options.m_date_format ).c_str() );
				ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::avg_play_time_played_day ), string_number_days_format.c_str(), Utils::time_to_str( m_avg_session_played_day ).c_str(), m_played_days );
				ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::avg_play_time_day ), string_number_days_format.c_str(), Utils::time_to_str( m_avg_session_day ).c_str(), m_days_since_start );
				ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::remaining ), remaining_day_session_format.c_str(), m_remaining_days, m_remaining_played_days, m_remaining_sessions );
				ImGui_fzn::bicolor_text( ImGui_fzn::color::light_yellow, ImGui_fzn::color::white, Utils::localised_label_colon( LocID::est_last_day ), "%s", Utils::date_to_str( m_end_date, options.m_date_format ).c_str() );
			}

			ImGui::Spacing();
			ImGui::EndChild();
		}
	}

	void Stats::refresh( const Games& _games )
	{
		FZN_DBLOG( "Refreshing stats..." );
		reset();

		const SplitTime played{ g_splits_app->get_splits_manager().get_played() };
		const SplitTime remaining_time{ g_splits_app->get_splits_manager().get_remaining_time() };
		// vector containing all the days where a session happened.
		std::vector< ComboStat > played_days;
		std::vector< SplitDate > played_days_game;
		uint32_t nb_played_games{ 0 };

		for( const Game& game : _games )
		{
			if( game.get_state() == Game::State::none )
				continue;

			uint32_t nb_sessions{ 0 };
			SplitTime game_time{};
			played_days_game.clear();
			++nb_played_games;

			const Splits& splits{ game.get_splits() };

			for( const Split& split : splits )
			{
				if( Utils::is_time_valid( split.m_segment_time ) == false )
					continue;

				++nb_sessions;
				game_time += split.m_segment_time;

				if( m_game_longest_session.m_time < split.m_segment_time )
				{
					m_game_longest_session.m_string = game.get_name();
					m_game_longest_session.m_time = split.m_segment_time;
				}

				if( m_game_shortest_session.m_time > split.m_segment_time )
				{
					m_game_shortest_session.m_string = game.get_name();
					m_game_shortest_session.m_time = split.m_segment_time;
				}

				if( Utils::is_date_valid( split.m_date ) == false )
					continue;

				auto it_date = std::ranges::find( played_days, split.m_date, &ComboStat::m_date );
				ComboStat* day_stats{ nullptr };

				// If the split date isn't already in the played dates array, add it.
				if( it_date == played_days.end() )
				{
					played_days.push_back( { .m_date = split.m_date } );
					day_stats = &played_days.back();
				}
				else
					day_stats = &( *it_date );

				++day_stats->m_number;
				day_stats->m_time += split.m_segment_time;

				if( std::ranges::find( played_days_game, split.m_date ) == played_days_game.end() )
					played_days_game.push_back( split.m_date );
			}

			if( nb_sessions == 0 )
				continue;

			m_nb_sessions += nb_sessions;
			m_avg_sessions += nb_sessions;
			m_avg_session_time += game_time;
			game_time /= nb_sessions;

			if( m_game_most_sessions.m_number < nb_sessions )
			{
				m_game_most_sessions.m_string = game.get_name();
				m_game_most_sessions.m_number = nb_sessions;
			}

			if( m_game_longest_sessions.m_time < game_time )
			{
				m_game_longest_sessions.m_string = game.get_name();
				m_game_longest_sessions.m_time = game_time;
			}

			if( m_game_shortest_sessions.m_time > game_time )
			{
				m_game_shortest_sessions.m_string = game.get_name();
				m_game_shortest_sessions.m_time = game_time;
			}

			if( m_game_most_days.m_number < played_days_game.size() )
			{
				m_game_most_days.m_string = game.get_name();
				m_game_most_days.m_number = played_days_game.size();
			}

			if( m_game_fewest_days.m_number > played_days_game.size() )
			{
				m_game_fewest_days.m_string = game.get_name();
				m_game_fewest_days.m_number = played_days_game.size();
			}

			if( Utils::is_date_valid( m_begin_date ) == false || Utils::is_date_valid( game.get_begin_date() ) && m_begin_date > game.get_begin_date() )
					m_begin_date = game.get_begin_date();
		}

		if( nb_played_games == 0 || m_nb_sessions == 0 )
			return;

		m_avg_sessions = m_nb_sessions / static_cast<float>( nb_played_games );
		m_avg_session_time = played / m_nb_sessions;

		if( Utils::is_date_valid( m_begin_date ) == false )
			return;


		if( played_days.empty() == false )
		{
			m_played_days = played_days.size();
			m_avg_session_played_day = played / m_played_days;
			m_remaining_played_days = remaining_time / m_avg_session_played_day;

			m_avg_sessions_played_day = m_nb_sessions / static_cast< float >( m_played_days );

			for( const ComboStat& played_day : played_days )
			{
				if( m_day_most_sessions.m_number < played_day.m_number )
				{
					m_day_most_sessions.m_date = played_day.m_date;
					m_day_most_sessions.m_number = played_day.m_number;
				}

				if( m_day_shortest_played.m_time > played_day.m_time)
				{
					m_day_shortest_played.m_date = played_day.m_date;
					m_day_shortest_played.m_time = played_day.m_time;
				}

				if( m_day_longest_played.m_time < played_day.m_time )
				{
					m_day_longest_played.m_date = played_day.m_date;
					m_day_longest_played.m_time = played_day.m_time;
				}
			}
		}

		m_days_since_start = Utils::days_between_dates( m_begin_date, Utils::today() );
		m_avg_session_day = played / m_days_since_start;
		m_avg_sessions_day = static_cast< float >( m_nb_sessions ) / m_days_since_start;
		m_remaining_days = remaining_time / m_avg_session_day;
		m_remaining_sessions = ceil( m_remaining_played_days * m_avg_sessions_played_day );
		m_end_date = Utils::add_days_to_date( Utils::today(), m_remaining_days );
	}

	void Stats::reset()
	{
		m_nb_sessions				= 0;
		m_avg_sessions				= 0.f;
		m_avg_sessions_played_day			= 0.f;
		m_avg_session_time			= SplitTime{};
		m_played_days				= 0;
		m_days_since_start			= 0;

		m_begin_date				= SplitDate{};
		m_avg_session_day			= SplitTime{};
		m_avg_session_played_day	= SplitTime{};

		m_game_most_sessions		= ComboStat{};
		m_game_longest_sessions		= ComboStat{};
		m_game_longest_session		= ComboStat{};
		m_game_shortest_sessions	= ComboStat{};
		m_game_shortest_session		= ComboStat{};

		m_day_most_sessions			= ComboStat{};
		m_day_shortest_played		= ComboStat{};
		m_day_longest_played		= ComboStat{};
		m_game_most_days			= ComboStat{};
		m_game_fewest_days			= ComboStat{};

		m_game_shortest_session.m_time	= Utils::get_time_from_string( "99:59:59" );
		m_game_shortest_sessions.m_time = Utils::get_time_from_string( "99:59:59" );

		m_day_shortest_played.m_time	= Utils::get_time_from_string( "99:59:59" );
		m_game_fewest_days.m_number		= 9999;
	}
}
