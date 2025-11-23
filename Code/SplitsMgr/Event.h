#pragma once


namespace SplitsMgr
{
	class Game;

	class Event
	{
	public:
		enum class Type
		{
			session_added,				// A session has been added to a game. (m_game_event)
			json_done_reading,			// We finished parsing the json file.
			new_current_game_selected,	// A game has been selected via right click menu to be the current one. (m_game_event)
			current_game_changed,		// The confirmation of the new current game selection.
			game_estimate_changed,		// The estimated of a game changed.
			COUNT
		};

		struct GameEvent
		{
			Game*		m_game{ nullptr };
			uint32_t	m_split{ 0 };
			bool		m_game_finished{ false };
		};

		Event() = default;

		Event( const Type& _eType )
			: m_type( _eType )
		{}

		~Event() {}

		Type m_type = Type::COUNT;

		union
		{
			GameEvent m_game_event;		// Game event informations. (session_added, new_current_game_selected)
		};
	};
}
