#include <fstream>

#include <Externals/json/json.h>

#include <FZN/Managers/FazonCore.h>

#include "SplitsManagerApp.h"


namespace SplitsMgr
{
	/**
	* @brief Construction of the application, will look for lss and json files path in the options json and read them if there are any saved.
	**/
	SplitsManagerApp::SplitsManagerApp()
	{
		g_pFZN_Core->AddCallback( this, &SplitsManagerApp::display, fzn::DataCallbackType::Display );
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
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_IndentSpacing, 35.f );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_WindowRounding, 0.f );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_WindowBorderSize, 0.f );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
		ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.10f, 0.16f, 0.22f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_CheckMark, ImVec4( 0.f, 1.f, 0.f, 1.f ) );

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		//window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::Begin( "Splits Manager", NULL, window_flags );
		ImGui::PopStyleVar( 3 );

		_display_menu_bar();

		m_splits_mgr.display();

		ImGui::End();

		ImGui::PopStyleVar( 1 );
		ImGui::PopStyleColor( 2 );
	}

	void SplitsManagerApp::_display_menu_bar()
	{
		if( ImGui::BeginMainMenuBar() )
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				if( ImGui::MenuItem( "Load LSS" ) )
					_load_lss();

				if( ImGui::MenuItem( "Load JSON" ) )
				{}

				ImGui::Separator();

				if( ImGui::MenuItem( "Save LSS" ) )
				{}

				if( ImGui::MenuItem( "Save JSON" ) )
				{}

				ImGui::EndMenu();
			}

			ImGui::Text( "Sessions: %d", m_splits_mgr.get_nb_sessions() );

			ImGui::EndMainMenuBar();
		}
	}

	void SplitsManagerApp::_load_options()
	{
		auto file = std::ifstream{ g_pFZN_Core->GetSaveFolderPath() + "/options.json" };

		if( file.is_open() == false )
			return;

		auto root = Json::Value{};

		file >> root;

		/*auto load_color = [&root]( const char* _color_name, ImVec4& _color )
		{
			_color.x = root[ _color_name ][ ColorChannel::red ].asUInt() / 255.f;
			_color.y = root[ _color_name ][ ColorChannel::green ].asUInt() / 255.f;
			_color.z = root[ _color_name ][ ColorChannel::blue ].asUInt() / 255.f;
			_color.w = root[ _color_name ][ ColorChannel::alpha ].isNull() ? 1.f : root[ _color_name ][ ColorChannel::alpha ].asUInt() / 255.f;
		};

		load_color( "canvas_color", m_options_datas.m_canvas_background_color );
		load_color( "area_highlight_color", m_options_datas.m_area_highlight_color );
		load_color( "grid_color", m_options_datas.m_grid_color );

		m_options_datas.m_area_highlight_thickness = root[ "area_highlight_thickness" ].asFloat();
		m_options_datas.m_grid_same_color_as_canvas = root[ "grid_same_color_as_canvas" ].asBool();
		m_options_datas.m_show_grid = root[ "show_grid" ].asBool();

		m_options_datas.m_bindings = g_pFZN_InputMgr->GetActionKeys();*/
	}

	void SplitsManagerApp::_save_options()
	{

	}

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
			m_splits_mgr.read_lss( open_file_name.lpstrFile );
	}
}
