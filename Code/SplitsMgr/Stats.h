#pragma once

#include "Game.h"
#include "Utils.h"


namespace SplitsMgr
{
	class ComboStat
	{
	public:
		std::string		m_string{};
		uint32_t		m_number{ 0 };
		SplitTime		m_time{};
		SplitDate		m_date{};
	};

	class Stats
	{
	public:
		void display();

		void refresh( const Games& _games );

		float get_avg_sessions_days() const				{ return m_avg_sessions_days; }
		SplitTime get_avg_session_played_day() const	{ return m_avg_session_played_day; }
		SplitTime get_avg_session_day() const			{ return m_avg_session_day; }
		SplitDate get_begin_date() const				{ return m_begin_date; }

		void reset();

	private:
		// Displayed final variables
		uint32_t	m_nb_sessions{ 0 };
		float		m_avg_sessions{ 0.f };
		float		m_avg_sessions_days{ 0.f };
		SplitTime	m_avg_session_time{};

		SplitDate	m_begin_date{};					// The earliest date available in the game list.
		uint32_t	m_remaining_days{};
		uint32_t	m_remaining_played_days{};
		uint32_t	m_remaining_sessions{ 0 };
		SplitTime	m_avg_session_day{};			// Average time on the period between current day and starting day. (taking non played days in account)
		SplitTime	m_avg_session_played_day{};		// Average time by played day
		SplitDate	m_end_date{};
		uint32_t	m_played_days{ 0 };
		uint32_t	m_days_since_start{ 0 };

		ComboStat	m_game_most_sessions;
		ComboStat	m_game_longest_sessions;
		ComboStat	m_game_longest_session;
		ComboStat	m_game_shortest_sessions;
		ComboStat	m_game_shortest_session;

		ComboStat	m_day_most_sessions;
		ComboStat	m_day_shortest_played;
		ComboStat	m_day_longest_played;
		ComboStat	m_game_most_days;
		ComboStat	m_game_fewest_days;
	};
} // namespace SplitsMgr
