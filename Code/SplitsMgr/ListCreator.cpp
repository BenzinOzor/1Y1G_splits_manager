#include <codecvt>

#include <FZN/Managers/WindowManager.h>
#include <FZN/UI/ImGui.h>
#include <FZN/Tools/Logging.h>

#include "ListCreator.h"


namespace SplitsMgr
{
	void ListCreator::show_creation_popup()
	{
		m_show_creation_popup = true;
		m_copy_paste_options.fill( true );
	}

	void ListCreator::display_creation_popup()
	{
		if( m_show_creation_popup == false )
			return;

		if( m_show_creation_popup )
		{
			ImGui::OpenPopup( "Create Game List" );

			ImVec2 popup_size{};

			popup_size.x = 500.f + ImGui::GetStyle().WindowPadding.x * 2.f;
			popup_size.y = 500.f + Utils::game_cover_size.y + ImGui::GetStyle().WindowPadding.y * 2.f + ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
			sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();

			ImGui::SetNextWindowPos( { window_size.x * 0.5f - popup_size.x * 0.5f, window_size.y * 0.5f - popup_size.y * 0.5f }, ImGuiCond_FirstUseEver );
			ImGui::SetNextWindowSize( popup_size, ImGuiCond_FirstUseEver );

			if( ImGui::BeginPopupModal( "Create Game List", &m_show_creation_popup ) )
			{
				ImGui::InputTextMultiline( "##Source", &m_game_list_source, { ImGui::GetContentRegionAvail().x, 300.f }, ImGuiInputTextFlags_AllowTabInput );

				ImGui::Text( "Select which fields are present in the pasted text above." );

				for( uint32_t field{ 0 }; field < CopyPasteField::COUNT; ++field)
				{
					if( field > 0 )
						ImGui::SameLine();

					bool temp{ false };
					ImGui::Checkbox( copy_paste_field_to_str( static_cast< CopyPasteField >( field ) ).c_str(), &m_copy_paste_options[ field ] );
				}

				ImGui::Checkbox( "Merge year and game name", &m_merge_year_and_game );
				ImGui::SameLine();
				ImGui_fzn::helper_simple_tooltip( "Combine yar and game name to create a new name using this format: '<year> - <game name>'." );

				if( ImGui::Button( "Generate Game List" ) )
					generate_game_list_from_copy_paste();

				ImGui::PushStyleColor( ImGuiCol_Separator, ImGui_fzn::color::white );

				ImGui::BeginChild( "Games", ImVec2{ 0.f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() } );
				for( Game& game : m_games )
				{
					game.display();
				}
				ImGui::EndChild();

				ImGui::PopStyleColor();

				ImGui::Button( "Confirm" );

				ImGui::EndPopup();
			}
		}
	}

	std::string ListCreator::copy_paste_field_to_str( CopyPasteField _value ) const
	{
		switch( _value )
		{
			case CopyPasteField::state:
				return "State";
			case CopyPasteField::year:
				return "Year";
			case CopyPasteField::name:
				return "Name";
			case CopyPasteField::type:
				return "Type";
			case CopyPasteField::platform:
				return "Platform";
			case CopyPasteField::version:
				return "Version";
			case CopyPasteField::estimate:
				return "Estimate";
			case CopyPasteField::played:
				return "Played";
			default:
				return "";
		};
	}

	ListCreator::GameInfosMap ListCreator::_get_elements( std::string_view _raw_game_infos )
	{
		StringVector elements{ fzn::Tools::split( _raw_game_infos, '\t', false ) };

		auto get_next_selected_field = [this]( uint32_t _current_index = UINT32_MAX )
		{
			if( _current_index == UINT32_MAX )
			{
				_current_index = 0;

				if( m_copy_paste_options[ _current_index ] )
					return _current_index;
			}

			++_current_index;

			while( _current_index < static_cast< uint32_t >( CopyPasteField::COUNT ) )
			{
				if( m_copy_paste_options[ _current_index ] )
					return _current_index;

				++_current_index;
			}

			return _current_index;
		};

		uint32_t field_index{ get_next_selected_field() };
		GameInfosMap result_map{ {CopyPasteField::state, ""},{CopyPasteField::year, ""},{CopyPasteField::name, ""},{CopyPasteField::type, ""},{CopyPasteField::platform, ""},{CopyPasteField::version, ""},{CopyPasteField::estimate, ""},{CopyPasteField::played, ""}, };

		for( const std::string& element: elements )
		{
			result_map[ static_cast< CopyPasteField >( field_index ) ] = element;
			field_index = get_next_selected_field( field_index );

			if( field_index >= static_cast<uint32_t>( CopyPasteField::COUNT ) )
				break;
		}

		return result_map;
	}

