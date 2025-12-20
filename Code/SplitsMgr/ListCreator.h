#pragma once

#include "Game.h"


namespace SplitsMgr
{
	class ListCreator
	{
	public:
		enum class CreationMethod
		{
			copy_paste,		// Copy the gsheet contents and past it in the creation popup text box.
			csv,			// Import from csv file.
			manual,			// Create the list from scratch.
			COUNT
		};

		enum CopyPasteField
		{
			state,
			year,
			name,
			type,
			platform,
			version,
			estimate,
			played,
			COUNT
		};

		using GameInfosMap = std::unordered_map< CopyPasteField, std::string >;

		void show_creation_popup();
		void display_creation_popup();

	private:
		std::string copy_paste_field_to_str( CopyPasteField _value ) const;

		GameInfosMap _get_elements( std::string_view _raw_game_infos );
		Game::Desc create_desc_from_elements( GameInfosMap& _elements );
		void generate_game_list_from_copy_paste();

		bool m_show_creation_popup{ false };
		bool m_merge_year_and_game{ true };

		std::string m_game_list_source{};

		std::array< bool, CopyPasteField::COUNT > m_copy_paste_options;

		Games m_games;
	};
}
