#include <fstream>
#include <filesystem>

#include <Externals/json/json.h>

#include <FZN/Managers/FazonCore.h>
#include <FZN/UI/ImGui.h>

#include "Options.h"
#include "Utils.h"


namespace SplitsMgr
{
	Options::Options()
	{
		g_pFZN_Core->AddCallback( this, &Options::on_event, fzn::DataCallbackType::Event );

		_load_options();
	}

	void Options::show_window()
	{
		m_show_window = true;
		m_temp_options_datas = m_options_datas;
		m_need_save = false;

		g_pFZN_InputMgr->BackupActionKeys();
	}

	void Options::update()
	{
		if( m_show_window == false )
			return;

		auto second_column_widget = [&]( bool _widget_edited ) -> bool
		{
			if( _widget_edited )
				m_need_save = true;

			return _widget_edited;
		};

		ImGui::SetNextWindowSize( { 400.f, 0.f } );

		if( ImGui::Begin( "Options", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse ) )
		{
			const float column_width = ImGui::GetContentRegionAvail().x * 0.45f;

			if( _begin_option_table( column_width ) )
			{
				_first_column_text( "Global keybinds" );
				if( second_column_widget( ImGui::Checkbox( "##GlobalKeybinds", &m_options_datas.m_global_keybinds ) ) )
					g_pFZN_InputMgr->SetInputSystem( m_options_datas.m_global_keybinds ? fzn::InputManager::ScanSystem : fzn::InputManager::EventSystem );

				ImGui::SameLine();
				ImGui_fzn::helper_simple_tooltip( "If activated, the window doesn't need to be in focus to detect keybinds." );

				ImGui::EndTable();
			}

			_draw_keybinds( column_width );

			Utils::window_bottom_table( 2, [&]()
			{
				if( ImGui_fzn::deactivable_button( "Apply", m_need_save == false, false, DefaultWidgetSize ) )
				{
					m_show_window = false;
					_save_options();
				}

				ImGui::TableSetColumnIndex( 2 );
				if( ImGui::Button( "Cancel", DefaultWidgetSize ) )
				{
					m_show_window = false;
					m_options_datas = m_temp_options_datas;
					g_pFZN_InputMgr->ResetActionKeys();
				}
			} );
		}

		ImGui::End();
	}

	void Options::on_event()
	{
		sf::Event sf_event = g_pFZN_WindowMgr->GetWindowEvent();

		switch( sf_event.type )
		{
			case sf::Event::Closed:
			{
				_save_options();
				return;
			};
			case sf::Event::Resized:
			{
				m_options_datas.m_window_size = g_pFZN_WindowMgr->GetWindowSize();
			};
		};

		fzn::Event oEvent = g_pFZN_Core->GetEvent();

		if( oEvent.m_eType == fzn::Event::eActionKeyBindDone )
			m_need_save = true;
	}

