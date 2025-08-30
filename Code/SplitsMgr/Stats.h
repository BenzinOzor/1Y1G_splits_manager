#pragma once

#include "Game.h"
#include "Utils.h"


namespace SplitsMgr
{
	class GameStat
	{
	public:
		std::string		m_name;
		uint32_t		m_nb_sessions{ 0 };
		SplitTime		m_time{};
	};

	class Stats
	{
	public:
		void display();

		void refresh( const Games& _games );

	private:
		void _reset();

		// Displayed final variables
		uint32_t	m_nb_sessions{ 0 };
		float		m_avg_sessions{ 0.f };
		SplitTime	m_avg_session_time{};

		// Working variables used while refreshing the stats
		uint32_t	m_nb_games{ 0 };
		GameStat	m_game_most_sessions;
		GameStat	m_game_longest_sessions;
		GameStat	m_game_longest_session;
		GameStat	m_game_shortest_sessions;
		GameStat	m_game_shortest_session;
	};
} // namespace SplitsMgr
