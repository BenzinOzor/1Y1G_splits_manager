#include <functional>
#include <fstream>
#include <regex>

#include <Externals/json/json.h>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/UI/ImGui.h>

#include "SplitsManagerApp.h"

#include <ShlObj.h>

SplitsMgr::SplitsManagerApp* g_splits_app = nullptr;

namespace SplitsMgr
{
	static constexpr uint32_t version_major = 2;
	static constexpr uint32_t version_minor = 0;
	static constexpr uint32_t version_feature = 4;
	static constexpr uint32_t version_bugfix = 4;

	/**
	* @brief Construction of the application, will look for lss and json files path in the options json and read them if there are any saved.
	**/
	SplitsManagerApp::SplitsManagerApp()
	{
		g_pFZN_Core->AddCallback( this, &SplitsManagerApp::display, fzn::DataCallbackType::Display );

		_load_options();

		if( m_aio_path.empty() == false )
			m_splits_mgr.read_all_in_one_file( m_aio_path.string() );

		g_splits_app = this;
	}

	SplitsManagerApp::~SplitsManagerApp()
	{
		g_pFZN_Core->RemoveCallback( this, &SplitsManagerApp::display, fzn::DataCallbackType::Display );
	}

	/**
	* @brief Display of the main ImGui interface.
	**/
	void SplitsManagerApp::display()
	{
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
		m_options.update();

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

		ImGui::End();

		ImGui::PopStyleVar( 1 );
		ImGui::PopStyleColor( 8 );
	}

	uint32_t SplitsManagerApp::get_current_split_index() const
	{
		return m_splits_mgr.get_current_split_index();
	}

	/**
	* @brief Display the window menu bar.
	**/
	void SplitsManagerApp::_display_menu_bar()
	{
		auto menu_item = [&]( const char* _label, bool _disable, std::function< void(void) > _fct )
		{
			if( _disable )
				ImGui::BeginDisabled();

			if( ImGui::MenuItem( _label ) )
				_fct();

			if( _disable )
				ImGui::EndDisabled();
		};

		if( ImGui::BeginMainMenuBar() )
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				const bool aio_invalid = m_aio_path.empty();
				menu_item( "Load...", false, [&]() { _load_aio(); } );
				menu_item( "Save", aio_invalid, [&]() { _save_aio(); } );
				ImGui_fzn::simple_tooltip_on_hover( fzn::Tools::Sprintf( "Loaded file path: %s", m_aio_path.string().c_str() ) );
				
				ImGui::Separator();
				menu_item( "Reload files", aio_invalid, [&]() { m_splits_mgr.read_all_in_one_file( m_aio_path.generic_string().c_str() ); } );

				ImGui::Separator();
				menu_item( "Options...", false, [&]() { m_options.show_window(); } );

				ImGui::EndMenu();
			}

			const std::string version{ fzn::Tools::Sprintf( "Ver. %d.%d.%d.%d", version_major, version_minor, version_feature, version_bugfix ) };
			const ImVec2 version_size{ ImGui::CalcTextSize( version.c_str() ) };
			const sf::Vector2u window_size{ g_pFZN_WindowMgr->GetWindowSize() };

			ImGui::SameLine( window_size.x - ImGui::CalcTextSize( version.c_str() ).x - 2.f * ImGui::GetStyle().WindowPadding.x );
			ImGui::TextColored( ImGui_fzn::color::light_gray, version.c_str() );

			ImGui::EndMainMenuBar();
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

		m_lss_path = root[ "lss_path" ].asString();
		m_json_path = root[ "json_path" ].asString();
		m_aio_path = root[ "aio_path" ].asString();
		m_covers_path = root[ "covers_path" ].asString();
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

		root[ "lss_path" ] = m_lss_path.string().c_str();
		root[ "json_path" ] = m_json_path.string().c_str();
		root[ "aio_path" ] = m_aio_path.string().c_str();
		root[ "covers_path" ] = m_covers_path.string().c_str();

