#pragma once

#include <filesystem>

#include "SplitsManager.h"


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

		const SplitsManager& get_splits_manager() const { return m_splits_mgr; }

		Game*		get_current_game() const		{ return m_splits_mgr.get_current_game(); }
		uint32_t	get_current_split_index() const;

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
		* @brief Read the splits file used for 1Y1G.
		**/
		void _load_lss();

		/**
		* @brief Write the splits file once it's done being edited.
		**/
		void _save_lss();

		/**
		* @brief Read the json containing the exported times of the current 1Y1G.
		**/
		void _load_json();

		/**
		* @brief Write the json file with new times.
		**/
		void _save_json();

		void _load_covers();

		std::filesystem::path m_lss_path;
		std::filesystem::path m_json_path;
		std::filesystem::path m_covers_path;

		SplitsManager m_splits_mgr;
	};
}

extern SplitsMgr::SplitsManagerApp* g_splits_app;
