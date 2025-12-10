#pragma once

#include <Externals/ImGui/imgui.h>
#include <FZN/UI/ImGuiAdditions.h>
#include <FZN/Managers/InputManager.h>


namespace SplitsMgr
{
	class Options
	{
	public:
		enum DateFormat
		{
			ISO8601,		// yyyy-mm-dd
			DMYName,		// dd month name yyyy
			COUNT
		};

		struct OptionsDatas
		{
			bool m_global_keybinds{ false };		// If true, the app doesn't need to be in focus to handle keybinds.
			DateFormat m_date_format{ DateFormat::ISO8601 };

			sf::Vector2u m_window_size{ 900, 800 };

			std::vector< fzn::ActionKey > m_bindings;
		};

		Options();

		void show_window();
		void update();

		void on_event();

		const OptionsDatas& get_options_datas() const { return m_options_datas; }

	private:
		void _draw_keybinds( float _column_width );

		void _load_options();
		void _save_options();

		bool _begin_option_table( float _column_width );
		void _first_column_text( const char* _text );

		bool m_show_window{ false };

		OptionsDatas m_options_datas;
		OptionsDatas m_temp_options_datas;
		bool m_need_save{ false };
	};
} // namespace Pixeler
