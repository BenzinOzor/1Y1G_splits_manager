#pragma once

#include <string>
#include <vector>

#include "Game.h"

namespace SplitsMgr
{
	class SplitsManager
	{
	public:
		void display();

		uint32_t get_nb_sessions() { return m_nb_sessions; }

		void read_lss( std::string_view _path );
		void read_json( std::string_view _path );

		void write_lss();
		void write_json();

	private:
		std::string m_game_icon_desc;
		std::string m_game_name;
		std::string m_category;
		std::string m_layout_path;
		uint32_t m_nb_sessions{ 0 };	// Determined from the number of splits, then written in the .lss. Never read so it's always the number of split when starting a new session.

		std::vector< Game > m_games;
	};
} // SplitsMgr
