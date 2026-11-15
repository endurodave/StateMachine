#ifndef _PLAYER_H
#define _PLAYER_H

#include "StateMachine.h"

class Player : public StateMachine
{
public:
	Player();

	// External events taken by this state machine
	void OpenClose();
	void Play();
	void Stop();
	void Pause();
	void EndPause();

private:
	// State enumeration order must match the order of state method entries
	// in the state map.
	enum States
	{
		ST_EMPTY,
		ST_OPEN,
		ST_STOPPED,
		ST_PAUSED,
		ST_PLAYING,
		ST_MAX_STATES
	};

	// Define the state machine states
	STATE_DECLARE(Player, 	Empty,			NoEventData)
	STATE_DECLARE(Player, 	Open,			NoEventData)
	STATE_DECLARE(Player, 	Stopped,		NoEventData)
	STATE_DECLARE(Player, 	Paused,			NoEventData)
	STATE_DECLARE(Player, 	Playing,		NoEventData)

	// State map to define state object order. Each state map entry defines a
	// state object.
	BEGIN_STATE_MAP
		STATE_MAP_ENTRY(&Empty)
		STATE_MAP_ENTRY(&Open)
		STATE_MAP_ENTRY(&Stopped)
		STATE_MAP_ENTRY(&Paused)
		STATE_MAP_ENTRY(&Playing)
	END_STATE_MAP	
};

#endif