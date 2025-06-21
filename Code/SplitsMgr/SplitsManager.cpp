#include <format>

#include <Externals/json/json.h>

#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Tools.h>

#include "SplitsManager.h"
#include "Utils.h"


namespace SplitsMgr
{
	void SplitsManager::display()
	{
		if( m_games.empty() )
			return;

		ImGui::NewLine();
		std::string title = m_game_name + " - " + m_category;
		ImGui::SetWindowFontScale( 2.f );
		ImVec2 text_size = ImGui::CalcTextSize( title.c_str() );
		ImGui::Text("");
		ImGui::SameLine( ImGui::GetContentRegionAvail().x * 0.5f - text_size.x * 0.5f );
		ImGui::Text( title.c_str() );
		ImGui::SetWindowFontScale( 1.f );
		ImGui::NewLine();
		ImGui::Text( "Current game: %s (%u)", _get_current_game().c_str(), m_current_split );

		ImGui::Text( "Current run time:" );
		ImGui::SameLine();
		ImGui::Text( std::format( "{:%H:%M:%S}", m_run_time ).c_str() );

		ImGui::Separator();

		for( Game& game : m_games )
		{
			game.display();
		}
	}

	void SplitsManager::read_lss( std::string_view _path )
	{
		auto xml_file = tinyxml2::XMLDocument{};

		if( xml_file.LoadFile( _path.data() ) )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Failure : %s (%s)", xml_file.ErrorName(), xml_file.ErrorStr() );
			return;
		}

		FZN_DBLOG( "Parsing %s...", _path.data() );

		tinyxml2::XMLElement* run = xml_file.FirstChildElement( "Run" );

		if( run == nullptr )
			return;

		m_game_icon_desc = "\<![CDATA[" + Utils::get_xml_child_element_text( run, "GameIcon" ) + "]]\>";
		m_game_name = Utils::get_xml_child_element_text( run, "GameName" );
		m_category = Utils::get_xml_child_element_text( run, "CategoryName" );
		m_layout_path = Utils::get_xml_child_element_text( run, "LayoutPath" );

		tinyxml2::XMLElement* segments = run->FirstChildElement( "Segments" );

		if( segments == nullptr )
			return;

		tinyxml2::XMLElement* segment = segments->FirstChildElement( "Segment" );
		uint32_t split_index{ 0 };

		while( segment != nullptr )
		{
			auto game = Game{};

			segment = game.parse_game( segment, split_index );

			m_games.push_back( game );
		}
	}

	void SplitsManager::read_json( std::string_view _path )
	{
		auto file = std::ifstream{ _path.data() };

		if( file.is_open() == false )
			return;

		auto root = Json::Value{};

		file >> root;
		
		m_current_split = root[ "CurrentSplitIndex" ].asUInt();
		m_run_time = Utils::get_time_from_string( root[ "CurrentTime" ].asString() );

		Json::Value splits = root[ "Splits" ];

		Json::Value::iterator it = splits.begin();
		SplitTime run_time{};

		for( Game& game : m_games )
		{
			// This game didn't fill all its splits, no need to go further.
			if( game.parse_split_times( it, run_time ) == false )
				return;
		}
	}

	void SplitsManager::write_lss( tinyxml2::XMLDocument& _document )
	{
		tinyxml2::XMLElement* run{ _document.NewElement( "Run" ) };
		run->SetAttribute( "version", "1.7.0" );
		_document.InsertEndChild( run );

		Utils::create_xml_child_element_with_text( _document, run, "GameIcon", m_game_icon_desc );
		Utils::create_xml_child_element_with_text( _document, run, "GameName", m_game_name );
		Utils::create_xml_child_element_with_text( _document, run, "CategoryName", m_category );
		Utils::create_xml_child_element_with_text( _document, run, "LayoutPath", m_layout_path );
		Utils::create_xml_child_element_with_text( _document, run, "Offset", "00:00:00" );
		Utils::create_xml_child_element_with_text( _document, run, "AttemptCount", fzn::Tools::Sprintf( "%u", m_nb_sessions ) );

		tinyxml2::XMLElement* segments{ _document.NewElement( "Segments" ) };

		for( Game& game : m_games )
		{
			game.write_game( _document, segments );
		}

		run->InsertEndChild( segments );
	}

	void SplitsManager::write_json()
	{

	}

	std::string SplitsManager::_get_current_game()
	{
		for( Game& game : m_games )
		{
			if( game.contains_split_index( m_current_split ) )
				return game.get_name();
		}

		return {};
	}
}
