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
		_set_option_loc_entry( OptionsLocEntry::bindings_title,				LocID::keybinds );
		_set_option_loc_entry( OptionsLocEntry::bindings_empty,				LocID::bind_empty );
		_set_option_loc_entry( OptionsLocEntry::bindings_set,				LocID::bind_set );
		_set_option_loc_entry( OptionsLocEntry::bindings_replace,			LocID::bind_replace );
		_set_option_loc_entry( OptionsLocEntry::bindings_replace_title,		LocID::bind_replace_title );
		_set_option_loc_entry( OptionsLocEntry::bindings_replace_text,		LocID::bind_replace_text );
		_set_option_loc_entry( OptionsLocEntry::bindings_replace_cancel,	LocID::cancel );
		_set_option_loc_entry( OptionsLocEntry::bindings_delete_tooltip,	LocID::bind_delete_tooltip );
		_set_option_loc_entry( OptionsLocEntry::bindings_reset_default,		LocID::bind_reset_to_default );

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
			m_data.m_language_id = g_pFZN_LocMgr->get_current_language_id();

			_first_column_label( g_pFZN_LocMgr->get_string(  LocID::global_keybinds ), g_pFZN_LocMgr->get_string( LocID::global_keybinds_tooltip ) );
			_second_column_widget( [ & ]() -> bool
				{
					bool ret = ImGui::Checkbox( "##GlobalKeybinds", &m_data.m_global_keybinds );

					if( ret )
						g_pFZN_InputMgr->SetInputSystem( m_data.m_global_keybinds ? fzn::InputManager::ScanSystem : fzn::InputManager::EventSystem );

					return ret;
				} );

			_first_column_label( g_pFZN_LocMgr->get_string( LocID::date_format ) );
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
					else if( ImGui::IsItemHovered() )
						ImGui::SetTooltip( Utils::date_to_str( dummy_date, m_data.m_date_format ).c_str() );

					return m_edited;
				}, m_second_column_width );

			ImGui::EndTable();
		}

		_display_bindings();

		ImGui_fzn::window_bottom_table( 2, [ & ]()
			{
				if( ImGui_fzn::deactivable_button( g_pFZN_LocMgr->get_string( LocID::apply ).data(), m_edited == false, false, ImGui_fzn::default_widget_size ) )
					_confirm_options();

				ImGui::TableSetColumnIndex( 2 );
				if( ImGui::Button( g_pFZN_LocMgr->get_string( LocID::cancel ).data(), ImGui_fzn::default_widget_size ) )
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
		m_data.m_language_id = _root[ "language" ].asUInt();

		g_pFZN_LocMgr->set_current_language( m_data.m_language_id );
		g_pFZN_InputMgr->SetInputSystem( m_data.m_global_keybinds ? fzn::InputManager::ScanSystem : fzn::InputManager::EventSystem );
	}

	/**
	* @brief Custom save function called by the default one to let user fill the given json root.
	* @param _root The json root to fill with custom data.
	**/
	void Options::_save_options_to_json( Json::Value& _root )
	{
		_root[ "global_keybinds" ] = m_data.m_global_keybinds;
		_root[ "date_format" ] = m_data.m_date_format;
		_root[ "language" ] = m_data.m_language_id;
	}
}