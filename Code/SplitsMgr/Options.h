#pragma once

#include <Externals/ImGui/imgui.h>

#include <FZN/UI/ImGuiAdditions.h>
#include <FZN/Managers/InputManager.h>
#include <FZN/Application/AppOptions.h>


namespace SplitsMgr
{
	class Options : public fzn::AppOptions
	{
	public:
		enum DateFormat
		{
			ISO8601,		// yyyy-mm-dd
			DMYName,		// dd month name yyyy
			COUNT
		};

		struct Data
		{
			bool m_global_keybinds{ false };		// If true, the app doesn't need to be in focus to handle keybinds.
			DateFormat m_date_format{ DateFormat::ISO8601 };
			StringVector m_languages;

			sf::Vector2u m_window_size{ 900, 800 };
		};

		Options();
		~Options();

		/**
		* @brief Prepare the option window to be displayed. Setup its first state.
		**/
		void open_options() override;

		/**
		* @brief Retrieve current options data.
		* @return The current options.
		**/
		const Data& get_options_datas() const { return m_data; }

	private:
		/**
		* @brief Main display function for custom options data. Called by display().
		**/
		void _display_custom_options() override;

		/**
		* @brief Cancel edited options and restore old ones, then close window.
		**/
		void _cancel_options() override;

		/**
		* @brief Custom load function called by the default one to let user read the given json root.
		* @param _root The json root to read the custom data it contains.
		**/
		void _load_options_from_json( Json::Value& _root ) override;
		/**
		* @brief Custom save function called by the default one to let user fill the given json root.
		* @param _root The json root to fill with custom data.
		**/
		void _save_options_to_json( Json::Value& _root ) override;

	private:
		Data m_data;			// All the options of the application.
		Data m_data_backup;		// A backup to restore if options edition is canceled.
	};
} // namespace Pixeler
