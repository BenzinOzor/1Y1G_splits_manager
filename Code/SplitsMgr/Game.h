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

		const std::string& get_name() const						{ return m_name; }
		bool contains_split_index( uint32_t _index ) const;
		bool is_finished() const								{ return m_state == State::finished; }
		bool is_current() const									{ return m_state == State::current; }
		bool sessions_over() const								{ return m_state == State::finished || m_state == State::abandonned; }
		State get_state() const									{ return m_state; }
		const char* get_state_str() const;
		State get_state_from_str( std::string_view _state ) const;
		const Splits& get_splits() const						{ return m_splits; }
		SplitTime get_run_time() const;
		SplitTime get_estimate() const							{ return m_estimation; }
		SplitTime get_delta() const								{ return m_delta; }
		SplitTime get_played() const;
		SplitTime get_last_valid_segment_time() const;
		SplitDate get_begin_date() const						{ return m_begin_date; }
		sf::Texture* get_cover() const							{ return m_cover; }

		/**
		* @brief Get the time of the run at the given split index.
		* @param _split_index The index of the split at which we want the run time.
		* @return The run time corresponding to the given split index.
		**/
		SplitTime get_split_run_time( uint32_t _split_index ) const;

		/**
		* @brief Update the last split available on the game because a session has just been made.
		* @param _run_time The new (total) run time tu put in the split.
		* @param _segment_time The time of this split. The previous one isn't necessarily in the same game so it has to be given.
		* @param _segment_date The date of the split.
		* @param _game_finished True if the game is finished with this new time. If not, some adaptations tho the splits will be needed.
		**/
		void update_last_split( const SplitTime& _run_time, const SplitTime& _segment_time, const SplitDate& _segment_date, bool _game_finished );
		/**
		* @brief Increment all split indexes because a session has been added before this game.
		**/
		void update_data( const SplitTime& _delta_to_add, bool _incremeted_splits_index );

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
		SplitTime m_time{};			// The timer for the game duration.
		SplitDate m_begin_date{};
		State m_state{ State::none };

		Splits m_splits;

		sf::Texture* m_cover{ nullptr };
		std::string m_cover_data{};

		std::string m_new_session_time;
		std::string m_new_session_date;
		bool m_new_session_game_finished{ false };

		bool m_finished_game_popup{ false };
		Stats m_game_stats;
	};
	using Games = std::vector< Game >;
}