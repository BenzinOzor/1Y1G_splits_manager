#include <functional>
#include <fstream>
#include <regex>

#include <Externals/json/json.h>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/UI/ImGui.h>

#include "SplitsManagerApp.h"
#include "localisation.h"

#include <ShlObj.h>

SplitsMgr::SplitsManagerApp* g_splits_app = nullptr;

namespace SplitsMgr
{
	static constexpr uint32_t version_major = 4;
	static constexpr uint32_t version_minor = 0;
	static constexpr uint32_t version_feature = 0;
	static constexpr uint32_t version_bugfix = 0;
	static constexpr bool WIP_version = false;

	/**
	* @brief Construction of the application, will look for lss and json files path in the options json and read them if there are any saved.
	**/
	SplitsManagerApp::SplitsManagerApp()
	{
		g_splits_app = this;
		g_pFZN_Core->AddCallback( this, &SplitsManagerApp::display, fzn::DataCallbackType::Display );
		g_pFZN_Core->AddCallback( this, &SplitsManagerApp::on_event, fzn::DataCallbackType::Event );

		_load_options();

		if( m_aio_path.empty() == false )
		{
			if( m_splits_mgr.read_json( m_aio_path.string() ) == false )
				m_aio_path.clear();
		}

	}

	SplitsManagerApp::~SplitsManagerApp()
	{
		g_pFZN_Core->RemoveCallback( this, &SplitsManagerApp::display, fzn::DataCallbackType::Display );
		g_pFZN_Core->RemoveCallback( this, &SplitsManagerApp::on_event, fzn::DataCallbackType::Event );
	}

