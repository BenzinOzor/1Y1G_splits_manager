#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Tools.h>

#include "SplitsManager.h"
#include "Utils.h"


namespace SplitsMgr
{
	void SplitsManager::display()
	{
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

		while( segment != nullptr )
		{
			auto game = Game{};

			segment = game.parse_game( segment );

			m_games.push_back( game );
		}
	}

	void SplitsManager::read_json( std::string_view _path )
	{

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
}