	void Options::_draw_keybinds( float _column_width )
	{
		const bool popup_open{ g_pFZN_InputMgr->IsWaitingActionKeyBind() };
		std::string_view replaced_binding_name{};
		static std::string popup_name{};
		ImGui::SeparatorText( "Keybinds" );

		if( ImGui::BeginTable( "Keybinds", 3 ) )
		{
			ImGui::TableSetupColumn( "##Action", ImGuiTableColumnFlags_WidthFixed, _column_width );
			ImGui::TableSetupColumn( "##Bind", ImGuiTableColumnFlags_WidthStretch );
			ImGui::TableSetupColumn( "##Del.", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize( "Del." ).x + ImGui::GetStyle().FramePadding.x + ImGui::GetStyle().ItemSpacing.x );

			for( const fzn::ActionKey& action_key : g_pFZN_InputMgr->GetActionKeys() )
			{

				ImGui::PushID( &action_key );
				ImGui::TableNextRow();
				_first_column_text( action_key.m_sName.c_str() );

				if( ImGui::Button( g_pFZN_InputMgr->GetActionKeyString( action_key.m_sName, true, 0, false ).c_str(), { ImGui::GetContentRegionAvail().x, 0.f } ) )
				{
					g_pFZN_InputMgr->replace_action_key_bind( action_key.m_sName, fzn::InputManager::BindTypeFlag_All, 0 );
					replaced_binding_name = action_key.m_sName;
				}

				if( ImGui::IsItemHovered() )
					ImGui::SetTooltip( "Replace" );

				ImGui::TableNextColumn();
				if( ImGui::Button( "Del.", { ImGui::GetContentRegionAvail().x, 0.f } ) )
				{
					if( g_pFZN_InputMgr->RemoveActionKeyBind( action_key.m_sName, fzn::InputManager::BindType::eKey ) )
					{
						m_need_save = true;
					}
				}

				if( ImGui::IsItemHovered() )
					ImGui::SetTooltip( "Delete shortcut" );

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		if( popup_open != g_pFZN_InputMgr->IsWaitingActionKeyBind() )
		{
			popup_name = fzn::Tools::Sprintf( "Replace binding: %s", replaced_binding_name.data() );
			ImGui::OpenPopup( popup_name.c_str() );
		}

		if( g_pFZN_InputMgr->IsWaitingActionKeyBind() )
		{
			const ImVec2 title_size{ ImGui::CalcTextSize( popup_name.c_str() ) };
			static const ImVec2 text_size{ ImGui::CalcTextSize( "Press any key to replace this binding" ) };

			float popup_width{ text_size.x > title_size.x ? text_size.x : title_size.x + ImGui::GetStyle().WindowPadding.x * 2.f };
			sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();

			ImGui::SetNextWindowPos( { window_size.x * 0.5f - popup_width * 0.5f, window_size.y * 0.5f - popup_width * 0.5f }, ImGuiCond_Appearing );
			ImGui::SetNextWindowSize( { popup_width, ImGui::GetFrameHeightWithSpacing() * 3.f } );

			if( ImGui::BeginPopupModal( popup_name.c_str(), nullptr, ImGuiWindowFlags_NoMove ) )
			{
				ImGui::NewLine();
				ImGui::Text( "Press any key to replace this binding" );

				ImGui::EndPopup();
			}
		}
	}

	void Options::_load_options()
	{
		const std::string option_file_path{ g_pFZN_Core->GetSaveFolderPath() + "/options.json" };
		auto file = std::ifstream{ option_file_path };

		if( file.is_open() == false )
		{
			// If the reason we could not open the option file is that it doesn't exist, create it.
			if( std::filesystem::exists( option_file_path ) == false )
			{
				m_options_datas.m_window_size = g_pFZN_WindowMgr->GetWindowSize();
				_save_options();
			}

			return;
		}

		auto root = Json::Value{};

		file >> root;

		m_options_datas.m_global_keybinds = root[ "global_keybinds" ].asBool();

		m_options_datas.m_window_size.x = std::max( root[ "window_size" ][ 0 ].asUInt(), 800u );
		m_options_datas.m_window_size.y = std::max( root[ "window_size" ][ 1 ].asUInt(), 600u );

		g_pFZN_WindowMgr->SetWindowSize( m_options_datas.m_window_size );

		RECT desktop_size;
		const HWND desktop_handle = GetDesktopWindow();
		GetWindowRect( desktop_handle, &desktop_size );

		g_pFZN_InputMgr->SetInputSystem( m_options_datas.m_global_keybinds ? fzn::InputManager::ScanSystem : fzn::InputManager::EventSystem );
		g_pFZN_WindowMgr->SetWindowPosition( { desktop_size.right / 2 - static_cast<int>( m_options_datas.m_window_size.x ) / 2, desktop_size.bottom / 2 - static_cast<int>( m_options_datas.m_window_size.y ) / 2 } );

		m_options_datas.m_bindings = g_pFZN_InputMgr->GetActionKeys();
	}

	void Options::_save_options()
	{
		auto file = std::ofstream{ g_pFZN_Core->GetSaveFolderPath() + "/options.json" };
		auto root = Json::Value{};
		Json::StyledWriter json_writer;

		root[ "global_keybinds" ] = m_options_datas.m_global_keybinds;

		root[ "window_size" ][ 0 ] = m_options_datas.m_window_size.x;
		root[ "window_size" ][ 1 ] = m_options_datas.m_window_size.y;

		file << json_writer.write( root );

		g_pFZN_InputMgr->SaveCustomActionKeysToFile();
	}

	bool Options::_begin_option_table( float _column_width )
	{
		const bool ret = ImGui::BeginTable( "options", 2 );

		ImGui::TableSetupColumn( "label", ImGuiTableColumnFlags_WidthFixed, _column_width );
		ImGui::TableNextRow();

		return ret;
	}

	void Options::_first_column_text( const char* _text )
	{
		ImGui::TableSetColumnIndex( 0 );
		ImGui::SameLine( ImGui::GetStyle().IndentSpacing * 0.5f );
		ImGui::Text( _text );

		ImGui::TableSetColumnIndex( 1 );
	}
}