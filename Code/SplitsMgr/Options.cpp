#include <fstream>
#include <filesystem>

#include <Externals/json/json.h>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Managers/LocalisationManager.h>
#include <FZN/UI/ImGui.h>

#include "Options.h"
#include "localisation.h"
#include "Utils.h"


namespace SplitsMgr
{
	Options::Options():
		AppOptions()
	{
		g_pFZN_Core->AddCallback( this, &Options::on_event, fzn::DataCallbackType::Event );

		_load_options();
	}

	Options::~Options()
	{
		g_pFZN_Core->RemoveCallback( this, &Options::on_event, fzn::DataCallbackType::Event );
	}

	/**
	* @brief Prepare the option window to be displayed. Setup its first state.
	**/
	void Options::open_options()
	{
		AppOptions::open_options();

		m_data_backup = m_data;
	}

	static std::string date_format_to_string( Options::DateFormat _format )
	{
		switch( _format )
		{
			case Options::DateFormat::ISO8601:
				return "yyyy-mm-dd";
			case Options::DMYName:
				return "dd Mon yyyy";

			default:
				return"";
		};
	}

	/**
	* @brief Main display function for custom options data. Called by display().
	**/
	void Options::_display_custom_options()
	{
		static const SplitDate dummy_date = Utils::today();

		if( _begin_option_table() )
		{
			_display_language_setting( LocID::language );

			_first_column_label( "Global keybinds", "If activated, the window doesn't need to be in focus to detect keybinds." );
			_second_column_widget( [ & ]() -> bool
				{
					bool ret = ImGui::Checkbox( "##GlobalKeybinds", &m_data.m_global_keybinds );

					if( ret )
						g_pFZN_InputMgr->SetInputSystem( m_data.m_global_keybinds ? fzn::InputManager::ScanSystem : fzn::InputManager::EventSystem );

					return ret;
				} );

			_first_column_label( "Date format" );
			_second_column_widget( [ & ]() -> bool
				{
					if( ImGui::BeginCombo( "##DateFormat", date_format_to_string( m_data.m_date_format ).c_str() ) )
					{
						for( uint32_t format{ 0 }; format < Options::DateFormat::COUNT; ++format )
						{
							auto enum_format = static_cast< Options::DateFormat >( format );
							if( ImGui::Selectable( date_format_to_string( enum_format ).c_str(), format == m_data.m_date_format ) )
							{
								m_data.m_date_format = enum_format;
								m_edited = true;
							}

							if( ImGui::IsItemHovered() )
								ImGui::SetTooltip( Utils::date_to_str( dummy_date, enum_format ).c_str() );
						}

						ImGui::EndCombo();
					}

					return m_edited;
				}, m_second_column_width );

			ImGui::EndTable();
		}

		_display_bindings();

		ImGui_fzn::window_bottom_table( 2, [ & ]()
			{
				if( ImGui_fzn::deactivable_button( "Apply", m_edited == false, false, ImGui_fzn::default_widget_size ) )
					_confirm_options();

				ImGui::TableSetColumnIndex( 2 );
				if( ImGui::Button( "Cancel", ImGui_fzn::default_widget_size ) )
					_cancel_options();
			} );
	}

	/**
	* @brief Fill the languages vectors with the languages we want to offer to the user in the options menu. Called by the constructor.
	**/
	void Options::_fill_available_languages()
	{
		_set_language_string( LocID::english, Language::english );
		_set_language_string( LocID::french, Language::french );
	}

	/**
	* @brief Cancel edited options and restore old ones, then close window.
	**/
	void Options::_cancel_options()
	{
		AppOptions::_cancel_options();

		m_data = m_data_backup;
	}

	/**
	* @brief Custom load function called by the default one to let user read the given json root.
	* @param _root The json root to read the custom data it contains.
	**/
	void Options::_load_options_from_json( Json::Value& _root )
	{
		m_data.m_global_keybinds = _root[ "global_keybinds" ].asBool();
		m_data.m_date_format = static_cast< Options::DateFormat >( _root[ "date_format" ].asUInt() );

		m_data.m_window_size.x = std::max( _root[ "window_size" ][ 0 ].asUInt(), 800u );
		m_data.m_window_size.y = std::max( _root[ "window_size" ][ 1 ].asUInt(), 600u );

		g_pFZN_WindowMgr->SetWindowSize( m_data.m_window_size );

		RECT desktop_size;
		const HWND desktop_handle = GetDesktopWindow();
		GetWindowRect( desktop_handle, &desktop_size );

		g_pFZN_InputMgr->SetInputSystem( m_data.m_global_keybinds ? fzn::InputManager::ScanSystem : fzn::InputManager::EventSystem );
		g_pFZN_WindowMgr->SetWindowPosition( { desktop_size.right / 2 - static_cast< int >( m_data.m_window_size.x ) / 2, desktop_size.bottom / 2 - static_cast< int >( m_data.m_window_size.y ) / 2 } );
	}

	/**
	* @brief Custom save function called by the default one to let user fill the given json root.
	* @param _root The json root to fill with custom data.
	**/
	void Options::_save_options_to_json( Json::Value& _root )
	{
		_root[ "global_keybinds" ] = m_data.m_global_keybinds;
		_root[ "date_format" ] = m_data.m_date_format;

		_root[ "window_size" ][ 0 ] = m_data.m_window_size.x;
		_root[ "window_size" ][ 1 ] = m_data.m_window_size.y;
	}
}