	Game::Desc ListCreator::create_desc_from_elements( GameInfosMap& _elements )
	{
		auto game_desc = Game::Desc{};

		if( _elements.empty() )
			return game_desc;

		std::string game_year{};

		if( m_copy_paste_options[ CopyPasteField::state ] )
		{
			const std::string& state{ _elements[ CopyPasteField::state ] };
			// We don't want to consider replaced games or ignored years.
			if( state.contains( "Remplac" ) || state.contains( "Ignor" ) )
			{
				return game_desc;
			}

			if( state.contains( "En cours" ) )
				game_desc.m_state = Game::State::playing;
			else if( state.contains( "Termin" ) )
				game_desc.m_state = Game::State::finished;
			else if( state.contains( "Abandonn" ) )
				game_desc.m_state = Game::State::abandonned;
		}

		if( m_copy_paste_options[ CopyPasteField::name ] )
		{
			if( m_merge_year_and_game && m_copy_paste_options[ CopyPasteField::year ] )
				game_desc.m_name = fzn::Tools::Sprintf( "%s - %s", _elements[ CopyPasteField::year ].c_str(), _elements[ CopyPasteField::name ].c_str() );
			else
				game_desc.m_name = _elements[ CopyPasteField::name ];
		}

		if( m_copy_paste_options[ CopyPasteField::estimate ] )
		{
			game_desc.m_estimation = Utils::get_time_from_string( _elements[ CopyPasteField::estimate ], _elements[ CopyPasteField::estimate ].size() > 5 ? "%4H:%M:%S" : "%4H:%M" );
		}

		if( m_copy_paste_options[ CopyPasteField::played ] )
		{
			game_desc.m_played = Utils::get_time_from_string( _elements[ CopyPasteField::played ], _elements[ CopyPasteField::played ].size() > 5 ? "%4H:%M:%S" : "%4H:%M" );
		}

		return game_desc;
	}

	void ListCreator::generate_game_list_from_copy_paste()
	{
		FZN_LOG( "Generating game list from copy pasted google sheet content..." );

		std::string options{};

		for( uint32_t field{ 0 }; field < CopyPasteField::COUNT; ++field )
		{
			if( m_copy_paste_options[ field ] == false )
				continue;

			fzn::Tools::sprintf_cat( options, options.empty() ? "%s" : ", %s", copy_paste_field_to_str( static_cast<CopyPasteField>( field ) ).c_str() );
		}

		FZN_LOG( "Selected options: %s", options.c_str() );

		m_games.clear();

		size_t cursor{ 0 };
		size_t end_of_line{ std::string::npos };
		Utils::ParsingInfos parsing_infos{};
		GameInfosMap elements;

		while( cursor < m_game_list_source.size() )
		{
			end_of_line = m_game_list_source.find_first_of( '\n', cursor );

			if( end_of_line == std::string::npos )	
			{
				end_of_line = m_game_list_source.size();
			}

			std::string line = m_game_list_source.substr( cursor, end_of_line - cursor );

			FZN_LOG( "Current line: %s", line.c_str() );

			elements = _get_elements( line );
			auto game_desc{ create_desc_from_elements( elements ) };

			if( game_desc.is_valid() )
			{
				m_games.push_back( Game( game_desc, parsing_infos ) );

				const Game& last_game{ m_games.back() };
				FZN_LOG( "Game added: %s (%s) | %s / %s", last_game.get_name().c_str(), last_game.get_state_str(), Utils::time_to_str( last_game.get_played() ).c_str(), Utils::time_to_str( last_game.get_estimate() ).c_str() );
			}

			cursor = end_of_line + 1;
		}
	}
}
