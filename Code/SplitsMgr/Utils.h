#pragma once

#include <string_view>
#include <chrono>

#include "Externals/ImGui/imgui.h"


namespace tinyxml2
{
	class XMLElement;
	class XMLDocument;
}

namespace SplitsMgr
{
	using SplitTime = std::chrono::duration<int, std::milli>;

	namespace Utils
	{
		namespace Color
		{
			static constexpr ImVec4		current_game_frame_bg			{ 0.58f, 0.43f, 0.03f, 1.f };
			static constexpr ImVec4		current_game_header				{ 1.f, 0.8f , 0.05f, 1.f };
			static constexpr ImVec4		current_game_header_hovered		{ 1.f, 0.87f , 0.05f, 1.f };
			static constexpr ImVec4		current_game_header_active		{ 1.f, 0.99f , 0.05f, 1.f };

			static constexpr ImVec4		finished_game_frame_bg			{ 0.14f, 0.45f , 0.14f, 1.f };
			static constexpr ImVec4		finished_game_header			{ 0.24f, 0.75f , 0.24f, 1.f };
			static constexpr ImVec4		finished_game_header_hovered	{ 0.28f, 0.90f , 0.28f, 1.f };
			static constexpr ImVec4		finished_game_header_active		{ 0.31f, 1.f , 0.31f, 1.f };

			static constexpr ImVec4		abandonned_game_frame_bg		{ 0.55f, 0.17f , 0.17f, 1.f };
			static constexpr ImVec4		abandonned_game_header			{ 0.8f, 0.25f , 0.25f, 1.f };
			static constexpr ImVec4		abandonned_game_header_hovered	{ 0.95f, 0.3f, 0.3f, 1.f };
			static constexpr ImVec4		abandonned_game_header_active	{ 1.f, 0.36f , 0.31f, 1.f };

			static constexpr ImVec4		ongoing_game_frame_bg			{ 0.34f, 0.23f , 0.38f, 1.f };
			static constexpr ImVec4		ongoing_game_header				{ 0.67f, 0.52f , 0.72f, 1.f };
			static constexpr ImVec4		ongoing_game_header_hovered		{ 0.77f, 0.60f , 0.84f, 1.f };
			static constexpr ImVec4		ongoing_game_header_active		{ 0.91f, 0.71f , 1.f, 1.f };
		}

		struct ParsingInfos
		{
			uint32_t m_current_split{ 0 };
			uint32_t m_split_index{ 0 };
			SplitTime m_total_time{};
		};

		std::string get_xml_child_element_text( tinyxml2::XMLElement* _container, std::string_view _child_name );
		void create_xml_child_element_with_text( tinyxml2::XMLDocument& _document, tinyxml2::XMLElement* _container, std::string_view _child_name, std::string_view _text );

		SplitTime get_time_from_string( std::string_view _time, std::string_view _format = "%4H:%M:%S" );
		std::string time_to_str( const SplitTime& _time, bool _floor_seconds = true, bool _separate_days = false );
		bool is_time_valid( const SplitTime& _time );

		std::string get_cover_data( std::string_view _cover_path );
	}
}
