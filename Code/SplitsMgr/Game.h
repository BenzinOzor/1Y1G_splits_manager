#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>

#include <Externals/json/json.h>

#include "Utils.h"


namespace tinyxml2
{
	class XMLElement;
	class XMLDocument;
}

class sf::Texture;

namespace SplitsMgr
{
	struct Split
	{
		uint32_t m_split_index{ 0 };
		uint32_t m_session_index{ 0 };
		SplitTime m_run_time;
		SplitTime m_segment_time;
		SplitDate m_date;
	};
	using Splits = std::vector< Split >;

	class Game
	{
	public:
		enum class State
		{
			none,		// No specific state, the game hasn't been played yet.
			current,	// This is the game currently being played (the main focus, in the order of the list).
			finished,	// Finished game, no more sessions possible.
			abandonned,	// Didn't reach the end of the game but no sessions will be added.
			ongoing,	// Sessions have been added to the game but it's not the current one.
			COUNT
		};

		void display();
		void on_event();

		bool display_finished_stats();
		void display_end_date_predition();

		const std::string& get_name() const						{ return m_name; }
		bool contains_split_index( uint32_t _index ) const;
		bool is_finished() const								{ return m_state == State::finished; }
		bool is_current() const									{ return m_state == State::current; }
		bool are_sessions_over() const							{ return m_state == State::finished || m_state == State::abandonned; }
		bool has_sessions() const;
		State get_state() const									{ return m_state; }
		void set_state( State _state )							{ m_state = _state; }
		const char* get_state_str() const;
		State get_state_from_str( std::string_view _state ) const;
		const Splits& get_splits() const						{ return m_splits; }
		SplitTime get_run_time() const;
		SplitTime get_estimate() const							{ return m_estimation; }
		SplitTime get_delta() const								{ return m_delta; }
		SplitTime get_played() const;
		SplitTime get_last_valid_segment_time() const;
		SplitDate get_begin_date() const						{ return m_stats.m_begin_date; }
		sf::Texture* get_cover() const							{ return m_cover; }

		/**
		* @brief Add a session to the game, from timer or manual add. Run time will be determined thanks to the game splits themselves.
		* @param _time The time of the session we want to add.
		* @param _date The date of the session.
		* @param _state The new state of the game.
		**/
		void add_session( const SplitTime& _time, const SplitDate& _date, State _state );
		/**
		* @brief Update game datas by incrementing its splits indexes and adding a time to their run time.
		* @param _delta_to_add The time delta that has been added on a game before this one that we need to add.
		**/
		void update_data( const SplitTime& _delta_to_add );
		/**
		* @brief Calculate at which date the game could be finished, either by using its stats if it has any sessions, or the global stats compiled from all the previous games.
		**/
		void compute_end_date();

		/**
		* @brief Read the Json value containing all the informations about the game.
		* @param _game The game informations.
		* @param [in out] _parsing_infos State of the parsing.
		* @return True if this is the current game.
		**/
		bool read( const Json::Value& _game, Utils::ParsingInfos& _parsing_infos );
		/**
		* @brief Write the game infos into the given Json value
		* @param [in out] _game The Json value that will hold the game informations.
		**/
		void write( Json::Value& _game ) const;

	private:
		/**
		* @brief Stats displayed in the finished game popup.
		**/
		struct Stats
		{
			SplitTime m_average_session_time;
			SplitTime m_shortest_session;
			SplitTime m_longest_sesion;

			SplitDate	m_begin_date{};					// The earliest date available in the game list.
			uint32_t	m_remaining_days{};
			uint32_t	m_remaining_played_days{};
			uint32_t	m_remaining_sessions{ 0 };
			SplitTime	m_avg_session_day{};			// Average time on the period between current day and starting day. (taking non played days in account)
			float		m_avg_sessions_days{ 0.f };
			SplitTime	m_avg_session_played_day{};		// Average time by played day
			SplitDate	m_end_date{};
			uint32_t	m_played_days{ 0 };
			uint32_t	m_days_since_start{ 0 };
		};

		/**
		* @brief Add a new session to the game using m_new_session_time.
		**/
		void _add_new_session_time();
		void _refresh_game_time();
		void _refresh_state();

		void _push_state_colors( State _state );
		void _pop_state_colors( State _state );
		void _handle_game_background( State _state );
		void _right_click( State _state );
		void _tooltip();
		void _estimate_and_delta( State _state );

		void _select_cover();

		/**
		* @brief Compute all game stats from its estimate, time played and sessions.
		**/
		void _compute_game_stats();
		/**
		* @brief Displayed computed game stats, weither be in its tooltip or in the finished game popup.
		**/
		void _display_game_stats_table( float _window_width );

		std::string m_name;
		std::string m_icon_desc;
		SplitTime m_estimation{};
		SplitTime m_delta{};
		SplitTime m_played{};			// The timer for the game duration.
		State m_state{ State::none };

		Splits m_splits;

		sf::Texture* m_cover{ nullptr };
		std::string m_cover_data{};

		std::string m_new_session_time;
		std::string m_new_session_date;
		bool m_new_session_game_finished{ false };

		bool m_finished_game_popup{ false };
		Stats m_stats;
	};
	using Games = std::vector< Game >;
}