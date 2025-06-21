#pragma once

#include <string>
#include <vector>
#include <chrono>

#include <Externals/json/json.h>

#include "Utils.h"


namespace tinyxml2
{
	class XMLElement;
	class XMLDocument;
}

namespace SplitsMgr
{
	struct Split
	{
		uint32_t m_index{ 0 };
		std::string m_name;
		SplitTime m_run_time;
		SplitTime m_segment_time;
	};
	using Splits = std::vector< Split >;

	class Game
	{
	public:
		void display();

		const std::string& get_name() const { return m_name; }
		bool contains_split_index( uint32_t _index ) const;

		/**
		* @brief Parse splits file to fill the games sessions.
		* @param [in] _element Pointer to the current xml element. Will be modified while retrieving splits informations.
		* @param [in,out] _split_index Current split index value. To be stored in the created splits and incremented as the parsing goes along.
		**/
		tinyxml2::XMLElement* parse_game( tinyxml2::XMLElement* _element, uint32_t& _split_index );

		/**
		* @brief Parse split times for this game. Will iterate in the array as long as it has splits.
		* @param [in,out] _it_splits The current iterator in the times read from the json file. Will be incremented while reading the times for the splits.
		* @return True if all the splits have retrieved a time. False if at least one doesn't have a time, meaning the timer stopped here and there's no need to go further.
		**/
		bool parse_split_times( Json::Value::iterator& _it_splits );
		void write_game( tinyxml2::XMLDocument& _document, tinyxml2::XMLElement* _segments );

	private:
		std::string m_name;
		std::string m_icon_desc;
		SplitTime m_estimation;

		Splits m_splits;

		std::string m_new_session_time;
	};
}