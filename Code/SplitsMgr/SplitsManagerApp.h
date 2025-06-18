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

	private:
		void _display_menu_bar();

		void _load_options();
		void _save_options();

		void _load_lss();
		void _save_lss();

		std::filesystem::path m_lss_path;
		std::filesystem::path m_json_path;

		SplitsManager m_splits_mgr;
	};
}
