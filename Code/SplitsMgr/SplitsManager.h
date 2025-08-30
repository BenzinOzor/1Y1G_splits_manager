#pragma once

#include <string>
#include <vector>

#include <Externals/json/json.h>

#include "Game.h"
#include "Event.h"


namespace tinyxml2
{
	class XMLDocument;
}

namespace SplitsMgr
{
	class SplitsManager
	{
	public:
		SplitsManager();
		~SplitsManager();

		void display_left_panel();
		void display_right_panel();
		void on_event();

		Game*		get_current_game() const		{ return m_current_game; }
		uint32_t	get_nb_sessions() const			{ return m_nb_sessions; }
		uint32_t	get_current_split_index() const	{ return m_current_split; }

		/**
		* @brief Get the time of the run at the given split index.
		* @param _split_index The index of the split at which we want the run time.
		* @return The run time corresponding to the given split index.
		**/
		SplitTime get_split_run_time( uint32_t _split_index ) const;

		/**
		* @brief Search for the last valid time in the "run", starting at the given game and then going back to previous ones.
		* @param _threshold_game The game from which we start looking for a valid run time.
		* @return The most recent valid run time. default SplitTime value if nothing has been found.
		**/
		SplitTime get_last_valid_run_time( const Game* _threshold_game ) const;

		void read_lss( std::string_view _path );
		void read_json( std::string_view _path );

		void write_lss( tinyxml2::XMLDocument& _document );
		void write_json( Json::Value& _root );

	private:
		void _refresh_current_game_ptr();
		void _update_sessions( bool _game_finished );
		void _on_game_session_added( const Event::GameEvent& _event_infos );

		void _display_stats();
		void _display_update_sessions_buttons();

		/**
		* @brief Update splits index and run time of games coming after the given one.
		* @param _game The game that has been updated, and from which the update will start.
		**/
		void _update_games_data( const Game* _game );

		/**
		* @brief Update current split index, game, and global run time. Called after a session has been added to one of the games.
		**/
		void _update_run_data();

		void _update_run_stats();

		std::string m_game_icon_desc;
		std::string m_game_name;
		std::string m_category;
		std::string m_layout_path;
		uint32_t m_nb_sessions{ 0 };	// Determined from the number of splits, then written in the .lss. Never read so it's always the number of split when starting a new session.
		SplitTime m_estimate{};
		SplitTime m_delta{};
		SplitTime m_remaining_time{};
		SplitTime m_estimated_final_time{};

		std::vector< Game > m_games;
		Game* m_current_game{ nullptr };

		uint32_t m_current_split{ 0 };
		SplitTime m_run_time{};

		bool m_sessions_updated{ false };	// True if all the sessions have been updated with the right splits created.
	};
} // SplitsMgr