	/**
	* @brief Display of the main ImGui interface.
	**/
	void SplitsManagerApp::display()
	{
		_handle_actions();
		const auto window_size = g_pFZN_WindowMgr->GetWindowSize();

		ImGui::SetNextWindowPos( { 0.f, 0.f } );
		ImGui::SetNextWindowSize( window_size );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_IndentSpacing, 20.f );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_WindowRounding, 0.f );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_WindowBorderSize, 0.f );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
		ImGui::PushStyleColor( ImGuiCol_WindowBg,			ImVec4( 0.10f, 0.16f, 0.22f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_CheckMark,			ImVec4( 0.f, 1.f, 0.f, 1.f ) );

		// Overriding those so we don't have any alpha trouble when changing the background of the games section
		ImGui::PushStyleColor( ImGuiCol_FrameBg,			ImVec4{ 0.13f, 0.23f, 0.36f, 1.f } );
		ImGui::PushStyleColor( ImGuiCol_FrameBgHovered,		ImVec4{ 0.16f, 0.33f, 0.53f, 1.f } );
		ImGui::PushStyleColor( ImGuiCol_FrameBgActive,		ImVec4{ 0.21f, 0.45f, 0.73f, 1.f } );
		ImGui::PushStyleColor( ImGuiCol_Button,				ImVec4{ 0.16f, 0.33f, 0.53f, 1.f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered,		ImVec4{ 0.26f, 0.59f, 0.98f, 1.f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive,		ImVec4{ 0.06f, 0.53f, 0.98f, 1.f } );

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::Begin( "Splits Manager", nullptr, window_flags );
		ImGui::PopStyleVar( 3 );

		_display_menu_bar();
		m_options.display();

		ImVec2 panel_size{ window_size };

		if( ImGui::GetCurrentWindowRead() != nullptr )
			panel_size.y = window_size.y - ImGui::GetCurrentWindowRead()->MenuBarHeight();

		panel_size.x = window_size.x * 0.5f;

		window_flags ^= ImGuiWindowFlags_MenuBar;

		ImGui::BeginChild( "Left Panel", panel_size, 0, window_flags );
		m_splits_mgr.display_left_panel();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild( "Right Panel", panel_size, 0, window_flags );
		m_splits_mgr.display_right_panel();
		ImGui::EndChild();

		m_creator.display_creation_popup();

		ImGui::End();

		ImGui::PopStyleVar( 1 );
		ImGui::PopStyleColor( 8 );
	}

	void SplitsManagerApp::on_event()
	{
		const fzn::Event& fzn_event = g_pFZN_Core->GetEvent();

		if( fzn_event.m_eType != fzn::Event::eUserEvent || fzn_event.m_pUserData == nullptr )
			return;

		Event* split_event = static_cast<Event*>(fzn_event.m_pUserData);

		if( split_event == nullptr )
			return;

		switch( split_event->m_type )
		{
			case Event::Type::game_list_generated:
			{
				close_game_list();
				break;
			}
		};
	}

	/**
	* @brief Clear current game list and all in one file path.
	*/
	void SplitsManagerApp::close_game_list()
	{
		m_aio_path.clear();
	}

	/**
	* @brief Display the window menu bar.
	**/
	void SplitsManagerApp::_display_menu_bar()
	{
		auto menu_item = [&]( const char* _label, bool _dots, bool _disable, std::function< void(void) > _fct )
		{
			if( _disable )
				ImGui::BeginDisabled();

			if( ImGui::MenuItem( _dots ? fzn::Tools::Sprintf( "%s...", _label ).c_str() : _label ) )
				_fct();

			if( _disable )
				ImGui::EndDisabled();
		};

		if( ImGui::BeginMainMenuBar() )
		{
			if( ImGui::BeginMenu( g_pFZN_LocMgr->get_string( LocID::file ).data() ) )
			{
				const bool aio_invalid	= m_aio_path.empty();
				const bool no_games		= m_splits_mgr.are_there_games() == false;
				menu_item( g_pFZN_LocMgr->get_string( LocID::create ).data(), true, false, [ & ]() { _create_json(); } );
				ImGui_fzn::simple_tooltip_on_hover( g_pFZN_LocMgr->get_string( LocID::create_tooltip ).data() );

				menu_item( g_pFZN_LocMgr->get_string( LocID::open ).data(), true, false, [&]() { _load_json(); } );
				menu_item( g_pFZN_LocMgr->get_string( LocID::save ).data(), false, aio_invalid, [&]() { _save_json(); } );
				ImGui_fzn::simple_tooltip_on_hover( "%s: %s", g_pFZN_LocMgr->get_string( LocID::save_tooltip ).data(), m_aio_path.string().c_str() );

				menu_item( g_pFZN_LocMgr->get_string( LocID::save_as ).data(), true, no_games, [&]() { _save_json_as(); } );

				ImGui::Separator();
				menu_item( g_pFZN_LocMgr->get_string( LocID::close_list ).data(), false, no_games, [&]() { close_game_list(); m_splits_mgr.close_game_list(); } );
				menu_item( g_pFZN_LocMgr->get_string( LocID::refresh_json ).data(), false, aio_invalid, [&]() { m_splits_mgr.read_json( m_aio_path.generic_string().c_str() ); } );

				ImGui::Separator();
				menu_item( g_pFZN_LocMgr->get_string( LocID::options ).data(), true, false, [&]() { m_options.open_options(); } );

				ImGui::EndMenu();
			}

			const std::string version{ fzn::Tools::Sprintf( "Ver. %d.%d.%d.%d%s", version_major, version_minor, version_feature, version_bugfix, WIP_version ? " - WIP" : "" ) };
			const ImVec2 version_size{ ImGui::CalcTextSize( version.c_str() ) };
			const sf::Vector2u window_size{ g_pFZN_WindowMgr->GetWindowSize() };

			ImGui::SameLine( window_size.x - ImGui::CalcTextSize( version.c_str() ).x - 2.f * ImGui::GetStyle().WindowPadding.x );
			ImGui::TextColored( ImGui_fzn::color::light_gray, version.c_str() );

			ImGui::EndMainMenuBar();
		}
	}

	void SplitsManagerApp::_handle_actions()
	{
		if( m_aio_path.empty() == false )
		{
			if( g_pFZN_InputMgr->IsActionPressed( "Save" ) )
			{
				_save_json();
			}
			else if( g_pFZN_InputMgr->IsActionPressed( "Reload json" ) )
			{
				m_splits_mgr.read_json( m_aio_path.generic_string().c_str() );
			}
		}
		else if( m_splits_mgr.are_there_games() && g_pFZN_InputMgr->IsActionPressed( "Save As" ) )
		{
			_save_json_as();
		}
	}

	/**
	* @brief Read the saved options file in the Fazon Apps folder.
	**/
	void SplitsManagerApp::_load_options()
	{
		auto file = std::ifstream{ g_pFZN_Core->GetSaveFolderPath() + "/resources_paths.json" };

		if( file.is_open() == false )
			return;

		auto root = Json::Value{};

		file >> root;

		m_aio_path = root[ "aio_path" ].asString();
	}

	/**
	* @brief Write the saved options file in the Fazon Apps folder.
	**/
	void SplitsManagerApp::_save_options()
	{
		auto file = std::ofstream{ g_pFZN_Core->GetSaveFolderPath() + "/resources_paths.json" };
		auto root = Json::Value{};
		Json::StreamWriterBuilder writer_builder;

		writer_builder.settings_["emitUTF8"] = true;
		std::unique_ptr<Json::StreamWriter> writer( writer_builder.newStreamWriter() );

		root[ "aio_path" ] = m_aio_path.string().c_str();

		writer->write( root, &file );
	}

	/**
	* @brief Select game informations json file in explorer and read it. The path will be saved for later writing in the file.
	**/
	void SplitsManagerApp::_load_json()
	{
		const std::string path = fzn::Tools::open_file( "(*.json) 1A1J Games List\0*.json\0 (*.*) All files \0*.*\0", g_pFZN_LocMgr->get_string( LocID::open_dialog_title ) );

		if( path.empty() )
			return;

		m_aio_path = path;
		m_splits_mgr.read_json( path );

		_save_options();
	}

	/**
	* @brief Save current games informations to the previously loaded json file.
	**/
	void SplitsManagerApp::_save_json()
	{
		auto file = std::ofstream{ m_aio_path };
		auto root = Json::Value{};

		Json::StreamWriterBuilder writer_builder;

		writer_builder.settings_[ "emitUTF8" ] = true;
		std::unique_ptr<Json::StreamWriter> writer( writer_builder.newStreamWriter() );

		m_splits_mgr.write_json( root );

		writer->write( root, &file );
	}

	void SplitsManagerApp::_save_json_as()
	{
		std::string new_path = fzn::Tools::save_file_as( "(*.json) 1A1J Games List\0*.json\0 (*.*) All files \0*.*\0", ".json", g_pFZN_LocMgr->get_string( LocID::save_dialog_title ) );

		if( new_path.empty() )
			return;

		m_aio_path = new_path;

		_save_options();
		_save_json();
	}

	void SplitsManagerApp::_create_json()
	{
		m_creator.show_creation_popup();
	}
}
