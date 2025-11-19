#pragma once

#include <string>
#include <vector>
#include <chrono>

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
			ongoing,	// Sessions have been added to the game but it's not the current one.
			COUNT
		};

		void display();
		void on_event();

		const std::string& get_name() const						{ return m_name; }
		bool contains_split_index( uint32_t _index ) const;
		bool is_finished() const								{ return m_state == State::finished; }
		bool is_current() const									{ return m_state == State::current; }
		State get_state() const									{ return m_state; }
		const char* get_state_str() const;
		const Splits& get_splits() const						{ return m_splits; }
		SplitTime get_run_time() const;
		SplitTime get_estimate() const							{ return m_estimation; }
		SplitTime get_delta() const								{ return m_delta; }
		SplitTime get_played() const;
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
		* @param _game_finished True if the game is finished with this new time. If not, some adaptations tho the splits will be needed.
		**/
		void update_last_split( const SplitTime& _run_time, const SplitTime& _segment_time, bool _game_finished );

		/**
		* @brief Increment all split indexes because a session has been added before this game.
		**/
		void update_data( SplitTime& _last_run_time, bool _incremeted_splits_index );

		/**
		* @brief Parse splits file to fill the games sessions.
		* @param [in]		_element Pointer to the current xml element. Will be modified while retrieving splits informations.
		* @param [in,out]	_split_index Current split index value. To be stored in the created splits and incremented as the parsing goes along.
		**/
		tinyxml2::XMLElement* parse_game( tinyxml2::XMLElement* _element, uint32_t& _split_index );

		/**
		* @brief Parse split times for this game. Will iterate in the array as long as it has splits.
		* @param [in,out] _it_splits The current iterator in the times read from the json file. Will be incremented while reading the times for the splits.
		* @return True if all the splits have retrieved a time. False if at least one doesn't have a time, meaning the timer stopped here and there's no need to go further.
		**/
		bool parse_split_times( Json::Value::iterator& _it_splits, SplitTime& _last_time );
		void write_game( tinyxml2::XMLDocument& _document, tinyxml2::XMLElement* _segments );

		void write_split_times( Json::Value& _root ) const;

		void load_cover( std::string_view _path );

	private:
		/**
		* @brief Add a new session to the game using m_new_session_time.
		**/
		void _add_new_session_time();
		void _refresh_game_time();
		void _refresh_state();

		void _push_state_colors( State _state );
		void _pop_state_colors( State _state );
		void _handle_game_background( State _state );
		void _right_click();
		void _estimate_and_delta( State _state );

		std::string m_name;
		std::string m_icon_desc;
		SplitTime m_estimation{};
		SplitTime m_delta{};
		SplitTime m_time{};			// The timer for the game duration.
		State m_state{ State::none };

		Splits m_splits;

		sf::Texture* m_cover{ nullptr };

		std::string m_new_session_time;
		bool m_new_session_game_finished{ false };
	};
	using Games = std::vector< Game >;
}