#pragma once

#include <string>
#include <vector>

#include <Externals/json/json.h>

#include <FZN/Tools/Chrono.h>

#include "Game.h"
#include "Event.h"
#include "Stats.h"


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

		Game*			get_current_game() const		{ return m_current_game; }
		uint32_t		get_nb_sessions() const			{ return m_nb_sessions; }
		SplitTime		get_played() const				{ return m_played; }
		SplitTime		get_remaining_time() const		{ return m_remaining_time; }
		const Stats&	get_stats() const				{ return m_stats; }

		/**
		* @brief Open and read the Json file containing all games informations.
		* @param _path The path to the Json file.
		**/
		void read_json( std::string_view _path );
		/**
		* @brief Write games informations in the given Json root.
		* @param [in out] _root The Json root that will hold all the games informations
		**/
		void write_json( Json::Value& _root );

	private:
		void _update_sessions( bool _game_finished );
		void _on_game_session_added( const Event::GameEvent& _event_infos );

		void _display_timers( const ImVec4& _timer_color );
		void _display_controls();
		void _display_update_sessions_buttons();

		/**
		* @brief Update splits index and run time of games coming after the given one.
		* @param _game The game that has been updated, and from which the update will start.
		**/
		void _update_games_data( const Game* _game );

		/**
		* @brief Update current game and global run time. Called after a session has been added to one of the games.
		**/
		void _update_run_data();

		void _update_run_stats();

		void _handle_actions();

		void _start_split();
		void _toggle_pause();
		void _stop();

		std::string m_title;
		std::string m_category;
		std::string m_layout_path;
		uint32_t m_nb_sessions{ 0 };	// Determined from the number of splits, then written in the .lss. Never read so it's always the number of split when starting a new session.
		SplitTime m_estimate{};
		SplitTime m_played{};
		SplitTime m_delta{};
		SplitTime m_remaining_time{};
		SplitTime m_estimated_final_time{};
		
		fzn::Chrono m_chrono;

		Games m_games;
		Game* m_current_game{ nullptr };
		Game* m_finished_game{ nullptr };

		SplitTime m_run_time{};

		bool m_sessions_updated{ false };	// True if all the sessions have been updated with the right splits created.

		Stats m_stats;
	};
} // SplitsMgr
