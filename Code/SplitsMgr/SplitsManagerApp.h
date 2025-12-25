#pragma once

#include <filesystem>

#include "ListCreator.h"
#include "SplitsManager.h"
#include "Options.h"


namespace SplitsMgr
{
	/************************************************************************
	* @brief Handling of the ImGui interface
	************************************************************************/
	class SplitsManagerApp
	{
	public:
		/**
		* @brief Construction of the application, will look for lss and json files path in the options json and read them if there are any saved.
		**/
		SplitsManagerApp();
		~SplitsManagerApp();

		/**
		* @brief Display of the main ImGui interface.
		**/
		void display();
		void on_event();

		/**
		* @brief Clear current game list and all in one file path.
		*/
		void close_game_list();

		const SplitsManager&	get_splits_manager() const		{ return m_splits_mgr; }
		const Options&			get_options() const				{ return m_options; }

		Game*					get_current_game() const		{ return m_splits_mgr.get_current_game(); }

	private:
		/**
		* @brief Display the window menu bar.
		**/
		void _display_menu_bar();

		/**
		* @brief Read the saved options file in the Fazon Apps folder.
		**/
		void _load_options();
		/**
		* @brief Write the saved options file in the Fazon Apps folder.
		**/
		void _save_options();

		/**
		* @brief Select game informations json file in explorer and read it. The path will be saved for later writing in the file.
		**/
		void _load_json();
		/**
		* @brief Save current games informations to the previously loaded json file.
		**/
		void _save_json();
		void _save_json_as();

		void _create_json();

		std::filesystem::path m_aio_path;

		SplitsManager m_splits_mgr;
		Options m_options;
		ListCreator m_creator;
	};
}

extern SplitsMgr::SplitsManagerApp* g_splits_app;
