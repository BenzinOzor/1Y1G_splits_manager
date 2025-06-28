#pragma once


namespace SplitsMgr
{
	class Game;

	class Event
	{
	public:
		enum class Type
		{
			session_added,		// A session has been added to a game. (m_game_event)
			json_done_reading,	// We finished parsing the json file.
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
			GameEvent m_game_event;		// Game event informations. (session_added)
		};
	};
}
