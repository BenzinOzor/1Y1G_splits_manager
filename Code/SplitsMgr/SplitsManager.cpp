#include <format>

#include <Externals/json/json.h>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Tools.h>

#include "SplitsManager.h"
#include "Utils.h"


namespace SplitsMgr
{

	SplitsManager::SplitsManager()
	{
		g_pFZN_Core->AddCallback( this, &SplitsManager::on_event, fzn::DataCallbackType::Event );
	}

	SplitsManager::~SplitsManager()
	{
		g_pFZN_Core->RemoveCallback( this, &SplitsManager::on_event, fzn::DataCallbackType::Event );
	}

	void SplitsManager::display()
	{
		if( m_games.empty() )
			return;

		ImGui::NewLine();
		std::string title = m_game_name + " - " + m_category;
		ImGui::SetWindowFontScale( 2.f );
		ImVec2 text_size = ImGui::CalcTextSize( title.c_str() );
		ImGui::NewLine();
		ImGui::SameLine( ImGui::GetContentRegionAvail().x * 0.5f - text_size.x * 0.5f );
		ImGui::Text( title.c_str() );
		ImGui::SetWindowFontScale( 1.f );
		ImGui::NewLine();

		if( m_current_game != nullptr )
			ImGui::Text( "Current game: %s (%u)", m_current_game->get_name().c_str(), m_current_split );

		ImGui::Text( "Current run time:" );
		ImGui::SameLine();
		ImGui::Text( std::format( "{:%H:%M:%S}", m_run_time ).c_str() );

		ImGui::SeparatorText( "Update sessions" );

		if( ImGui::BeginTable( "add_session_table", 2 ) )
		{
			const bool disable_buttons{ m_current_game == nullptr || m_sessions_updated };

			if( disable_buttons )
				ImGui::BeginDisabled();

			const float column_size = ImGui::GetContentRegionAvail().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;
			ImGui::TableSetupColumn( "btn1", ImGuiTableColumnFlags_WidthFixed, column_size );
			ImGui::TableSetupColumn( "btn2", ImGuiTableColumnFlags_WidthFixed, column_size );

			ImGui::TableNextColumn();
			if( ImGui::Button( "Game finished", { ImGui::GetContentRegionAvail().x, 0.f } ) )
				_update_sessions( true );

			ImGui::TableNextColumn();
			if( ImGui::Button( "Game still going", { ImGui::GetContentRegionAvail().x, 0.f } ) )
				_update_sessions( false );

			if( disable_buttons )
				ImGui::EndDisabled();

			ImGui::EndTable();
		}

		ImGui::Separator();

		for( Game& game : m_games )
		{
			game.display();
		}
	}

	void SplitsManager::on_event()
	{
		const fzn::Event& fzn_event = g_pFZN_Core->GetEvent();

		if( fzn_event.m_eType != fzn::Event::eUserEvent || fzn_event.m_pUserData == nullptr )
			return;

		Event* split_event = static_cast< Event* >( fzn_event.m_pUserData );

		if( split_event == nullptr )
			return;

		switch( split_event->m_type )
		{
			case Event::Type::session_added:
			{
				_on_game_session_added( split_event->m_game_event );
				break;
			}
		};
	}

	/**
	* @brief Get the time of the run at the given split index.
	* @param _split_index The index of the split at which we want the run time.
	* @return The run time corresponding to the given split index.
	**/
	SplitTime SplitsManager::get_split_run_time( uint32_t _split_index ) const
	{
		for( const Game& game : m_games )
		{
			if( game.contains_split_index( _split_index ) == false )
				continue;

			return game.get_split_run_time( _split_index );
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
				break;
		}

		_get_current_game();

		if( m_current_split == 0 )
			return;

		// If the previous split has the same time as the run time, it means the sessions can't be updated or it will create a segment time of 00:00:00
		if( get_split_run_time( m_current_split - 1 ) == m_run_time )
			m_sessions_updated = true;
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

	void SplitsManager::write_json( Json::Value& _root )
	{
		_root[ "CurrentSplitIndex" ] = m_current_split;
		_root[ "CurrentTime" ] = std::format( "{:%H:%M:%S}", m_run_time ).c_str();

		for( const Game& game : m_games )
		{
			game.write_split_times( _root );
		}
	}

	void SplitsManager::_get_current_game()
	{
		for( Game& game : m_games )
		{
			if( game.contains_split_index( m_current_split ) )
			{
				m_current_game = &game;
				return;
			}
		}
	}

	void SplitsManager::_update_sessions( bool _game_finished )
	{
		if( m_current_game == nullptr )
			return;

		const SplitTime previous_run_time{ m_current_split > 0 ? get_split_run_time( m_current_split - 1 ) : SplitTime{} };
		const SplitTime new_segment_time{ m_run_time - previous_run_time };

		m_current_game->update_last_split( m_run_time, new_segment_time, _game_finished );

		m_sessions_updated = true;

		if( _game_finished )
		{
			++m_current_split;
			_get_current_game();
			return;
		}

		_update_games_splits_indexes( m_current_game );

		++m_current_split;
	}

	void SplitsManager::_update_games_splits_indexes( const Game* _game )
	{
		if( _game == nullptr )
			return;

		// Looping on all the games coming after the given one to update their split index.
		bool current_game_found{ false };
		for( Game& game : m_games )
		{
			if( game.get_name() == _game->get_name() )
			{
				current_game_found = true;
				continue;
			}

			if( current_game_found == false )
				continue;

			game.update_split_indexes();
		}
	}

	void SplitsManager::_on_game_session_added( const Event::GameEvent& _event_infos )
	{
		_update_games_splits_indexes( _event_infos.m_game );

		const Splits& event_game_splits{ _event_infos.m_game->get_splits() };
		const Splits& current_game_splits{ m_current_game->get_splits() };

		// If the added session is in a game after the current one, we don't need tu update the current split.
		if( event_game_splits.front().m_split_index > current_game_splits.front().m_split_index )
			return;

		bool current_game_found{ false };
		bool current_split_updated{ false };
		for( const Game& game : m_games )
		{
			if( game.get_name() == m_current_game->get_name() )
			{
				current_game_found = true;
			}

			if( current_game_found == false )
				continue;

			for( const Split& split : game.get_splits() )
			{
				if( split.m_run_time == SplitTime{} )
				{
					m_current_split = split.m_split_index;
					current_split_updated = true;
					break;
				}
			}

			if( current_split_updated )
				break;
		}

		_get_current_game();
	}

}