		writer->write( root, &file );
	}

	/**
	* @brief Read the splits file used for 1Y1G.
	**/
	void SplitsManagerApp::_load_lss()
	{
		char file[ 100 ];
		OPENFILENAME open_file_name;
		ZeroMemory( &open_file_name, sizeof( open_file_name ) );

		open_file_name.lStructSize = sizeof( open_file_name );
		open_file_name.hwndOwner = NULL;
		open_file_name.lpstrFile = file;
		open_file_name.lpstrFile[ 0 ] = '\0';
		open_file_name.nMaxFile = sizeof( file );
		open_file_name.lpstrFileTitle = NULL;
		open_file_name.nMaxFileTitle = 0;
		GetOpenFileName( &open_file_name );

		if( open_file_name.lpstrFile[ 0 ] != '\0' )
		{
			m_lss_path = open_file_name.lpstrFile;
			m_splits_mgr.read_lss( open_file_name.lpstrFile );
		}

		_save_options();
	}

	/**
	* @brief Write the splits file once it's done being edited.
	**/
	void SplitsManagerApp::_save_lss()
	{
		tinyxml2::XMLDocument dest_file{};
		m_splits_mgr.write_lss( dest_file );

		if( dest_file.SaveFile( m_lss_path.string().c_str() ) )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Failure : %s (%s)", dest_file.ErrorName(), dest_file.ErrorStr() );
			return;
		}

		FZN_DBLOG( "Saved .lss file at '%s'", m_lss_path.string().c_str() );

		auto file = std::ifstream{ m_lss_path.string() };
		std::string data( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
		file.close();

		data = std::regex_replace( data, std::regex{ "&gt;" }, ">" );
		data = std::regex_replace( data, std::regex{ "&lt;" }, "<" );

		auto out_file = std::ofstream{ m_lss_path.string() };
		if( out_file.is_open() )
			out_file << data.c_str();
	}

	/**
	* @brief Read the json containing the exported times of the current 1Y1G.
	**/
	void SplitsManagerApp::_load_json()
	{
		char file[ 100 ];
		OPENFILENAME open_file_name;
		ZeroMemory( &open_file_name, sizeof( open_file_name ) );

		open_file_name.lStructSize = sizeof( open_file_name );
		open_file_name.hwndOwner = NULL;
		open_file_name.lpstrFile = file;
		open_file_name.lpstrFile[ 0 ] = '\0';
		open_file_name.nMaxFile = sizeof( file );
		open_file_name.lpstrFileTitle = NULL;
		open_file_name.nMaxFileTitle = 0;
		GetOpenFileName( &open_file_name );

		if( open_file_name.lpstrFile[ 0 ] != '\0' )
		{
			m_json_path = open_file_name.lpstrFile;
			m_splits_mgr.read_json( open_file_name.lpstrFile );
		}

		_save_options();
	}

	void SplitsManagerApp::_save_json()
	{
		auto file = std::ofstream{ m_json_path };
		auto root = Json::Value{};
		Json::StyledWriter json_writer;

		root[ "TimingMethod" ] = 0;
		root[ "CurrentSplitTime" ] = Json::Value{};

		m_splits_mgr.write_json( root );

		file << root;
	}

	void SplitsManagerApp::_load_aio()
	{
		char file[ 100 ];
		OPENFILENAME open_file_name;
		ZeroMemory( &open_file_name, sizeof( open_file_name ) );

		open_file_name.lStructSize = sizeof( open_file_name );
		open_file_name.hwndOwner = NULL;
		open_file_name.lpstrFile = file;
		open_file_name.lpstrFile[ 0 ] = '\0';
		open_file_name.nMaxFile = sizeof( file );
		open_file_name.lpstrFileTitle = NULL;
		open_file_name.nMaxFileTitle = 0;
		GetOpenFileName( &open_file_name );

		if( open_file_name.lpstrFile[ 0 ] != '\0' )
		{
			m_aio_path = open_file_name.lpstrFile;
			m_splits_mgr.read_all_in_one_file( open_file_name.lpstrFile );
		}

		_save_options();
	}

	void SplitsManagerApp::_save_aio()
	{
		auto file = std::ofstream{ m_aio_path };
		auto root = Json::Value{};

		Json::StreamWriterBuilder writer_builder;

		writer_builder.settings_[ "emitUTF8" ] = true;
		std::unique_ptr<Json::StreamWriter> writer( writer_builder.newStreamWriter() );

		m_splits_mgr.write_all_in_one_file( root );

		writer->write( root, &file );
	}

	void SplitsManagerApp::_load_covers()
	{
		BROWSEINFO browseInfo;
		LPITEMIDLIST pItemIDList;
		TCHAR szDisplayName[ 256 ] = "";
		TCHAR szPath[ MAX_PATH ];
		browseInfo.hwndOwner = NULL;
		browseInfo.pidlRoot = NULL;
		browseInfo.pszDisplayName = szDisplayName;
		browseInfo.lpszTitle = "Select destination folder";
		browseInfo.lParam = 0;
		browseInfo.lpfn = NULL;
		browseInfo.ulFlags = 0;
		browseInfo.iImage = 0;// Open dialog
		pItemIDList = SHBrowseForFolder( &browseInfo );
		SHGetPathFromIDList( pItemIDList, szPath );
		if( szPath[ 0 ] )
		{
			m_covers_path = szPath;
			m_splits_mgr.load_covers( szPath );
		}

		_save_options();
	}

}
